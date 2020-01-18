#ifndef HJSON_AFOWENFOWANEFWOAFNLL
#define HJSON_AFOWENFOWANEFWOAFNLL

#include <string>
#include <memory>
#include <map>
#include <stdexcept>


namespace Hjson {


// Exists only to avoid ambiguous conversions for the Hjson::Value constructors.
struct Int64_tag {};


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
  // Always place string values in double quotation marks ("), and escape
  // any special chars inside the string value
  bool quoteAlways;
  // Always place keys in quotes
  bool quoteKeys;
  // Indent string
  std::string indentBy;
  // Allow the -0 value (unlike ES6)
  bool allowMinusZero;
  // Encode unknown values as 'null'
  bool unknownAsNull;
  // Output a comma separator between elements. If true, always place strings
  // in quotes (overriding the "quoteAlways" setting).
  bool separator;
  // Only affects the order of elements in objects. If true, the key/value
  // pairs for all objects will be placed in the same order as they were added.
  // If false, the key/value pairs are placed in alphabetical key order.
  bool preserveInsertionOrder;
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
  // The int64 constructor is tagged to avoid ambiguous conversions for the
  // overloaded constructors. Example usage:
  //   Hjson::Value hval(9223372036854775807, Hjson::Int64_tag{});
  Value(std::int64_t, Int64_tag);
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
  // Returns true if this Value was unmarshalled from a string representation
  // of an integer without decimal point (i.e. not "1.0", because that string
  // will cause this function to return false, since it contains a decimal
  // point). The number must also be within the valid range for being
  // represented by an int64_t variable (min: -9223372036854775808,
  // max: 9223372036854775807), otherwise the number will be stored as
  // floating point internally in this Value. This function also returns true
  // if this Value was created using the int or int64_t Value constructor.
  bool is_int64() const;
  size_t size() const;
  bool deep_equal(const Value&) const;
  Value clone() const;

  // -- VECTOR and MAP specific functions
  // For a VECTOR, the input argument is the index in the vector for the value
  // that should be erased. For a MAP, the input argument is the index in the
  // insertion order of the MAP for the value that should be erased.
  void erase(int);
  // Move value on index `from` to index `to`. If `from` is less than `to` the
  // element will actually end up at index `to - 1`. For an Hjson::Value of
  // type MAP, calling this function changes the insertion order but does not
  // affect the iteration order, since iterations are always done in
  // alphabetical key order.
  void move(int from, int to);

  // -- VECTOR specific function
  void push_back(const Value&);

  // -- MAP specific functions
  // Get key by its zero-based insertion index.
  std::string key(int) const;
  // Iterations are always done in alphabetical key order.
  std::map<std::string, Value>::iterator begin();
  std::map<std::string, Value>::iterator end();
  std::map<std::string, Value>::const_iterator begin() const;
  std::map<std::string, Value>::const_iterator end() const;
  size_t erase(const std::string&);
  size_t erase(const char*);

  // These functions throw an error if used on VECTOR or MAP
  double to_double() const;
  std::int64_t to_int64() const;
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

// Returns a properly indented text representation of the input value tree.
// Extra options can be specified in the input parameter "options".
std::string MarshalWithOptions(Value v, EncoderOptions options);

// Returns a properly indented text representation of the input value tree.
std::string Marshal(Value v);

// Returns a properly indented JSON text representation of the input value
// tree.
std::string MarshalJson(Value v);

// Calls `Marshal(v)` and outputs the result to the stream.
std::ostream &operator <<(std::ostream &out, Value v);

// Creates a Value tree from input text.
Value Unmarshal(const char *data, size_t dataSize);

// Creates a Value tree from input text.
// The input parameter "data" must be null-terminated.
Value Unmarshal(const char *data);

// Creates a Value tree from input text.
Value Unmarshal(const std::string&);

// Returns a Value tree that is a combination of the input parameters "base"
// and "ext".
//
// If "base" and "ext" contain a map on the same place in the tree, the
// returned tree will on that place have a map containing a combination of
// all keys from the "base" and "ext" maps. If a key existed in both "base"
// and "ext", the value from "ext" is used. Except for if the value in "ext"
// is of type UNDEFINED: then the value from "base" is used.
//
// Vectors are not merged: if a vector exists in the same place in the "base"
// and "ext" trees, the one from "ext" will be used in the returned tree.
//
// Maps and vectors are cloned, not copied. Therefore changes in the returned
// tree will not affect the input variables "base" and "ext.
//
// If "ext" is of type UNDEFINED, a clone of "base" is returned.
//
Value Merge(const Value base, const Value ext);


}


#endif
