# hjson-cpp

[![Build Status](https://github.com/hjson/hjson-cpp/workflows/test/badge.svg)](https://github.com/hjson/hjson-cpp/actions)
[![C++](https://img.shields.io/github/release/hjson/hjson-cpp.svg?style=flat-square&label=c%2b%2b)](https://github.com/hjson/hjson-cpp/releases)
![license](https://img.shields.io/github/license/mashape/apistatus.svg)

![Hjson Intro](https://hjson.github.io/hjson1.gif)

```
{
  # specify rate in requests/second (because comments are helpful!)
  rate: 1000

  // prefer c-style comments?
  /* feeling old fashioned? */

  # did you notice that rate doesn't need quotes?
  hey: look ma, no quotes for strings either!

  # best of all
  notice: []
  anything: ?

  # yes, commas are optional!
}
```

To quickly get up and running with a new C++ application project using Hjson configuration files, use the [hjson-cpp-example](https://github.com/hjson/hjson-cpp-example) template repository.

For other platforms see [hjson.github.io](https://hjson.github.io).

# Compiling

The easiest way to use hjson-cpp is to simply include all of the files from the folders `src` and `include` into your own project. The only requirement is that your compiler fully supports C++11.

GCC 4.8 has the C++11 headers for regex, but unfortunately not a working implementation, so for GCC at least version 4.9 is required.

## Cmake

The second easiest way to use hjson-cpp is to either add it as a subfolder to your own Cmake project, or to install the Hjson lib on your system by using Cmake. Works on Linux, Windows and MacOS. Your mileage may vary on other platforms. Cmake version 3.10 or newer is required.

### Cmake subfolder

Instead of building a lib, you can choose to include Hjson as a subfolder in your own Cmake project. That way, Hjson will be built together with your own project, with the same settings. In Visual Studio that also means the Hjson source code will be visible in a project in your own solution. Example `CMakeLists.txt` for your own project, if your executable is called `myapp`:
```cmake
add_executable(myapp main.cpp)

add_subdirectory(../hjson-cpp ${CMAKE_BINARY_DIR}/hjson)
target_link_libraries(myapp hjson)
```

### Cmake options

A list of Hjson Cmake options and their default values:

```bash
BUILD_SHARED_LIBS=OFF
BUILD_WITH_STATIC_CRT=  # Can be set to Yes or No. Only used on Windows.
CMAKE_BUILD_TYPE=  # Set to Debug for debug symbols, or Release for optimization.
CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS=ON  # Needed for shared libs on Windows. Introduced in Cmake 3.4.
HJSON_ENABLE_INSTALL=OFF
HJSON_ENABLE_TEST=OFF
HJSON_ENABLE_PERFTEST=OFF
HJSON_NUMBER_PARSER=StringStream  # Possible values are StringStream, StrToD and CharConv.
HJSON_VERSIONED_INSTALL=OFF  # Use version suffix on header and lib folders.
```

### Linux lib

1. First create a Makefile using Cmake.
```bash
$ cd hjson-cpp
$ mkdir build
$ cd build
$ cmake .. -DHJSON_ENABLE_TEST=ON -DHJSON_ENABLE_INSTALL=ON -DCMAKE_BUILD_TYPE=Release
```
2. Then you can optionally run the tests.
```bash
$ make runtest
```
3. Install the include files and lib to make them accessible system wide (optional).
```bash
$ sudo make install
```
4. If you haven't done either step 2 or step 3, you must at least compile the code into a library by calling make.
```bash
$ make
```

### Windows lib

The Cmake GUI is the most convenient way to generate a Visual Studio solution. Make sure that you tick the boxes for `HJSON_ENABLE_TEST` and `HJSON_ENABLE_INSTALL` if you want to run tests or make the Hjson lib accessible system wide.

After generating the solution and opening it in Visual Studio you can run the tests by right-clicking the project `runtest` and selecting `Build`. The test results are shown in the `Output` window (the same window that shows the build result).

In order to make the Hjson lib accessible system wide you must run Visual Studio as an administrator. In Windows 7 you can do that by right-clicking on the Visual Studio icon in the start menu and selecting `Run as an administrator`. Then open the Hjson solution, right-click the `INSTALL` project and select `Build`. Make sure to do that for both the `Debug` and `Release` targets.

## Linking

The hjson lib can now be used in your own Cmake projects, for example like this if your application is called `myapp`:
```cmake
add_executable(myapp main.cpp)

find_package(hjson REQUIRED)
target_link_libraries(myapp hjson)
```

On Windows it's important that you compiled or installed both `Debug` and `Release` Hjson libraries before Cmake-generating your own project. Otherwise the target type you didn't compile/install will not be available in your own project.

If you did not install Hjson system wide, you can still use it like in the example above if you specify the location of the Hjson build when running Cmake for your own project. Example:
```bash
$ cmake .. -Dhjson_DIR=../hjson-cpp/build
```

# Usage in C++

### Functions

The most important functions in the Hjson namespace are:

```cpp
std::string Marshal(const Value& v, const EncoderOptions& options = EncoderOptions());

void MarshalToFile(const Value& v, const std::string& path,
  const EncoderOptions& options = EncoderOptions());

Value Unmarshal(const std::string& data,
  const DecoderOptions& options = DecoderOptions());

Value UnmarshalFromFile(const std::string& path,
  const DecoderOptions& options = DecoderOptions());

Value Merge(const Value& base, const Value& ext);
```

*Marshal* is the output-function, transforming an *Hjson::Value* tree (represented by its root node) to a string that can be written to a file.

*MarshalToFile* writes the output directly to a file instead of returning a string.

*Unmarshal* is the input-function, transforming a string to a *Hjson::Value* tree. The string is expected to be UTF8 encoded. Other encodings might work too, but have not been tested. The function comes in three flavors: char pointer with or without the `dataSize` parameter, or std::string. For a char pointer without `dataSize` parameter the `data` parameter must be null-terminated (like all normal strings). All of the unmarshal functions throw an *Hjson::syntax_error* exception if the input string is not fully valid Hjson syntax.

*UnmarshalFromFile* reads directly from a file instead of taking a string as input.

*Merge* returns an *Hjson::Value* tree that is a cloned combination of the input *Hjson::Value* trees `base` and `ext`, with values from `ext` used whenever both `base` and `ext` has a value for some specific position in the tree. The function is convenient when implementing an application with a default configuration (`base`) that can be overridden by input parameters (`ext`).

### Stream operator

An *Hjson::Value* can be inserted into a stream, for example like this:

```cpp
Hjson::Value myValue = 3.0;
std::cout << myValue;
```

The stream operator marshals the *Hjson::Value* using standard options, so this code will produce the exact same result, but uses more RAM since the full output is temporarily stored in a string instead of being written directly to the stream:

```cpp
Hjson::Value myValue = 3.0;
std::cout << Hjson::Marshal(myValue);
```

If you want to use custom encoding options they can be communicated like this:

```cpp
Hjson::Value myValue = 3.0;
Hjson::EncoderOptions encOpt;
encOpt.omitRootBraces = true;
std::cout << Hjson::StreamEncoder(myValue, encOpt);
```

Likewise for reading from a stream. Hjson will consume the entire stream from its current position, and throws an *Hjson::syntax_error* exception if not all of the stream (from its current position) is valid Hjson syntax.

```cpp
Hjson::Value myValue;
std::cin >> myValue;
```

```cpp
Hjson::Value myValue;
Hjson::DecoderOptions decOpt;
decOpt.comments = false;
std::cin >> Hjson::StreamDecoder(myValue, decOpt);
```

### Hjson::Value

Input strings are unmarshalled into a tree representation where each node in the tree is an object of the type *Hjson::Value*. The class *Hjson::Value* mimics the behavior of Javascript in that you can assign any type of primitive value to it without casting. Existing *Hjson::Value* objects can change type when given a new assignment. Examples:

```cpp
Hjson::Value myValue(true);
Hjson::Value myValue2 = 3.0;
myValue2 = "A text.";
```

These are the possible types for an *Hjson::Value*:

    Undefined
    Null
    Bool
    Double
    Int64
    String
    Vector
    Map

The default constructor creates an *Hjson::Value* of the type *Hjson::Type::Undefined*.

An *Hjson::Value* can behave both like a vector (array) and like a map (*Object* in Javascript):

```cpp
Hjson::Value map;
map["down1"]["down2"]["down3"] = "three levels deep!";
map["down1"]["number"] = 7;

Hjson::Value arr;
arr.push_back("first");
std::string myString = arr[0];
```

If you try to access a map element that doesn't exist, an *Hjson::Value* of type *Hjson::Type::Undefined* is returned. But if you try to access a vector element that doesn't exist, an *Hjson::index_out_of_bounds* exception is thrown.

In order to make it possible to check for the existence of a specific key in a map without creating an empty element in the map with that key, a temporary object of type *Hjson::MapProxy* is returned from the string bracket operators:

```cpp
MapProxy operator[](const std::string&);
MapProxy operator[](const char*);
```

The *Hjson::MapProxy* desctructor creates or updates an element in the map if needed. Therefore the *Hjson::MapProxy* copy and move constructors are private, so that objects of that class do not stay alive longer than a single line of code. The downside of that is that you cannot store the returned value in an auto declared variable.

```cpp
Hjson::Value map;

// This statement won't compile.
auto badValue = map["key"];

// This statement will compile just fine.
Hjson::Value goodValue = map["key"];
```

Because of *Hjson::MapProxy*, you cannot get a reference to a map element from the string bracket operator. You can however get such a reference from the integer bracket operator, or from the function *Hjson::Value::at(const std::string& key)*. Both the integer bracket operator and the *at* function throw an *Hjson::index_out_of_bounds* exception if the element could not be found.

```cpp
Hjson::Value map;
map["myKey"] = "myValue";

auto use_reference = [](Hjson::Value& input) {
};

// This statement won't compile in gcc or clang. Visual Studio will compile
// and use a reference to the MapProxy temporary object.
use_reference(map["myKey"]);

// This statement will compile just fine.
use_reference(map.at("myKey"));
```

### Number representations

The C++ implementation of Hjson can both read and write 64-bit integers. No special care is needed, you can simply assign the value.

Example:

```cpp
Hjson::Value myValue = 9223372036854775807;
assert(myValue == 9223372036854775807);
```

All integers are stored with 64-bit precision. An *Hjson::Value* containing an integer will be of the type *Hjson::Type::Int64*. The only other way that a number is stored in an *Hjson::Value* is in the form of a double precision floating point representation, which can handle up to 52 bits of integer precision. An *Hjson::Value* containing a floating point value will be of the type *Hjson::Type::Double*.

An *Hjson::Value* that has been unmarshalled from a string that contains a decimal point (for example the string `"1.0"`), or a string containing a number that is bigger or smaller than what can be represented by an *std::int64_t* variable (bigger than 9223372036854775807 or smaller than -9223372036854775808) will be stored as a double precision floating point number internally in the *Hjson::Value*.

Any *Hjson::Value* of type *Hjson::Type::Double* will be represented by a string containing a decimal point when marshalled (for example `"1.0"`), so that the string can be unmarshalled back into an *Hjson::Type::Double*, i.e. no information is lost in the marshall-unmarshall cycle.

The function *Hjson::Value::is_numeric()* returns true if the *Hjson::Value* is of type *Hjson::Type::Double* or *Hjson::Type::Int64*.

### Avoiding type mismatch errors

When *static_cast<>()* is used on an *Hjson::Value*, an *Hjson::type_mismatch* error is thrown if the *Hjson::Value* is not of a compatible type. For example, if you do `static_cast<const char*>(myVal)` then `myVal` must be of the type *Hjson::Type::String*, otherwise an *Hjson::type_mismatch* error is thrown.

So if you use static or implicit casting, errors can be thrown depending on the content of the input *Hjson* file. Maybe the *Hjson* file contains a collection of first and last names in quoteless lowercase strings and [Christopher Null](https://en.wikipedia.org/wiki/Christopher_Null) is one of the persons in the collection... The unquoted string `null` is stored in an *Hjson::Value* of type *Hjson::Type::Null* instead of *Hjson::Type::String* as the other names.

To avoid errors being thrown when casting, instead use these *Hjson::Value* member functions that will return the best possible representation of the value regardless of *Hjson::Type*:

```cpp
double Value::to_double() const;
std::int64_t Value::to_int64() const;
std::string Value::to_string() const;
```

For example, if `myVal` is of type *Hjson::Type::Null* then `myVal.to_string()` returns the string `"null"`.

### Operators

This table shows all operators defined for *Hjson::Value*, and the *Hjson::Type* they require. If an operator is used on an *Hjson::Value* of a type for which that operator is not valid, the exception *Hjson::type_mismatch* is thrown.

| | Undefined | Null | Bool | Double | Int64 | String | Vector | Map |
| :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: | :-: |
| = | X | X | X | X | X | X | X | X |
| == | X | X | X | X | X | X | X | X |
| != | X | X | X | X | X | X | X | X |
| + | | | | X | X | X | | |
| - | | | | X | X | | | |
| ++ | | | | X | X | | | |
| -- | | | | X | X | | | |
| += | | | | X | X | X | | |
| -= | | | | X | X | | | |
| < | | | | X | X | X | | |
| > | | | | X | X | X | | |
| <= | | | | X | X | X | | |
| >= | | | | X | X | X | | |
| \* | | | | X | X | | | |
| \/ | | | | X | X | | | |
| \*= | | | | X | X | | | |
| \/= | | | | X | X | | | |
| % | | | | | X | | | |
| %= | | | | | X | | | |
| \[int\] | X | | | | | | X | X |
| \[string\] | X | | | | | | | X |

The equality operator (*==*) returns true if the two *Hjson::Value* objects are both of type *Hjson::Type::Undefined* or *Hjson::Type::Null*.

When comparing *Hjson::Value* objects of type *Hjson::Type::Vector* or *Hjson::Type::Map*, the equality operator (*==*) returns true if both objects reference the same underlying vector or map (i.e. the same behavior as when comparing pointers).

### Order of map elements

Iterators for an *Hjson::Value* of type *Hjson::Type::Map* are always ordered by the keys in alphabetic order. But when editing a configuration file you might instead want the output to have the same order of elements as the file you read for input. That is the default ordering in the output from *Hjson::Marshal()*, thanks to *true* being the default value of the option *preserveInsertionOrder* in *Hjson::EncoderOptions*.

The elements in an *Hjson::Value* of type *Hjson::Type::Map* can be accessed directly using the bracket operator with either the string key or the insertion index as input parameter.

```cpp
Hjson::Value val1;
val1["zeta"] = 1;
assert(val1[0] == 1);
val1[0] = 99;
assert(val1["zeta"] == 99);
```

The key for a given insertion index can be viewed using the function *Hjson::Value::key(int index)*.

```cpp
Hjson::Value val1;
val1["zeta"] = 1;
assert(val1.key(0) == "zeta");
```

The insertion order can be changed using the function *Hjson::Value::move(int from, int to)*. If the input parameter `from` is lower than the input parameter `to`, the value will end up at index `to - 1`.

```cpp
Hjson::Value val1;
val1["zeta"] = 1;
val1["y"] = 2;
val1.move(0, 2);
assert(val1.key(1) == "zeta");
assert(val1[0] == 2);
```

The insertion order is kept when cloning or merging *Hjson::Value* maps.

### Comments

The Hjson unmarshal functions will by default store any comments in the resulting *Hjson::Value* tree, so that you can easily create an app that updates existing Hjson documents without losing the comments. In this example, any comments in the Hjson file are kept:

```cpp
Hjson::Value root = Hjson::UnmarshalFromFile(szPath);
root["myKey"] = "aNewValue";
Hjson::MarshalToFile(root, szPath);
```

The *Hjson::Value* assignment operator does not assign new comments unless the receiving *Hjson::Value* is of type *Hjson::Type::Undefined*. The reason for that is that comments should not be lost when assigning a new value, as in the example here above. If you actually do want to replace both the value and the existing comments, use the function *Hjson::Value::assign_with_comments()*:

```cpp
// Changes the value but does not change any comments in root["myKey"], unless
// root["myKey"] was undefined before this assignment.
root["myKey"] = otherValue;

// Changes the value and also copies all comments from otherValue into root["myKey"].
root["myKey"].assign_with_comments(otherValue);
```

There are four types of comments: *before*, *key*, *inside* and *after*. If a comment is found, all chars (including line feeds and white spaces) between the values and/or separators are included in the string that becomes stored as a comment in an *Hjson::Value*.

```cpp
# This is a *before* comment to the object that starts on the next line.
{
  # This is a *before* comment to value1.
  // This is still the same *before* comment to value1.
  key1: /* This is a *key* comment to value1. */ "value1" // This is an *after* comment to value1.
  /* This is a *before* comment to value2. */
  key2:
  // This is a *key* comment to value2.
  value2 /* This is an *after* comment to value2.
  key3: {
    // This is an *inside* comment.
  }
  key4: value4
  // This is an *after* comment to value4. Would have been a *before* comment
  // to value5 if this map contained a fifth value.
}
// This is an *after* comment to the root object.
```

An *inside* comment is shown right after `{` or `[` in a map or vector. The unmarshal functions will only assign a comment as an *inside* comment if the map or vector is empty. Otherwise such a comment will be assigned to the *before* comment of the first element in the map or vector.

A *key* comment is shown between the key and the value if the value is an element in a map. If the value is not an element in a map, the *key* comment is shown between the *before* comment and the value.

After unmarshalling (with comments) the example here above, some of the comment strings would look like this:

```cpp
root["key1"].get_comment_before() == "\n  # This is a *before* comment to value1.\n  // This is still the same *before* comment to value1.\n  ";

root.get_comment_after() == "\n// This is an *after* comment to the root object.";
```

The *Hjson::Value::set_comment_X* functions do not analyze the strings they get as input, so you will change the meaning of the Hjson output if you are not careful when using them. For example, a comment starting with *//* or *#* must end with a line feed or else the marshal functions will place the value on the same line as the comment, thus making the value a part of the comment.

If you want to keep blank lines and other formatting in an Hjson document even if there are no comments, set the option *whitespaceAsComments* to *true* in *DecoderOptions*. Then the output from the marshal functions will look exactly like the input to the unmarshal functions, except possibly changes in root braces, quotation and comma separators. When *whitespaceAsComments* is *true*, the option *comments* is ignored (treated as *true*).

### Performance

Hjson is not much optimized for speed. But if you require fast(-ish) execution, escpecially in a multithreaded application, you can experiment with different values for the Cmake option `HJSON_NUMBER_PARSER`. The default value `StringStream` uses C++ string streams with the locale `classic` imbued to ensure that dots are used as decimal separators rather than commas.

The value `StrToD` gives better performance, especially in multi threaded applications, but will use whatever locale the application is using. If the current locale uses commas as decimal separator *Hjson* will umarshal floating point numbers into strings, and will use commas in floating point numbers in the marshal output, which means that other parsers will treat that value as a string.

Setting `HJSON_NUMBER_PARSER` to `CharConv` gives the best performance, and uses dots as comma separator regardless of the application locale. Using `CharConv` will automatically cause the code to be compiled using the C++17 standard (or a newer standard if required by your project). Unfortunately neither GCC 10.1 or Clang 10.0 implement the required feature of C++17 (*std::from_chars()* for *double*), but GCC 11 will have it. It does work in Visual Studio 17 and later.

Another way to increase performance and reduce memory usage is to disable reading and writing of comments. Set the option *comments* to *false* in *DecoderOptions* and *EncoderOptions*. In this example, any comments in the Hjson file are ignored:

```cpp
Hjson::DecoderOptions decOpt;
decOpt.comments = false;
Hjson::Value root = Hjson::UnmarshalFromFile(szPath, decOpt);
Hjson::EncoderOptions encOpt;
encOpt.comments = false;
Hjson::MarshalToFile(root, szPath, encOpt);
```

### Example code

```cpp
#include <hjson.h>


int main() {

  // Now let's look at decoding Hjson data into Hjson::Value.
  std::string sampleText = R"(
{
  # specify rate in requests/second
  rate: 1000
  array:
  [
    foo
    bar
  ]
}
)";

  // Decode. Throws Hjson::syntax_error if the string is not fully valid Hjson.
  Hjson::Value dat = Hjson::Unmarshal(sampleText.c_str(), sampleText.size());

  // Values can be assigned directly without casting.
  int rate = dat["rate"];
  printf("%d\n", rate);

  // Sometimes it's difficult for the compiler to know
  // what primitive type to convert the Hjson::Value to.
  // Then an explicit cast can be used.
  printf("%s\n", static_cast<const char*>(dat["array"][0]));

  // To encode to Hjson with default options:
  Hjson::Value sampleMap;
  sampleMap["apple"] = 5;
  sampleMap["lettuce"] = 7;
  std::string hjson = Hjson::Marshal(sampleMap);
  printf("%s\n", hjson.c_str());
}
```

Iterating through the elements of an *Hjson::Value* of type *Hjson::Type::Vector*:

```cpp
for (int index = 0; index < int(arr.size()); ++index) {
  std::cout << arr[index] << std::endl;
}
```

Iterating through the elements of an *Hjson::Value* of type *Hjson::Type::Map* in alphabetical order:

```cpp
for (auto it = map.begin(); it != map.end(); ++it) {
  std::cout << "key: " << it->first << "  value: " << it->second << std::endl;
}
```

Having a default configuration:

```cpp
#include <hjson.h>
#include <cstdio>


static const char *_szDefaultConfig = R"(
{
  imageSource: NO DEFAULT
  showImages: true
  writeImages: true
  printFrameIndex: false
  printFrameRate: true
}
)";


Hjson::Value GetConfig(const char *szConfigPath) {
  Hjson::Value defaultConfig = Hjson::Unmarshal(_szDefaultConfig);

  Hjson::Value inputConfig;
  try {
    inputConfig = Hjson::UnmarshalFromFile(szConfigPath);
  } catch(const std::exception& e) {
    std::fprintf(stderr, "Error in config: %s\n\n", e.what());
    std::fprintf(stdout, "Default config:\n");
    std::fprintf(stdout, _szDefaultConfig);

    return Hjson::Value();
  }

  return Hjson::Merge(defaultConfig, inputConfig);
}
```

For a full multi-platform example application using Hjson configuration files, see the [hjson-cpp-example](https://github.com/hjson/hjson-cpp-example) template repository.

# History

[see releases](https://github.com/hjson/hjson-cpp/releases)

