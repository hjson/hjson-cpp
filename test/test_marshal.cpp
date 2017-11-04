#include <hjson.h>
#include <assert.h>
#include <fstream>
#include <iostream>


static std::string _readStream(std::ifstream *pInfile) {
  assert(pInfile->is_open());

  std::string ret;
  ret.resize(pInfile->tellg());
  pInfile->seekg(0, std::ios::beg);
  pInfile->read(&ret[0], ret.size());
  pInfile->close();

  return ret;
}


static std::string _readFile(std::string path) {
  std::ifstream infile(path, std::ifstream::ate | std::ifstream::binary);

  return _readStream(&infile);
}


static std::string _getTestContent(std::string name) {
  std::ifstream infile("assets/" + name + "_test.hjson",
    std::ifstream::ate | std::ifstream::binary);

  if (!infile.is_open()) {
    infile.open("assets/" + name + "_test.json",
      std::ifstream::ate | std::ifstream::binary);
  }

  return _readStream(&infile);
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

  auto rhjson = _readFile("assets/sorted/" + name + "_result.hjson");
  auto actualHjson = Marshal(root);

  //std::ofstream outputFile("out.hjson");
  //outputFile << actualHjson;
  //outputFile.close();

  assert(actualHjson == rhjson);

  auto rjson = _readFile("assets/sorted/" + name + "_result.json");
  auto actualJson = MarshalJson(root);

  //std::ofstream outputFile("out.json");
  //outputFile << actualJson;
  //outputFile.close();

  assert(actualJson == rjson);
}


void test_marshal() {
  std::ifstream infile("assets/testlist.txt");

  std::string line;
  while (std::getline(infile, line)) {
    _examine(line);
  }
}
