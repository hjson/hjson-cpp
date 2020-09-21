#include <hjson.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include "hjson_test.h"


#define WRITE_FACIT 0


static std::string _readStream(std::ifstream *pInfile) {
  assert(pInfile->is_open());

  std::string ret;
  ret.resize(pInfile->tellg());
  pInfile->seekg(0, std::ios::beg);
  pInfile->read(&ret[0], ret.size());
  pInfile->close();

  return ret;
}


static std::string _readFile(std::string pathBeginning, std::string extra,
  std::string pathEnd)
{
  // The output from Hjson::Marshal() always uses Unix EOL, but git might have
  // converted files to Windows EOL on Windows, therefore we open the file in
  // text mode instead of binary mode.
  std::ifstream infile(pathBeginning + extra + pathEnd, std::ifstream::ate);
  if (!infile.is_open()) {
    infile.open(pathBeginning + pathEnd, std::ifstream::ate);
  }

  return _readStream(&infile);
}


static Hjson::Value _getTestContent(std::string name) {
  try {
    return Hjson::UnmarshalFromFile("assets/" + name + "_test.hjson");
  } catch (Hjson::file_error e) {}

  return Hjson::UnmarshalFromFile("assets/" + name + "_test.json");
}


static void _evaluate(std::string expected, std::string got) {
  // Visual studio will have trailing null chars in rhjson if there was any
  // CRLF conversion when reading it from the file. If so, `==` would return
  // `false`, therefore we need to use `strcmp`.
  if (std::strcmp(expected.c_str(), got.c_str())) {
    for (int a = 0; a < expected.size() && a < got.size(); a++) {
      if (got[a] != expected[a]) {
        std::cout << std::endl << "first diff at index " << a << std::endl;
        break;
      }
    }
    std::cout << std::endl << "Expected: (size " << expected.size() << ")" <<
      std::endl << expected << std::endl << std::endl << "Got: (size " <<
      got.size() << ")" << std::endl << got << std::endl <<
      std::endl;
    assert(std::strcmp(expected.c_str(), got.c_str()) == 0);
  }
}


static void _examine(std::string filename) {
  size_t pos = filename.find("_test.");
  if (pos == std::string::npos) {
    return;
  }
  std::string name(filename.begin(), filename.begin() + pos);

  std::cout << "running " << name << '\n';

  bool shouldFail = !name.compare(0, 4, "fail");

  Hjson::Value root;
  try {
    root = _getTestContent(name);
    if (shouldFail) {
      std::cout << "Should have failed on " << name << "\n";
      assert(false);
    }
  } catch (Hjson::syntax_error e) {
    if (!shouldFail) {
      std::cout << "Should NOT have failed on " << name << "\n";
      assert(false);
    } else {
      return;
    }
  }

  std::string extra = "";
#if HJSON_USE_CHARCONV
  extra = "charconv/";
#endif

  auto rhjson = _readFile("assets/sorted/", extra, name + "_result.hjson");
  auto actualHjson = Hjson::Marshal(root);

#if WRITE_FACIT
  std::ofstream outputFile("assets/sorted/" + name + "_result.hjson", std::ofstream::binary);
  outputFile << actualHjson;
  outputFile.close();
#endif

  _evaluate(rhjson, actualHjson);

  auto rjson = _readFile("assets/sorted/", extra, name + "_result.json");
  auto actualJson = Hjson::MarshalJson(root);

#if WRITE_FACIT
  outputFile = std::ofstream("assets/sorted/" + name + "_result.json", std::ofstream::binary);
  outputFile << actualJson;
  outputFile.close();
#endif

  _evaluate(rjson, actualJson);

  auto opt = Hjson::DefaultOptions();
  opt.preserveInsertionOrder = true;

  rhjson = _readFile("assets/", extra, name + "_result.hjson");
  actualHjson = Hjson::MarshalWithOptions(root, opt);

#if WRITE_FACIT
  outputFile = std::ofstream("assets/" + name + "_result.hjson", std::ofstream::binary);
  outputFile << actualHjson;
  outputFile.close();
#endif

  _evaluate(rhjson, actualHjson);

  opt.bracesSameLine = true;
  opt.quoteAlways = true;
  opt.quoteKeys = true;
  opt.separator = true;

  rjson = _readFile("assets/", extra, name + "_result.json");
  actualJson = Hjson::MarshalWithOptions(root, opt);

#if WRITE_FACIT
  outputFile = std::ofstream("assets/" + name + "_result.json", std::ofstream::binary);
  outputFile << actualJson;
  outputFile.close();
#endif

  _evaluate(rjson, actualJson);
}


void test_marshal() {
  std::ifstream infile("assets/testlist.txt");

  std::string line;
  while (std::getline(infile, line)) {
    _examine(line);
  }
}
