#ifndef HJSON_AFOWENFOWANEFWOAFNLL
#define HJSON_AFOWENFOWANEFWOAFNLL

#include <string>
#include <memory>
#include <map>
#include <stdexcept>


namespace Hjson {


class type_mismatch : public std::logic_error {
  using std::logic_error::logic_error;
};


class index_out_of_bounds : public std::out_of_range {
  using std::out_of_range::out_of_range;
};


class syntax_error : public std::runtime_error {
  using std::runtime_error::runtime_error;
};


// EncoderOptions defines options for encoding to Hjson.
struct EncoderOptions {
  // End of line, should be either \n or \r\n
  std::string eol;
  // Place braces on the same line
  bool bracesSameLine;
  // Always place string in quotes
  bool quoteAlways;
  // Indent string
  std::string indentBy;
  // Allow the -0 value (unlike ES6)
  bool allowMinusZero;
  // Encode unknown values as 'null'
  bool unknownAsNull;
};


class MapProxy;


class Value {
  friend class MapProxy;

private:
  class ValueImpl;
  std::shared_ptr<ValueImpl> prv;
  Value(std::shared_ptr<ValueImpl>);

public:
  enum Type {
    UNDEFINED,
    HJSON_NULL,
    BOOL,
    DOUBLE,
    STRING,
    VECTOR,
    MAP
  };

  Value();
  Value(bool);
  Value(double);
  Value(int);
  Value(const char*);
  Value(const std::string&);
  Value(Type);
  virtual ~Value();

  const Value operator[](const std::string&) const;
  MapProxy operator[](const std::string& name);
  const Value operator[](const char*) const;
  MapProxy operator[](const char*);
  const Value operator[](int) const;
  Value &operator[](int);
  bool operator ==(bool) const;
  bool operator !=(bool) const;
  bool operator ==(double) const;
  bool operator !=(double) const;
  bool operator ==(int) const;
  bool operator !=(int) const;
  bool operator ==(const char*) const;
  bool operator !=(const char*) const;
  bool operator ==(const std::string&) const;
  bool operator !=(const std::string&) const;
  bool operator ==(const Value&) const;
  bool operator !=(const Value&) const;
  bool operator <(double) const;
  bool operator >(double) const;
  bool operator <(int) const;
  bool operator >(int) const;
  bool operator <(const char*) const;
  bool operator >(const char*) const;
  bool operator <(const std::string&) const;
  bool operator >(const std::string&) const;
  bool operator <(const Value&) const;
  bool operator >(const Value&) const;
  double operator +(double) const;
  double operator +(int) const;
  std::string operator +(const char*) const;
  std::string operator +(const std::string&) const;
  Value operator +(const Value&) const;
  double operator -(double) const;
  double operator -(int) const;
  double operator -(const Value&) const;
  explicit operator bool() const;
  operator double() const;
  operator const char*() const;
  operator const std::string() const;

  bool defined() const;
  bool empty() const;
  Type type() const;
  size_t size() const;
  bool deep_equal(const Value&) const;

  // VECTOR specific functions
  void erase(int);
  void push_back(const Value&);

  // MAP specific functions
  std::map<std::string, Value>::iterator begin();
  std::map<std::string, Value>::iterator end();
  std::map<std::string, Value>::const_iterator begin() const;
  std::map<std::string, Value>::const_iterator end() const;
  size_t erase(const std::string&);
  size_t erase(const char*);

  // Throws if used on VECTOR or MAP
  double to_double() const;
  std::string to_string() const;
};


class MapProxy : public Value {
  friend class Value;

private:
  std::shared_ptr<ValueImpl> parentPrv;
  std::string key;
  // True if an explicit assignment has been made to this MapProxy.
  bool wasAssigned;

  MapProxy(std::shared_ptr<ValueImpl> parent, std::shared_ptr<ValueImpl> child,
    const std::string &key);

public:
  ~MapProxy();
  MapProxy &operator =(const MapProxy&);
  MapProxy &operator =(const Value&);
};


EncoderOptions DefaultOptions();

std::string MarshalWithOptions(Value v, EncoderOptions options);

std::string Marshal(Value v);

Value Unmarshal(const char *data, size_t dataSize);


}


#endif
