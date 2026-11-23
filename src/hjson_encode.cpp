#include "hjson.h"
#include <sstream>
#include <vector>
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
  int indent;
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


// The functions below decide how a string must be written. They scan the
// string byte by byte (they are not UTF-8 aware), in linear time and without
// recursion, so that strings of any length can be encoded.


// Returns true for the whitespace characters: space, \t, \n, \v, \f and \r.
static inline bool _isWhite(unsigned char c) {
  return c == ' ' || (c >= '\t' && c <= '\r');
}


// If text[i] is the first byte of one of the UTF-8 encoded characters that
// must always be escaped, the length of that character in bytes is returned.
// Otherwise 0 is returned. The characters are U+00AD, U+0600-U+0604, U+070F,
// U+17B4, U+17B5, U+200C, U+200F, U+2028-U+202F, U+2060-U+206F, U+FEFF and
// U+FFF0-U+FFFF.
static size_t _specialCharLen(const std::string& text, size_t i) {
  const size_t n = text.size() - i;
  if (n < 2) {
    return 0;
  }

  const unsigned char c0 = text[i], c1 = text[i + 1];

  switch (c0) {
  case 0xc2:
    return c1 == 0xad ? 2 : 0;
  case 0xd8:
    return (c1 >= 0x80 && c1 <= 0x84) ? 2 : 0;
  case 0xdc:
    return c1 == 0x8f ? 2 : 0;
  }

  if (n < 3) {
    return 0;
  }

  const unsigned char c2 = text[i + 2];

  switch (c0) {
  case 0xe1:
    return (c1 == 0x9e && (c2 == 0xb4 || c2 == 0xb5)) ? 3 : 0;
  case 0xe2:
    if (c1 == 0x80) {
      return (c2 == 0x8c || c2 == 0x8f || (c2 >= 0xa8 && c2 <= 0xaf)) ? 3 : 0;
    } else if (c1 == 0x81) {
      return (c2 >= 0xa0 && c2 <= 0xaf) ? 3 : 0;
    }
    return 0;
  case 0xef:
    if (c1 == 0xbb) {
      return c2 == 0xbf ? 3 : 0;
    } else if (c1 == 0xbf) {
      return (c2 >= 0xb0 && c2 <= 0xbf) ? 3 : 0;
    }
    return 0;
  }

  return 0;
}


// If text[i] is the first byte of a character that must be escaped in a
// quoted string, the length of that character in bytes is returned.
// Otherwise 0 is returned.
static size_t _escapeLen(const std::string& text, size_t i) {
  const unsigned char c = text[i];

  if (c < 0x20 || c == '"' || c == '\\') {
    return 1;
  }

  return _specialCharLen(text, i);
}


// Returns true if the string cannot be written without escapes.
static bool _needsEscape(const std::string& text) {
  for (size_t i = 0; i < text.size(); ++i) {
    if (_escapeLen(text, i)) {
      return true;
    }
  }

  return false;
}


// Returns true if the string cannot be written as a quoteless string.
static bool _needsQuotes(const std::string& text) {
  if (text.empty()) {
    return false;
  }

  const unsigned char first = text[0];

  if (_isWhite(first) || _isWhite(text.back())) {
    return true;
  }

  switch (first) {
  case '"':
  case '\'':
  case '#':
  case '{':
  case '}':
  case '[':
  case ']':
  case ':':
  case ',':
    return true;
  case '/':
    if (text.size() > 1 && (text[1] == '*' || text[1] == '/')) {
      return true;
    }
    break;
  }

  for (size_t i = 0; i < text.size(); ++i) {
    if (static_cast<unsigned char>(text[i]) < 0x20 || _specialCharLen(text, i)) {
      return true;
    }
  }

  return false;
}


// Returns true if the string cannot be written as a multiline string.
static bool _needsEscapeML(const std::string& text) {
  bool onlyWhite = !text.empty();

  for (size_t i = 0; i < text.size(); ++i) {
    const unsigned char c = text[i];

    if (!_isWhite(c)) {
      onlyWhite = false;
    }

    if ((c < 0x20 && c != '\t' && c != '\n' && c != '\r') ||
      (c == '\'' && i + 2 < text.size() && text[i + 1] == '\'' &&
      text[i + 2] == '\'') ||
      _specialCharLen(text, i))
    {
      return true;
    }
  }

  return onlyWhite;
}


// Returns true if the string starts with true, false or null, optionally
// followed by whitespace and then either nothing or a comment or one of the
// characters ,]} followed by anything except a line feed or carriage return.
// Such a string would not be read back as a string if written without quotes.
static bool _startsWithKeyword(const std::string& text) {
  size_t i;

  if (!text.compare(0, 4, "true") || !text.compare(0, 4, "null")) {
    i = 4;
  } else if (!text.compare(0, 5, "false")) {
    i = 5;
  } else {
    return false;
  }

  while (i < text.size() && _isWhite(text[i])) {
    ++i;
  }

  if (i == text.size()) {
    return true;
  }

  const char c = text[i];

  if (c != ',' && c != ']' && c != '}' && c != '#' && !(c == '/' &&
    i + 1 < text.size() && (text[i + 1] == '/' || text[i + 1] == '*')))
  {
    return false;
  }

  return text.find_first_of("\r\n", i) == std::string::npos;
}


// Returns true if the key cannot be written without quotes.
static bool _needsEscapeName(const std::string& text) {
  for (size_t i = 0; i < text.size(); ++i) {
    const unsigned char c = text[i];

    if (_isWhite(c)) {
      return true;
    }

    switch (c) {
    case ',':
    case '{':
    case '[':
    case '}':
    case ']':
    case ':':
    case '#':
    case '"':
    case '\'':
      return true;
    case '/':
      if (i + 1 < text.size() && (text[i + 1] == '/' || text[i + 1] == '*')) {
        return true;
      }
      break;
    }
  }

  return false;
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

  for (size_t i = 0; i < text.size();) {
    const size_t matchLength = _escapeLen(text, i);

    if (!matchLength) {
      ++i;
      continue;
    }

    const char *szReplacement = _meta(text[i]);

    if (i > uIndexStart) {
      // Append non-matching text.
      *e->os << text.substr(uIndexStart, i - uIndexStart);
    }

    if (szReplacement) {
      *e->os << szReplacement;
    } else {
      const char *pC = text.data() + i;
      size_t nS = matchLength;

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

    i += matchLength;
    uIndexStart = i;
  }

  if (uIndexStart < text.length()) {
    // Append remaining text.
    *e->os << text.substr(uIndexStart, text.length() - uIndexStart);
  }
}


// wrap the string into the ''' (multiline) format
static void _mlString(Encoder *e, const std::string& value) {
  size_t uIndexStart = 0;
  // Each \r and each \n is treated as a line break, so \r\n counts as two.
  size_t lineBreakPos = value.find_first_of("\r\n");

  if (lineBreakPos == std::string::npos) {
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
      auto indent = e->indent + 1;
      if (lineBreakPos == uIndexStart) {
        indent = 0;
      }
      _writeIndent(e, indent);
      if (lineBreakPos > uIndexStart) {
        *e->os << value.substr(uIndexStart, lineBreakPos - uIndexStart);
      }
      uIndexStart = lineBreakPos + 1;
      lineBreakPos = value.find_first_of("\r\n", uIndexStart);
    } while (lineBreakPos != std::string::npos);

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
    _needsQuotes(value) ||
    startsWithNumber(value.c_str(), value.size()) ||
    _startsWithKeyword(value) ||
    hasCommentAfter)
  {

    // If the string contains no control characters, no quote characters, and no
    // backslash characters, then we can safely slap some quotes around it.
    // Otherwise we first check if the string can be expressed in multiline
    // format or we must replace the offending characters with safe escape
    // sequences.

    if (!_needsEscape(value)) {
      if (bSep) {
        *e->os << " ";
      }
      *e->os << '"' << value << '"';
    } else if (!e->opt.quoteAlways && !_needsEscapeML(value) &&
      e->vParent.size() > 1)
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
  } else if (e->opt.quoteKeys || _needsEscapeName(name) || _needsEscape(name))
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
