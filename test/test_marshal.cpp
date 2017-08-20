#include <hjson.h>
#include <assert.h>
#include <fstream>
#include <iostream>


static std::string _getTestContent(std::string name) {
  std::ifstream infile("assets/" + name + "_test.hjson",
    std::ifstream::ate | std::ifstream::binary);

  if (!infile.is_open()) {
    infile.open("assets/" + name + "_test.json",
      std::ifstream::ate | std::ifstream::binary);
  }

  std::string ret;
  ret.resize(infile.tellg());
  infile.seekg(0, std::ios::beg);
  infile.read(&ret[0], ret.size());
  infile.close();

  return ret;
}


static void _examine(std::string filename) {
  size_t pos = filename.find("_test.");
  if (pos == std::string::npos) {
    return;
  }
  std::string name(filename.begin(), filename.begin() + pos);

  std::cout << "running " << name << '\n';

  bool shouldFail = !name.compare(0, 4, "fail");

  auto testContent = _getTestContent(name);
  Hjson::Value root;
  try {
    root = Hjson::Unmarshal(testContent.c_str(), testContent.size());
    if (shouldFail) {
      std::cout << "Should have failed on " << name << std::endl;
      assert(false);
    }
  } catch (Hjson::syntax_error e) {
    if (!shouldFail) {
      std::cout << "Should NOT have failed on " << name << std::endl;
      assert(false);
    } else {
      return;
    }
  }

  std::ifstream resultFile("assets/sorted/" + name + "_result.hjson",
    std::ifstream::ate | std::ifstream::binary);

  std::string rhjson;
  rhjson.resize(resultFile.tellg());
  resultFile.seekg(0, std::ios::beg);
  resultFile.read(&rhjson[0], rhjson.size());
  resultFile.close();

  auto actualHjson = Marshal(root);

  //std::ofstream outputFile("out.hjson");
  //outputFile << actualHjson;
  //outputFile.close();

  assert(actualHjson == rhjson);
}


void test_marshal() {
  std::ifstream infile("assets/testlist.txt");

  std::string line;
  while (std::getline(infile, line)) {
    _examine(line);
  }
}
