#include "hjson.h"
#include <sstream>
#include <regex>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cctype>


namespace Hjson {


struct Encoder {
  EncoderOptions opt;
  std::ostream *os;
  std::locale loc;
  int indent;
  std::regex needsEscape, needsQuotes, needsEscapeML, startsWithKeyword,
    needsEscapeName, lineBreak;
};


bool startsWithNumber(const char *text, size_t textSize);
static void _objElem(Encoder *e, const std::string& key, const Value& value, bool *pIsFirst,
  bool isRootObject, const std::string& commentAfterPrevObj);


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
  *e->os << e->opt.eol;

  for (int i = 0; i < indent; i++) {
    *e->os << e->opt.indentBy;
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

  if (*pnS < nS) {
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


static void _quoteReplace(Encoder *e, const std::string& text) {
  size_t uIndexStart = 0;

  for (std::sregex_iterator it = std::sregex_iterator(text.begin(), text.end(),
    e->needsEscape); it != std::sregex_iterator(); ++it)
  {
    std::smatch match = *it;
    const char *szReplacement = _meta(match.str()[0]);

    if (size_t(match.position()) > uIndexStart) {
      // Append non-matching text.
      *e->os << text.substr(uIndexStart, match.position() - uIndexStart);
    }

    if (szReplacement) {
      *e->os << szReplacement;
    } else {
      const char *pC = text.data() + match.position();
      size_t nS = match.length();

      *e->os << std::hex << std::setfill('0');
      while (nS) {
        int nRet = _fromUtf8((const unsigned char**) &pC, &nS);
        if (nRet < 0) {
          // Not UTF8. Just dump it.
          *e->os << std::string(pC, nS);
          break;
        }
        *e->os << "\\u" << std::setw(4) << nRet;
      }
    }

    uIndexStart = match.position() + match.length();
  }

  if (uIndexStart < text.length()) {
    // Append remaining text.
    *e->os << text.substr(uIndexStart, text.length() - uIndexStart);
  }
}


// wrap the string into the ''' (multiline) format
static void _mlString(Encoder *e, const std::string& value, const char *separator) {
  size_t uIndexStart = 0;
  std::sregex_iterator it = std::sregex_iterator(value.begin(), value.end(),
    e->lineBreak);

  if (it == std::sregex_iterator()) {
    // The string contains only a single line. We still use the multiline
    // format as it avoids escaping the \ character (e.g. when used in a
    // regex).
    *e->os << separator << "'''";
    *e->os << value;
  } else {
    _writeIndent(e, e->indent + 1);
    *e->os << "'''";

    do {
      std::smatch match = *it;
      auto indent = e->indent + 1;
      if (match.position() == uIndexStart) {
        indent = 0;
      }
      _writeIndent(e, indent);
      if (size_t(match.position()) > uIndexStart) {
        *e->os << value.substr(uIndexStart, match.position() - uIndexStart);
      }
      uIndexStart = match.position() + match.length();
      ++it;
    } while (it != std::sregex_iterator());

    if (uIndexStart < value.length()) {
      // Append remaining text.
      _writeIndent(e, e->indent + 1);
      *e->os << value.substr(uIndexStart, value.length() - uIndexStart);
    } else {
      // Trailing line feed.
      _writeIndent(e, 0);
    }

    _writeIndent(e, e->indent + 1);
  }

  *e->os << "'''";
}


// Check if we can insert this string without quotes
// see hjson syntax (must not parse as true, false, null or number)
static void _quote(Encoder *e, const std::string& value, const char *separator,
  bool isRootObject, bool hasCommentAfter)
{
  if (value.size() == 0) {
    *e->os << separator << "\"\"";
  } else if (e->opt.quoteAlways ||
    std::regex_search(value, e->needsQuotes) ||
    startsWithNumber(value.c_str(), value.size()) ||
    std::regex_search(value, e->startsWithKeyword) ||
    hasCommentAfter)
  {

    // If the string contains no control characters, no quote characters, and no
    // backslash characters, then we can safely slap some quotes around it.
    // Otherwise we first check if the string can be expressed in multiline
    // format or we must replace the offending characters with safe escape
    // sequences.

    if (!std::regex_search(value, e->needsEscape)) {
      *e->os << separator << '"' << value << '"';
    } else if (!e->opt.quoteAlways && !std::regex_search(value,
      e->needsEscapeML) && !isRootObject)
    {
      _mlString(e, value, separator);
    } else {
      *e->os << separator << '"';
      _quoteReplace(e, value);
      *e->os << '"';
    }
  } else {
    // return without quotes
    *e->os << separator << value;
  }
}


static void _quoteName(Encoder *e, const std::string& name) {
  if (name.empty()) {
    *e->os << "\"\"";
  } else if (e->opt.quoteKeys || std::regex_search(name, e->needsEscapeName)) {
    *e->os << '"';
    if (std::regex_search(name, e->needsEscape)) {
      _quoteReplace(e, name);
    } else {
      *e->os << name;
    }

    *e->os << '"';
  } else {
    // without quotes
    *e->os << name;
  }
}


static void _bracesIndent(Encoder *e, bool isObjElement, const Value& value, const char *separator) {
  if (
    isObjElement
    && !e->opt.bracesSameLine
    && (
      !value.empty()
      || (
        e->opt.comments
        && !value.get_comment_inside().empty()
      )
    )
    && (
      !e->opt.comments
      || value.get_comment_key().empty()
    )
  ) {
    _writeIndent(e, e->indent);
  } else {
    *e->os << separator;
  }
}


static bool _quoteForComment(Encoder *e, const std::string& comment) {
  if (!e->opt.comments) {
    return false;
  }

  for (char ch : comment) {
    switch (ch)
    {
    case '\r':
    case '\n':
      return false;
    case '/':
    case '#':
      return true;
    default:
      break;
    }
  }

  return false;
}


// Produce a string from value.
static void _str(Encoder *e, const Value& value, bool isRootObject, bool isObjElement) {
  const char *separator = ((isObjElement && (!e->opt.comments ||
    value.get_comment_key().empty())) ? " " : "");

  if (e->opt.comments) {
    if (isRootObject) {
      *e->os << value.get_comment_before();
    }
    *e->os << value.get_comment_key();
  }

  switch (value.type()) {
  case Type::Double:
    *e->os << separator;

    if (std::isnan(static_cast<double>(value)) || std::isinf(static_cast<double>(value))) {
      *e->os << Value(Type::Null).to_string();
    } else if (!e->opt.allowMinusZero && value == 0 && std::signbit(static_cast<double>(value))) {
      *e->os << Value(0).to_string();
    } else {
      *e->os << value.to_string();
    }
    break;

  case Type::String:
    _quote(e, value, separator, isRootObject, _quoteForComment(e, value.get_comment_after()));
    break;

  case Type::Vector:
    {
      _bracesIndent(e, isObjElement, value, separator);
      *e->os << "[";

      e->indent++;

      // Join all of the element texts together, separated with newlines
      bool isFirst = true;
      std::string commentAfter = value.get_comment_inside();
      for (int i = 0; size_t(i) < value.size(); ++i) {
        if (value[i].defined()) {
          bool shouldIndent = (!e->opt.comments || value[i].get_comment_key().empty());

          if (isFirst) {
            isFirst = false;

            if (e->opt.comments && !commentAfter.empty()) {
              *e->os << commentAfter;
              // This is the first element, so commentAfterPrevObj is the inner comment
              // of the parent vector. The inner comment probably expects "]" to come
              // after it and therefore needs one more level of indentation.
              *e->os << e->opt.indentBy;
              shouldIndent = false;
            }
          } else {
            if (e->opt.separator) {
              *e->os << ",";
            }

            if (e->opt.comments) {
              *e->os << commentAfter;
            }
          }

          if (e->opt.comments && !value[i].get_comment_before().empty()) {
            *e->os << value[i].get_comment_before();
          } else if (shouldIndent) {
            _writeIndent(e, e->indent);
          }

          _str(e, value[i], false, false);

          commentAfter = value[i].get_comment_after();
        }
      }

      if (e->opt.comments && !commentAfter.empty()) {
        *e->os << commentAfter;
      } else if (!value.empty()) {
        _writeIndent(e, e->indent - 1);
      }

      *e->os << "]";
      e->indent--;
    }
    break;

  case Type::Map:
    {
      if (!e->opt.omitRootBraces || !isRootObject || value.empty()) {
        _bracesIndent(e, isObjElement, value, separator);
        *e->os << "{";

        e->indent++;
      }

      // Join all of the member texts together, separated with newlines
      bool isFirst = true;
      std::string commentAfter = value.get_comment_inside();
      if (e->opt.preserveInsertionOrder) {
        size_t limit = value.size();
        for (int index = 0; index < limit; index++) {
          if (value[index].defined()) {
            _objElem(e, value.key(index), value[index], &isFirst, isRootObject, commentAfter);
            commentAfter = value[index].get_comment_after();
          }
        }
      } else {
        for (auto it : value) {
          if (it.second.defined()) {
            _objElem(e, it.first, it.second, &isFirst, isRootObject, commentAfter);
            commentAfter = it.second.get_comment_after();
          }
        }
      }

      if (e->opt.comments && !commentAfter.empty()) {
        *e->os << commentAfter;
      } else if (!value.empty() && (!e->opt.omitRootBraces || !isRootObject)) {
        _writeIndent(e, e->indent - 1);
      }

      if (!e->opt.omitRootBraces || !isRootObject || value.empty()) {
        e->indent--;
        *e->os << "}";
      }
    }
    break;

  default:
    *e->os << separator << value.to_string();
  }

  if (e->opt.comments && isRootObject) {
    *e->os << value.get_comment_after();
  }
}


static void _objElem(Encoder *e, const std::string& key, const Value& value, bool *pIsFirst,
  bool isRootObject, const std::string& commentAfterPrevObj)
{
  bool hasCommentBefore = (e->opt.comments && !value.get_comment_before().empty());

  if (*pIsFirst) {
    *pIsFirst = false;
    bool shouldIndent = ((!e->opt.omitRootBraces || !isRootObject) && !hasCommentBefore);

    if (e->opt.comments && !commentAfterPrevObj.empty()) {
      *e->os << commentAfterPrevObj;
      // This is the first element, so commentAfterPrevObj is the inner comment
      // of the parent map. The inner comment probably expects "}" to come
      // after it and therefore needs one more level of indentation, unless
      // this is the root object without braces.
      if (shouldIndent) {
        *e->os << e->opt.indentBy;
      }
    } else if (shouldIndent) {
      _writeIndent(e, e->indent);
    }
  } else {
    if (e->opt.separator) {
      *e->os << ",";
    }
    if (e->opt.comments) {
      *e->os << commentAfterPrevObj;
    }
    if (!hasCommentBefore) {
      _writeIndent(e, e->indent);
    }
  }

  if (hasCommentBefore) {
    *e->os << value.get_comment_before();
  }

  _quoteName(e, key);
  *e->os << ":";
  _str(
    e,
    value,
    false,
    true
  );
}


static void _marshalStream(const Value& v, const EncoderOptions& options,
  std::ostream *pStream)
{
  Encoder e;
  e.os = pStream;
  e.opt = options;
  e.indent = 0;

  if (e.opt.separator) {
    e.opt.quoteAlways = true;
  }

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

  _str(&e, v, true, false);
}


// Marshal returns the Hjson encoding of v.
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
// string. The map keys are used as JSON object keys.
//
// JSON cannot represent cyclic data structures and Marshal does not
// handle them. Passing cyclic structures to Marshal will result in
// an infinite recursion.
//
std::string Marshal(const Value& v, const EncoderOptions& options) {
  std::ostringstream oss;

  _marshalStream(v, options, &oss);

  return oss.str();
}


void MarshalToFile(const Value& v, const std::string &path, const EncoderOptions& options) {
  std::ofstream outputFile(path, std::ofstream::binary);
  if (!outputFile.is_open()) {
    throw file_error("Could not open file '" + path + "' for writing");
  }
  _marshalStream(v, options, &outputFile);
  outputFile << options.eol;
  outputFile.close();
}


// MarshalJson returns the Json encoding of v using
// default options + "bracesSameLine", "quoteAlways", "quoteKeys" and
// "separator".
//
// See Marshal.
//
std::string MarshalJson(const Value& v) {
  EncoderOptions opt;

  opt.bracesSameLine = true;
  opt.quoteAlways = true;
  opt.quoteKeys = true;
  opt.separator = true;
  opt.comments = false;

  return Marshal(v, opt);
}


std::ostream &operator <<(std::ostream& out, const Value& v) {
  _marshalStream(v, EncoderOptions(), &out);
  return out;
}


StreamEncoder::StreamEncoder(const Value& _v, const EncoderOptions& _o)
  : v(_v), o(_o)
{
}


std::ostream &operator <<(std::ostream& out, const StreamEncoder& se) {
  _marshalStream(se.v, se.o, &out);
  return out;
}


}
