#include <hjson.h>
#include <assert.h>
#include <cmath>
#include <cstring>


static std::string _test_string_param(std::string param) {
  return param;
}


void test_value() {
  std::int64_t ii = 1000000;
  ii *= ii;
  Hjson::Value hval(ii);
  printf("%lld\n", hval.to_int64());
  assert(hval.to_int64() == ii);

  {
    Hjson::Value valVec(Hjson::Value::VECTOR);
    assert(valVec.type() == Hjson::Value::VECTOR);
    Hjson::Value valMap(Hjson::Value::MAP);
    assert(valMap.type() == Hjson::Value::MAP);
  }

  {
    Hjson::Value val(true);
    assert(val.type() == Hjson::Value::BOOL);
    assert(val);
    assert(val == true);
    assert(val != false);
    assert(true == (bool) val);
    assert(false != (bool) val);
    val = false;
    assert(!val);
    assert(!val.empty());
    assert(val.size() == 1);
    assert(val.to_string() == "false");
    assert(val.to_double() == 0);
    val = true;
    assert(val.to_double() == 1);
    assert(val.to_string() == "true");
    assert(val.begin() == val.end());
  }

  {
    Hjson::Value val(Hjson::Value::HJSON_NULL);
    assert(val.type() == Hjson::Value::HJSON_NULL);
    assert(!val);
    assert(val.empty());
    assert(val.size() == 0);
    Hjson::Value val2(Hjson::Value::HJSON_NULL);
    assert(val == val2);
    Hjson::Value val3;
    assert(val != val3);
    assert(val.to_double() == 0);
    assert(val.to_string() == "null");
    assert(val3.to_double() == 0);
    assert(val3.to_string() == "");
    assert(val.begin() == val.end());
    assert(val3.begin() == val3.end());
  }

  {
    const Hjson::Value val = 3.0;
    assert(val == 3.0);
    assert(val != 4.0);
    assert(3.0 == val);
    assert(4.0 != val);
    double third = val;
    assert(val == 3);
    assert(val != 2);
    assert(3 == val);
    assert(2 != val);
    assert(val < 4);
    assert(4 > val);
    assert(val < 4.0);
    assert(4.0 > val);
    assert(val * 3 == 9);
    assert(3 * val == 9);
    assert(val * 3.0 == 9.0);
    assert(3.0 * val == 9.0);
    assert(val / 3 == 1);
    assert(3 / val == 1);
    assert(val / 3.0 == 1.0);
    assert(3.0 / val == 1.0);
    assert(val + 1 == 4);
    assert(1 + int(val) == 4);
    assert(val + 1.0 == 4.0);
    assert(1.0 + val == 4.0);
    assert(val - 1 == 2);
    assert(1 - val == -2);
    assert(val - 1.0 == 2.0);
    assert(1.0 - val == -2.0);
    assert(val.to_double() == 3);
    assert(val.to_string() == "3");
    assert(val.begin() == val.end());
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
    Hjson::Value val("alpha");
    Hjson::Value val2 = "alpha";
    assert(val == val2);
    assert(val != "beta");
    assert("beta" != val);
    assert(_test_string_param(val) == "alpha");
    val = std::string("alpha");
    std::string st = val;
    assert(val == val2);
    assert(val2 == std::string("alpha"));
    assert(val2 != std::string("beta"));
    assert(std::string("alpha") == val2.operator const std::string());
    assert(std::string("beta") != val2.operator const std::string());
    assert(val.to_double() == 0);
    assert(val.to_string() == "alpha");
    assert(val.begin() == val.end());
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
    assert(val.begin() == val.end());
  }

  {
    Hjson::Value val("3.0");
    assert(val.to_double() == 3);
    Hjson::Value val2(3.0);
    assert(val != val2);
    assert(!(val == val2));
    try {
      bool a = val > val2;
      assert(!"Did not throw error when using < operator on values of different types.");
    } catch(Hjson::type_mismatch e) {}
    try {
      bool a = val < val2;
      assert(!"Did not throw error when using < operator on values of different types.");
    } catch(Hjson::type_mismatch e) {}
    try {
      std::string a = val + val2;
      assert(!"Did not throw error when using + operator on values of different types.");
    } catch(Hjson::type_mismatch e) {}
    try {
      double a = val2 - val;
      assert(!"Did not throw error when using - operator on values of different types.");
    } catch(Hjson::type_mismatch e) {}
    try {
      Hjson::Value val3 = val - Hjson::Value("0");
      assert(!"Did not throw error when using - operator on value of type STRING.");
    } catch(Hjson::type_mismatch e) {}
  }

  {
    Hjson::Value val;
    val["first"] = "leaf1";
    Hjson::Value val2 = val["first"];
    val["second"] = val["first"];
    val["fourth"] = 4.0;
    double fourth = val["fourth"];
    assert(fourth == val["fourth"]);
    try {
      std::string fourthString = val["fourth"];
      assert(!"Did not throw error when assigning double to string.");
    } catch(Hjson::type_mismatch e) {}
    std::string leaft1 = val["first"];
    assert(leaft1 == "leaf1");
    assert(val[std::string("first")] == "leaf1");
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

  try {
    Hjson::Value val;
    val["first"] = "leaf1";
    Hjson::Value undefined = val["first"]["down1"]["down2"];
    assert(!"Did not throw error when using brackets on string Value.");
  } catch (Hjson::type_mismatch e) {}

  {
    Hjson::Value val;
    Hjson::Value undefined = val["down1"]["down2"]["down3"];
    assert(undefined.type() == Hjson::Value::UNDEFINED);
  }

  {
    Hjson::Value val;
    val["down1"]["down2"]["down3"] = "three levels deep!";
    std::string tld = val["down1"]["down2"]["down3"];
    assert(tld == "three levels deep!");
    assert(val["down1"]["down2"]["down3"] == "three levels deep!");
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
    auto val2 = root["key1"]["key2"]["key3"];
    val2["B"] = 5;
    assert(root["key1"]["key2"]["key3"]["B"] == 5);
  }

  {
    Hjson::Value val;
    try {
      val[0] = 0;
      assert(!"Did not throw error when trying to assign Value to VECTOR index that is out of bounds.");
    } catch(Hjson::index_out_of_bounds e) {}
    try {
      const Hjson::Value val2;
      const auto val3 = val2[0];
      assert(!"Did not throw error when trying to access VECTOR index that is out of bounds.");
    } catch(Hjson::index_out_of_bounds e) {}
    try {
      if (val[0].empty()) {
        assert(!"Did not throw error when trying to access VECTOR index that is out of bounds.");
      }
      assert(!"Did not throw error when trying to access VECTOR index that is out of bounds.");
    } catch(Hjson::index_out_of_bounds e) {}
    val.push_back("first");
    val.push_back(2);
    std::string f = val[0];
    Hjson::Value val2 = val[0];
    assert(val2 == std::string("first"));
    assert(val2 == "first");
    try {
      val2.push_back(0);
      assert(!"Did not throw error when trying to push_back() on Value that is not a VECTOR.");
    } catch(Hjson::type_mismatch e) {}
    assert(val[1] == 2);
    assert(val[1].type() == Hjson::Value::DOUBLE);
    val[0] = 3;
    assert(val[0] == 3);
    assert(val.size() == 2);
    try {
      val[2] = 2;
      assert(!"Did not throw error when trying to assign Value to VECTOR index that is out of bounds.");
    } catch(Hjson::index_out_of_bounds e) {}
    try {
      if (val[2].empty()) {
        assert(!"Did not throw error when trying to access VECTOR index that is out of bounds.");
      }
      assert(!"Did not throw error when trying to access VECTOR index that is out of bounds.");
    } catch(Hjson::index_out_of_bounds e) {}
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
    assert(val["åäö"].type() == Hjson::Value::UNDEFINED);
    // Assert that the comparison didn't create an element.
    assert(val.size() == 0);
    Hjson::Value sub1, sub2;
    val["abc"] = sub1;
    val["åäö"] = sub2;
    assert(val["åäö"].type() == Hjson::Value::UNDEFINED);
    assert(!val["åäö"].defined());
    // Assert that explicit assignment creates an element.
    assert(val.size() == 2);
    std::string generatedHjson = Hjson::Marshal(val);
    assert(generatedHjson == "{\n}");
    sub1["sub1"] = "abc";
    sub2["sub2"] = "åäö";
    generatedHjson = Hjson::Marshal(val);
    assert(generatedHjson == "{\n  abc:\n  {\n    sub1: abc\n  }\n  åäö:\n  {\n    sub2: åäö\n  }\n}");
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
    assert(val.begin() == val.end());
  }

  {
    Hjson::Value val;
    if (val.erase("key1")) {
      assert(!"Returned non-zero number when trying to do MAP erase on UNDEFINED Value.");
    }
    val["key1"] = "first";
    val["key2"] = "second";
    val.erase("key1");
    assert(val.size() == 1);
    assert(val["key1"].empty());
    if (val.erase("key1")) {
      assert(!"Returned non-zero number when trying to do MAP erase with a non-existing key.");
    }
    val.erase(std::string("key2"));
    assert(val.empty());
    if (val.erase("key1")) {
      assert(!"Returned non-zero number when trying to do MAP erase on an empty MAP.");
    }
    Hjson::Value val2("secondVal");
    try {
      val2.erase("key1");
      assert(!"Did not throw error when trying to do MAP erase on a STRING Value.");
    } catch(Hjson::type_mismatch e) {}
  }

  {
    Hjson::Value val;
    try {
      val.erase(1);
      assert(!"Did not throw error when trying to do VECTOR erase on UNDEFINED Value.");
    } catch(Hjson::index_out_of_bounds e) {}
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
      assert(!"Did not throw error when trying to do VECTOR erase with an out-of-bounds index.");
    } catch(Hjson::index_out_of_bounds e) {}
    val.erase(0);
    assert(val.empty());
    try {
      val.erase(0);
      assert(!"Did not throw error when trying to do VECTOR erase on an empty VECTOR.");
    } catch(Hjson::index_out_of_bounds e) {}
    Hjson::Value val3(3);
    try {
      val3.erase(0);
      assert(!"Did not throw error when trying to do VECTOR erase on a DOUBLE Value.");
    } catch(Hjson::type_mismatch e) {}
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
    val1 = Hjson::Value(Hjson::Value::VECTOR);
    val2 = Hjson::Value(Hjson::Value::VECTOR);
    assert(val1.deep_equal(val2));
    val1 = Hjson::Value(Hjson::Value::MAP);
    assert(!val1.deep_equal(val2));
    val2 = Hjson::Value(Hjson::Value::MAP);
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
    assert(val1["first"].size() == 1);
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
    Hjson::Value base = Hjson::Unmarshal(R"(
{
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
}
)");

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
  }
}
