#ifndef HJSON_AFOWENFOWANEFWOAFNLL
#define HJSON_AFOWENFOWANEFWOAFNLL

#include <string>
#include <memory>
#include <map>
#include <stdexcept>

#define HJSON_OP_DECL_VAL(_T, _O) \
friend Value operator _O(_T, const Value&); \
friend Value operator _O(const Value&, _T);

#define HJSON_OP_DECL_BOOL(_T, _O) \
friend bool operator _O(_T, const Value&); \
friend bool operator _O(const Value&, _T);

#define HJSON_OP_DECL_ASS(_T, _O) \
Value& operator _O(_T);

#define HJSON_OPERATORS_DECLARATION_A(_T) \
HJSON_OP_DECL_BOOL(_T, <) \
HJSON_OP_DECL_BOOL(_T, >) \
HJSON_OP_DECL_BOOL(_T, <=) \
HJSON_OP_DECL_BOOL(_T, >=) \
HJSON_OP_DECL_BOOL(_T, ==) \
HJSON_OP_DECL_BOOL(_T, !=) \
HJSON_OP_DECL_ASS(_T, +=)

#define HJSON_OPERATORS_DECLARATION_B(_T) \
HJSON_OPERATORS_DECLARATION_A(_T) \
HJSON_OP_DECL_VAL(_T, +) \
HJSON_OP_DECL_VAL(_T, -) \
HJSON_OP_DECL_VAL(_T, *) \
HJSON_OP_DECL_VAL(_T, /) \
HJSON_OP_DECL_ASS(_T, -=) \
HJSON_OP_DECL_ASS(_T, *=) \
HJSON_OP_DECL_ASS(_T, /=)

#define HJSON_OPERATORS_DECLARATION_C(_T) \
HJSON_OPERATORS_DECLARATION_B(_T) \
HJSON_OP_DECL_VAL(_T, %) \
HJSON_OP_DECL_ASS(_T, %=)


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


class file_error : public std::runtime_error {
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
  // If true, omits root braces.
  bool omitRootBraces;
};


class MapProxy;


class Value {
  friend class MapProxy;

private:
  class ValueImpl;
  std::shared_ptr<ValueImpl> prv;
  Value(std::shared_ptr<ValueImpl>);

public:
  enum class Type {
    Undefined,
    Null,
    Bool,
    Double,
    Int64,
    String,
    Vector,
    Map
  };

  Value();
  Value(bool);
  Value(float);
  Value(double);
  Value(long double);
  Value(char);
  Value(unsigned char);
  Value(short);
  Value(unsigned short);
  Value(int);
  Value(unsigned int);
  Value(long);
  Value(unsigned long);
  Value(long long);
  Value(unsigned long long);
  Value(const char*);
  Value(const std::string&);
  Value(Type);
  virtual ~Value();

  const Value operator[](const std::string&) const;
  MapProxy operator[](const std::string&);
  const Value operator[](const char*) const;
  MapProxy operator[](const char*);
  const Value operator[](int) const;
  Value& operator[](int);

  bool operator ==(bool) const;
  bool operator !=(bool) const;

  friend std::string operator +(const char*, const Value&);
  friend std::string operator +(const Value&, const char*);
  friend std::string operator +(const std::string&, const Value&);
  friend std::string operator +(const Value&, const std::string&);

  HJSON_OPERATORS_DECLARATION_A(const char*)
  HJSON_OPERATORS_DECLARATION_A(const std::string&)
  HJSON_OPERATORS_DECLARATION_B(float)
  HJSON_OPERATORS_DECLARATION_B(double)
  HJSON_OPERATORS_DECLARATION_B(long double)
  HJSON_OPERATORS_DECLARATION_C(char)
  HJSON_OPERATORS_DECLARATION_C(unsigned char)
  HJSON_OPERATORS_DECLARATION_C(short)
  HJSON_OPERATORS_DECLARATION_C(unsigned short)
  HJSON_OPERATORS_DECLARATION_C(int)
  HJSON_OPERATORS_DECLARATION_C(unsigned int)
  HJSON_OPERATORS_DECLARATION_C(long)
  HJSON_OPERATORS_DECLARATION_C(unsigned long)
  HJSON_OPERATORS_DECLARATION_C(long long)
  HJSON_OPERATORS_DECLARATION_C(unsigned long long)
  HJSON_OPERATORS_DECLARATION_C(const Value&)

  Value operator+() const;
  Value operator-() const;
  Value& operator++();
  Value& operator--();
  Value operator++(int);
  Value operator--(int);

  explicit operator bool() const;
  operator float() const;
  operator double() const;
  operator long double() const;
  operator char() const;
  operator unsigned char() const;
  operator short() const;
  operator unsigned short() const;
  operator int() const;
  operator unsigned int() const;
  operator long() const;
  operator unsigned long() const;
  operator long long() const;
  operator unsigned long long() const;
  operator const char*() const;
  operator const std::string() const;

  bool defined() const;
  bool empty() const;
  Type type() const;
  // Returns true if the type of this Value is Double or Int64.
  bool is_numeric() const;
  size_t size() const;
  bool deep_equal(const Value&) const;
  Value clone() const;

  // -- Vector and Map specific functions
  // For a Vector, the input argument is the index in the vector for the value
  // that should be erased. For a Map, the input argument is the index in the
  // insertion order of the MAP for the value that should be erased.
  void erase(int);
  // Move value on index `from` to index `to`. If `from` is less than `to` the
  // element will actually end up at index `to - 1`. For an Hjson::Value of
  // type Map, calling this function changes the insertion order but does not
  // affect the iteration order, since iterations are always done in
  // alphabetical key order.
  void move(int from, int to);

  // -- Vector specific function
  void push_back(const Value&);

  // -- Map specific functions
  // Get key by its zero-based insertion index.
  std::string key(int) const;
  // Iterations are always done in alphabetical key order.
  std::map<std::string, Value>::iterator begin();
  std::map<std::string, Value>::iterator end();
  std::map<std::string, Value>::const_iterator begin() const;
  std::map<std::string, Value>::const_iterator end() const;
  size_t erase(const std::string&);
  size_t erase(const char*);

  // These functions throw an error if used on Vector or Map, but will return
  // 0 or 0.0 for the types Undefined and Null. Will parse strings to numbers
  // and print numbers to strings if necessary.
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
    const std::string& key);

public:
  ~MapProxy();
  MapProxy &operator =(const MapProxy&);
  MapProxy &operator =(const Value&);
};


EncoderOptions DefaultOptions();

// Deprecated, use Marshal(const Value& v, EncoderOptions options) instead.
std::string MarshalWithOptions(const Value&, EncoderOptions);

// Returns a properly indented text representation of the input value tree.
// Extra options can be specified in the input parameter "options".
std::string Marshal(const Value& v, EncoderOptions options = DefaultOptions());

// Writes (in binary mode, so using Unix EOL) a properly indented text
// representation of the input value tree to the file specified by the input
// parameter "path". Extra options can be specified in the input parameter
// "options". Throws Hjson::file_error if the file cannot be opened for writing.
void MarshalToFile(const Value& v, const std::string& path, EncoderOptions options = DefaultOptions());

// Returns a properly indented JSON text representation of the input value
// tree.
std::string MarshalJson(const Value&);

// Calls `Marshal(v)` and outputs the result to the stream.
std::ostream &operator <<(std::ostream& out, const Value& v);

// Creates a Value tree from input text.
Value Unmarshal(const char *data, size_t dataSize);

// Creates a Value tree from input text.
// The input parameter "data" must be null-terminated.
Value Unmarshal(const char *data);

// Creates a Value tree from input text.
Value Unmarshal(const std::string&);

// Reads the entire file (in binary mode) and unmarshals it. Throws
// Hjson::file_error if the file cannot be opened for reading.
Value UnmarshalFromFile(const std::string& path);

// Returns a Value tree that is a combination of the input parameters "base"
// and "ext".
//
// If "base" and "ext" contain a map on the same place in the tree, the
// returned tree will on that place have a map containing a combination of
// all keys from the "base" and "ext" maps. If a key existed in both "base"
// and "ext", the value from "ext" is used. Except for if the value in "ext"
// is of type Undefined: then the value from "base" is used.
//
// Vectors are not merged: if a vector exists in the same place in the "base"
// and "ext" trees, the one from "ext" will be used in the returned tree.
//
// Maps and vectors are cloned, not copied. Therefore changes in the returned
// tree will not affect the input variables "base" and "ext.
//
// If "ext" is of type Undefined, a clone of "base" is returned.
//
Value Merge(const Value& base, const Value& ext);


}


#endif
