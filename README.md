# hjson-cpp

[![Build Status](https://travis-ci.org/hjson/hjson-cpp.svg?branch=master)](https://travis-ci.org/hjson/hjson-cpp)
[![C++](https://img.shields.io/github/release/hjson/hjson-cpp.svg?style=flat-square&label=c%2b%2b)](https://github.com/hjson/hjson-cpp/releases)
![license](https://img.shields.io/github/license/mashape/apistatus.svg)

![Hjson Intro](http://hjson.org/hjson1.gif)

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

The C++ implementation of Hjson is based on [hjson-go](https://github.com/hjson/hjson-go). For other platforms see [hjson.org](http://hjson.org).

# Compiling

The easiest way to use hjson-cpp is to simply include all of the files from the folders `src` and `include` into you own project. The only requirement is that your compiler fully supports C++11.

GCC 4.8 has the C++11 headers for regex, but unfortunately not a working implementation, so for GCC at least version 4.9 is required.

## Cmake

The second easiest way to use hjson-cpp is to either add it as a subfolder to your own Cmake project, or to install the hjson lib on your system by using Cmake. Works on Linux, Windows and MacOS. Your mileage may vary on other platforms.

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
std::string Marshal(Value v);
Value Unmarshal(const char *data, size_t dataSize);
Value Unmarshal(const char *data);
Value Merge(const Value base, const Value ext);
```

*Marshal* is the output-function, transforming an *Hjson::Value* tree (represented by its root node) to a string that can be written to a file.

*Unmarshal* is the input-function, transforming a string to a *Hjson::Value* tree. The string is expected to be UTF8 encoded. Other encodings might work too, but have not been tested. The function comes in two flavors: with or without the `dataSize` parameter. Without it, the `data` parameter must be null-terminated (like all normal strings).

*Merge* returns an *Hjson::Value* tree that is a cloned combination of the input *Hjson::Value* trees `base` and `ext`, with values from `ext` used whenever both `base` and `ext` has a value for some specific position in the tree. The function is convenient when implementing an application with a default configuration (`base`) that can be overridden by input parameters (`ext`).

Two more functions exist, allowing adjustments to the output formatting when creating an Hjson string:

```cpp
EncoderOptions DefaultOptions();
std::string MarshalWithOptions(Value v, EncoderOptions options);
```

### Hjson::Value

Input strings are unmarshalled into a tree representation where each node in the tree is an object of the type *Hjson::Value*. The class *Hjson::Value* mimics the behavior of Javascript in that you can assign any type of primitive value to it without casting. Examples:

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

If you try to access a map element that doesn't exist, an *Hjson::Value* of type *Hjson::Value::UNDEFINED* is returned. But if you try to access a vector element that doesn't exist, an *Hjson::index_out_of_bounds* exception is thrown.

These are the possible types for an *Hjson::Value*:

    UNDEFINED
    HJSON_NULL
    BOOL
    DOUBLE
    STRING
    VECTOR
    MAP

The default constructor creates an *Hjson::Value* of the type *Hjson::Value::UNDEFINED*.

### 64-bit integers

The C++ implementation of Hjson can both read and write 64-bit integers. But since functions and operators overloaded in C++ cannot differ on the return value alone, *Hjson::Value* is treated as *double* in arithmetic operations. That works fine up to 52 bits of integer precision. For the full 64-bit integer precision the function *Hjson::Value::to_int64()* can be used.

The *Hjson::Value* constructor for 64-bit integers also requires a special solution, in order to avoid *ambiguous overload* errors for some compilers. An empty struct is used as the second parameter so that all ambiguity is avoided. An *Hjson::Value* created using the 64-bit constructor will be of the type *Hjson::Value::DOUBLE*, but has the full 64-bit integer precision internally.

Example:

```cpp
Hjson::Value myValue(9223372036854775807, Hjson::Int64_tag{});
assert(myValue.to_int64() == 9223372036854775807);
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
  // this is short for:
  // auto options = Hjson::DefaultOptions();
  // std::string hjson = Hjson::MarshalWithOptions(sampleMap, options);
  printf("%s\n", hjson.c_str());
}
```

Iterating through the elements of an *Hjson::Value* of type *Hjson::Value::VECTOR*:

```cpp
for (int index = 0; index < int(arr.size()); ++index) {
  std::cout << arr[index].to_string() << std::endl;
}
```

Iterating through the elements of an *Hjson::Value* of type *Hjson::Value::MAP*:

```cpp
for (auto it = map.begin(); it != map.end(); ++it) {
  std::cout << "key: " << it->first << "  value: " << it->second.to_string() << std::endl;
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


Hjson::Value GetConfig(const char *szInputConfig) {
  Hjson::Value defaultConfig = Hjson::Unmarshal(_szDefaultConfig);

  Hjson::Value inputConfig
  try {
    inputConfig = Hjson::Unmarshal(szInputConfig);
  } catch(std::exception e) {
    std::fprintf(stderr, "Error: Failed to unmarshal input config\n");
    std::fprintf(stdout, "Default config:\n");
    std::fprintf(stdout, _szDefaultConfig);

    return Hjson::Value();
  }

  return Hjson::Merge(defaultConfig, inputConfig);
}
```

# History

[see releases](https://github.com/hjson/hjson-cpp/releases)

