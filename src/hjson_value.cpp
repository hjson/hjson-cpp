#include "hjson.h"
#include <vector>
#include <assert.h>
#include <cstring>
#include <algorithm>
#if HJSON_USE_CHARCONV
# include <charconv>
# include <array>
#elif HJSON_USE_STRTOD
# include <cstdlib>
# include <cerrno>
# include <cstdio>
#else
# include <sstream>
#endif


namespace Hjson {


typedef std::vector<std::string> KeyVec;
typedef std::vector<Value> ValueVec;
typedef std::map<std::string, Value> ValueMap;


class ValueVecMap {
public:
  KeyVec v;
  ValueMap m;
};


class Value::ValueImpl {
public:
  Type type;
  union {
    bool b;
    double d;
    std::int64_t i;
    std::string *s;
    ValueVec *v;
    ValueVecMap *m;
  };

  ValueImpl();
  ValueImpl(bool);
  ValueImpl(double);
  explicit ValueImpl(std::int64_t);
  ValueImpl(const std::string&);
  ValueImpl(Type);
  ~ValueImpl();
};


class Value::Comments {
public:
  std::string m_commentBefore, m_commentKey, m_commentInside, m_commentAfter;
};


Value::ValueImpl::ValueImpl()
  : type(Type::Undefined)
{
}


Value::ValueImpl::ValueImpl(bool input)
  : type(Type::Bool),
  b(input)
{
}


Value::ValueImpl::ValueImpl(double input)
  : type(Type::Double),
  d(input)
{
}


Value::ValueImpl::ValueImpl(std::int64_t input)
  : type(Type::Int64),
  i(input)
{
}


Value::ValueImpl::ValueImpl(const std::string &input)
  : type(Type::String),
  s(new std::string(input))
{
}


Value::ValueImpl::ValueImpl(Type _type)
  : type(_type)
{
  switch (_type)
  {
  case Type::String:
    s = new std::string();
    break;
  case Type::Vector:
    v = new ValueVec();
    break;
  case Type::Map:
    m = new ValueVecMap();
    break;
  default:
    break;
  }
}


Value::ValueImpl::~ValueImpl() {
  switch (type)
  {
  case Type::String:
    delete s;
    break;
  case Type::Vector:
    delete v;
    break;
  case Type::Map:
    delete m;
    break;
  default:
    break;
  }
}


// Sacrifice efficiency for predictability: It is allowed to do bracket
// assignment on an Undefined Value, and thereby turn it into a Map Value.
// A Map Value is passed by reference, therefore an Undefined Value should also
// be passed by reference, to avoid surprises when doing bracket assignment
// on a Value that has been passed around but is still of type Undefined.
Value::Value()
  : prv(std::make_shared<ValueImpl>(Type::Undefined))
{
}


Value::Value(bool input)
  : prv(std::make_shared<ValueImpl>(input))
{
}


Value::Value(float input)
  : prv(std::make_shared<ValueImpl>(static_cast<double>(input)))
{
}


Value::Value(double input)
  : prv(std::make_shared<ValueImpl>(input))
{
}


Value::Value(long double input)
  : prv(std::make_shared<ValueImpl>(static_cast<double>(input)))
{
}


Value::Value(char input)
  : prv(std::make_shared<ValueImpl>(static_cast<std::int64_t>(input)))
{
}


Value::Value(unsigned char input)
  : prv(std::make_shared<ValueImpl>(static_cast<std::int64_t>(input)))
{
}


Value::Value(short input)
  : prv(std::make_shared<ValueImpl>(static_cast<std::int64_t>(input)))
{
}


Value::Value(unsigned short input)
  : prv(std::make_shared<ValueImpl>(static_cast<std::int64_t>(input)))
{
}


Value::Value(int input)
  : prv(std::make_shared<ValueImpl>(static_cast<std::int64_t>(input)))
{
}


Value::Value(unsigned int input)
  : prv(std::make_shared<ValueImpl>(static_cast<std::int64_t>(input)))
{
}


Value::Value(long input)
  : prv(std::make_shared<ValueImpl>(static_cast<std::int64_t>(input)))
{
}


Value::Value(unsigned long input)
  : prv(std::make_shared<ValueImpl>(static_cast<std::int64_t>(input)))
{
}


Value::Value(long long input)
  : prv(std::make_shared<ValueImpl>(static_cast<std::int64_t>(input)))
{
}


Value::Value(unsigned long long input)
  : prv(std::make_shared<ValueImpl>(static_cast<std::int64_t>(input)))
{
}


Value::Value(const char *input)
  : prv(std::make_shared<ValueImpl>(std::string(input)))
{
}


Value::Value(const std::string& input)
  : prv(std::make_shared<ValueImpl>(input))
{
}


Value::Value(Type _type)
  : prv(std::make_shared<ValueImpl>(_type))
{
}


Value::Value(const Value& other)
  : prv(other.prv)
{
  if (other.cm) {
    // Clone the comments instead of sharing the reference. This way a change
    // in the other Value does not affect the comments in this Value.
    cm.reset(new Comments(*other.cm));
  }
}


Value::Value(Value&& other)
  : prv(other.prv),
    cm(other.cm)
{
}


// Even though the MapProxy is temporary, it contains references that are owned
// by a non-temporary object. Make sure the lvalue constructor is called.
Value::Value(MapProxy&& other)
  : Value(other)
{
}


Value::Value(std::shared_ptr<ValueImpl> _prv, std::shared_ptr<Comments> _cm)
  : prv(_prv),
    cm(_cm)
{
}


Value::~Value() {
}


Value& Value::operator=(const Value& other) {
  // So that comments are kept when assigning a Value to a new key in a map,
  // or to a variable that has not been assigned any other value yet.
  if (!this->defined()) {
    this->set_comments(other);
  }

  this->prv = other.prv;

  return *this;
}


Value& Value::operator=(Value&& other) {
  // So that comments are kept when assigning a Value to a new key in a map,
  // or to a variable that has not been assigned any other value yet.
  if (!this->defined()) {
    this->cm = other.cm;
  }

  this->prv = other.prv;

  return *this;
}


const Value& Value::at(const std::string& name) const {
  switch (prv->type)
  {
  case Type::Undefined:
    throw index_out_of_bounds("Key not found.");
  case Type::Map:
    try {
      return prv->m->m.at(name);
    } catch(const std::out_of_range&) {}
    throw index_out_of_bounds("Key not found.");
  default:
    throw type_mismatch("Must be of type Map for that operation.");
  }
}


Value& Value::at(const std::string& name) {
  switch (prv->type)
  {
  case Type::Undefined:
    throw index_out_of_bounds("Key not found.");
  case Type::Map:
    try {
      return prv->m->m.at(name);
    } catch(const std::out_of_range&) {}
    throw index_out_of_bounds("Key not found.");
  default:
    throw type_mismatch("Must be of type Map for that operation.");
  }
}


const Value& Value::at(const char *name) const {
  return at(std::string(name));
}


Value& Value::at(const char *name) {
  return at(std::string(name));
}


const Value Value::operator[](const std::string& name) const {
  if (prv->type == Type::Undefined) {
    return Value();
  } else if (prv->type == Type::Map) {
    auto it = prv->m->m.find(name);
    if (it == prv->m->m.end()) {
      return Value();
    }
    return it->second;
  }

  throw type_mismatch("Must be of type Undefined or Map for that operation.");
}


MapProxy Value::operator[](const std::string& name) {
  if (prv->type == Type::Undefined) {
    prv->~ValueImpl();
    // Recreate the private object using the same memory block.
    new(&(*prv)) ValueImpl(Type::Map);
  } else if (prv->type != Type::Map) {
    throw type_mismatch("Must be of type Undefined or Map for that operation.");
  }

  auto it = prv->m->m.find(name);
  if (it == prv->m->m.end()) {
    return MapProxy(prv, name, 0);
  }
  return MapProxy(prv, name, &it->second);
}


const Value Value::operator[](const char *input) const {
  return operator[](std::string(input));
}


MapProxy Value::operator[](const char *input) {
  return operator[](std::string(input));
}


const Value Value::operator[](char *input) const {
  return operator[](std::string(input));
}


MapProxy Value::operator[](char *input) {
  return operator[](std::string(input));
}


const Value& Value::operator[](int index) const {
  switch (prv->type)
  {
  case Type::Undefined:
    throw index_out_of_bounds("Index out of bounds.");
  case Type::Vector:
  case Type::Map:
    if (index < 0 || index >= size()) {
      throw index_out_of_bounds("Index out of bounds.");
    }

    switch (prv->type)
    {
    case Type::Vector:
      return prv->v[0][index];
    case Type::Map:
      {
        auto it = prv->m->m.find(prv->m->v[index]);
        assert(it != prv->m->m.end());
        return it->second;
      }
    default:
      break;
    }
  default:
    throw type_mismatch("Must be of type Undefined, Vector or Map for that operation.");
  }
}


Value& Value::operator[](int index) {
  switch (prv->type)
  {
  case Type::Undefined:
    throw index_out_of_bounds("Index out of bounds.");
  case Type::Vector:
  case Type::Map:
    if (index < 0 || index >= size()) {
      throw index_out_of_bounds("Index out of bounds.");
    }

    switch (prv->type)
    {
    case Type::Vector:
      return prv->v[0][index];
    case Type::Map:
      {
        auto it = prv->m->m.find(prv->m->v[index]);
        assert(it != prv->m->m.end());
        return it->second;
      }
    default:
      break;
    }
  default:
    throw type_mismatch("Must be of type Undefined, Vector or Map for that operation.");
  }
}


bool Value::operator==(bool input) const {
  return operator bool() == input;
}


bool Value::operator!=(bool input) const {
  return !(*this == input);
}


#define RET_VAL(_T, _O) \
Value operator _O(_T a, const Value& b) { \
  return Value(a) _O b; \
} \
Value operator _O(const Value& a, _T b) { \
  return a _O Value(b); \
}

#define RET_BOOL(_T, _O) \
bool operator _O(_T a, const Value& b) { \
  return Value(a) _O b; \
} \
bool operator _O(const Value& a, _T b) { \
  return a _O Value(b); \
}

#define HJSON_OP_IMPL_A(_T) \
RET_BOOL(_T, <) \
RET_BOOL(_T, >) \
RET_BOOL(_T, <=) \
RET_BOOL(_T, >=) \
RET_BOOL(_T, ==) \
RET_BOOL(_T, !=)

#define HJSON_OP_IMPL_B(_T) \
HJSON_OP_IMPL_A(_T) \
RET_VAL(_T, +) \
RET_VAL(_T, -) \
RET_VAL(_T, *) \
RET_VAL(_T, /)

#define HJSON_OP_IMPL_C(_T) \
HJSON_OP_IMPL_B(_T) \
RET_VAL(_T, %)

HJSON_OP_IMPL_A(const char*)
HJSON_OP_IMPL_A(const std::string&)
HJSON_OP_IMPL_B(float)
HJSON_OP_IMPL_B(double)
HJSON_OP_IMPL_B(long double)
HJSON_OP_IMPL_C(char)
HJSON_OP_IMPL_C(unsigned char)
HJSON_OP_IMPL_C(short)
HJSON_OP_IMPL_C(unsigned short)
HJSON_OP_IMPL_C(int)
HJSON_OP_IMPL_C(unsigned int)
HJSON_OP_IMPL_C(long)
HJSON_OP_IMPL_C(unsigned long)
HJSON_OP_IMPL_C(long long)
HJSON_OP_IMPL_C(unsigned long long)


std::string operator+(const char *a, const Value& b) {
  return std::string(a) + b.to_string();
}


std::string operator+(const Value& a ,const char *b) {
  return  a.to_string() + std::string(b);
}


std::string operator+(const std::string &a, const Value& b) {
  return a + b.to_string();
}


std::string operator+(const Value& a, const std::string &b) {
  return a.to_string() + b;
}


Value operator+(const Value& a, const Value& b) {
  if (a.prv->type == Type::Double && b.prv->type == Type::Int64) {
    return a.prv->d + b.prv->i;
  } else if (a.prv->type == Type::Int64 && b.prv->type == Type::Double) {
    return a.prv->i + b.prv->d;
  }

  if (a.prv->type != b.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (a.prv->type) {
  case Type::Double:
    return a.prv->d + b.prv->d;
  case Type::Int64:
    return a.prv->i + b.prv->i;
  case Type::String:
    return *a.prv->s + *b.prv->s;
  default:
    break;
  }

  throw type_mismatch("The values must be of type Double, Int64 or String for this operation.");
}


bool operator<(const Value& a, const Value& b) {
  if (a.prv->type == Type::Double && b.prv->type == Type::Int64) {
    return a.prv->d < b.prv->i;
  } else if (a.prv->type == Type::Int64 && b.prv->type == Type::Double) {
    return a.prv->i < b.prv->d;
  }

  if (a.prv->type != b.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (a.prv->type) {
  case Type::Double:
    return a.prv->d < b.prv->d;
  case Type::Int64:
    return a.prv->i < b.prv->i;
  case Type::String:
    return *a.prv->s < *b.prv->s;
  default:
    break;
  }

  throw type_mismatch("The values must be of type Double, Int64 or String for this operation.");
}


bool operator>(const Value& a, const Value& b) {
  if (a.prv->type == Type::Double && b.prv->type == Type::Int64) {
    return a.prv->d > b.prv->i;
  } else if (a.prv->type == Type::Int64 && b.prv->type == Type::Double) {
    return a.prv->i > b.prv->d;
  }

  if (a.prv->type != b.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (a.prv->type) {
  case Type::Double:
    return a.prv->d > b.prv->d;
  case Type::Int64:
    return a.prv->i > b.prv->i;
  case Type::String:
    return *a.prv->s > *b.prv->s;
  default:
    break;
  }

  throw type_mismatch("The values must be of type Double, Int64 or String for this operation.");
}


bool operator<=(const Value& a, const Value& b) {
  if (a.prv->type == Type::Double && b.prv->type == Type::Int64) {
    return a.prv->d <= b.prv->i;
  } else if (a.prv->type == Type::Int64 && b.prv->type == Type::Double) {
    return a.prv->i <= b.prv->d;
  }

  if (a.prv->type != b.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (a.prv->type) {
  case Type::Double:
    return a.prv->d <= b.prv->d;
  case Type::Int64:
    return a.prv->i <= b.prv->i;
  case Type::String:
    return *a.prv->s <= *b.prv->s;
  default:
    break;
  }

  throw type_mismatch("The values must be of type Double, Int64 or String for this operation.");
}


bool operator>=(const Value& a, const Value& b) {
  if (a.prv->type == Type::Double && b.prv->type == Type::Int64) {
    return a.prv->d >= b.prv->i;
  } else if (a.prv->type == Type::Int64 && b.prv->type == Type::Double) {
    return a.prv->i >= b.prv->d;
  }

  if (a.prv->type != b.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (a.prv->type) {
  case Type::Double:
    return a.prv->d >= b.prv->d;
  case Type::Int64:
    return a.prv->i >= b.prv->i;
  case Type::String:
    return *a.prv->s >= *b.prv->s;
  default:
    break;
  }

  throw type_mismatch("The values must be of type Double, Int64 or String for this operation.");
}


bool operator==(const Value& a, const Value& b) {
  if (a.prv->type == Type::Double && b.prv->type == Type::Int64) {
    return a.prv->d == b.prv->i;
  } else if (a.prv->type == Type::Int64 && b.prv->type == Type::Double) {
    return a.prv->i == b.prv->d;
  }

  if (a.prv->type != b.prv->type) {
    return false;
  }

  switch (a.prv->type) {
  case Type::Undefined:
  case Type::Null:
    return true;
  case Type::Bool:
    return a.prv->b == b.prv->b;
  case Type::Double:
    return a.prv->d == b.prv->d;
  case Type::String:
    return *a.prv->s == *b.prv->s;
  case Type::Vector:
    return a.prv->v == b.prv->v;
  case Type::Map:
    return a.prv->m == b.prv->m;
  case Type::Int64:
    return a.prv->i == b.prv->i;
  }

  assert(!"Unknown type");

  return false;
}


bool operator!=(const Value& a, const Value& b) {
  return !(a == b);
}


Value operator-(const Value& a, const Value& b) {
  if (a.prv->type == Type::Double && b.prv->type == Type::Int64) {
    return a.prv->d - b.prv->i;
  } else if (a.prv->type == Type::Int64 && b.prv->type == Type::Double) {
    return a.prv->i - b.prv->d;
  }

  if (a.prv->type != b.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (a.prv->type) {
  case Type::Double:
    return a.prv->d - b.prv->d;
  case Type::Int64:
    return a.prv->i - b.prv->i;
  default:
    break;
  }

  throw type_mismatch("The values must be of type Double or Int64 for this operation.");
}


Value operator*(const Value& a, const Value& b) {
  if (a.prv->type == Type::Double && b.prv->type == Type::Int64) {
    return a.prv->d * b.prv->i;
  } else if (a.prv->type == Type::Int64 && b.prv->type == Type::Double) {
    return a.prv->i * b.prv->d;
  }

  if (a.prv->type != b.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (a.prv->type) {
  case Type::Double:
    return a.prv->d * b.prv->d;
  case Type::Int64:
    return a.prv->i * b.prv->i;
  default:
    break;
  }

  throw type_mismatch("The values must be of type Double or Int64 for this operation.");
}


Value operator/(const Value& a, const Value& b) {
  if (a.prv->type == Type::Double && b.prv->type == Type::Int64) {
    return a.prv->d / b.prv->i;
  } else if (a.prv->type == Type::Int64 && b.prv->type == Type::Double) {
    return a.prv->i / b.prv->d;
  }

  if (a.prv->type != b.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (a.prv->type) {
  case Type::Double:
    return a.prv->d / b.prv->d;
  case Type::Int64:
    return a.prv->i / b.prv->i;
  default:
    break;
  }

  throw type_mismatch("The values must be of type Double or Int64 for this operation.");
}


Value operator%(const Value& a, const Value& b) {
  if (a.prv->type != b.prv->type || a.prv->type != Type::Int64) {
    throw type_mismatch("The values must be of the Int64 type for this operation.");
  }

  return a.prv->i % b.prv->i;
}


#define OP_ASS(_O, _T) \
Value& Value::operator _O(_T b) { \
  return operator _O(Value(b)); \
}

#define HJSON_ASS_IMPL_A(_T) \
OP_ASS(+=, _T) \
OP_ASS(-=, _T) \
OP_ASS(*=, _T) \
OP_ASS(/=, _T)

#define HJSON_ASS_IMPL_B(_T) \
HJSON_ASS_IMPL_A(_T) \
OP_ASS(%=, _T)

HJSON_ASS_IMPL_A(float)
HJSON_ASS_IMPL_A(double)
HJSON_ASS_IMPL_A(long double)
HJSON_ASS_IMPL_B(char)
HJSON_ASS_IMPL_B(unsigned char)
HJSON_ASS_IMPL_B(short)
HJSON_ASS_IMPL_B(unsigned short)
HJSON_ASS_IMPL_B(int)
HJSON_ASS_IMPL_B(unsigned int)
HJSON_ASS_IMPL_B(long)
HJSON_ASS_IMPL_B(unsigned long)
HJSON_ASS_IMPL_B(long long)
HJSON_ASS_IMPL_B(unsigned long long)


Value& Value::operator+=(const char *b) {
  return operator+=(std::string(b));
}


Value& Value::operator+=(const std::string& b) {
  if (prv->type != Type::String) {
    throw type_mismatch("The value must be of type String for this operation.");
  }

  *prv->s += b;

  return *this;
}


Value& Value::operator+=(const Value& b) {
  if (prv->type == Type::Double && b.prv->type == Type::Int64) {
    prv->d += b.prv->i;
  } else if (prv->type == Type::Int64 && b.prv->type == Type::Double) {
    prv->i += static_cast<int64_t>(b.prv->d);
  } else {
    if (prv->type != b.prv->type) {
      throw type_mismatch("The values must be of the same type for this operation.");
    }

    switch (prv->type) {
    case Type::Double:
      prv->d += b.prv->d;
      break;
    case Type::Int64:
      prv->i += b.prv->i;
      break;
    case Type::String:
      *prv->s += *b.prv->s;
      break;
    default:
      throw type_mismatch("The values must be of type Double, Int64 or String for this operation.");
      break;
    }
  }

  return *this;
}


Value& Value::operator-=(const Value& b) {
  operator +=(-b);

  return *this;
}


Value& Value::operator*=(const Value& b) {
  if (prv->type == Type::Double && b.prv->type == Type::Int64) {
    prv->d *= b.prv->i;
  } else if (prv->type == Type::Int64 && b.prv->type == Type::Double) {
    prv->i = static_cast<int64_t>(prv->i * b.prv->d);
  } else {
    if (prv->type != b.prv->type) {
      throw type_mismatch("The values must be of the same type for this operation.");
    }

    switch (prv->type) {
    case Type::Double:
      prv->d *= b.prv->d;
      break;
    case Type::Int64:
      prv->i *= b.prv->i;
      break;
    default:
      throw type_mismatch("The values must be of type Double or Int64 for this operation.");
      break;
    }
  }

  return *this;
}


Value& Value::operator/=(const Value& b) {
  if (prv->type == Type::Double && b.prv->type == Type::Int64) {
    prv->d /= b.prv->i;
  } else if (prv->type == Type::Int64 && b.prv->type == Type::Double) {
    prv->i = static_cast<int64_t>(prv->i / b.prv->d);
  } else {
    if (prv->type != b.prv->type) {
      throw type_mismatch("The values must be of the same type for this operation.");
    }

    switch (prv->type) {
    case Type::Double:
      prv->d /= b.prv->d;
      break;
    case Type::Int64:
      prv->i /= b.prv->i;
      break;
    default:
      throw type_mismatch("The values must be of type Double or Int64 for this operation.");
      break;
    }
  }

  return *this;
}


Value& Value::operator%=(const Value& b) {
  if (prv->type != b.prv->type || prv->type != Type::Int64) {
    throw type_mismatch("The values must be of the Int64 type for this operation.");
  }

  prv->i %= b.prv->i;

  return *this;
}


Value Value::operator+() const {
  switch (prv->type) {
  case Type::Double:
    return prv->d;
  case Type::Int64:
    return prv->i;
  default:
    throw type_mismatch("The value must be of type Double or Int64 for this operation.");
    break;
  }

  return *this;
}


Value Value::operator-() const {
  switch (prv->type) {
  case Type::Double:
    return -prv->d;
  case Type::Int64:
    return -prv->i;
  default:
    throw type_mismatch("The value must be of type Double or Int64 for this operation.");
    break;
  }

  return *this;
}


Value& Value::operator++() {
  switch (prv->type) {
  case Type::Double:
    prv->d++;
    break;
  case Type::Int64:
    prv->i++;
    break;
  default:
    throw type_mismatch("The values must be of type Double or Int64 for this operation.");
    break;
  }

  return *this;
}


Value& Value::operator--() {
  switch (prv->type) {
  case Type::Double:
    prv->d--;
    break;
  case Type::Int64:
    prv->i--;
    break;
  default:
    throw type_mismatch("The values must be of type Double or Int64 for this operation.");
    break;
  }

  return *this;
}


Value Value::operator++(int) {
  Value ret;

  switch (prv->type) {
  case Type::Double:
    ret = prv->d;
    prv->d++;
    break;
  case Type::Int64:
    ret = prv->i;
    prv->i++;
    break;
  default:
    throw type_mismatch("The values must be of type Double or Int64 for this operation.");
    break;
  }

  return ret;
}


Value Value::operator--(int) {
  Value ret;

  switch (prv->type) {
  case Type::Double:
    ret = prv->d;
    prv->d--;
    break;
  case Type::Int64:
    ret = prv->i;
    prv->i--;
    break;
  default:
    throw type_mismatch("The values must be of type Double or Int64 for this operation.");
    break;
  }

  return ret;
}


Value::operator bool() const {
  switch (prv->type)
  {
  case Type::Double:
    return !!prv->d;
  case Type::Int64:
    return !!prv->i;
  case Type::Bool:
    return prv->b;
  default:
    break;
  }

  return !empty();
}


Value::operator float() const {
  return static_cast<float>(operator double());
}


Value::operator double() const {
  switch (prv->type)
  {
  case Type::Double:
    return prv->d;
  case Type::Int64:
    return static_cast<double>(prv->i);
  default:
    break;
  }

  throw type_mismatch("Must be of type Double or Int64 for that operation.");

  return 0.0;
}


Value::operator long double() const {
  return static_cast<long double>(operator double());
}


#define HJSON_CONV_INT_IMPL(_T) \
Value::operator _T() const { \
  return static_cast<_T>(operator long long()); \
}

HJSON_CONV_INT_IMPL(char)
HJSON_CONV_INT_IMPL(unsigned char)
HJSON_CONV_INT_IMPL(short)
HJSON_CONV_INT_IMPL(unsigned short)
HJSON_CONV_INT_IMPL(int)
HJSON_CONV_INT_IMPL(unsigned int)
HJSON_CONV_INT_IMPL(long)
HJSON_CONV_INT_IMPL(unsigned long)
HJSON_CONV_INT_IMPL(unsigned long long)


Value::operator long long() const {
  switch (prv->type)
  {
  case Type::Double:
    return static_cast<long long>(prv->d);
  case Type::Int64:
    return prv->i;
  default:
    break;
  }

  throw type_mismatch("Must be of type Double or Int64 for that operation.");

  return 0;
}


Value::operator const char*() const {
  if (prv->type != Type::String) {
    throw type_mismatch("Must be of type String for that operation.");
  }

  return prv->s->c_str();
}


Value::operator std::string() const {
  if (prv->type != Type::String) {
    throw type_mismatch("Must be of type String for that operation.");
  }

  return *prv->s;
}


bool Value::defined() const {
  return prv->type != Type::Undefined;
}


bool Value::empty() const {
  return (prv->type == Type::Undefined ||
    prv->type == Type::Null ||
    (prv->type == Type::String && prv->s->empty()) ||
    (prv->type == Type::Vector && prv->v->empty()) ||
    (prv->type == Type::Map && prv->m->m.empty()));
}


Type Value::type() const {
  return prv->type;
}


bool Value::is_container() const {
  return prv->type == Type::Vector || prv->type == Type::Map;
}


bool Value::is_numeric() const {
  return prv->type == Type::Double || prv->type == Type::Int64;
}


size_t Value::size() const {
  switch (prv->type)
  {
  case Type::Vector:
    return prv->v->size();
  case Type::Map:
    return prv->m->m.size();
  default:
    break;
  }

  return 0;
}


bool Value::deep_equal(const Value& other) const {
  if (*this == other) {
    return true;
  }

  if (this->type() != other.type() || this->size() != other.size()) {
    return false;
  }

  switch (prv->type)
  {
  case Type::Vector:
    {
      auto itA = this->prv->v->begin();
      auto endA = this->prv->v->end();
      auto itB = other.prv->v->begin();
      while (itA != endA) {
        if (!itA->deep_equal(*itB)) {
          return false;
        }
        ++itA;
        ++itB;
      }
    }
    return true;

  case Type::Map:
    {
      auto itA = this->begin(), endA = this->end(), itB = other.begin();
      while (itA != endA) {
        if (!itA->second.deep_equal(itB->second)) {
          return false;
        }
        ++itA;
        ++itB;
      }
    }
    return true;

  default:
    break;
  }

  return false;
}


Value Value::clone() const {
  switch (prv->type) {
  case Type::Vector:
    {
      Value ret;
      for (int index = 0; index < int(size()); ++index) {
        ret.push_back(operator[](index).clone());
      }
      ret.set_comments(*this);
      return ret;
    }

  case Type::Map:
    {
      Value ret;
      for (int index = 0; index < size(); ++index) {
        ret[key(index)] = operator[](index).clone();
      }
      ret.set_comments(*this);
      return ret;
    }

  default:
    break;
  }

  return *this;
}


void Value::clear() {
  switch (prv->type) {
  case Type::Vector:
    prv->v->clear();
    break;

  case Type::Map:
    prv->m->m.clear();
    prv->m->v.clear();
    break;

  default:
    break;
  }
}


void Value::erase(int index) {
  switch (prv->type)
  {
  case Type::Undefined:
  case Type::Vector:
  case Type::Map:
    if (index < 0 || index >= size()) {
      throw index_out_of_bounds("Index out of bounds.");
    }

    switch (prv->type)
    {
    case Type::Vector:
      {
        prv->v->erase(prv->v->begin() + index);
      }
      break;
    case Type::Map:
      {
        prv->m->m.erase(prv->m->v[index]);
        prv->m->v.erase(prv->m->v.begin() + index);
      }
      break;
    default:
      break;
    }
    break;

  default:
    throw type_mismatch("Must be of type Vector or Map for that operation.");
  }
}


void Value::push_back(const Value& other) {
  if (prv->type == Type::Undefined) {
    prv->~ValueImpl();
    // Recreate the private object using the same memory block.
    new(&(*prv)) ValueImpl(Type::Vector);
  } else if (prv->type != Type::Vector) {
    throw type_mismatch("Must be of type Undefined or Vector for that operation.");
  }

  prv->v->push_back(other);
}


void Value::move(int from, int to) {
  switch (prv->type)
  {
  case Type::Undefined:
  case Type::Vector:
  case Type::Map:
    if (from < 0 || to < 0 || from >= size() || to > size()) {
      throw index_out_of_bounds("Index out of bounds.");
    }

    if (from == to) {
      return;
    }

    switch (prv->type)
    {
    case Type::Vector:
      {
        auto it = prv->v->begin();

        prv->v->insert(it + to, it[from]);
        if (to < from) {
          ++from;
        }
        prv->v->erase(prv->v->begin() + from);
      }
      break;
    case Type::Map:
      {
        auto vec = &prv->m->v;
        auto it = vec->begin();

        vec->insert(it + to, it[from]);
        if (to < from) {
          ++from;
        }
        vec->erase(vec->begin() + from);
      }
      break;
    default:
      break;
    }
    break;

  default:
    throw type_mismatch("Must be of type Vector or Map for that operation.");
  }
}


std::string Value::key(int index) const {
  switch (prv->type)
  {
  case Type::Undefined:
  case Type::Map:
    if (index < 0 || index >= size()) {
      throw index_out_of_bounds("Index out of bounds.");
    }
    return prv->m->v[index];
  default:
    throw type_mismatch("Must be of type Map for that operation.");
  }
}


ValueMap::iterator Value::begin() {
  if (prv->type != Type::Map) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::iterator();
  }

  return prv->m->m.begin();
}


ValueMap::iterator Value::end() {
  if (prv->type != Type::Map) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::iterator();
  }

  return prv->m->m.end();
}


ValueMap::const_iterator Value::begin() const {
  if (prv->type != Type::Map) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::const_iterator();
  }

  return prv->m->m.begin();
}


ValueMap::const_iterator Value::end() const {
  if (prv->type != Type::Map) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::const_iterator();
  }

  return prv->m->m.end();
}


size_t Value::erase(const std::string &key) {
  if (prv->type == Type::Undefined) {
    return 0;
  } else if (prv->type != Type::Map) {
    throw type_mismatch("Must be of type Map for that operation.");
  }

  size_t ret = prv->m->m.erase(key);

  if (ret > 0) {
    auto v = &prv->m->v;
    auto it = std::find(v->begin(), v->end(), key);
    if (it == v->end()) {
      assert(!"Value found in map but not in vector");
    } else {
      v->erase(it);
    }
  }

  return ret;
}


size_t Value::erase(const char *key) {
  return erase(std::string(key));
}


double Value::to_double() const {
  switch (prv->type) {
  case Type::Undefined:
  case Type::Null:
    return 0.0;
  case Type::Bool:
    return (prv->b ? 1.0 : 0.0);
  case Type::Double:
    return prv->d;
  case Type::Int64:
    return static_cast<double>(prv->i);
  case Type::String:
    {
      double ret;

#if HJSON_USE_CHARCONV
      const char *pCh = prv->s->c_str();
      const char *pEnd = pCh + prv->s->size();

      auto res = std::from_chars(pCh, pEnd, ret);

      if (res.ptr != pEnd || res.ec == std::errc::result_out_of_range) {
#elif HJSON_USE_STRTOD
      const char *pCh = prv->s->c_str();
      char *endptr;
      errno = 0;

      ret = std::strtod(pCh, &endptr);

      if (errno || endptr - pCh != prv->s->size()) {
#else
      std::stringstream ss(*prv->s);

      // Make sure we expect dot (not comma) as decimal point.
      ss.imbue(std::locale::classic());

      ss >> ret;

      if (!ss.eof() || ss.fail()) {
#endif
        return 0.0;
      }

      return ret;
    }
  default:
    break;
  }

  throw type_mismatch("Illegal type for this operation.");
}


std::int64_t Value::to_int64() const {
  switch (prv->type) {
  case Type::Undefined:
  case Type::Null:
    return 0;
  case Type::Bool:
    return (prv->b ? 1 : 0);
  case Type::Double:
    return static_cast<std::int64_t>(prv->d);
  case Type::Int64:
    return prv->i;
  case Type::String:
    {
      std::int64_t ret;

#if HJSON_USE_CHARCONV
      const char *pCh = prv->s->c_str();
      const char *pEnd = pCh + prv->s->size();

      auto res = std::from_chars(pCh, pEnd, ret);

      if (res.ptr != pEnd || res.ec == std::errc::result_out_of_range) {
#elif HJSON_USE_STRTOD
      const char *pCh = prv->s->c_str();
      char *endptr;
      errno = 0;

      ret = std::strtoll(pCh, &endptr, 0);

      if (errno || endptr - pCh != prv->s->size()) {
#else
      std::stringstream ss(*prv->s);

      // Avoid localization surprises.
      ss.imbue(std::locale::classic());

      ss >> ret;

      if (!ss.eof() || ss.fail()) {
#endif
        // Perhaps the string contains a decimal point or exponential part.
        return static_cast<std::int64_t>(to_double());
      }

      return ret;
    }
  default:
    break;
  }

  throw type_mismatch("Illegal type for this operation.");
}


std::string Value::to_string() const {
  switch (prv->type) {
  case Type::Undefined:
    return "";
  case Type::Null:
    return "null";
  case Type::Bool:
    return (prv->b ? "true" : "false");
  case Type::Double:
    {
#if HJSON_USE_CHARCONV
      std::array<char, 32> buf;

      auto res = std::to_chars(buf.data(), buf.data() + buf.size(), prv->d);

      if (res.ptr - buf.data() >= buf.size() || res.ec != std::errc()) {
        return "";
      }

      // to_chars() does not set a zero termination, which is needed by strchr().
      *res.ptr = '\0';

      // Always output a decimal point.
      if (strchr(buf.data(), '.') == nullptr) {
        if (res.ptr - buf.data() >= buf.size() - 1) {
          return "";
        }
        *(res.ptr++) = '.';
        *(res.ptr++) = '0';
      }

      return std::string(buf.data(), res.ptr);
#elif HJSON_USE_STRTOD
      char buf[32];
      int nChars = snprintf(buf, sizeof(buf), "%.15g", prv->d);

      if (nChars < 0 || nChars >= static_cast<int>(sizeof(buf))) {
        return "";
      }

      // Always output a decimal point.
      if (strchr(buf, '.') == nullptr) {
        if (nChars >= static_cast<int>(sizeof(buf)) - 1) {
          return "";
        }
        buf[nChars++] = '.';
        buf[nChars++] = '0';
      }

      return std::string(buf, nChars);
#else
      std::ostringstream oss;

      // Make sure we use dot (not comma) as decimal point.
      oss.imbue(std::locale::classic());
      oss.precision(15);

      oss << prv->d;

      // Always output a decimal point. Done like this to avoid printing more
      // decimals than needed, which would be the result of using
      // std::ios::showpoint.
      if (oss.str().find_first_of('.', 0) == std::string::npos) {
        oss << ".0";
      }

      return oss.str();
#endif
    }
  case Type::Int64:
    {
#if HJSON_USE_CHARCONV
      std::array<char, 32> buf;

      auto res = std::to_chars(buf.data(), buf.data() + buf.size(), prv->i);

      if (res.ec != std::errc()) {
        return "";
      }

      return std::string(buf.data(), res.ptr);
#elif HJSON_USE_STRTOD
      return std::to_string(prv->i);
#else
      std::ostringstream oss;

      // Avoid localization surprises.
      oss.imbue(std::locale::classic());

      oss << prv->i;

      return oss.str();
#endif
    }
  case Type::String:
    return *prv->s;
  default:
    break;
  }

  throw type_mismatch("Illegal type for this operation.");
}


void Value::set_comment_before(const std::string& str) {
  if (!cm) {
    if (str.empty()) {
      return;
    }
    cm.reset(new Comments());
  }

  cm->m_commentBefore = str;
}


std::string Value::get_comment_before() const {
  if (cm) {
    return cm->m_commentBefore;
  }

  return "";
}


void Value::set_comment_key(const std::string& str) {
  if (!cm) {
    if (str.empty()) {
      return;
    }
    cm.reset(new Comments());
  }

  cm->m_commentKey = str;
}


std::string Value::get_comment_key() const {
  if (cm) {
    return cm->m_commentKey;
  }

  return "";
}


void Value::set_comment_inside(const std::string& str) {
  if (!cm) {
    if (str.empty()) {
      return;
    }
    cm.reset(new Comments());
  }

  cm->m_commentInside = str;
}


std::string Value::get_comment_inside() const {
  if (cm) {
    return cm->m_commentInside;
  }

  return "";
}


void Value::set_comment_after(const std::string& str) {
  if (!cm) {
    if (str.empty()) {
      return;
    }
    cm.reset(new Comments());
  }

  cm->m_commentAfter = str;
}


std::string Value::get_comment_after() const {
  if (cm) {
    return cm->m_commentAfter;
  }

  return "";
}


void Value::set_comments(const Value& other) {
  if (other.cm) {
    if (!cm) {
      cm.reset(new Comments());
    }

    *cm = *other.cm;
  } else {
    clear_comments();
  }
}


void Value::clear_comments() {
  cm.reset();
}


Value& Value::assign_with_comments(const Value& other) {
  // If this object is of type Undefined set_comments() will be called in the
  // assignment operator, no need to call it here.
  if (defined()) {
    set_comments(other);
  }
  return operator=(other);
}


Value& Value::assign_with_comments(Value&& other) {
  // If this object is of type Undefined set_comments() will be called in the
  // assignment operator, no need to call it here.
  if (defined()) {
    cm = other.cm;
  }
  return operator=(std::move(other));
}


MapProxy::MapProxy(std::shared_ptr<ValueImpl> _parent, const std::string &_key,
  Value *_pTarget)
  : Value(_pTarget ? _pTarget->prv : std::make_shared<ValueImpl>(Type::Undefined),
      _pTarget ? _pTarget->cm : 0),
    parentPrv(_parent),
    key(_key),
    pTarget(_pTarget),
    wasAssigned(false)
{
}


MapProxy::MapProxy(Value&& other)
  : Value(std::move(other))
{
}


MapProxy::~MapProxy() {
  if (wasAssigned || !empty()) {
    if (pTarget) {
      // Can have changed due to assignment.
      pTarget->prv = this->prv;
      // In case cm was 0 but now has been created by a call to set_comment_x.
      pTarget->cm = this->cm;
    } else {
      // If the key is new we must add it to the order vector also.
      parentPrv->m->v.push_back(key);

      // We waited until now because we don't want to insert a Value object of
      // type Undefined into the parent map, unless such an object was explicitly
      // assigned (e.g. `val["key"] = Hjson::Value()`).
      // Without this requirement, checking for the existence of an element
      // would create an Undefined element for that key if it didn't already exist
      // (e.g. `if (val["key"] == 1) {` would create an element for "key").
      parentPrv->m->m.emplace(key, Value(this->prv, this->cm));
    }
  }
}


MapProxy& MapProxy::operator =(const MapProxy &other) {
  return operator=(static_cast<Value>(other));
}


MapProxy& MapProxy::operator =(const Value& other) {
  Value::operator=(other);
  wasAssigned = true;
  return *this;
}


MapProxy& MapProxy::operator =(Value&& other) {
  Value::operator=(std::move(other));
  wasAssigned = true;
  return *this;
}


MapProxy& MapProxy::assign_with_comments(const MapProxy& other) {
  return assign_with_comments(static_cast<Value>(other));
}


MapProxy& MapProxy::assign_with_comments(const Value& other) {
  // If this object is of type Undefined set_comments() will be called in the
  // assignment operator, no need to call it here.
  if (defined()) {
    set_comments(other);
  }
  return operator=(other);
}


MapProxy& MapProxy::assign_with_comments(Value&& other) {
  // If this object is of type Undefined set_comments() will be called in the
  // assignment operator, no need to call it here.
  if (defined()) {
    cm = other.cm;
  }
  return operator=(std::move(other));
}


Value Merge(const Value& base, const Value& ext) {
  Value merged;

  if (!ext.defined()) {
    merged = base.clone();
  } else if (base.type() == Type::Map && ext.type() == Type::Map) {
    for (int index = 0; index < ext.size(); ++index) {
      if (base[ext.key(index)].defined()) {
        merged[ext.key(index)] = Merge(base[ext.key(index)], ext[index]);
      } else {
        merged[ext.key(index)] = ext[index].clone();
      }
    }

    for (int index = 0; index < base.size(); ++index) {
      if (!merged[base.key(index)].defined()) {
        merged[base.key(index)] = base[index].clone();
      }
    }

    merged.set_comments(ext);
  } else {
    merged = ext.clone();
  }

  return merged;
}


}
