#include "hjson.h"
#include <sstream>
#include <cmath>


namespace Hjson {


struct Parser {
  const unsigned char *data;
  size_t dataSize;
  int at;
  unsigned char ch;
};


static bool _parseFloat(double *pNumber, const std::string &str) {
  std::stringstream ss(str);

  // Make sure we expect dot (not comma) as decimal point.
  ss.imbue(std::locale::classic());

  ss >> *pNumber;

  return ss.eof() && !ss.fail() && !std::isinf(*pNumber) && !std::isnan(*pNumber);
}


static bool _parseInt(std::int64_t *pNumber, const std::string &str) {
  std::stringstream ss(str);

  ss >> *pNumber;

  return ss.eof() && !ss.fail();
}


static bool _next(Parser *p) {
  // get the next character.
  if (p->at < p->dataSize) {
    p->ch = p->data[p->at++];
    return true;
  }

  if (p->at == p->dataSize) {
    p->at++;
    p->ch = 0;
  }

  return false;
}


// Parse a number value.
bool tryParseNumber(Value *pValue, const char *text, size_t textSize, bool stopAtNext) {
  Parser p = {
    (const unsigned char*) text,
    textSize,
    0,
    ' '
  };

  int leadingZeros = 0;
  bool testLeading = true;
  bool ret = false;
  bool isFloat = false;

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
    isFloat = true;
    while (_next(&p) && p.ch >= '0' && p.ch <= '9') {
    }
  }
  if (p.ch == 'e' || p.ch == 'E') {
    isFloat = true;
    _next(&p);
    if (p.ch == '-' || p.ch == '+') {
      _next(&p);
    }
    while (p.ch >= '0' && p.ch <= '9') {
      _next(&p);
    }
  }

  auto end = p.at;

  // skip white/to (newline)
  while (p.ch > 0 && p.ch <= ' ') {
    _next(&p);
  }

  if (stopAtNext) {
    // end scan if we find a punctuator character like ,}] or a comment
    if (p.ch == ',' || p.ch == '}' || p.ch == ']' ||
      p.ch == '#' || p.ch == '/' && (p.data[p.at] == '/' || p.data[p.at] == '*'))
    {
      p.ch = 0;
    }
  }

  if (p.ch > 0 || leadingZeros != 0) {
    // Invalid number.
    return false;
  }

  if (isFloat) {
    double d;
    ret = _parseFloat(&d, std::string((char*)p.data, end - 1));
    *pValue = Value(d);
  } else {
    std::int64_t i;
    ret = _parseInt(&i, std::string((char*)p.data, end - 1));
    *pValue = Value::from_int64(i);
  }

  return ret;
}


bool startsWithNumber(const char *text, size_t textSize) {
  Value number;
  return tryParseNumber(&number, text, textSize, true);
}


}
