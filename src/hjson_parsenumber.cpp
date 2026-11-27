#include "hjson.h"
#include <cmath>
#if HJSON_USE_CHARCONV
# include <charconv>
#elif HJSON_USE_STRTOD
# include <cstdlib>
# include <cerrno>
#else
# include <sstream>
#endif


namespace Hjson {


struct NumberParser {
  const unsigned char *data;
  size_t dataSize;
  int indexNext;
  unsigned char ch;
};


static bool _parseFloat(double *pNumber, const char *pCh, size_t nCh) {
#if HJSON_USE_CHARCONV
  auto res = std::from_chars(pCh, pCh + nCh, *pNumber);

  return res.ptr == pCh + nCh && res.ec != std::errc::result_out_of_range &&
    !std::isinf(*pNumber) && !std::isnan(*pNumber);
#elif HJSON_USE_STRTOD
  // strtod() reads until it finds a char that cannot be part of the number,
  // so it must get a copy that is zero terminated after nCh chars. Otherwise
  // it could read past the end of the input data.
  std::string str(pCh, nCh);
  char *endptr;
  errno = 0;
  *pNumber = std::strtod(str.c_str(), &endptr);

  return !errno && endptr - str.c_str() == nCh && !std::isinf(*pNumber) &&
    !std::isnan(*pNumber);
#else
  std::string str(pCh, nCh);
  std::stringstream ss(str);

  // Make sure we expect dot (not comma) as decimal point.
  ss.imbue(std::locale::classic());

  ss >> *pNumber;

  return ss.eof() && !ss.fail() && !std::isinf(*pNumber) && !std::isnan(*pNumber);
#endif
}


static bool _parseInt(std::int64_t *pNumber, const char *pCh, size_t nCh) {
#if HJSON_USE_CHARCONV
  auto res = std::from_chars(pCh, pCh + nCh, *pNumber);

  return res.ptr == pCh + nCh && res.ec != std::errc::result_out_of_range;
#elif HJSON_USE_STRTOD
  // Zero terminated copy, see _parseFloat().
  std::string str(pCh, nCh);
  char *endptr;
  errno = 0;
  *pNumber = std::strtoll(str.c_str(), &endptr, 0);

  return !errno && endptr - str.c_str() == nCh;
#else
  std::string str(pCh, nCh);
  std::stringstream ss(str);

  // Avoid localization surprises.
  ss.imbue(std::locale::classic());

  ss >> *pNumber;

  return ss.eof() && !ss.fail();
#endif
}


static bool _next(NumberParser *p) {
  // get the next character.
  if (p->indexNext < p->dataSize) {
    p->ch = p->data[p->indexNext++];
    return true;
  }

  if (p->indexNext == p->dataSize) {
    p->indexNext++;
  }

  p->ch = 0;

  return false;
}


// Returns true if the text is a number according to the Hjson syntax, without
// checking if the number fits in a double. If so, *pNumberSize is set to the
// number of chars in the number (not including any trailing whitespace or
// comment). Only the first "textSize" chars of "text" are read, "text" does
// not need to be zero terminated.
static bool _scanNumber(const char *text, size_t textSize, bool stopAtNext,
  size_t *pNumberSize)
{
  NumberParser p = {
    (const unsigned char*) text,
    textSize,
    0,
    ' '
  };

  int leadingZeros = 0;
  bool testLeading = true;

  _next(&p);

  if (p.ch == '-') {
    _next(&p);
  }

  while (p.ch >= '0' && p.ch <= '9') {
    if (testLeading) {
      if (p.ch == '0') {
        leadingZeros++;
      } else {
        testLeading = false;
      }
    }
    _next(&p);
  }

  if (testLeading) {
    leadingZeros--;
  } // single 0 is allowed

  if (p.ch == '.') {
    _next(&p);
    // A fraction needs at least one digit, otherwise the text is a quoteless
    // string (e.g. "1." or "1.e2").
    if (p.ch < '0' || p.ch > '9') {
      return false;
    }
    while (p.ch >= '0' && p.ch <= '9') {
      _next(&p);
    }
  }
  if (p.ch == 'e' || p.ch == 'E') {
    _next(&p);
    if (p.ch == '-' || p.ch == '+') {
      _next(&p);
    }
    // An exponent needs at least one digit.
    if (p.ch < '0' || p.ch > '9') {
      return false;
    }
    while (p.ch >= '0' && p.ch <= '9') {
      _next(&p);
    }
  }

  auto end = p.indexNext;

  // skip white/to (newline)
  while (p.ch > 0 && p.ch <= ' ') {
    _next(&p);
  }

  if (stopAtNext) {
    // end scan if we find a punctuator character like ,}] or a comment
    if (p.ch == ',' || p.ch == '}' || p.ch == ']' ||
      p.ch == '#' || (p.indexNext < textSize && p.ch == '/' &&
      (p.data[p.indexNext] == '/' || p.data[p.indexNext] == '*')))
    {
      p.ch = 0;
    }
  }

  if (p.ch > 0 || leadingZeros != 0) {
    // Invalid number.
    return false;
  }

  *pNumberSize = end - 1;

  return true;
}


// Parse a number value. Only the first "textSize" chars of "text" are read,
// "text" does not need to be zero terminated.
bool tryParseNumber(Value *pValue, const char *text, size_t textSize, bool stopAtNext) {
  size_t numberSize;
  if (!_scanNumber(text, textSize, stopAtNext, &numberSize)) {
    return false;
  }

  std::int64_t i;
  if (_parseInt(&i, text, numberSize)) {
    *pValue = Value(i);
    return true;
  } else {
    double d;
    if (_parseFloat(&d, text, numberSize)) {
      *pValue = Value(d);
      return true;
    }
  }

  return false;
}


// Returns true if the text would be read as a number by an Hjson parser. A
// number that does not fit in a double (e.g. "1e400") counts as a number
// here, even though this library reads it as a string: such a string must be
// quoted when it is written, because the Hjson syntax makes it a number.
bool startsWithNumber(const char *text, size_t textSize) {
  size_t numberSize;
  return _scanNumber(text, textSize, true, &numberSize);
}


}
