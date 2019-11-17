#include "hjson.h"
#include <sstream>
#include <vector>
#include <assert.h>
#include <cstring>


namespace Hjson {


typedef std::vector<Value> ValueVec;
typedef std::map<std::string, Value> ValueMap;


class Value::ValueImpl {
public:
  enum TypeImpl {
    IMPL_UNDEFINED,
    IMPL_HJSON_NULL,
    IMPL_BOOL,
    IMPL_DOUBLE,
    IMPL_STRING,
    IMPL_VECTOR,
    IMPL_MAP,
    IMPL_INT64
  };

  TypeImpl type;
  union {
    bool b;
    double d;
    std::int64_t i;
    void *p;
  };

  ValueImpl();
  ValueImpl(bool);
  ValueImpl(double);
  ValueImpl(std::int64_t, Int64_tag);
  ValueImpl(const std::string&);
  ValueImpl(Type);
  ~ValueImpl();
};


Value::ValueImpl::ValueImpl()
  : type(IMPL_UNDEFINED)
{
}


Value::ValueImpl::ValueImpl(bool input)
  : type(IMPL_BOOL),
  b(input)
{
}


Value::ValueImpl::ValueImpl(double input)
  : type(IMPL_DOUBLE),
  d(input)
{
}


Value::ValueImpl::ValueImpl(std::int64_t input, Int64_tag)
  : type(IMPL_INT64),
  i(input)
{
}


Value::ValueImpl::ValueImpl(const std::string &input)
  : type(IMPL_STRING),
  p(new std::string(input))
{
}


Value::ValueImpl::ValueImpl(Type _type) {
  switch (_type)
  {
  case UNDEFINED:
    type = IMPL_UNDEFINED;
    break;
  case HJSON_NULL:
    type = IMPL_HJSON_NULL;
    break;
  case BOOL:
    type = IMPL_BOOL;
    break;
  case DOUBLE:
    type = IMPL_DOUBLE;
    d = 0.0;
    break;
  case STRING:
    type = IMPL_STRING;
    p = new std::string();
    break;
  case VECTOR:
    type = IMPL_VECTOR;
    p = new ValueVec();
    break;
  case MAP:
    type = IMPL_MAP;
    p = new ValueMap();
    break;
  default:
    assert(!"Unknown type");
    break;
  }
}


Value::ValueImpl::~ValueImpl() {
  switch (type)
  {
  case IMPL_STRING:
    delete (std::string*) p;
    break;
  case IMPL_VECTOR:
    delete (ValueVec*)p;
    break;
  case IMPL_MAP:
    delete (ValueMap*)p;
    break;
  default:
    break;
  }
}


Value::Value(std::shared_ptr<ValueImpl> _prv)
  : prv(_prv)
{
}


// Sacrifice efficiency for predictability: It is allowed to do bracket
// assignment on an UNDEFINED Value, and thereby turn it into a MAP Value.
// A MAP Value is passed by reference, therefore an UNDEFINED Value should also
// be passed by reference, to avoid surprises when doing bracket assignment
// on a Value that has been passed around but is still of type UNDEFINED.
Value::Value()
  : prv(std::make_shared<ValueImpl>(UNDEFINED))
{
}


Value::Value(bool input)
  : prv(std::make_shared<ValueImpl>(input))
{
}


Value::Value(double input)
  : prv(std::make_shared<ValueImpl>(input))
{
}


Value::Value(int input)
  : prv(std::make_shared<ValueImpl>(input, Int64_tag{}))
{
}


Value::Value(std::int64_t input, Int64_tag)
  : prv(std::make_shared<ValueImpl>(input, Int64_tag{}))
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


Value::~Value() {
}


const Value Value::operator[](const std::string& name) const {
  if (prv->type == ValueImpl::IMPL_UNDEFINED) {
    return Value();
  } else if (prv->type == ValueImpl::IMPL_MAP) {
    auto it = ((ValueMap*)prv->p)->find(name);
    if (it == ((ValueMap*)prv->p)->end()) {
      return Value();
    }
    return it->second;
  }

  throw type_mismatch("Must be of type UNDEFINED or MAP for that operation.");
}


MapProxy Value::operator[](const std::string& name) {
  if (prv->type == ValueImpl::IMPL_UNDEFINED) {
    prv->~ValueImpl();
    // Recreate the private object using the same memory block.
    new(&(*prv)) ValueImpl(MAP);
  } else if (prv->type != ValueImpl::IMPL_MAP) {
    throw type_mismatch("Must be of type UNDEFINED or MAP for that operation.");
  }

  auto it = ((ValueMap*)prv->p)->find(name);
  if (it == ((ValueMap*)prv->p)->end()) {
    return MapProxy(prv, std::make_shared<ValueImpl>(UNDEFINED), name);
  }
  return MapProxy(prv, it->second.prv, name);
}


const Value Value::operator[](const char *input) const {
  return operator[](std::string(input));
}


MapProxy Value::operator[](const char *input) {
  return operator[](std::string(input));
}


const Value Value::operator[](int index) const {
  if (prv->type == ValueImpl::IMPL_UNDEFINED) {
    throw index_out_of_bounds("Index out of bounds.");
  } else if (prv->type != ValueImpl::IMPL_VECTOR) {
    throw type_mismatch("Must be of type UNDEFINED or VECTOR for that operation.");
  }

  if (index < 0 || index >= size()) {
    throw index_out_of_bounds("Index out of bounds.");
  }

  return ((const ValueVec*)prv->p)[0][index];
}


Value &Value::operator[](int index) {
  if (prv->type == ValueImpl::IMPL_UNDEFINED) {
    throw index_out_of_bounds("Index out of bounds.");
  } else if (prv->type != ValueImpl::IMPL_VECTOR) {
    throw type_mismatch("Must be of type UNDEFINED or VECTOR for that operation.");
  }

  if (index < 0 || index >= size()) {
    throw index_out_of_bounds("Index out of bounds.");
  }

  return ((ValueVec*)prv->p)[0][index];
}


bool Value::operator==(bool input) const {
  return operator bool() == input;
}


bool Value::operator!=(bool input) const {
  return !(*this == input);
}


bool Value::operator==(double input) const {
  return operator double() == input;
}


bool Value::operator!=(double input) const {
  return !(*this == input);
}


bool Value::operator==(int input) const {
  return (prv->type == ValueImpl::IMPL_INT64 ? prv->i == input : operator double() == input);
}


bool Value::operator!=(int input) const {
  return !(*this == input);
}


bool Value::operator==(const char *input) const {
  return !strcmp(operator const char*(), input);
}


bool Value::operator!=(const char *input) const {
  return !(*this == input);
}


bool Value::operator==(const std::string &input) const {
  return operator const std::string() == input;
}


bool Value::operator!=(const std::string &input) const {
  return !(*this == input);
}


bool Value::operator==(const Value &other) const {
  if (prv->type == ValueImpl::IMPL_DOUBLE && other.prv->type == ValueImpl::IMPL_INT64) {
    return prv->d == other.prv->i;
  } else if (prv->type == ValueImpl::IMPL_INT64 && other.prv->type == ValueImpl::IMPL_DOUBLE) {
    return prv->i == other.prv->d;
  }

  if (prv->type != other.prv->type) {
    return false;
  }

  switch (prv->type) {
  case ValueImpl::IMPL_UNDEFINED:
  case ValueImpl::IMPL_HJSON_NULL:
    return true;
  case ValueImpl::IMPL_BOOL:
    return prv->b == other.prv->b;
  case ValueImpl::IMPL_DOUBLE:
    return prv->d == other.prv->d;
  case ValueImpl::IMPL_STRING:
    return *((std::string*) prv->p) == *((std::string*)other.prv->p);
  case ValueImpl::IMPL_VECTOR:
  case ValueImpl::IMPL_MAP:
    return prv->p == other.prv->p;
  case ValueImpl::IMPL_INT64:
    return prv->i == other.prv->i;
  }

  assert(!"Unknown type");

  return false;
}


bool Value::operator!=(const Value &other) const {
  return !(*this == other);
}


bool Value::operator>(double input) const {
  return operator double() > input;
}


bool Value::operator<(double input) const {
  return operator double() < input;
}


bool Value::operator>(int input) const {
  return (prv->type == ValueImpl::IMPL_INT64 ? prv->i > input : operator double() > input);
}


bool Value::operator<(int input) const {
  return (prv->type == ValueImpl::IMPL_INT64 ? prv->i < input : operator double() < input);
}


bool Value::operator>(const char *input) const {
  return operator const std::string() > input;
}


bool Value::operator<(const char *input) const {
  return operator const std::string() < input;
}


bool Value::operator>(const std::string &input) const {
  return operator const std::string() > input;
}


bool Value::operator<(const std::string &input) const {
  return operator const std::string() < input;
}


bool Value::operator>(const Value &other) const {
  if (prv->type == ValueImpl::IMPL_DOUBLE && other.prv->type == ValueImpl::IMPL_INT64) {
    return prv->d > other.prv->i;
  } else if (prv->type == ValueImpl::IMPL_INT64 && other.prv->type == ValueImpl::IMPL_DOUBLE) {
    return prv->i > other.prv->d;
  }

  if (prv->type != other.prv->type) {
    throw type_mismatch("The compared values must be of the same type.");
  }

  switch (prv->type) {
  case ValueImpl::IMPL_DOUBLE:
    return prv->d > other.prv->d;
  case ValueImpl::IMPL_INT64:
    return prv->i > other.prv->i;
  case ValueImpl::IMPL_STRING:
    return *((std::string*)prv->p) > *((std::string*)other.prv->p);
  default:
    break;
  }

  throw type_mismatch("The compared values must be of type DOUBLE or STRING.");
}


bool Value::operator<(const Value &other) const {
  if (prv->type == ValueImpl::IMPL_DOUBLE && other.prv->type == ValueImpl::IMPL_INT64) {
    return prv->d < other.prv->i;
  } else if (prv->type == ValueImpl::IMPL_INT64 && other.prv->type == ValueImpl::IMPL_DOUBLE) {
    return prv->i < other.prv->d;
  }

  if (prv->type != other.prv->type) {
    throw type_mismatch("The compared values must be of the same type.");
  }

  switch (prv->type) {
  case ValueImpl::IMPL_DOUBLE:
    return prv->d < other.prv->d;
  case ValueImpl::IMPL_INT64:
    return prv->i < other.prv->i;
  case ValueImpl::IMPL_STRING:
    return *((std::string*)prv->p) < *((std::string*)other.prv->p);
  default:
    break;
  }

  throw type_mismatch("The compared values must be of type DOUBLE or STRING.");
}


double Value::operator+(int input) const {
  return operator double() + input;
}


double Value::operator+(double input) const {
  return operator double() + input;
}


std::string Value::operator+(const char *input) const {
  return operator const std::string() + input;
}


std::string Value::operator+(const std::string &input) const {
  return operator const std::string() + input;
}


Value Value::operator+(const Value &other) const {
  if (prv->type == ValueImpl::IMPL_DOUBLE && other.prv->type == ValueImpl::IMPL_INT64) {
    return prv->d + other.prv->i;
  } else if (prv->type == ValueImpl::IMPL_INT64 && other.prv->type == ValueImpl::IMPL_DOUBLE) {
    return prv->i + other.prv->d;
  }

  if (prv->type != other.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (prv->type) {
  case ValueImpl::IMPL_DOUBLE:
    return prv->d + other.prv->d;
  case ValueImpl::IMPL_INT64:
    return Value(prv->i + other.prv->i, Int64_tag{});
  case ValueImpl::IMPL_STRING:
    return *((std::string*)prv->p) + *((std::string*)other.prv->p);
  default:
    break;
  }

  throw type_mismatch("The values must be of type DOUBLE or STRING for this operation.");
}


double Value::operator-(int input) const {
  return operator double() - input;
}


double Value::operator-(double input) const {
  return operator double() - input;
}


double Value::operator-(const Value &other) const {
  return operator double() - other.operator double();
}


Value::operator bool() const {
  switch (prv->type)
  {
  case ValueImpl::IMPL_DOUBLE:
    return !!prv->d;
  case ValueImpl::IMPL_INT64:
    return !!prv->i;
  case ValueImpl::IMPL_BOOL:
    return prv->b;
  default:
    break;
  }

  return !empty();
}


Value::operator double() const {
  switch (prv->type)
  {
  case ValueImpl::IMPL_DOUBLE:
    return prv->d;
  case ValueImpl::IMPL_INT64:
    return static_cast<double>(prv->i);
  default:
    break;
  }

  throw type_mismatch("Must be of type DOUBLE for that operation.");

  return 0.0;
}


Value::operator const char*() const {
  if (prv->type != ValueImpl::IMPL_STRING) {
    throw type_mismatch("Must be of type STRING for that operation.");
  }

  return ((std::string*)(prv->p))->c_str();
}


Value::operator const std::string() const {
  if (prv->type != ValueImpl::IMPL_STRING) {
    throw type_mismatch("Must be of type STRING for that operation.");
  }

  return *((const std::string*)(prv->p));
}


bool Value::defined() const {
  return prv->type != ValueImpl::IMPL_UNDEFINED;
}


bool Value::empty() const {
  return (prv->type == ValueImpl::IMPL_UNDEFINED ||
    prv->type == ValueImpl::IMPL_HJSON_NULL ||
    (prv->type == ValueImpl::IMPL_STRING && ((std::string*)prv->p)->empty()) ||
    (prv->type == ValueImpl::IMPL_VECTOR && ((ValueVec*)prv->p)->empty()) ||
    (prv->type == ValueImpl::IMPL_MAP && ((ValueMap*)prv->p)->empty()));
}


Value::Type Value::type() const {
  switch (prv->type)
  {
  case ValueImpl::IMPL_UNDEFINED:
    return UNDEFINED;
  case ValueImpl::IMPL_HJSON_NULL:
    return HJSON_NULL;
  case ValueImpl::IMPL_BOOL:
    return BOOL;
  case ValueImpl::IMPL_DOUBLE:
  case ValueImpl::IMPL_INT64:
    // There is no public INT64.
    return DOUBLE;
  case ValueImpl::IMPL_STRING:
    return STRING;
  case ValueImpl::IMPL_VECTOR:
    return VECTOR;
  case ValueImpl::IMPL_MAP:
    return MAP;
  default:
    assert(!"Unknown type");
    break;
  }

  return UNDEFINED;
}


size_t Value::size() const {
  if (prv->type == ValueImpl::IMPL_UNDEFINED || prv->type == ValueImpl::IMPL_HJSON_NULL) {
    return 0;
  }

  switch (prv->type)
  {
  case ValueImpl::IMPL_STRING:
    return ((std::string*)prv->p)->size();
  case ValueImpl::IMPL_VECTOR:
    return ((ValueVec*)prv->p)->size();
  case ValueImpl::IMPL_MAP:
    return ((ValueMap*)prv->p)->size();
  default:
    break;
  }

  assert(prv->type == ValueImpl::IMPL_BOOL || prv->type == ValueImpl::IMPL_DOUBLE ||
    prv->type == ValueImpl::IMPL_INT64);

  return 1;
}


bool Value::deep_equal(const Value &other) const {
  if (*this == other) {
    return true;
  }

  if (this->type() != other.type() || this->size() != other.size()) {
    return false;
  }

  switch (prv->type)
  {
  case ValueImpl::IMPL_VECTOR:
    {
      auto itA = ((ValueVec*)(this->prv->p))->begin();
      auto endA = ((ValueVec*)(this->prv->p))->end();
      auto itB = ((ValueVec*)(other.prv->p))->begin();
      while (itA != endA) {
        if (!itA->deep_equal(*itB)) {
          return false;
        }
        ++itA;
        ++itB;
      }
    }
    return true;

  case ValueImpl::IMPL_MAP:
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
  case ValueImpl::IMPL_VECTOR:
    {
      Value ret;
      for (int index = 0; index < int(size()); ++index) {
        ret.push_back(operator[](index).clone());
      }
      return ret;
    }

  case ValueImpl::IMPL_MAP:
    {
      Value ret;
      for (auto it = begin(); it != end(); ++it) {
        ret[it->first] = it->second.clone();
      }
      return ret;
    }

  default:
    break;
  }

  return *this;
}


void Value::erase(int index) {
  if (prv->type != ValueImpl::IMPL_UNDEFINED && prv->type != ValueImpl::IMPL_VECTOR) {
    throw type_mismatch("Must be of type VECTOR for that operation.");
  } else if (index < 0 || index >= size()) {
    throw index_out_of_bounds("Index out of bounds.");
  }

  auto vec = (ValueVec*) prv->p;
  vec->erase(vec->begin() + index);
}


void Value::push_back(const Value &other) {
  if (prv->type == ValueImpl::IMPL_UNDEFINED) {
    prv->~ValueImpl();
    // Recreate the private object using the same memory block.
    new(&(*prv)) ValueImpl(VECTOR);
  } else if (prv->type != ValueImpl::IMPL_VECTOR) {
    throw type_mismatch("Must be of type UNDEFINED or VECTOR for that operation.");
  }

  ((ValueVec*)prv->p)->push_back(other);
}


ValueMap::iterator Value::begin() {
  if (prv->type != ValueImpl::IMPL_MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::iterator();
  }

  return ((ValueMap*)prv->p)->begin();
}


ValueMap::iterator Value::end() {
  if (prv->type != ValueImpl::IMPL_MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::iterator();
  }

  return ((ValueMap*)prv->p)->end();
}


ValueMap::const_iterator Value::begin() const {
  if (prv->type != ValueImpl::IMPL_MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::const_iterator();
  }

  return ((const ValueMap*)prv->p)->begin();
}


ValueMap::const_iterator Value::end() const {
  if (prv->type != ValueImpl::IMPL_MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::const_iterator();
  }

  return ((const ValueMap*)prv->p)->end();
}


size_t Value::erase(const std::string &key) {
  if (prv->type == ValueImpl::IMPL_UNDEFINED) {
    return 0;
  } else if (prv->type != ValueImpl::IMPL_MAP) {
    throw type_mismatch("Must be of type MAP for that operation.");
  }

  return ((ValueMap*)(prv->p))->erase(key);
}


size_t Value::erase(const char *key) {
  return erase(std::string(key));
}


double Value::to_double() const {
  switch (prv->type) {
  case ValueImpl::IMPL_UNDEFINED:
  case ValueImpl::IMPL_HJSON_NULL:
    return 0;
  case ValueImpl::IMPL_BOOL:
    return (prv->b ? 1 : 0);
  case ValueImpl::IMPL_DOUBLE:
    return prv->d;
  case ValueImpl::IMPL_INT64:
    return static_cast<double>(prv->i);
  case ValueImpl::IMPL_STRING:
    {
      double ret;
      std::stringstream ss(*((std::string*)prv->p));

      // Make sure we expect dot (not comma) as decimal point.
      ss.imbue(std::locale::classic());

      ss >> ret;

      if (!ss.eof() || ss.fail()) {
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
  case ValueImpl::IMPL_UNDEFINED:
  case ValueImpl::IMPL_HJSON_NULL:
    return 0;
  case ValueImpl::IMPL_BOOL:
    return (prv->b ? 1 : 0);
  case ValueImpl::IMPL_DOUBLE:
    return static_cast<std::int64_t>(prv->d);
  case ValueImpl::IMPL_INT64:
    return prv->i;
  case ValueImpl::IMPL_STRING:
    {
      std::int64_t ret;
      std::stringstream ss(*((std::string*)prv->p));

      ss >> ret;

      if (!ss.eof() || ss.fail()) {
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
  case ValueImpl::IMPL_UNDEFINED:
    return "";
  case ValueImpl::IMPL_HJSON_NULL:
    return "null";
  case ValueImpl::IMPL_BOOL:
    return (prv->b ? "true" : "false");
  case ValueImpl::IMPL_DOUBLE:
    {
      std::ostringstream oss;

      // Make sure we expect dot (not comma) as decimal point.
      oss.imbue(std::locale::classic());
      oss.precision(15);

      oss << prv->d;

      return oss.str();
    }
  case ValueImpl::IMPL_INT64:
    {
      std::ostringstream oss;

      oss << prv->i;

      return oss.str();
    }
  case ValueImpl::IMPL_STRING:
    return *((std::string*)prv->p);
  default:
    break;
  }

  throw type_mismatch("Illegal type for this operation.");
}


MapProxy::MapProxy(std::shared_ptr<ValueImpl> _parent,
  std::shared_ptr<ValueImpl> _child, const std::string &_key)
  : parentPrv(_parent),
    key(_key),
    wasAssigned(false)
{
  prv = _child;
}


MapProxy::~MapProxy() {
  if (wasAssigned || !empty()) {
    // We waited until now because we don't want to insert a Value object of
    // type UNDEFINED into the parent map, unless such an object was explicitly
    // assigned (e.g. `val["key"] = Hjson::Value()`).
    // Without this requirement, checking for the existence of an element
    // would create an UNDEFINED element for that key if it didn't already exist
    // (e.g. `if (val["key"] == 1) {` would create an element for "key").
    ((ValueMap*)parentPrv->p)[0][key] = Value(prv);
  }
}


MapProxy &MapProxy::operator =(const MapProxy &other) {
  prv = other.prv;
  wasAssigned = true;
  return *this;
}


MapProxy &MapProxy::operator =(const Value &other) {
  prv = other.prv;
  wasAssigned = true;
  return *this;
}


Value Merge(const Value base, const Value ext) {
  Value merged;

  if (!ext.defined()) {
    merged = base.clone();
  } else if (base.type() == Value::MAP && ext.type() == Value::MAP) {
    for (auto it = ext.begin(); it != ext.end(); ++it) {
      merged[it->first] = Merge(base[it->first], it->second);
    }

    for (auto it = base.begin(); it != base.end(); ++it) {
      if (!merged[it->first].defined()) {
        merged[it->first] = it->second.clone();
      }
    }
  } else {
    merged = ext.clone();
  }

  return merged;
}


}
