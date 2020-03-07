#include <hjson.h>

#include <chrono>
#include <thread>
#include <future>
#include <vector>
#include <iostream>


static int _run_test() {
  int elementCount = 0;

  std::string inString = R"(
{
  # the comma forces a whitespace check
  numbers:
  [
    0
    0   ,
    -0
    42  ,
    42.1  ,
    -5
    -5.1
    17.01e2
    -17.01e2
    12345e-3  ,
    -12345e-3  ,
  ]
  native:
  [
    true   ,
    true
    false  ,
    false
    null   ,
    null
  ]
  strings:
  [
    x 0
    .0
    00
    01
    0 0 0
    42 x
    42.1 asdf
    1.2.3
    -5 0 -
    -5.1 --
    17.01e2 +
    -17.01e2 :
    12345e-3 @
    -12345e-3 $
    true true
    x true
    false false
    x false
    null null
    x null
  ]
}
)";

  for (int a = 0; a < 10000; ++a) {
     elementCount += Hjson::Unmarshal(inString).size();
  }

  return elementCount;
}


void perf_multithread() {
  std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
  std::vector<std::future<int> > futures;
  int elementCount = 0;

  for (int a = 0; a < 16; ++a) {
    futures.push_back(std::async(_run_test));
  }

  for (auto &fut : futures) {
    elementCount += fut.get();
  }

  std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now();

  std::cout << "Runtime: " << std::chrono::duration<double>(stop -
    start).count() << " seconds" << std::endl;

  // Also output total first level element count, to prove that the unmarshal
  // calls have not been optimized away.
  std::cout << "Total first level element count: " << elementCount << std::endl;
}
