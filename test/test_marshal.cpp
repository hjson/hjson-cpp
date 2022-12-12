#include <hjson.h>
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <algorithm>
#include "hjson_test.h"


#define WRITE_FACIT 0


static std::string _readStream(std::ifstream *pInfile) {
  assert(pInfile->is_open());

  std::string ret;
  size_t len = pInfile->tellg();
  ret.resize(len);
  pInfile->seekg(0, std::ios::beg);
  pInfile->read(&ret[0], ret.size());
  pInfile->close();

  while (len > 0 && ret.at(len - 1) == '\0') {
    --len;
  }

  if (len > 0 && ret.at(len - 1) == '\n') {
    --len;
  }
  if (len > 0 && ret.at(len - 1) == '\r') {
    --len;
  }

  ret.resize(len);

  return ret;
}


static std::string _readFile(std::string pathBeginning, std::string extra,
  std::string pathEnd, bool *pUsedExtra)
{
  // The output from Hjson::Marshal() always uses Unix EOL, but git might have
  // converted files to Windows EOL on Windows, therefore we open the file in
  // text mode instead of binary mode.
  std::ifstream infile(pathBeginning + extra + pathEnd, std::ifstream::ate);
  if (!infile.is_open()) {
    infile.open(pathBeginning + pathEnd, std::ifstream::ate);
    *pUsedExtra = false;
  } else {
    *pUsedExtra = true;
  }
  if (!infile.is_open()) {
    return "";
  }

  return _readStream(&infile);
}


static inline void _filterComment(Hjson::Value *val, std::string (Hjson::Value::*fg)() const,
  void (Hjson::Value::*fs)(const std::string&))
{
  auto str = (val->*fg)();
  str.erase(std::remove(str.begin(), str.end(), '\r'), str.end());
  (val->*fs)(str);
}


static Hjson::Value _getTestContent(std::string name, Hjson::DecoderOptions opt=Hjson::DecoderOptions()) {
  Hjson::Value root;

  try {
    root = Hjson::UnmarshalFromFile("assets/" + name + "_test.hjson", opt);
  } catch (const Hjson::file_error& e) {
    root = Hjson::UnmarshalFromFile("assets/" + name + "_test.json", opt);
  }

  // Convert EOL to '\n' in comments because the env might have autocrlf=true in git.
  class Parent {
  public:
    Hjson::Value *v;
    int i;
  };
  Hjson::Value *cur = &root;
  std::vector<Parent> parents;
  do {
    _filterComment(cur, &Hjson::Value::get_comment_after, &Hjson::Value::set_comment_after);
    _filterComment(cur, &Hjson::Value::get_comment_before, &Hjson::Value::set_comment_before);
    _filterComment(cur, &Hjson::Value::get_comment_inside, &Hjson::Value::set_comment_inside);
    _filterComment(cur, &Hjson::Value::get_comment_key, &Hjson::Value::set_comment_key);

    if (cur->is_container() && !cur->empty()) {
      parents.push_back({cur, 0});
      cur = &(*cur)[0];
    } else if (!parents.empty()) {
      ++parents.back().i;

      while (parents.back().i >= parents.back().v->size()) {
        parents.pop_back();
        if (parents.empty()) {
          break;
        }
        ++parents.back().i;
      }

      if (!parents.empty()) {
        cur = &parents.back().v[0][parents.back().i];
      }
    }
  } while (!parents.empty());

  return root;
}


static bool _evaluate(std::string name, std::string expected, Hjson::Value root, std::string got) {
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
    std::cout << "\nExpected: (size " << expected.size() << ")\n" <<
      expected << "\n\nGot: (size " << got.size() << ")\n" << got << "\n\n";
    return false;
  }

  // pass5 contains values bigger than int64, just skip that test.
  if (name != "pass5") {
    auto newRoot = Hjson::Unmarshal(got);
    if (!newRoot.deep_equal(root)) {
      std::cout << "\nUnmarshalling this resulting Hjson did not produce a tree equal to the original test Hjson:\n" << got << "\n\n";
      return false;
    }
  }

  return true;
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
  } catch (const Hjson::syntax_error& e) {
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

  Hjson::EncoderOptions opt;
  opt.bracesSameLine = true;

  bool bUsedExtra = false;
  auto rhjson = _readFile("assets/comments2/", extra, name + "_result.hjson", &bUsedExtra);
  auto actualHjson = Hjson::Marshal(root, opt);

#if WRITE_FACIT
  std::ofstream outputFile = std::ofstream("assets/comments2/" +
    (bUsedExtra ? extra + '/' : "") + name + "_result.hjson", std::ofstream::binary);
  outputFile << actualHjson << '\n';
  outputFile.close();
#endif

  assert(_evaluate(name, rhjson, root, actualHjson));

  opt.bracesSameLine = false;

  rhjson = _readFile("assets/comments/", extra, name + "_result.hjson", &bUsedExtra);
  actualHjson = Hjson::Marshal(root, opt);

#if WRITE_FACIT
  outputFile = std::ofstream("assets/comments/" + (bUsedExtra ? extra + '/' : "") +
    name + "_result.hjson", std::ofstream::binary);
  outputFile << actualHjson << '\n';
  outputFile.close();
#endif

  assert(_evaluate(name, rhjson, root, actualHjson));

  opt.comments = false;

  rhjson = _readFile("assets/", extra, name + "_result.hjson", &bUsedExtra);
  actualHjson = Hjson::Marshal(root, opt);

#if WRITE_FACIT
  outputFile = std::ofstream("assets/" + (bUsedExtra ? extra + '/' : "") +
    name + "_result.hjson", std::ofstream::binary);
  outputFile << actualHjson << '\n';
  outputFile.close();
#endif

  assert(_evaluate(name, rhjson, root, actualHjson));

  auto rjson = _readFile("assets/", extra, name + "_result.json", &bUsedExtra);
  auto actualJson = Hjson::MarshalJson(root);

#if WRITE_FACIT
  outputFile = std::ofstream("assets/" + (bUsedExtra ? extra + '/' : "") +
    name + "_result.json", std::ofstream::binary);
  outputFile << actualJson << '\n';
  outputFile.close();
#endif

  assert(_evaluate(name, rjson, root, actualJson));

  opt.preserveInsertionOrder = false;

  rhjson = _readFile("assets/sorted/", extra, name + "_result.hjson", &bUsedExtra);
  actualHjson = Hjson::Marshal(root, opt);

#if WRITE_FACIT
  outputFile = std::ofstream("assets/sorted/" + (bUsedExtra ? extra + '/' : "") +
    name + "_result.hjson", std::ofstream::binary);
  outputFile << actualHjson << '\n';
  outputFile.close();
#endif

  assert(_evaluate(name, rhjson, root, actualHjson));

  opt.bracesSameLine = true;
  opt.quoteAlways = true;
  opt.quoteKeys = true;
  opt.separator = true;
  opt.comments = false;

  rjson = _readFile("assets/sorted/", extra, name + "_result.json", &bUsedExtra);
  actualJson = Hjson::Marshal(root, opt);

#if WRITE_FACIT
  outputFile = std::ofstream("assets/sorted/" + (bUsedExtra ? extra + '/' : "") +
    name + "_result.json", std::ofstream::binary);
  outputFile << actualJson << '\n';
  outputFile.close();
#endif

  assert(_evaluate(name, rjson, root, actualJson));

  Hjson::DecoderOptions decOpt;
  decOpt.whitespaceAsComments = true;
  try {
    root = _getTestContent(name, decOpt);
  } catch (const Hjson::syntax_error& e) {
    std::cout << "Failed to read with whitespace as comments: " << name << "\n";
    assert(false);
  }

  opt = Hjson::EncoderOptions();
  rhjson = _readFile("assets/comments3/", extra, name + "_result.hjson", &bUsedExtra);
  actualHjson = Hjson::Marshal(root, opt);

#if WRITE_FACIT
  outputFile = std::ofstream("assets/comments3/" + (bUsedExtra ? extra + '/' : "") +
    name + "_result.hjson", std::ofstream::binary);
  outputFile << actualHjson << '\n';
  outputFile.close();
#endif

  assert(_evaluate(name, rhjson, root, actualHjson));
}


void test_marshal() {
  std::ifstream infile("assets/testlist.txt");

  std::string line;
  while (std::getline(infile, line)) {
    _examine(line);
  }
}
