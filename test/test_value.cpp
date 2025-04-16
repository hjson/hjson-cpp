#include <hjson.h>
#include <cmath>
#include <cstring>
#include <sstream>
#include <cstdio>
#include "hjson_test.h"


static std::string _test_string_param(std::string param) {
  return param;
}


void test_value() {
  {
    Hjson::Value valVec(Hjson::Type::Vector);
    assert(valVec.type() == Hjson::Type::Vector);
    Hjson::Value valMap(Hjson::Type::Map);
    assert(valMap.type() == Hjson::Type::Map);
  }

  {
    Hjson::Value val(true);
    assert(val.type() == Hjson::Type::Bool);
    assert(val);
    assert(val == true);
    assert(val != false);
    assert(true == (bool) val);
    assert(false != (bool) val);
    assert(val && val);
    // The assignment statement must not be performed (short-circuit of ||).
    assert(val || (val = false));
    assert(!!val);
    {
      std::stringstream ss;
      ss << val;
      assert(ss.str() == "true");
    }
    val = false;
    assert(!val);
    assert(!val.empty());
    // size() is the number of child elements, can only be > 0 for Vector or Map. 
    assert(val.size() == 0);
    assert(val.to_string() == "false");
    assert(val.to_double() == 0);
    assert(val.to_int64() == 0);
    val = true;
    assert(val.to_double() == 1);
    assert(val.to_int64() == 1);
    assert(val.to_string() == "true");
    // The result of the comparison is undefined in C++11.
    // assert(val.begin() == val.end());
  }

  {
    Hjson::Value val(Hjson::Type::Null);
    assert(val.type() == Hjson::Type::Null);
    assert(!val);
    assert(val.empty());
    assert(val.size() == 0);
    Hjson::Value val2(Hjson::Type::Null);
    assert(val == val2);
    Hjson::Value val3;
    assert(val != val3);
    assert(val.to_double() == 0);
    assert(val.to_int64() == 0);
    assert(val.to_string() == "null");
    {
      std::stringstream ss;
      ss << val;
      assert(ss.str() == "null");
    }
    assert(val3.to_double() == 0);
    assert(val3.to_int64() == 0);
    assert(val3.to_string() == "");
    // The result of the comparison is undefined in C++11.
    // assert(val.begin() == val.end());
    // assert(val3.begin() == val3.end());
  }

  {
    const Hjson::Value val = 3.0;
    assert(val == 3.0);
    assert(val != 4.0);
    assert(3.0 == val);
    assert(4.0 != val);
    double third = val;
    int fourth = val;
    assert(fourth == 3);
    third = static_cast<int>(val);
    assert(third == 3.0);
    assert(val == 3);
    assert(val != 2);
    assert(3 == val);
    assert(2 != val);
    assert(val < 4);
    assert(4 > val);
    assert(val < 4.0);
    assert(4.0 > val);
    assert(val * 3 == 9);
    third = val * 3;
    assert(third == 9);
    third = 3 * val;
    assert(third == 9);
    assert(3 * val == 9);
    assert(val * 3.0 == 9.0);
    assert(3.0 * val == 9.0);
    assert(val / 3 == 1);
    assert(3 / val == 1);
    assert(val / 3.0 == 1.0);
    assert(3.0 / val == 1.0);
    assert(val + 1 == 4);
    assert(1 + val == 4);
    assert(val + 1.0 == 4.0);
    assert(1.0 + val == 4.0);
    assert(val - 1 == 2);
    assert(1 - val == -2);
    assert(val - 1.0 == 2.0);
    assert(1.0 - val == -2.0);
    assert(val.to_double() == 3);
    assert(val.to_int64() == 3);
    assert(val.to_string() == "3.0");
    {
      std::stringstream ss;
      ss << val;
      assert(ss.str() == "3.0");
    }
    assert(val.type() != Hjson::Type::Int64);
    // The result of the comparison is undefined in C++11.
    // assert(val.begin() == val.end());
  }

  {
    Hjson::Value val = 3.0;
    Hjson::Value val2 = 3.0;
    assert(val == val2);
    assert(val != val2 + 1);
    assert(val2 + 1 != val);
    double third = val + val2;
    assert(third == 6.0);
    val2 = 6;
    assert(val * val2 == 18);
    assert(val2 / val == 2.0);
    assert(val + val2 == 9);
    assert(val - val2 == -3.0);
  }

  {
    Hjson::Value val = 1;
    assert(val == 1);
    assert(val != 2);
    assert(val != 2.0);
    assert(val.to_double() == 1.0);
    assert(val.to_int64() == 1);
    assert(val.to_string() == "1");
    {
      std::stringstream ss;
      ss << val;
      assert(ss.str() == "1");
    }
    assert(val.type() == Hjson::Type::Int64);
    int i = 2;
    Hjson::Value val2(i);
    assert(val2 != val);
    assert(val2 > val);
    assert(val < val2);
    assert(val2 > 1);
    assert(val2 < 3);
    assert(1 < val2);
    assert(3 > val2);
    assert(3 > val2.to_int64());
    assert(val2 == i);
    assert((val2 + 1) == static_cast<double>(i + 1));
    assert((val2 - 1) == static_cast<double>(i - 1));
    char i3 = 4;
    Hjson::Value val3(i3);
    assert(val3 == 4);
    assert(4 == val3);
    assert(val3 == i3);
    assert(i3 == val3);
    assert(!(i3 > val3));
    assert(!(val3 > i3));
    assert(!(i3 < val3));
    assert(!(val3 < i3));
    i3 = val3;
    assert(i3 == 4);
    Hjson::Value val4("-1");
    assert(val4.to_double() == -1);
    assert(val4.to_int64() == -1);
    assert(val4.to_string() == "-1");
    {
      std::stringstream ss;
      ss << val4;
      assert(ss.str() == "\"-1\"");
    }
    Hjson::Value val5(-1);
    assert(val5 == -1);
    assert(val5 < val);
    assert(val5 < 1.0);
    short i6 = -28;
    Hjson::Value val6(i6);
    assert(val6 == -28);
    assert(-28 == val6);
    assert(val6 == i6);
    assert(i6 == val6);
    assert(!(val6 > i6));
    assert(!(i6 > val6));
    assert(!(val6 < i6));
    assert(!(i6 < val6));
    i6 = val6;
    assert(i6 == -28);
    i6 = -val6;
    assert(i6 == 28);
    i6 = +val6;
    assert(i6 == -28);
    val6 += i6;
    assert(val6 == -56);
    val6 -= i6;
    assert(val6 == -28);
    unsigned short i7 = 29;
    Hjson::Value val7(i7);
    assert(val7 == 29);
    assert(29 == val7);
    assert(val7 == i7);
    assert(i7 == val7);
    assert(!(val7 > i7));
    assert(!(i7 > val7));
    assert(!(val7 < i7));
    assert(!(i7 < val7));
    val7 -= i7;
    assert(val7 == 0);
    unsigned char i8 = 4;
    Hjson::Value val8(i8);
    assert(val8 == 4);
    assert(4 == val8);
    assert(val8 == i8);
    assert(i8 == val8);
    assert(!(i8 > val8));
    assert(!(val8 > i8));
    assert(!(i8 < val8));
    assert(!(val8 < i8));
    unsigned int i9 = 4;
    Hjson::Value val9(i9);
    assert(val9 == 4);
    assert(4 == val9);
    assert(val9 == i9);
    assert(i9 == val9);
    assert(!(i9 > val9));
    assert(!(val9 > i9));
    assert(!(i9 < val9));
    assert(!(val9 < i9));
    i9 = val9;
    assert(i9 == 4);
  }

  {
    unsigned char i1 = 250;
    char i2 = 100;
    Hjson::Value val1(i1);
    Hjson::Value val2 = i2;
    assert(val1 + val2 == 350);
    assert(350 == val2 + val1);
    assert(i1 + val1 == 500);
    assert(val1 * val2 == 25000);
    assert(val1 / val2 == (250 / 100));
  }

  {
    Hjson::Value val(144115188075855873);
    assert(val.type() == Hjson::Type::Int64);
    assert(val == Hjson::Value(144115188075855873));
    assert(val != Hjson::Value(144115188075855874));
    assert(val.to_int64() == 144115188075855873);
    assert(static_cast<std::int64_t>(val) == 144115188075855873);
    val = Hjson::Value(9223372036854775807);
    assert(val.to_string() == "9223372036854775807");
    assert(val == Hjson::Value(9223372036854775807));
    assert(val != Hjson::Value(9223372036854775806));
    assert(val.to_int64() == 9223372036854775807);
    assert(val > Hjson::Value(9223372036854775806));
    std::int64_t i = 9223372036854775806;
    std::int64_t i2 = 9223372036854775806;
    Hjson::Value val2(i);
    assert(val2 == Hjson::Value(i));
    assert(val2 != val);
    assert(val2 < val);
    assert(val > val2);
    assert(val2 < Hjson::Value(9223372036854775807));
    assert(9223372036854775807 > val2);
    assert(9223372036854775807 > val2.to_int64());
    // These two assertions fail in GCC 5.4, which is ok because doubles can
    // only represent integers up to 9007199254000000 (2^53) without precision
    // loss.
    //assert((val2 + 1) == static_cast<double>(i + 1));
    //assert((val2 - 1) == static_cast<double>(i - 1));
    Hjson::Value val6 = 9223372036854775807;
    assert(val6 == 9223372036854775807);
    Hjson::Value val7("-9223372036854775806");
    assert(val7.to_int64() == -9223372036854775806);
    assert(val7.to_string() == "-9223372036854775806");
    Hjson::Value val8(-9223372036854775806);
    assert(val8 == Hjson::Value(-9223372036854775806));
    assert(val8.to_int64() == -9223372036854775806);
    assert(val8 < val);
    assert(val8 < 1.0);
    std::int64_t i3 = 144115188075855873;
    Hjson::Value val9 = i3;
    assert(val9 == i3);
    assert(i3 == val9);
    assert(!(val9 > i3));
    assert(!(val9 < i3));
    assert(!(i3 > val9));
    assert(!(i3 > val9));
    assert(val9 >= i3);
    assert(val9 <= i3);
    assert(i3 >= val9);
    assert(i3 >= val9);
    i3 = val9;
    assert(i3 == 144115188075855873);
    std::int64_t i4 = 1;
    assert(i4 != val9);
    assert(val9 != i4);
    assert(val9 + i4 == 144115188075855874);
    assert(i4 + val9 == 144115188075855874);
    val9 += i4;
    assert(val9 == 144115188075855874);
    assert(val9 - i4 == 144115188075855873);
    assert(i4 - val9  == -144115188075855873);
    val9 -= i4;
    assert(val9 == 144115188075855873);
    assert(val9 / i4 == val9);
    assert(i4 / val9 == 0);
    val9 /= i4;
    assert(val9 == 144115188075855873);
    assert(val9 % i4 == 0);
    assert(i4 % val9 == 1);
    val9 %= i4;
    assert(val9 == 0);
  }

  {
    Hjson::Value val1("92233720368547758073829419051489548484843823585675828488686");
    Hjson::Value val2("92233720368547758073829419051489548484843823585675828488686.0");
    Hjson::Value val3(92233720368547758073829419051489548484843823585675828488686.0);
    assert(val1.to_double() == val2.to_double());
    assert(val1.to_double() == val3.to_double());
  }

  {
    Hjson::Value val1 = 3;
    val1 += 1;
    assert(val1 == 4);
    assert(++val1 == 5);
    assert(val1 == 5);
    assert(val1++ == 5);
    assert(val1 == 6);
    val1 += 1.0;
    assert(val1 == 7);
    val1 -= 1;
    assert(val1 == 6);
    val1 -= 1.0;
    assert(val1 == 5);
    assert(--val1 == 4);
    assert(val1 == 4);
    assert(val1-- == 4);
    assert(val1 == 3);
  }

  {
    Hjson::Value val("alpha");
    Hjson::Value val2 = "alpha";
    assert(val == val2);
    assert(val != "beta");
    assert("beta" != val);
    assert(_test_string_param(val) == "alpha");
    val = std::string("alpha");
    std::string st = val;
    assert(st == val);
    assert(val == st);
    assert(val == val2);
    assert(val2 == std::string("alpha"));
    assert(val2 != std::string("beta"));
    assert(std::string("alpha") == val2.operator std::string());
    assert(std::string("beta") != val2.operator std::string());
    assert(val.to_double() == 0);
    assert(val.to_int64() == 0);
    assert(val.to_string() == "alpha");
    st = val + "beta";
    assert(st == "alphabeta");
    val2 = val + "beta";
    assert(val2 == "alphabeta");
    val2 = val + std::string("beta");
    assert(val2 == "alphabeta");
    val2 = "beta" + val;
    assert(val2 == "betaalpha");
    val2 = std::string("beta") + val;
    assert(val2 == "betaalpha");
    val += "beta";
    assert(val == "alphabeta");
    val += st;
    assert(val == "alphabetaalphabeta");
    val = 3;
    assert("a" + val == "a3");
    val = 3.0;
    assert("a" + val == "a3.0");
    // The result of the comparison is undefined in C++11.
    // assert(val.begin() == val.end());
  }

  {
    Hjson::Value val("alpha");
    Hjson::Value val2 = "beta";
    assert(val < val2);
    assert(val2 > val);
    assert(val + val2 == "alphabeta");
    assert(val < "beta");
    assert("beta" > val.to_string());
    assert(val + "beta" == "alphabeta");
    assert("alpha" + val2.to_string() == "alphabeta");
    // The result of the comparison is undefined in C++11.
    // assert(val.begin() == val.end());
  }

  {
    Hjson::Value val("3.0");
    assert(val.to_double() == 3);
    assert(val.to_int64() == 3);
    Hjson::Value val2(3.0);
    assert(val != val2);
    assert(!(val == val2));
    try {
      bool a = val > val2;
      assert(!"Did not throw error when using < operator on values of different types.");
    } catch(const Hjson::type_mismatch& e) {}
    try {
      bool a = val < val2;
      assert(!"Did not throw error when using < operator on values of different types.");
    } catch(const Hjson::type_mismatch& e) {}
    try {
      std::string a = val + val2;
      assert(!"Did not throw error when using + operator on values of different types.");
    } catch(const Hjson::type_mismatch& e) {}
    try {
      double a = val2 - val;
      assert(!"Did not throw error when using - operator on values of different types.");
    } catch(const Hjson::type_mismatch& e) {}
    try {
      Hjson::Value val3 = val - Hjson::Value("0");
      assert(!"Did not throw error when using - operator on value of type STRING.");
    } catch(const Hjson::type_mismatch& e) {}
  }

  {
    Hjson::Value val;
    val["first"] = "leaf1";
    Hjson::Value val2 = val["first"];
    val["second"] = val["first"];
    val["fourth"] = 4.0;
    const Hjson::Value& valC = val;
    double fourth = valC.operator[]("fourth");
    assert(fourth == valC["fourth"]);
    assert(fourth == valC.at("fourth"));
    assert(!valC["fifth"].defined());
    try {
      double fifth = valC.at("fifth");
      assert(!"Did not throw error when calling at() with invalid key.");
    } catch(const Hjson::index_out_of_bounds& e) {}
    fourth = val["fourth"];
    assert(fourth == val["fourth"]);
    assert(fourth == val.at("fourth"));
    try {
      double fifth = val.at("fifth");
      assert(!"Did not throw error when calling at() with invalid key.");
    } catch(const Hjson::index_out_of_bounds& e) {}
    try {
      std::string fourthString = val["fourth"];
      assert(!"Did not throw error when assigning double to string.");
    } catch(const Hjson::type_mismatch& e) {}
    std::string leaft1 = val["first"];
    assert(leaft1 == "leaf1");
    assert(val[std::string("first")] == "leaf1");
    assert(val.at(std::string("first")) == "leaf1");
    assert(val["first"] == "leaf1");
    assert(!strcmp("leaf1", val["first"]));

    auto it = val.begin();
    assert(it->first == "first");
    assert(it->second == "leaf1");
    ++it;
    assert(it->first == "fourth");
    assert(it->second == 4);
    ++it;
    assert(it->first == "second");
    assert(it->second == "leaf1");
    ++it;
    assert(it == val.end());

    const Hjson::Value valConst = val;
    std::map<std::string, Hjson::Value>::const_iterator itConst = valConst.begin();
    assert(itConst->first == "first");
    assert(itConst->second == "leaf1");
    ++itConst;
    assert(itConst->first == "fourth");
    assert(itConst->second == 4);
    ++itConst;
    assert(itConst->first == "second");
    assert(itConst->second == "leaf1");
    ++itConst;
    assert(itConst == valConst.end());
  }

  {
    Hjson::Value val;
    val["first"] = "leaf1";
    char szKey[20];
    sprintf(szKey, "first");
    assert(val[szKey] == "leaf1");
    char *szKey2 = szKey;
    assert(val[szKey2] == "leaf1");
  }

  {
    Hjson::Value val;

    val["one"] = "uno";
    val["two"] = "due";
    assert(val["one"] == "uno");
    val["one"].clear();
    // clear() does nothing for a string, only affects vector and map.
    assert(!val.at("one").empty());
    assert(val["two"] == "due");
    auto ptr = &val.at("two");
    assert(*ptr == "due");
    val["two"] = 2;
    assert(*ptr == 2);
    val.clear();
    assert(val.empty());
  }

  {
    Hjson::Value val;

    val.push_back(3);
    val.push_back(4);
    assert(val.size() == 2);
    auto ptr = &val[0];
    assert(*ptr == 3);
    val[0] = 5;
    assert(*ptr == 5);
    val.clear();
    assert(val.empty());
  }

  try {
    Hjson::Value val;
    val["first"] = "leaf1";
    Hjson::Value undefined = val["first"]["down1"]["down2"];
    assert(!"Did not throw error when using brackets on string Value.");
  } catch (const Hjson::type_mismatch& e) {}

  {
    const Hjson::Value val;
    Hjson::Value undefined = val["down1"]["down2"]["down3"];
    assert(undefined.type() == Hjson::Type::Undefined);
    assert(!val.defined());
  }

  {
    Hjson::Value val;
    Hjson::Value undefined = val["down1"]["down2"]["down3"];
    assert(undefined.type() == Hjson::Type::Undefined);
    // The type of val is set to Map because a MapProxy is created, no easy way
    // to avoid that.
    //assert(!val.defined());
  }

  {
    Hjson::Value val;
    val["down1"]["down2"]["down3"] = "three levels deep!";
    std::string tld = val["down1"]["down2"]["down3"];
    assert(tld == "three levels deep!");
    assert(val["down1"]["down2"]["down3"] == "three levels deep!");
  }

  {
    Hjson::Value root;
    root["one"] = 1;
    {
      Hjson::Value test1 = root["one"];
      root.erase(0);
    }
    assert(root.empty());
  }

  {
    Hjson::Value val1, val2;

    val2 = val1;
    val2["test1"] = "t1";
    std::string t1 = val1["test1"];
    // Assert that the assignment "val2 = val1" was by reference, not by value.
    assert(t1 == "t1");
    assert(val1["test1"] == "t1");
    assert(val1["test1"] == std::string("t1"));
    assert(val2["test1"] == "t1");
    assert(val2["test1"] == std::string("t1"));
  }

  {
    Hjson::Value root;
    root["key1"]["key2"]["key3"]["A"] = 4;
    Hjson::Value val2 = root["key1"]["key2"]["key3"];
    val2["B"] = 5;
    assert(root["key1"]["key2"]["key3"]["B"] == 5);
  }

  {
    int a = 0;
    char *szBrackets = new char[19];
    for (; a < 10; a++) {
      szBrackets[a] = '[';
      szBrackets[++a] = '\n';
    }
    --a;
    for (; a < 18; a++) {
      szBrackets[a] = ']';
      szBrackets[++a] = '\n';
    }
    szBrackets[18] = 0;
    auto root = Hjson::Unmarshal(szBrackets);

    Hjson::EncoderOptions opt;
    opt.indentBy = "";
    auto res = Hjson::Marshal(root, opt);

    assert(!std::strcmp(res.c_str(), szBrackets));

    delete[] szBrackets;
  }

  {
    Hjson::Value node;
    node["a"] = 1;
    {
      Hjson::Value root;
      root["n"] = node;
    }
    assert(node.size() == 1);
  }

  {
    Hjson::Value node;
    node["a"] = 1;
    node["a2"] = 2;
    {
      Hjson::Value node2;
      node2["b"] = node;
      node2["c"] = "alfa";
      node2["d"] = Hjson::Value(Hjson::Type::Undefined);
      {
        Hjson::Value root;
        root["n"] = node2;
      }
      assert(node2.size() == 3);
    }
    assert(node.size() == 2);
  }

  {
    Hjson::Value val;
    try {
      val[0] = 0;
      assert(!"Did not throw error when trying to assign Value to Type::Vector index that is out of bounds.");
    } catch(const Hjson::index_out_of_bounds& e) {}
    try {
      const Hjson::Value val2;
      const auto val3 = val2[0];
      assert(!"Did not throw error when trying to access Type::Vector index that is out of bounds.");
    } catch(const Hjson::index_out_of_bounds& e) {}
    try {
      if (val[0].empty()) {
        assert(!"Did not throw error when trying to access Type::Vector index that is out of bounds.");
      }
      assert(!"Did not throw error when trying to access Type::Vector index that is out of bounds.");
    } catch(const Hjson::index_out_of_bounds& e) {}
    val.push_back("first");
    val.push_back(2);
    std::string f = val[0];
    Hjson::Value val2 = val[0];
    assert(val2 == std::string("first"));
    assert(val2 == "first");
    try {
      val2.push_back(0);
      assert(!"Did not throw error when trying to push_back() on Value that is not a Type::Vector.");
    } catch(const Hjson::type_mismatch& e) {}
    assert(val[1] == 2);
    assert(val[1].type() == Hjson::Type::Int64);
    val[0] = 3;
    assert(val[0] == 3);
    assert(val.size() == 2);
    try {
      val[2] = 2;
      assert(!"Did not throw error when trying to assign Value to Type::Vector index that is out of bounds.");
    } catch(const Hjson::index_out_of_bounds& e) {}
    try {
      if (val[2].empty()) {
        assert(!"Did not throw error when trying to access Type::Vector index that is out of bounds.");
      }
      assert(!"Did not throw error when trying to access Type::Vector index that is out of bounds.");
    } catch(const Hjson::index_out_of_bounds& e) {}
  }

  {
    Hjson::Value val;
    {
      Hjson::Value val2;
      val2.push_back("first");
      val = val2[0];
    }
    assert(val == "first");
  }

  {
    Hjson::Value val;
    Hjson::Value val2 = val["åäö"];
    assert(!val2.defined());
    assert(val["åäö"].type() == Hjson::Type::Undefined);
    // Assert that the comparison didn't create an element.
    assert(val.size() == 0);
    Hjson::Value sub1, sub2;
    val["abc"] = sub1;
    val["åäö"] = sub2;
    assert(val["åäö"].type() == Hjson::Type::Undefined);
    assert(!val["åäö"].defined());
    // Assert that explicit assignment creates an element.
    assert(val.size() == 2);
    std::string generatedHjson = Hjson::Marshal(val);
    assert(generatedHjson == "{}");
    Hjson::EncoderOptions options;
    options.preserveInsertionOrder = false;
    generatedHjson = Hjson::Marshal(val, options);
    assert(generatedHjson == "{}");
    sub1["sub1"] = "abc";
    sub2["sub2"] = "åäö";
    generatedHjson = Hjson::Marshal(val);
    assert(generatedHjson == "{\n  abc: {\n    sub1: abc\n  }\n  åäö: {\n    sub2: åäö\n  }\n}");
    {
      std::stringstream ss;
      ss << val;
      assert(ss.str() == "{\n  abc: {\n    sub1: abc\n  }\n  åäö: {\n    sub2: åäö\n  }\n}");
    }
    Hjson::Value val3 = Hjson::Unmarshal(generatedHjson.c_str(), generatedHjson.size());
    assert(val3["abc"].defined());
    assert(val3["åäö"]["sub2"] == val["åäö"]["sub2"]);
    assert(val3.deep_equal(val));
    sub2["sub3"] = "sub3";
    assert(!val3.deep_equal(val));
  }

  {
    Hjson::Value val;
    if (val) {
      val.push_back(0);
    }
    assert(val.empty());
    if (!val) {
      val.push_back(0);
    }
    assert(!val.empty());
    assert(val.size() == 1);
    assert(val[0] == 0);
    if (val[0]) {
      assert(!"A 0 Value should be treated as false in boolean expressions.");
    }
    assert(!val[0].empty());
    assert(!val[0]);
    int val0 = val[0];
    assert(val0 == 0);
    double valD = val[0];
    assert(valD == 0);
    // The result of the comparison is undefined in C++11.
    // assert(val.begin() == val.end());
  }

  {
    Hjson::Value val;
    if (val.erase("key1")) {
      assert(!"Returned non-zero number when trying to do Type::Map erase on Type::Undefined Value.");
    }
    val["key1"] = "first";
    val["key2"] = "second";
    val.erase("key1");
    assert(val.size() == 1);
    assert(val["key1"].empty());
    if (val.erase("key1")) {
      assert(!"Returned non-zero number when trying to do Type::Map erase with a non-existing key.");
    }
    val.erase(std::string("key2"));
    assert(val.empty());
    if (val.erase("key1")) {
      assert(!"Returned non-zero number when trying to do Type::Map erase on an empty Type::Map.");
    }
    Hjson::Value val2("secondVal");
    try {
      val2.erase("key1");
      assert(!"Did not throw error when trying to do Type::Map erase on a STRING Value.");
    } catch(const Hjson::type_mismatch& e) {}
  }

  {
    Hjson::Value val;
    try {
      val.erase(1);
      assert(!"Did not throw error when trying to do Type::Vector erase on Type::Undefined Value.");
    } catch(const Hjson::index_out_of_bounds& e) {}
    val.push_back("first");
    val.push_back("second");
    Hjson::Value val2;
    val2["down1"] = "third";
    val.push_back(val2);
    assert(val[2]["down1"] == "third");
    val.erase(2);
    val.erase(0);
    assert(val.size() == 1);
    try {
      val.erase(1);
      assert(!"Did not throw error when trying to do Type::Vector erase with an out-of-bounds index.");
    } catch(const Hjson::index_out_of_bounds& e) {}
    val.erase(0);
    assert(val.empty());
    try {
      val.erase(0);
      assert(!"Did not throw error when trying to do Type::Vector erase on an empty Type::Vector.");
    } catch(const Hjson::index_out_of_bounds& e) {}
    Hjson::Value val3(3);
    try {
      val3.erase(0);
      assert(!"Did not throw error when trying to do Type::Vector erase on a Type::Double Value.");
    } catch(const Hjson::type_mismatch& e) {}
  }

  {
    Hjson::Value root = Hjson::Unmarshal("[3,4,5]", 8);
    assert(root[0] == 3);
    assert(root[1] == 4);
    assert(root[2] == 5);
    assert(root.size() == 3);
    std::string generatedHjson = Hjson::Marshal(root);
    assert(generatedHjson == "[\n  3\n  4\n  5\n]");
    Hjson::Value root2;
    root2.push_back(3);
    root2.push_back(4);
    root2.push_back(5);
    assert(root2.deep_equal(root));
  }

  {
    Hjson::Value val1;
    Hjson::Value val2;
    assert(val1 == val2);

    val1 = 3;
    val2 = 3;
    assert(val1 == val2);

    val1 = "alpha";
    val2 = "alpha";
    assert(val1 == val2);
  }

  {
    Hjson::Value root, val(0);
    root.push_back(1.0/val);
    root.push_back(std::sqrt(-1));
    std::string generatedHjson = Hjson::Marshal(root);
    assert(generatedHjson == "[\n  null\n  null\n]");
  }

  {
    Hjson::Value val1, val2;
    assert(val1.deep_equal(val2));
    val1 = 1;
    assert(!val1.deep_equal(val2));
    val2 = 1;
    assert(val1.deep_equal(val2));
    val1 = Hjson::Value();
    val1.push_back(2);
    assert(!val1.deep_equal(val2));
    val2 = Hjson::Value();
    val2["2"] = 2;
    assert(!val1.deep_equal(val2));
    val1 = Hjson::Value(Hjson::Type::Vector);
    val2 = Hjson::Value(Hjson::Type::Vector);
    assert(val1.deep_equal(val2));
    val1 = Hjson::Value(Hjson::Type::Map);
    assert(!val1.deep_equal(val2));
    val2 = Hjson::Value(Hjson::Type::Map);
    assert(val1.deep_equal(val2));
  }

  {
    Hjson::Value val1;
    val1["first"] = 1;
    Hjson::Value val2 = val1.clone();
    val1["second"] = 2;
    assert(val2.size() == 1);
    val1["third"]["first"] = 3;
    val2 = val1.clone();
    val2["third"]["second"] = 4;
    // size() is the number of child elements, can only be > 0 for Vector or Map. 
    assert(val1["first"].size() == 0);
  }

  {
    Hjson::Value val1;
    val1["zeta"] = 1;
    val1["y"] = 2;
    val1["xerxes"]["first"] = 3;
    assert(val1[0] == 1);
    val1[0] = 99;
    assert(val1["zeta"] == 99);
    assert(val1.key(2) == "xerxes");
    val1.move(0, 3);
    assert(val1.key(0) == "y");
    assert(val1[2] == 99);
    val1.move(1, 0);
    auto str = Hjson::Marshal(val1);
    assert(str == "{\n  xerxes: {\n    first: 3\n  }\n  y: 2\n  zeta: 99\n}");
    assert(val1[0]["first"] == 3);
    assert(val1.key(1) == "y");
  }

  {
    Hjson::Value val1;
    val1.push_back(1);
    Hjson::Value val2 = val1.clone();
    val1.push_back(2);
    assert(val2.size() == 1);
    val1.push_back(val2);
    val2 = val1.clone();
    val2[2].push_back(3);
    assert(val1[2].size() == 1);
  }

  {
    std::string baseStr = R"({
  debug: false
  rect: {
    x: 0
    y: 0
    width: 800
    height: 600
  }
  path: C:/temp
  seq: [
    0
    1
    2
  ]
  scale: 3
  window: {
    x: 13
    y: 37
    width: 200
    height: 200
  }
})";
    Hjson::Value base = Hjson::Unmarshal(baseStr);

    Hjson::Value ext = Hjson::Unmarshal(R"(
{
  debug: true
  rect: {
    x: 0
    y: 0
    height: 480
  }
  path: /tmp
  seq: [
    8
    9
  ]
  otherWindow: {
    x: 17
  }
}
)");

    Hjson::Value merged = Hjson::Merge(base, ext);
    assert(merged["debug"] == true);
    assert(merged["rect"]["width"] == 800);
    assert(merged["rect"]["height"] == 480);
    assert(merged["path"] == "/tmp");
    assert(merged["seq"].size() == 2);
    assert(merged["seq"][1] == 9);
    assert(merged["scale"] == 3);
    assert(merged["window"]["y"] == 37);
    assert(merged["otherWindow"]["x"] == 17);
    // The insertion order must have been kept in the merge.
    assert(merged.key(1) == "rect");
    // The insertion order must have been kept in the clone.
    auto baseClone = base.clone();
    auto baseCloneStr = Hjson::Marshal(baseClone);
    assert(baseCloneStr == baseStr);
    Hjson::EncoderOptions options;
    options.bracesSameLine = true;
    options.preserveInsertionOrder = true;
    baseCloneStr = Hjson::Marshal(baseClone, options);
    assert(baseCloneStr == baseStr);
  }

  {
    Hjson::Value val1;
    val1.push_back(1);
    Hjson::Value val2 = val1.clone();
    val1.push_back(2);
    assert(val2.size() == 1);
    val1.push_back(val2);
    val2 = val1.clone();
    val2[2].push_back(3);
    assert(val1[2].size() == 1);
  }

  {
    std::string baseStr = R"(// base 1
debug: false # base 2
# Still base 2
extraKey: yes
// base 2.1
rect: {
// base 3
  x: 0 // base 4
  // base 5
  y: 0
# base 6
  width: 800
  /* base 7 */
  height: 600
  // base 8
}
// base 9
path: C:/temp
// base 10
seq: [
  // base 11
  0
  # base 12
  1   /* base 13 */
  2
/* base 14 */
]
// base 15
scale: 3
// base 16
window: {
    # base 17
  x: 13
  y: 37
  width: 200
  height: 200
}

// base 18


)";

    std::string extStr = R"(

/* ext 1*/

debug: true
    /* ext 2 */
rect: {
// ext 3
  x: 0
  y: 0
  height: 480
    # ext 4
} // ext 5
path: /tmp
// ext 6
seq: [
  8
  9
]
// ext 8
otherWindow: {
  x: 17
}
)";

    std::string mergedStr = R"(

/* ext 1*/

debug: true
    /* ext 2 */
rect: {
// ext 3
  x: 0
  y: 0
  height: 480
    # ext 4

# base 6
  width: 800
} // ext 5
path: /tmp
// ext 6
seq: [
  8
  9
]
// ext 8
otherWindow: {
  x: 17
}

# Still base 2
extraKey: yes
// base 15
scale: 3
// base 16
window: {
    # base 17
  x: 13
  y: 37
  width: 200
  height: 200
}

// base 18


)";

    Hjson::DecoderOptions decOpt;
    decOpt.whitespaceAsComments = true;
    Hjson::Value base = Hjson::Unmarshal(baseStr, decOpt);
    Hjson::Value ext = Hjson::Unmarshal(extStr, decOpt);
    Hjson::Value merged = Hjson::Merge(base, ext);
    assert(merged["debug"] == true);
    assert(merged["rect"]["width"] == 800);
    assert(merged["rect"]["height"] == 480);
    assert(merged["path"] == "/tmp");
    assert(merged["seq"].size() == 2);
    assert(merged["seq"][1] == 9);
    assert(merged["scale"] == 3);
    assert(merged["window"]["y"] == 37);
    assert(merged["otherWindow"]["x"] == 17);
    // The insertion order of "ext" must have been kept in the merge.
    assert(merged.key(1) == "rect");
    // The comments from "ext" must have been kept in the merge.
    assert(merged["rect"].get_comment_after() == " // ext 5");
    // The insertion order and comments must have been kept in the clone.
    auto baseClone = base.clone();
    Hjson::EncoderOptions encOpt;
    encOpt.bracesSameLine = true;
    encOpt.preserveInsertionOrder = true;
    encOpt.omitRootBraces = true;
    auto baseCloneStr = Hjson::Marshal(baseClone, encOpt);
    assert(baseCloneStr == baseStr);
    auto extClone = ext.clone();
    auto extCloneStr = Hjson::Marshal(extClone, encOpt);
    assert(extCloneStr == extStr);
    auto mergedStrResult = Hjson::Marshal(merged, encOpt);
    assert(mergedStrResult == mergedStr);
  }

  {
    auto noRootBraces = R"(alfa: a
beta: b
obj: {
  number: 1
}
arr: [
  0
  1
  2
])";

    Hjson::EncoderOptions options;
    options.bracesSameLine = true;
    options.preserveInsertionOrder = true;
    options.omitRootBraces = true;

    auto root = Hjson::Unmarshal(noRootBraces);
    auto newStr = Hjson::Marshal(root, options);
    assert(newStr == noRootBraces);
  }

  {
    auto noLineFeedAtEnd = R"(alfa: a  // cm 1
beta: a// cm 2)";

    Hjson::DecoderOptions decOpt;
    decOpt.whitespaceAsComments = true;
    auto root = Hjson::Unmarshal(noLineFeedAtEnd, decOpt);

    auto newStr = Hjson::Marshal(root);

    auto expectedStr = R"({
  alfa: a  // cm 1
beta: a// cm 2
})";

    assert(newStr == expectedStr);

    auto lineFeedAtEnd = R"(alfa: a  // cm 1
beta: a// cm 2
)";

    root = Hjson::Unmarshal(lineFeedAtEnd, decOpt);
    newStr = Hjson::Marshal(root);
    assert(newStr == expectedStr);
  }

  {
    const char *szTmp = "tmpTestFile.hjson";

    auto root1 = Hjson::UnmarshalFromFile("assets/charset_test.hjson");
    assert(!root1.empty());
    try {
      root1 = Hjson::UnmarshalFromFile("does_not_exist");
      assert(!"Did not throw error for trying to open non-existing file");
    } catch(const Hjson::file_error& e) {}

    Hjson::MarshalToFile(root1, szTmp);
    try {
      Hjson::MarshalToFile(root1, "");
      assert(!"Did not throw error for trying to write to invalid filename");
    } catch(const Hjson::file_error& e) {}

    auto root2 = Hjson::UnmarshalFromFile(szTmp);
    assert(root2.deep_equal(root1));
    std::remove(szTmp);
  }

  {
    const char *szTmp = "tmpTestFile.hjson";
    Hjson::DecoderOptions decOpt;
    Hjson::EncoderOptions encOpt;

    decOpt.comments = true;
    encOpt.comments = true;

    auto root1 = Hjson::UnmarshalFromFile("assets/comments6_test.hjson", decOpt);
    assert(!root1.empty());

    Hjson::MarshalToFile(root1, szTmp, encOpt);
    auto root2 = Hjson::UnmarshalFromFile(szTmp, decOpt);
    assert(root2.deep_equal(root1));
    assert(root2.get_comment_after() == root1.get_comment_after());
    std::remove(szTmp);
  }

  {
    Hjson::Value val1(1), val2(2);

    assert(val1.get_comment_after() == "");

    val1.set_comment_after("after1");
    val2.set_comment_after("after2");

    val1 = val2;
    assert(val1.get_comment_after() == "after1");
    val1 = 3;
    assert(val1.get_comment_after() == "after1");
    assert(val2.get_comment_after() == "after2");

    Hjson::Value val3;
    val3["one"] = val1;
    val3["one"].set_comment_after("afterOne");
    val3["one"] = val2;
    assert(val3["one"].get_comment_after() == "afterOne");
    assert(val2.get_comment_after() == "after2");
    val2 = val3["one"];
    assert(val2.get_comment_after() == "after2");

    auto fnValOne = [](const Hjson::Value& val) {
      return val;
    };

    Hjson::Value val4 = fnValOne(val1);
    // val4 was created, should get the comments.
    assert(val4.get_comment_after() == "after1");

    val4 = fnValOne(val2);
    // val4 already existed, should not get new comments.
    assert(val4.get_comment_after() == "after1");

    auto fnValTwo = [](Hjson::Value val) {
      return val;
    };

    Hjson::Value val5 = fnValTwo(val1);
    // val5 was created, should get the comments.
    assert(val5.get_comment_after() == "after1");

    val5 = fnValTwo(val2);
    // val5 already existed, should not get new comments.
    assert(val5.get_comment_after() == "after1");

    Hjson::Value val6 = val1;
    // val6 was created, should get the comments.
    assert(val6.get_comment_after() == "after1");

    val6 = val2;
    // val6 already existed, should not get new comments.
    assert(val6.get_comment_after() == "after1");

    Hjson::Value val7;
    val7.push_back(val1);
    assert(val7[0].get_comment_after() == "after1");
    val7[0] = val2;
    assert(val7[0].get_comment_after() == "after1");

    val1.clear_comments();
    assert(val1.get_comment_after() == "");
    assert(val6.get_comment_after() == "after1");
    assert(val7[0].get_comment_after() == "after1");

    val5.set_comment_after("after5");
    assert(val6.get_comment_after() == "after1");
    assert(val7[0].get_comment_after() == "after1");

    val1.set_comments(val3["one"]);
    assert(val1.get_comment_after() == "afterOne");

    val3["one"].set_comment_after("after3");
    assert(val1.get_comment_after() == "afterOne");

    val1.set_comments(val2);
    val2.set_comment_after("afterTwo");
    assert(val1.get_comment_after() == "after2");

    Hjson::Value val8;
    val1.set_comments(val8);
    assert(val1.get_comment_after() == "");

    Hjson::Value val9;
    val8.set_comments(val9);
    assert(val8.get_comment_after() == "");
  }

  {
    Hjson::Value rootA;
    rootA["one"] = "uno";
    rootA["one"].set_comment_after("afterOne");

    {
      Hjson::Value val1 = rootA["one"];
      rootA["one"].set_comment_after("afterTwo");
      assert(rootA["one"].get_comment_after() == "afterTwo");
      assert(val1.get_comment_after() == "afterOne");

      Hjson::Value val2(rootA["one"]);
      rootA["one"].set_comment_after("afterThree");
      assert(rootA["one"].get_comment_after() == "afterThree");
      assert(val2.get_comment_after() == "afterTwo");

      // Comments are not changed in this assignment, val2 is not undefined.
      val2 = rootA["one"];
      rootA["one"].set_comment_after("afterFour");
      assert(rootA["one"].get_comment_after() == "afterFour");
      assert(val2.get_comment_after() == "afterTwo");
    }

    assert(rootA["one"].get_comment_after() == "afterFour");
  }

  {
    Hjson::Value root(Hjson::Type::Map);
    root.set_comment_inside("\n  // comment inside\n");
    root["one"] = 1;
    root["one"].set_comment_after(" # afterOne");
    root["two"] = 2;
    root["twoB"] = "2b";
    root["twoC"] = "2c";
    root["twoC"].set_comment_key("\n  // key comment for 2c\n  ");
    root["three"] = 3;
    root["three"].set_comment_before("\n  # beforeThree\n  ");
    root["three"] = 3; // Should not remove the comment
    root["three"].set_comment_after("\n  # final comment\n");
    Hjson::EncoderOptions opt;
    opt.separator = true;
    auto str = Hjson::Marshal(root, opt);
    assert(str == R"({
  // comment inside
  one: 1, # afterOne
  two: 2,
  twoB: "2b",
  twoC:
  // key comment for 2c
  "2c",
  # beforeThree
  three: 3
  # final comment
})");
  }

  {
    Hjson::Value root(Hjson::Type::Vector);
    root.set_comment_inside("\n  // comment inside\n");
    root.push_back(1);
    root[0].set_comment_after(" # afterOne");
    root.push_back(2);
    root.push_back("2b");
    root.push_back("2c");
    root[3].set_comment_key("\n  // key comment for 2c\n  ");
    root.push_back(3);
    root[4].set_comment_before("\n  # beforeThree\n  ");
    root[4] = 3; // Should not remove the comment
    root[4].set_comment_after("\n  # final comment\n");
    Hjson::EncoderOptions opt;
    opt.separator = true;
    auto str = Hjson::Marshal(root, opt);
    assert(str == R"([
  // comment inside
  1, # afterOne
  2,
  "2b",
  // key comment for 2c
  "2c",
  # beforeThree
  3
  # final comment
])");
  }

  {
    auto txt = R"([ 0, 1, 'c', 3, /*4, 5,*/ 6, 'h', /*'i', */'j', 'k' ])";

    Hjson::DecoderOptions decOpt;
    decOpt.whitespaceAsComments = true;
    auto root = Hjson::Unmarshal(txt, decOpt);

    Hjson::EncoderOptions encOpt;
    encOpt.separator = true;
    auto newStr = Hjson::Marshal(root, encOpt);

    auto expectedStr = R"([ 0, 1, "c", 3, /*4, 5,*/ 6, "h", /*'i', */"j", "k" ])";
    assert(newStr == expectedStr);

    encOpt.separator = false;
    newStr = Hjson::Marshal(root, encOpt);

    expectedStr = R"([
   0
   1
   c
   3
   /*4, 5,*/ 6
   h
   /*'i', */j
   k 
])";
    assert(newStr == expectedStr);

    decOpt.whitespaceAsComments = false;
    root = Hjson::Unmarshal(txt, decOpt);
    newStr = Hjson::Marshal(root, encOpt);

    expectedStr = R"([
  0
  1
  c
  3
   /*4, 5,*/ 6
  h
   /*'i', */j
  k
])";
    assert(newStr == expectedStr);

    encOpt.separator = true;
    newStr = Hjson::Marshal(root, encOpt);

    expectedStr = R"([
  0,
  1,
  "c",
  3, /*4, 5,*/ 6,
  "h", /*'i', */"j",
  "k"
])";
    assert(newStr == expectedStr);
  }

  {
    auto txt = R"({ k1: 0, k2:1, k3: 'c', k4: 3, /*k5:4, k6 : 5,*/ k7 : 6, k8:'h', /*k9:'i', */k10:'j', k11 : 'k' })";

    Hjson::DecoderOptions decOpt;
    decOpt.whitespaceAsComments = true;
    auto root = Hjson::Unmarshal(txt, decOpt);

    Hjson::EncoderOptions encOpt;
    encOpt.separator = true;
    auto newStr = Hjson::Marshal(root, encOpt);

    auto expectedStr = R"({ k1: 0, k2: 1, k3: "c", k4: 3, /*k5:4, k6 : 5,*/ k7: 6, k8: "h", /*k9:'i', */k10: "j", k11: "k" })";
    assert(newStr == expectedStr);

    encOpt.separator = false;
    newStr = Hjson::Marshal(root, encOpt);

    expectedStr = R"({ k1: 0
   k2: 1
   k3: c
   k4: 3
   /*k5:4, k6 : 5,*/ k7: 6
   k8: h
   /*k9:'i', */k10: j
   k11: k 
})";
    assert(newStr == expectedStr);

    decOpt.whitespaceAsComments = false;
    root = Hjson::Unmarshal(txt, decOpt);
    newStr = Hjson::Marshal(root, encOpt);

    expectedStr = R"({
  k1: 0
  k2: 1
  k3: c
  k4: 3
   /*k5:4, k6 : 5,*/ k7: 6
  k8: h
   /*k9:'i', */k10: j
  k11: k
})";
    assert(newStr == expectedStr);

    encOpt.separator = true;
    newStr = Hjson::Marshal(root, encOpt);

    expectedStr = R"({
  k1: 0,
  k2: 1,
  k3: "c",
  k4: 3, /*k5:4, k6 : 5,*/ k7: 6,
  k8: "h", /*k9:'i', */k10: "j",
  k11: "k"
})";
    assert(newStr == expectedStr);
  }

  {
    Hjson::Value val("");
    val.set_comment_key("// key comment\n");
    val.set_comment_after("\n# comment after");
    auto str = Hjson::Marshal(val);
    assert(str == R"(// key comment
""
# comment after)");
  }

  {
    Hjson::Value val("");
    val.set_comment_key("// key comment\n");
    val.set_comment_before("\n# comment before\n");
    val.set_comment_inside("/* comment inside */");
    auto str = Hjson::Marshal(val);
    assert(str == R"(
# comment before
// key comment
"")");
  }

  {
    std::string str = R"(

 [

awfoen
3
   # comment
{
  a: a
   b:   b
  #yes
 c: "c" // c-comment
}
[
1
2
]
]
)";
    Hjson::DecoderOptions decOpt;
    decOpt.whitespaceAsComments = true;
    auto root = Hjson::Unmarshal(str, decOpt);
    auto str2 = Hjson::Marshal(root);
    assert(str2 == str);
  }

  {
    auto str1 = R"(#comment a
alfa: "a"
beta: "b")";

    auto strPlain = R"({#comment a
alfa: a
  beta: b
})";

    Hjson::DecoderOptions decOpt;
    decOpt.comments = true;
    auto root = Hjson::Unmarshal(str1, decOpt);
    std::ostringstream oss;
    Hjson::EncoderOptions encOpt;
    encOpt.quoteAlways = true;
    encOpt.omitRootBraces = true;
    oss << Hjson::StreamEncoder(root, encOpt);
    auto str2 = oss.str();
    assert(str2 == str1);
    oss.str("");
    oss << root;
    str2 = oss.str();
    assert(str2 == strPlain);
    Hjson::Value root2;
    std::stringstream ss(str1);
    ss >> root2;
    assert(root2.deep_equal(root));
    ss.str(str1);
    ss >> Hjson::StreamDecoder(root2, decOpt);
    assert(root2.deep_equal(root));
    str2 = Hjson::Marshal(root2, encOpt);
    assert(str2 == str1);
    Hjson::StreamEncoder se(root, encOpt);
    oss.str("");
    oss << se;
    str2 = oss.str();
    assert(str2 == str1);
    Hjson::StreamDecoder sd(root2, decOpt);
    ss.str(str1);
    ss >> sd;
    assert(root2.deep_equal(root));
  }

  {
    std::string str = R"(
key: val1
key: val2
)";
    auto root = Hjson::Unmarshal(str);
    try {
      Hjson::DecoderOptions decOpt;
      decOpt.duplicateKeyException = true;
      root = Hjson::Unmarshal(str, decOpt);
      assert(!"Did not throw error for duplicate key");
    } catch(const Hjson::syntax_error& e) {}
  }
}
