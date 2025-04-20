#include "hjson.h"
#include <sstream>
#include <regex>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cmath>
#include <cctype>


namespace Hjson {


enum class EncodeState {
  ValueBegin,
  ValueEnd,
  VectorElemBegin,
  MapElemBegin,
};


class EncodeParent {
public:
  EncodeParent(const Value *_pVal) : pVal(_pVal), index(0), isEmpty(true) {}
  const Value *pVal;
  int index;
  bool isEmpty;
  std::string commentAfter;
  std::map<std::string, Value>::const_iterator it;
};


struct Encoder {
  EncoderOptions opt;
  std::ostream *os;
  std::locale loc;
  int indent;
  std::regex needsEscape, needsQuotes, needsEscapeML, startsWithKeyword,
    needsEscapeName, lineBreak;
  std::vector<EncodeState> vState;
  std::vector<EncodeParent> vParent;
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
  *e->os << e->opt.eol;

  if (!e->opt.indentBy.empty()) {
    for (int i = 0; i < indent; i++) {
      *e->os << e->opt.indentBy;
    }
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
static void _mlString(Encoder *e, const std::string& value) {
  size_t uIndexStart = 0;
  std::sregex_iterator it = std::sregex_iterator(value.begin(), value.end(),
    e->lineBreak);

  if (it == std::sregex_iterator()) {
    if (e->vState.size() > 1 && e->vState[e->vState.size() - 2] == EncodeState::MapElemBegin && (
      !e->opt.comments || e->vParent.back().pVal->get_comment_key().empty()))
    {
      *e->os << " ";
    }
    // The string contains only a single line. We still use the multiline
    // format as it avoids escaping the \ character (e.g. when used in a
    // regex).
    *e->os << "'''";
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
static void _quote(Encoder *e, const std::string& value,
  bool hasCommentAfter)
{
  bool bSep = false;
  if (e->vState.size() > 1 && e->vState[e->vState.size() - 2] == EncodeState::MapElemBegin && (
    !e->opt.comments || e->vParent.back().pVal->get_comment_key().empty()))
  {
    bSep = true;
  }

  if (value.size() == 0) {
    if (bSep) {
      *e->os << " ";
    }
    *e->os << "\"\"";
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
      if (bSep) {
        *e->os << " ";
      }
      *e->os << '"' << value << '"';
    } else if (!e->opt.quoteAlways && !std::regex_search(value,
      e->needsEscapeML) && e->vParent.size() > 1)
    {
      _mlString(e, value);
    } else {
      if (bSep) {
        *e->os << " ";
      }
      *e->os << '"';
      _quoteReplace(e, value);
      *e->os << '"';
    }
  } else {
    if (bSep) {
      *e->os << " ";
    }
    // return without quotes
    *e->os << value;
  }
}


static void _quoteName(Encoder *e, const std::string& name) {
  if (name.empty()) {
    *e->os << "\"\"";
  } else if (e->opt.quoteKeys || std::regex_search(name, e->needsEscapeName) ||
    std::regex_search(name, e->needsEscape))
  {
    *e->os << '"';
    _quoteReplace(e, name);
    *e->os << '"';
  } else {
    // without quotes
    *e->os << name;
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


// Returns true if we are inside a comment after outputting the string
// (i.e. the string contains an unterminated line comment).
// Also returns true for '/* # */' and similar, but that should be uncommon and
// will only cause an unnecessary line feed after the comment.
static bool _isInComment(const std::string& comment) {
  bool endsInsideComment = false;
  char prev = ' ';

  for (char ch : comment) {
    switch (ch) {
    case '\n':
      endsInsideComment = false;
      break;
    case '#':
      endsInsideComment = true;
      break;
    case '/':
      if (prev == '/') {
        endsInsideComment = true;
      }
      break;
    }
    prev = ch;
  }

  return endsInsideComment;
}


// Produce a string from value.
static void _writeValueBegin(Encoder *e) {
  const Value &value = *e->vParent.back().pVal;

  if (e->opt.comments) {
    *e->os << value.get_comment_key();
  }

  switch (value.type()) {
  case Type::Double:
    if (std::isnan(static_cast<double>(value)) || std::isinf(static_cast<double>(value))) {
      *e->os << Value(Type::Null).to_string();
    } else if (!e->opt.allowMinusZero && value == 0 && std::signbit(static_cast<double>(value))) {
      *e->os << Value(0).to_string();
    } else {
      *e->os << value.to_string();
    }
    break;

  case Type::String:
    _quote(e, value, _quoteForComment(e, value.get_comment_after()));
    break;

  case Type::Vector:
    *e->os << "[";
    e->indent++;
    e->vParent.back().commentAfter = value.get_comment_inside();
    e->vState.back() = EncodeState::VectorElemBegin;
    return;

  case Type::Map:
    if (!e->opt.omitRootBraces || e->vParent.size() > 1 || value.empty()) {
      *e->os << "{";
      e->indent++;
    }
    e->vParent.back().commentAfter = value.get_comment_inside();
    e->vParent.back().it = value.begin();
    e->vState.back() = EncodeState::MapElemBegin;
    return;

  default:
    *e->os << value.to_string();
  }

  e->vState.back() = EncodeState::ValueEnd;
}


static void _writeValueEnd(Encoder *e) {
  e->vState.pop_back();
  e->vParent.pop_back();
}


static void _writeVectorElemBegin(Encoder *e) {
  EncodeParent &ep = e->vParent.back();
  const Value &value = *ep.pVal;

  for (; ep.index < value.size(); ep.index++) {
    const Value &elem= value[ep.index];
    if (elem.defined()) {
      bool shouldIndent = (!e->opt.comments || elem.get_comment_key().empty());

      if (ep.isEmpty) {
        ep.isEmpty = false;

        if (e->opt.comments && !ep.commentAfter.empty()) {
          *e->os << ep.commentAfter;
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
          *e->os << ep.commentAfter;
        }
      }

      if (e->opt.comments && !elem.get_comment_before().empty()) {
        if (!e->opt.separator &&
          elem.get_comment_before().find("\n") == std::string::npos)
        {
          _writeIndent(e, e->indent);
        }
        *e->os << elem.get_comment_before();
      } else if (shouldIndent) {
        _writeIndent(e, e->indent);
      }

      ep.commentAfter = elem.get_comment_after();
      ep.index++;
      // Invalidates ep
      e->vParent.push_back(EncodeParent(&elem));
      e->vState.push_back(EncodeState::ValueBegin);
      return;
    }
  }
  if (e->opt.comments && !ep.commentAfter.empty()) {
    *e->os << ep.commentAfter;
  }
  if (!ep.isEmpty && (!e->opt.comments || ep.commentAfter.empty() ||
    !e->opt.separator && ep.commentAfter.find("\n") == std::string::npos))
  {
    _writeIndent(e, e->indent - 1);
  }

  *e->os << "]";
  e->indent--;
  e->vState.back() = EncodeState::ValueEnd;
}


static void _objElem(Encoder *e, const std::string& key, const Value& value, bool *pIsFirst,
  const std::string& commentAfterPrevObj)
{
  bool hasCommentBefore = (e->opt.comments && !value.get_comment_before().empty());

  if (*pIsFirst) {
    *pIsFirst = false;
    bool shouldIndent = ((!e->opt.omitRootBraces || e->vParent.size() > 1) && !hasCommentBefore);

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
    if (!hasCommentBefore || !e->opt.separator &&
      value.get_comment_before().find("\n") == std::string::npos)
    {
      _writeIndent(e, e->indent);
    }
  }

  if (hasCommentBefore) {
    *e->os << value.get_comment_before();
  }

  _quoteName(e, key);
  *e->os << ":";
  if (
    !e->opt.bracesSameLine
    && value.is_container()
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
    )
  {
    _writeIndent(e, e->indent);
  } else if (value.type() != Type::String && (!e->opt.comments || value.get_comment_key().empty())) {
    *e->os << " ";
  }
  e->vParent.push_back(EncodeParent(&value));
  e->vState.push_back(EncodeState::ValueBegin);
}


static void _writeMapElemBegin(Encoder *e) {
  EncodeParent &ep = e->vParent.back();
  const Value &value = *ep.pVal;

  if (e->opt.preserveInsertionOrder) {
    for (; ep.index < value.size(); ++ep.index) {
      const Value &elem = value[ep.index];
      if (elem.defined()) {
        int oldParentIndex = e->vParent.size() - 1;

        // Invalidates ep
        _objElem(e, value.key(ep.index), elem, &ep.isEmpty, ep.commentAfter);

        e->vParent[oldParentIndex].commentAfter = elem.get_comment_after();
        ++e->vParent[oldParentIndex].index;
        return;
      }
    }
  } else {
    for (; ep.it != value.end(); ++ep.it) {
      if (ep.it->second.defined()) {
        int oldParentIndex = e->vParent.size() - 1;
        auto oldIt = ep.it;

        // Invalidates ep
        _objElem(e, oldIt->first, oldIt->second, &ep.isEmpty, ep.commentAfter);

        e->vParent[oldParentIndex].commentAfter = oldIt->second.get_comment_after();
        ++e->vParent[oldParentIndex].it;
        return;
      }
    }
  }

  if (e->opt.comments && !ep.commentAfter.empty()) {
    *e->os << ep.commentAfter;
  }
  if (!ep.isEmpty && (!e->opt.omitRootBraces || e->vParent.size() > 1) &&
    (!e->opt.comments || ep.commentAfter.empty() ||
    !e->opt.separator && ep.commentAfter.find("\n") == std::string::npos))
  {
    _writeIndent(e, e->indent - 1);
  }

  if (!e->opt.omitRootBraces || e->vParent.size() > 1 || value.empty()) {
    e->indent--;
    if (e->vParent.size() == 1 && e->opt.comments && !ep.commentAfter.empty() &&
      _isInComment(ep.commentAfter))
    {
      _writeIndent(e, e->indent);
    }
    *e->os << "}";
  }

  e->vState.back() = EncodeState::ValueEnd;
}


static void _marshalLoop(Encoder *e, const Value &v) {
  while (!e->vState.empty()) {
    switch (e->vState.back()) {
    case EncodeState::ValueBegin:
      _writeValueBegin(e);
      break;
    case EncodeState::ValueEnd:
      _writeValueEnd(e);
      break;
    case EncodeState::VectorElemBegin:
      _writeVectorElemBegin(e);
      break;
    case EncodeState::MapElemBegin:
      _writeMapElemBegin(e);
      break;
    }
  }
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

  e.vParent.push_back(EncodeParent(&v));
  e.vState.push_back(EncodeState::ValueBegin);
  if (e.opt.comments) {
    *e.os << v.get_comment_before();
  }
  _marshalLoop(&e, v);
  if (e.opt.comments) {
    *e.os << v.get_comment_after();
  }
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
