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
  Type type;
  std::string commentPre;      // multiple line prefix-like commentary before key
  std::string commentPost;     // suffix-like commentary in same line as key
  std::string commentInside;   // commentary only used for object and array
  union {
    bool b;
    double d;
    ValueMap* m;
    ValueVec* v;
    std::string* s;
  };

  ValueImpl();
  ValueImpl(bool);
  ValueImpl(double);
  ValueImpl(const std::string&);
  ValueImpl(Type);
  ~ValueImpl();
};


Value::ValueImpl::ValueImpl()
  : type(UNDEFINED)
{
}


Value::ValueImpl::ValueImpl(bool input)
  : type(BOOL),
  b(input)
{
}


Value::ValueImpl::ValueImpl(double input)
  : type(DOUBLE),
  d(input)
{
}


Value::ValueImpl::ValueImpl(const std::string &input)
  : type(STRING),
  s(new std::string(input))
{
}


Value::ValueImpl::ValueImpl(Type _type)
  : type(_type)
{
  switch (_type)
  {
  case DOUBLE:
    d = 0.0;
    break;
  case STRING:
    s = new std::string();
    break;
  case VECTOR:
    v = new ValueVec();
    break;
  case MAP:
    m = new ValueMap();
    break;
  default:
    break;
  }
}


Value::ValueImpl::~ValueImpl() {
  switch (type)
  {
  case STRING:
    delete s;
    break;
  case VECTOR:
    delete v;
    break;
  case MAP:
    delete m;
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
  : prv(std::make_shared<ValueImpl>(static_cast<double>(input)))
{
}


Value::Value(unsigned input)
  : prv(std::make_shared<ValueImpl>(static_cast<double>(input)))
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


bool Value::exists(const std::vector<std::string>& keyChain) const {
  return get(keyChain).defined();
}

bool Value::exists(const std::string& key) const {
  return get(std::vector<std::string>{ key }).defined();
}


// Try reading given key chain and return as specified type. Returns false on error.
// Explicit overload needed, because calling static_cast<std::string> is ambiguous
bool Value::getAs(std::string& val, const std::vector<std::string>& keyChain) const {
  try {
    val = get(keyChain).operator std::string();
    return true;
  }
  catch (...) {
    return false;
  }
}


Value Value::get(const std::vector<std::string>& keyChain) const {
  Value val;
  const Value* valPtr = this;
  if (keyChain.empty())
    return Value(*this);
  for (const auto& key : keyChain) {
    if (valPtr->prv->type != MAP)
      return Value();
    val = valPtr->operator[](key);
    valPtr = &val;
  }
  return val;
}


Value Value::get(const std::string& key) const {
  return get(std::vector<std::string>{ key });
}


void Value::set(const Value& val, const std::vector<std::string>& keyChain) {
  std::vector<MapProxy> proxyChain;
  Value* valPtr = this;
  if (keyChain.empty()) {
    *this = val;
    return;
  }
  for (const auto& key : keyChain) {
    if (valPtr->prv->type != MAP) {
      valPtr->prv->~ValueImpl();
      // Recreate the private object using the same memory block.
      new(&(*valPtr->prv)) ValueImpl(MAP);
    }
    proxyChain.push_back(valPtr->operator[](key));
    valPtr = &proxyChain.back();
  }
  // assign new value
  proxyChain.back() = val;
  // proxy destruction order is relevant for new keys
  for (auto i = proxyChain.size(); i > 0; --i)
    proxyChain.pop_back();
}


void Value::set(const Value& val, const std::string& key) {
  set(val, std::vector<std::string>{ key });
}


const Value Value::operator[](const std::string& name) const {
  if (prv->type == UNDEFINED) {
    return Value();
  } else if (prv->type == MAP) {
    auto it = prv->m->find(name);
    if (it == prv->m->end()) {
      return Value();
    }
    return it->second;
  }

  throw type_mismatch("Must be of type UNDEFINED or MAP for that operation.");
}


MapProxy Value::operator[](const std::string& name) {
  if (prv->type == UNDEFINED) {
    prv->~ValueImpl();
    // Recreate the private object using the same memory block.
    new(&(*prv)) ValueImpl(MAP);
  } else if (prv->type != MAP) {
    throw type_mismatch("Must be of type UNDEFINED or MAP for that operation.");
  }

  auto it = prv->m->find(name);
  if (it == prv->m->end()) {
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
  if (prv->type == UNDEFINED) {
    throw index_out_of_bounds("Index out of bounds.");
  } else if (prv->type != VECTOR) {
    throw type_mismatch("Must be of type UNDEFINED or VECTOR for that operation.");
  }

  if (index < 0 || static_cast<size_t>(index) >= size()) {
    throw index_out_of_bounds("Index out of bounds.");
  }

  return prv->v[0][index];
}


Value &Value::operator[](int index) {
  if (prv->type == UNDEFINED) {
    throw index_out_of_bounds("Index out of bounds.");
  } else if (prv->type != VECTOR) {
    throw type_mismatch("Must be of type UNDEFINED or VECTOR for that operation.");
  }

  if (index < 0 || static_cast<size_t>(index) >= size()) {
    throw index_out_of_bounds("Index out of bounds.");
  }

  return prv->v[0][index];
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
  return operator double() == input;
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
  return operator std::string() == input;
}


bool Value::operator!=(const std::string &input) const {
  return !(*this == input);
}


bool Value::operator==(const Value &other) const {
  if (prv->type != other.prv->type) {
    return false;
  }

  switch (prv->type) {
  case UNDEFINED:
  case HJSON_NULL:
    return true;
  case BOOL:
    return prv->b == other.prv->b;
  case DOUBLE:
    return prv->d == other.prv->d;
  case STRING:
    return *(prv->s) == *(other.prv->s);
  case VECTOR:
  case MAP:
    return prv->m == other.prv->m;
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
  return operator double() > input;
}


bool Value::operator<(int input) const {
  return operator double() < input;
}


bool Value::operator>(const char *input) const {
  return operator std::string() > input;
}


bool Value::operator<(const char *input) const {
  return operator std::string() < input;
}


bool Value::operator>(const std::string &input) const {
  return operator std::string() > input;
}


bool Value::operator<(const std::string &input) const {
  return operator std::string() < input;
}


bool Value::operator>(const Value &other) const {
  if (prv->type != other.prv->type) {
    throw type_mismatch("The compared values must be of the same type.");
  }

  switch (prv->type) {
  case DOUBLE:
    return prv->d > other.prv->d;
  case STRING:
    return *(prv->s) > *(other.prv->s);
  default:
    throw type_mismatch("The compared values must be of type DOUBLE or STRING.");;
  }
}


bool Value::operator<(const Value &other) const {
  if (prv->type != other.prv->type) {
    throw type_mismatch("The compared values must be of the same type.");
  }

  switch (prv->type) {
  case DOUBLE:
    return prv->d < other.prv->d;
  case STRING:
    return *(prv->s) < *(other.prv->s);
  default:
    throw type_mismatch("The compared values must be of type DOUBLE or STRING.");
  }
}


double Value::operator+(int input) const {
  return operator double() + input;
}


double Value::operator+(double input) const {
  return operator double() + input;
}


std::string Value::operator+(const char *input) const {
  return operator std::string() + input;
}


std::string Value::operator+(const std::string &input) const {
  return operator std::string() + input;
}


Value Value::operator+(const Value &other) const {
  if (prv->type != other.prv->type) {
    throw type_mismatch("The values must be of the same type for this operation.");
  }

  switch (prv->type) {
  case DOUBLE:
    return prv->d + other.prv->d;
  case STRING:
    return *(prv->s) + *(other.prv->s);
  default:
    throw type_mismatch("The values must be of type DOUBLE or STRING for this operation.");
  }
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
  return (prv->type == DOUBLE ? !!prv->d : (prv->type == BOOL ? prv->b : !empty()));
}


Value::operator double() const {
  if (prv->type != DOUBLE) {
    throw type_mismatch("Must be of type DOUBLE for that operation.");
  }

  return prv->d;
}


Value::operator const char*() const {
  if (prv->type != STRING) {
    throw type_mismatch("Must be of type STRING for that operation.");
  }

  return prv->s->c_str();
}


Value::operator std::string() const {
  if (prv->type != STRING) {
    throw type_mismatch("Must be of type STRING for that operation.");
  }

  return *(prv->s);
}


// Conversion to std::vector<std::string> for type VECTOR, iff containing only STRINGS
// Explicit overload needed, because calling static_cast<std::string> is ambiguous
Value::operator std::vector<std::string>() const {
  std::vector<std::string> vec;
  for (size_t i = 0; i < size(); ++i) {
    vec.push_back(operator [](i).operator std::string());
  }
  return vec;
}


bool Value::defined() const {
  return prv->type != UNDEFINED;
}


bool Value::empty() const {
  return (prv->type == UNDEFINED ||
    prv->type == HJSON_NULL ||
    (prv->type == STRING && prv->s->empty()) ||
    (prv->type == VECTOR && prv->v->empty()) ||
    (prv->type == MAP && prv->m->empty()));
}


Value::Type Value::type() const {
  return prv->type;
}


size_t Value::size() const {
  switch (prv->type)
  {
  case STRING:
    return prv->s->size();
  case VECTOR:
    return prv->v->size();
  case MAP:
    return prv->m->size();
  case BOOL:
  case DOUBLE:
    return 1;
  case UNDEFINED:
  case HJSON_NULL:
    return 0;
  default:
    throw type_mismatch("Unknown type.");
  }
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
  case VECTOR:
    {
      auto itA = prv->v->begin();
      auto endA = prv->v->end();
      auto itB = other.prv->v->begin();
      do {
        if (!itA->deep_equal(*itB)) {
          return false;
        }
        ++itA;
        ++itB;
      } while (itA != endA);
    }
    return true;

  case MAP:
    {
      auto itA = this->begin(), endA = this->end(), itB = other.begin();
      do {
        if (!itA->second.deep_equal(itB->second)) {
          return false;
        }
        ++itA;
        ++itB;
      } while (itA != endA);
    }
    return true;

  default:
    return false;
  }


}


Value Value::clone() const {
  switch (type()) {
  case VECTOR:
    {
      Value ret;
      for (int index = 0; index < int(size()); ++index) {
        ret.push_back(operator[](index).clone());
      }
      return ret;
    }

  case MAP:
    {
      Value ret;
      for (auto it = begin(); it != end(); ++it) {
        ret[it->first] = it->second.clone();
      }
      return ret;
    }

  default:
    return *this;
  }
}


void Value::erase(int index) {
  if (prv->type != UNDEFINED && prv->type != VECTOR) {
    throw type_mismatch("Must be of type VECTOR for that operation.");
  } else if (index < 0 || static_cast<size_t>(index) >= size()) {
    throw index_out_of_bounds("Index out of bounds.");
  }

  auto vec = prv->v;
  vec->erase(vec->begin() + index);
}


void Value::push_back(const Value &other) {
  if (prv->type == UNDEFINED) {
    prv->~ValueImpl();
    // Recreate the private object using the same memory block.
    new(&(*prv)) ValueImpl(VECTOR);
  } else if (prv->type != VECTOR) {
    throw type_mismatch("Must be of type UNDEFINED or VECTOR for that operation.");
  }

  prv->v->push_back(other);
}


ValueMap::iterator Value::begin() {
  if (prv->type != MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::iterator();
  }

  return prv->m->begin();
}


ValueMap::iterator Value::end() {
  if (prv->type != MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::iterator();
  }

  return prv->m->end();
}


ValueMap::const_iterator Value::begin() const {
  if (prv->type != MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::const_iterator();
  }

  return prv->m->begin();
}


ValueMap::const_iterator Value::end() const {
  if (prv->type != MAP) {
    // Some C++ compilers might not allow comparing this to another
    // default-constructed iterator.
    return ValueMap::const_iterator();
  }

  return prv->m->end();
}


size_t Value::erase(const std::string &key) {
  if (prv->type == UNDEFINED) {
    return 0;
  } else if (prv->type != MAP) {
    throw type_mismatch("Must be of type MAP for that operation.");
  }

  return prv->m->erase(key);
}


size_t Value::erase(const char *key) {
  return erase(std::string(key));
}


double Value::to_double() const {
  switch (prv->type) {
  case UNDEFINED:
  case HJSON_NULL:
    return 0;
  case BOOL:
    return (prv->b ? 1 : 0);
  case DOUBLE:
    return prv->d;
  case STRING: {
    double ret;
    std::stringstream ss(*(prv->s));

    // Make sure we expect dot (not comma) as decimal point.
    ss.imbue(std::locale::classic());

    ss >> ret;

    if (!ss.eof() || ss.fail()) {
      return 0.0;
    }

    return ret;
  }
  default:
    throw type_mismatch("Illegal type for this operation.");
  }
}


std::string Value::to_string() const {
  switch (prv->type) {
  case UNDEFINED:
    return "";
  case HJSON_NULL:
    return "null";
  case BOOL:
    return (prv->b ? "true" : "false");
  case DOUBLE:
    {
      std::ostringstream oss;

      // Make sure we expect dot (not comma) as decimal point.
      oss.imbue(std::locale::classic());
      oss.precision(15);

      oss << prv->d;

      return oss.str();
    }
  case STRING:
    return *(prv->s);
  default:
    throw type_mismatch("Illegal type for this operation.");
  }
}


void Value::set_comment_pre(std::string com)
{
  prv->commentPre = std::move(com);
}


void Value::set_comment_post(std::string com)
{
  prv->commentPost = std::move(com);
}


void Value::set_comment_inside(std::string com)
{
  prv->commentInside = std::move(com);
}


std::string Value::comment_pre() const
{
  return prv->commentPre;
}


std::string Value::comment_post() const
{
  return prv->commentPost;
}


std::string Value::comment_inside() const
{
  return prv->commentInside;
}


bool Value::has_comment_pre() const
{
  return !prv->commentInside.empty();
}


bool Value::has_comment_post() const
{
  return !prv->commentInside.empty();
}


bool Value::has_comment_inside() const
{
  return !prv->commentInside.empty();
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
    if (parentPrv->type == MAP)
      parentPrv->m[0][key] = Value(prv);
  }
}


MapProxy &MapProxy::operator =(const MapProxy &other) {
  if (other.prv->commentPre.empty())
    other.prv->commentPre = prv->commentPre;
  if (other.prv->commentPost.empty())
    other.prv->commentPost = prv->commentPost;
  if (other.prv->commentInside.empty())
    other.prv->commentInside = prv->commentInside;
  prv = other.prv;
  wasAssigned = true;
  return *this;
}


MapProxy &MapProxy::operator =(const Value &other) {
  if (other.prv->commentPre.empty())
    other.prv->commentPre = prv->commentPre;
  if (other.prv->commentPost.empty())
    other.prv->commentPost = prv->commentPost;
  if (other.prv->commentInside.empty())
    other.prv->commentInside = prv->commentInside;
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
