# hjson-cpp

[![Build Status](https://travis-ci.org/hjson/hjson-cpp.svg?branch=master)](https://travis-ci.org/hjson/hjson-cpp)
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

The C++ implementation of Hjson is based on [hjson-go](https://github.com/hjson/hjson-go). For other platforms see [hjson.github.io](https://hjson.github.io).

# Compiling

The easiest way to use hjson-cpp is to simply include all of the files from the folders `src` and `include` into your own project. The only requirement is that your compiler fully supports C++11.

GCC 4.8 has the C++11 headers for regex, but unfortunately not a working implementation, so for GCC at least version 4.9 is required.

## Cmake

The second easiest way to use hjson-cpp is to either add it as a subfolder to your own Cmake project, or to install the hjson lib on your system by using Cmake. Works on Linux, Windows and MacOS. Your mileage may vary on other platforms. Cmake version 3.10 or newer is required.

### Cmake subfolder

Instead of building a lib, you can choose to include hjson as a subfolder in your own Cmake project. That way, hjson will be built together with your own project, with the same settings. In Visual Studio that also means the hjson source code will be visible in a project in your own solution. Example `CMakeLists.txt` for your own project, if your executable is called `myapp`:
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

The Cmake GUI is the most convenient way to generate a Visual Studio solution. Make sure that you tick the boxes for `HJSON_ENABLE_TEST` and `HJSON_ENABLE_INSTALL` if you want to run tests or make the hjson lib accessible system wide.

After generating the solution and opening it in Visual Studio you can run the tests by right-clicking the project `runtest` and selecting `Build`. The test results are shown in the `Output` window (the same window that shows the build result).

In order to make the hjson lib accessible system wide you must run Visual Studio as an administrator. In Windows 7 you can do that by right-clicking on the Visual Studio icon in the start menu and selecting `Run as an administrator`. Then open the hjson solution, right-click the `INSTALL` project and select `Build`. Make sure to do that for both the `Debug` and `Release` targets.

## Linking

The hjson lib can now be used in your own Cmake projects, for example like this if your application is called `myapp`:
```cmake
add_executable(myapp main.cpp)

find_package(hjson REQUIRED)
target_link_libraries(myapp hjson)
```

On Windows it's important that you compiled or installed both `Debug` and `Release` hjson libraries before Cmake-generating your own project. Otherwise the target type you didn't compile/install will not be available in your own project.

If you did not install hjson system wide, you can still use it like in the example above if you specify the location of the hjson build when running Cmake for your own project. Example:
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

*Unmarshal* is the input-function, transforming a string to a *Hjson::Value* tree. The string is expected to be UTF8 encoded. Other encodings might work too, but have not been tested. The function comes in three flavors: char pointer with or without the `dataSize` parameter, or std::string. For a char pointer without `dataSize` parameter the `data` parameter must be null-terminated (like all normal strings).

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

Likewise for reading from a stream. Hjson will consume the entire stream from its current position, and throws an error if not all of the stream (from its current position) is valid Hjson syntax.

```cpp
Hjson::Value myValue;
std::cin >> myValue;
```

```cpp
Hjson::Value myValue;
Hjson::DecoderOptions decOpt;
decOpt.comments = true;
std::cin >> Hjson::StreamDecoder(myValue, decOpt);
```

### Hjson::Value

Input strings are unmarshalled into a tree representation where each node in the tree is an object of the type *Hjson::Value*. The class *Hjson::Value* mimics the behavior of Javascript in that you can assign any type of primitive value to it without casting. Existing *Hjson::Value* objects can change type when given a new assignment. Examples:

```cpp
Hjson::Value myValue(true);
Hjson::Value myValue2 = 3.0;
myValue2 = "A text.";
```

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

The MapProxy desctructor creates or updates an element in the map if needed. Therefore the MapProxy copy and move constructors are private, so that objects of that class do not stay alive longer than a single line of code. The downside of that is that you cannot store the returned value in an auto declared variable.

```cpp
Hjson::Value map;

// This statement won't compile.
auto badValue = map["key"];

// This statement will compile just fine.
Hjson::Value goodValue = map["key"];
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

### Operators

This table shows all operators defined for *Hjson::Value*, and the *Hjson::Type* they require. If an operator is used on an *Hjson::Value* of a type for which that operator is not valid, the exception *Hjson::type_mismatch* is thrown.

| | Undefined | Null | Bool | Double | Int64 | String | Vector | Map |
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

### Performance

Hjson is not much optimized for speed. But if you require fast(-ish) execution, escpecially in a multithreaded application, you can experiment with different values for the Cmake option `HJSON_NUMBER_PARSER`. The default value `StringStream` uses C++ string streams with the locale `classic` imbued to ensure that dots are used as decimal separators rather than commas.

The value `StrToD` gives better performance, especially in multi threaded applications, but will use whatever locale the application is using. If the current locale uses commas as decimal separator *Hjson* will umarshal floating point numbers into strings, and will use commas in floating point numbers in the marshal output, which means that other parsers will treat that value as a string.

Setting `HJSON_NUMBER_PARSER` to `CharConv` gives the best performance, and uses dots as comma separator regardless of the application locale. Using `CharConv` will automatically cause the code to be compiled using the C++17 standard (or a newer standard if required by your project). Unfortunately neither GCC 10.1 or Clang 10.0 implement the required features of C++17 (*std::from_chars()* for *double*). It does work in Visual Studio 17 and later.

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

  // Decode. Throws Hjson::syntax_error on failure.
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

# History

[see releases](https://github.com/hjson/hjson-cpp/releases)

