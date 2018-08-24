#include "hjson.h"
#include <sstream>
#include <regex>
#include <iomanip>
#include <iostream>
#include <cmath>

namespace Hjson {


// DefaultOptions returns the default encoding options.
EncoderOptions DefaultOptions() {
  EncoderOptions opt;

  opt.eol = "\n";
  opt.bracesSameLine = false;
  opt.quoteAlways = false;
  opt.quoteKeys = false;
  opt.indentBy = "  ";
  opt.allowMinusZero = false;
  opt.unknownAsNull = false;
  opt.separator = false;
  opt.outputCommentary = false;
  opt.emitRootBraces = true;

  return opt;
}


struct Encoder {
  EncoderOptions opt;
  std::ostringstream oss;
  std::locale loc;
  int indent;
  std::regex needsEscape, needsQuotes, needsEscapeML, startsWithKeyword, needsEscapeName, lineBreak;
};


bool startsWithNumber(const char *text, size_t textSize);


// table of character substitutions
static const char *_meta(char c) {
  switch (c)
  {
  case '\b':
    return "\\b";
  case '\t':
    return "\\t";
  case '\n':
    return "\\n";
  case '\f':
    return "\\f";
  case '\r':
    return "\\r";
  case '"':
    return "\\\"";
  case '\\':
    return "\\\\";
  }

  return 0;
}


static void _writeIndent(Encoder *e, int indent) {
  e->oss << e->opt.eol;

  for (int i = 0; i < indent; i++) {
    e->oss << e->opt.indentBy;
  }
}


static int _fromUtf8(const unsigned char **ppC, size_t *pnS) {
  int nS, nRet;
  const unsigned char *pC = *ppC;

  if (!*pnS) {
    return -1;
  }

  if (*pC < 0x80) {
    nS = 1;
    nRet = *pC++;
  } else if (*pC < 0xc0) {
    return -1;
  } else if (*pC < 0xe0) {
    nS = 2;
    nRet = *pC++ & 0x1f;
  } else if (*pC < 0xf0) {
    nS = 3;
    nRet = *pC++ & 0xf;
  } else if (*pC < 0xf8) {
    nS = 4;
    nRet = *pC++ & 0x7;
  } else {
    return -1;
  }

  if (*pnS < static_cast<size_t>(nS)) {
    return -1;
  }

  for (int a = nS - 1; a; --a) {
    if (*pC < 0x80) {
      return -1;
    }
    nRet = (nRet << 6) | (*pC++ & 0x3f);
  }

  pnS[0] -= nS;
  *ppC = pC;

  return nRet;
}


static void _quoteReplace(Encoder *e, std::string text) {
  size_t uIndexStart = 0;

  for (std::sregex_iterator it = std::sregex_iterator(text.begin(), text.end(),
    e->needsEscape); it != std::sregex_iterator(); ++it)
  {
    std::smatch match = *it;
    const char *szReplacement = _meta(match.str()[0]);

    if (size_t(match.position()) > uIndexStart) {
      // Append non-matching text.
      e->oss << text.substr(uIndexStart, match.position() - uIndexStart);
    }

    if (szReplacement) {
      e->oss << szReplacement;
    } else {
      const char *pC = text.data() + match.position();
      size_t nS = match.length();

      e->oss << std::hex << std::setfill('0');
      while (nS) {
        int nRet = _fromUtf8((const unsigned char**) &pC, &nS);
        if (nRet < 0) {
          // Not UTF8. Just dump it.
          e->oss << std::string(pC, nS);
          break;
        }
        e->oss << "\\u" << std::setw(4) << nRet;
      }
    }

    uIndexStart = match.position() + match.length();
  }

  if (uIndexStart < text.length()) {
    // Append remaining text.
    e->oss << text.substr(uIndexStart, text.length() - uIndexStart);
  }
}


// wrap the string into the ''' (multiline) format
static void _mlString(Encoder *e, std::string value, std::string separator) {
  size_t uIndexStart = 0;
  std::sregex_iterator it = std::sregex_iterator(value.begin(), value.end(),
    e->lineBreak);

  if (it == std::sregex_iterator()) {
    // The string contains only a single line. We still use the multiline
    // format as it avoids escaping the \ character (e.g. when used in a
    // regex).
    e->oss << separator << "'''";
    e->oss << value;
  } else {
    _writeIndent(e, e->indent + 1);
    e->oss << "'''";

    do {
      std::smatch match = *it;
      auto indent = e->indent + 1;
      if (match.position() == uIndexStart) {
        indent = 0;
      }
      _writeIndent(e, indent);
      if (size_t(match.position()) > uIndexStart) {
        e->oss << value.substr(uIndexStart, match.position() - uIndexStart);
      }
      uIndexStart = match.position() + match.length();
      ++it;
    } while (it != std::sregex_iterator());

    if (uIndexStart < value.length()) {
      // Append remaining text.
      _writeIndent(e, e->indent + 1);
      e->oss << value.substr(uIndexStart, value.length() - uIndexStart);
    } else {
      // Trailing line feed.
      _writeIndent(e, 0);
    }

    _writeIndent(e, e->indent + 1);
  }

  e->oss << "'''";
}


// Check if we can insert this string without quotes
// see hjson syntax (must not parse as true, false, null or number)
static void _quote(Encoder *e, const Value& value, std::string separator, bool isRootObject) {
  std::string valueStr = value.to_string();
  if (valueStr.size() == 0) {
    e->oss << separator << "\"\"";
  } else if (e->opt.quoteAlways ||
    std::regex_search(valueStr, e->needsQuotes) ||
    startsWithNumber(valueStr.c_str(), valueStr.size()) ||
    std::regex_search(valueStr, e->startsWithKeyword) ||
    e->opt.outputCommentary && !(value.comment_post().empty()))
  {

    // If the string contains no control characters, no quote characters, and no
    // backslash characters, then we can safely slap some quotes around it.
    // Otherwise we first check if the string can be expressed in multiline
    // format or we must replace the offending characters with safe escape
    // sequences.

    if (!std::regex_search(valueStr, e->needsEscape)) {
      e->oss << separator << '"' << valueStr << '"';
    } else if (!e->opt.quoteAlways && !std::regex_search(valueStr,
      e->needsEscapeML) && !isRootObject)
    {
      _mlString(e, valueStr, separator);
    } else {
      e->oss << separator + '"';
      _quoteReplace(e, valueStr);
      e->oss << '"';
    }
  } else {
    // return without quotes
    e->oss << separator << valueStr;
  }
}


static void _quoteName(Encoder *e, std::string name) {
  if (name.empty()) {
    e->oss << "\"\"";
  } else if (e->opt.quoteKeys || std::regex_search(name, e->needsEscapeName)) {
    e->oss << '"';
    if (std::regex_search(name, e->needsEscape)) {
      _quoteReplace(e, name);
    } else {
      e->oss << name;
    }

    e->oss << '"';
  } else {
    // without quotes
    e->oss << name;
  }
}


// Write the commentary
// Iff specialPrefix == "", a normal indent will be used
static void _writeComment(Encoder *e, const std::string& com, const std::string& specialPrefix = "", bool indent = true)
{
  if (e->opt.outputCommentary && !com.empty()) {
    if (specialPrefix.empty() && indent)
      _writeIndent(e, e->indent);
    if (!specialPrefix.empty() && com.front() > ' ')
      e->oss << specialPrefix; // insert ' ' if suffix-like comment does not start with any white-char
    for (const auto& it: com) {
      if (it == '\n') { // line break is encoder dependent
        _writeIndent(e, indent ? e->indent : 0);
      }
      else if (it != '\r') { // skip any '\r'
        e->oss << it;
      }
    }
  }
}


// Write seperator and commentary
static inline void _writeSeperatorAndComment(Encoder *e, bool& isFirst, const std::string& com)
{
  if (isFirst) {
    isFirst = false;
  }
  else {
    if (e->opt.separator)
      e->oss << ",";
    _writeComment(e, com, " ");
  }
}

// Produce a string from value.
static void _str(Encoder *e, const Value& value, bool noIndent, std::string separator, bool isRootObject) {
  switch (value.type()) {
  case Value::DOUBLE:
    e->oss << separator;

    if (std::isnan(double(value)) || std::isinf(double(value))) {
      e->oss << Value(Value::HJSON_NULL).to_string();
    } else if (!e->opt.allowMinusZero && value == 0 && std::signbit(double(value))) {
      e->oss << Value(0).to_string();
    } else {
      e->oss << value.to_string();
    }
    break;

  case Value::STRING:
    _quote(e, value, separator, isRootObject);
    break;

  case Value::VECTOR:
    if (value.empty() && (value.comment_inside().empty() || !e->opt.outputCommentary)) {
      e->oss << separator << "[]";
    } else {
      auto indent1 = e->indent;
      e->indent++;

      if (!noIndent && !e->opt.bracesSameLine) {
        _writeIndent(e, indent1);
      } else {
        e->oss << separator;
      }
      e->oss << "[";

      // Join all of the element texts together, separated with newlines
      bool isFirst = true;
      const std::string* com = nullptr;
      for (int i = 0; size_t(i) < value.size(); ++i) {
        if (value[i].defined()) {
          _writeSeperatorAndComment(e, isFirst, *com);
          _writeComment(e, value[i].comment_pre());
          _writeIndent(e, e->indent);
          _str(e, value[i], true, "", false);
          com = &value[i].comment_post();
        }
      }
      if (com)
        _writeComment(e, *com, " ");

      _writeComment(e, value.comment_inside());
      _writeIndent(e, indent1);
      e->oss << "]";

      e->indent = indent1;
    }
    break;

  case Value::MAP:
    if (value.empty() && (value.comment_inside().empty() || !e->opt.outputCommentary)) {
      e->oss << separator << "{}";
    } else {
      auto indent1 = e->indent;
      if (isRootObject && !value.comment_pre().empty()) {
        _writeComment(e, value.comment_pre() + '\n', "", !noIndent);
      }
      if (!isRootObject || e->opt.emitRootBraces)
        e->indent++;

      if (!noIndent && !e->opt.bracesSameLine) {
        _writeIndent(e, indent1);
      } else {
        e->oss << separator;
      }
      if (!isRootObject || e->opt.emitRootBraces)
        e->oss << "{";

      // Join all of the member texts together, separated with newlines
      bool isFirst = true;
      const std::string* com = nullptr;
      for (const auto& it : value) {
        if (it.second.defined()) {
          _writeSeperatorAndComment(e, isFirst, *com);
          _writeComment(e, it.second.comment_pre(), "", !isRootObject || e->opt.emitRootBraces);
          _writeIndent(e, e->indent);
          _quoteName(e, it.first);
          e->oss << ":";
          _str(e, it.second, false, " ", false);
          com = &it.second.comment_post();
        }
      }
      if (com)
        _writeComment(e, *com, " ");

      _writeComment(e, value.comment_inside());
      _writeIndent(e, indent1);
      if (!isRootObject || e->opt.emitRootBraces)
        e->oss << "}";

      e->indent = indent1;
      if (isRootObject) {
        _writeComment(e, value.comment_post());
        if (e->opt.emitRootBraces)
          _writeIndent(e, 0);
      }
    }
    break;

  default:
    e->oss << separator << value.to_string();
  }
}


// MarshalWithOptions returns the Hjson encoding of v.
//
// Marshal traverses the value v recursively.
//
// Boolean values encode as JSON booleans.
//
// Floating point, integer, and Number values encode as JSON numbers.
//
// String values encode as Hjson strings (quoteless, multiline or
// JSON).
//
// Array and slice values encode as JSON arrays.
//
// Map values encode as JSON objects. The map's key type must be a
// string. The map keys are sorted and used as JSON object keys.
//
// Pointer values encode as the value pointed to.
// A nil pointer encodes as the null JSON value.
//
// Interface values encode as the value contained in the interface.
// A nil interface value encodes as the null JSON value.
//
// JSON cannot represent cyclic data structures and Marshal does not
// handle them. Passing cyclic structures to Marshal will result in
// an infinite recursion.
//
std::string MarshalWithOptions(const Value& v, EncoderOptions options) {
  if (options.separator) {
    options.quoteAlways = true;
  }

  Encoder e;
  e.opt = options;
  e.indent = 0;

  // Regex should not be UTF8 aware, just treat the chars as values.
  e.loc = std::locale::classic();

  std::string commonRange = R"(]|\xc2\xad|\xd8[\x80-\x84]|\xdc\x8f|\xe1\x9e[\xb4\xb5]|\xe2\x80[\x8c\x8f]|\xe2\x80[\xa8-\xaf]|\xe2\x81[\xa0-\xaf]|\xef\xbb\xbf|\xef\xbf[\xb0-\xbf])";
  // needsEscape tests if the string can be written without escapes
  e.needsEscape.imbue(e.loc);
  e.needsEscape.assign(R"([\\\"\x00-\x1f)" + commonRange);
  // needsQuotes tests if the string can be written as a quoteless string (includes needsEscape but without \\ and \")
  e.needsQuotes.imbue(e.loc);
  e.needsQuotes.assign(R"(^\s|^"|^'|^#|^/\*|^//|^\{|^\}|^\[|^\]|^:|^,|\s$|[\x00-\x1f)" + commonRange);
  // needsEscapeML tests if the string can be written as a multiline string (like needsEscape but without \n, \r, \\, \", \t)
  e.needsEscapeML.imbue(e.loc);
  e.needsEscapeML.assign(R"('''|^[\s]+$|[\x00-\x08\x0b\x0c\x0e-\x1f)" + commonRange);
  // starts with a keyword and optionally is followed by a comment
  e.startsWithKeyword.imbue(e.loc);
  e.startsWithKeyword.assign(R"(^(true|false|null)\s*((,|\]|\}|#|//|/\*).*)?$)");
  e.needsEscapeName.imbue(e.loc);
  e.needsEscapeName.assign(R"([,\{\[\}\]\s:#"']|//|/\*)");
  e.lineBreak.assign(R"(\r|\n|\r\n)");

  _str(&e, v, true, "", true);

  return e.oss.str();
}


// Marshal returns the Hjson encoding of v using
// default options.
//
// See MarshalWithOptions.
//
std::string Marshal(const Value& v) {
  return MarshalWithOptions(v, DefaultOptions());
}


// MarshalJson returns the Json encoding of v using
// default options + "bracesSameLine", "quoteAlways", "quoteKeys" and
// "separator".
//
// See MarshalWithOptions.
//
std::string MarshalJson(const Value& v) {
  auto opt = DefaultOptions();

  opt.bracesSameLine = true;
  opt.quoteAlways = true;
  opt.quoteKeys = true;
  opt.separator = true;

  return MarshalWithOptions(v, opt);
}

}
