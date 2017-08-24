# hjson-cpp

[![Build Status](https://travis-ci.org/trobro/hjson-cpp.svg?branch=master)](https://travis-ci.org/trobro/hjson-cpp)
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

The second easiest way to use hjson-cpp is to either add it as a subfolder to your own Cmake project, or to install the hjson lib on your system by using Cmake. Works on Linux and Windows. Your mileage may vary on other platforms.

### Cmake subfolder

Instead of building a lib, you can choose to include hjson as a subfolder in your own Cmake project. That way, hjson will be built together with your own project, with the same settings. In Visual Studio that also means the hjson source code will be visible in a project in your own solution. Example `CMakeLists.txt` for your own project, if your executable is called `myapp`:
```cmake
add_executable(myapp main.cpp)

add_subdirectory(../hjson-cpp ${CMAKE_BINARY_DIR}/hjson)
target_link_libraries(myapp hjson)
```

### Linux lib

1. First create a Makefile using Cmake.
```bash
$ cd hjson-cpp
$ mkdir build
$ cd build
$ cmake .. -DHJSON_ENABLE_TEST=ON -DHJSON_ENABLE_INSTALL=ON
```
2. Then you can optionally run the tests.
```bash
$ make runtest
```
3. Install the include files and static lib to make them accessible system wide (optional).
```bash
$ sudo make install
```
4. If you haven't done either step 2 or step 3, you must at least compile the code into a library by calling make.
```bash
$ make
```

### Windows lib

The Cmake GUI is the most convenient way to generate a Visual Studio solution. Make sure that you tick the boxes for `HJSON_ENABLE_TEST` and `HJSON_ENABLE_INSTALL` if you want to run tests or make the hjson lib accessible system wide.

After generating the solution and opening it in Visual Studio you can run the tests by right-clicking the project `runtest` and selecting `Build`. Make sure to have selected a Debug target, otherwise the assertions will have been optimized away.

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
```

*Marshal* is the output-function, transforming an *Hjson::Value* tree (represented by its root node) to a string that can be written to a file.

*Unmarshal* is the input-function, transforming a string to a *Hjson::Value* tree. The string is expected to be UTF8 encoded. Other encodings might work too, but have not been tested.

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

# History

[see releases](https://github.com/trobro/hjson-cpp/releases)

