#include <gtest/gtest.h>
#include <iostream>
#include <exception>
#include "fjson.h"

using namespace fjson;


TEST(JsonTest, JsonArray)
{
    Json json = {1., 2., 3.};
    ASSERT_EQ(json[0].ToDouble(), 1.);
    ASSERT_EQ(json[1].ToDouble(), 2.);
    ASSERT_EQ(json[2].ToDouble(), 3.);

    json = Json(JsonValueType::Array);
    ASSERT_EQ(json.size(), 0);
    json.resize(1);
    ASSERT_EQ(json.size(), 1);
    json.resize(json.size() + 1);
    ASSERT_EQ(json.size(), 2);
    ASSERT_EQ(json[1].GetType(), JsonValueType::InvalidValue);
}


TEST(JsonTest, JsonString)
{
    Json json;
    json = "test";
    ASSERT_EQ(json.GetStringRef(), L"test");
}

TEST(JsonTest, JsonObject)
{
    Json json = {
        {"test", 0.2 + 0.1}, 
        {"pi", 3.14}, 
        {"nested", {
                {"key", "value"}, 
                {"ok", true}
            }
        }, 
        {"1." , 2. }
    };
    ASSERT_EQ(json["nested"]["key"].GetStringRef(), L"value");
}

TEST(JsonTest, JsonParseNumber)
{
    string_type s(L"3.14");
    Json json;
    string_type::difference_type i;
    ASSERT_NO_THROW(i = _ParseNumber(json, s.begin(), s.end()));
    ASSERT_EQ(i, 4);
    ASSERT_EQ(json.ToDouble(), 3.14);

    s = L"  3.14  ";
    ASSERT_NO_THROW(i = _ParseNumber(json, s.begin(), s.end()));
    ASSERT_EQ(i, 6);
    ASSERT_EQ(json.ToDouble(), 3.14);

    s = L"3.14, ";
    ASSERT_NO_THROW(i = _ParseNumber(json, s.begin(), s.end()));
    ASSERT_EQ(i, 4);
    ASSERT_EQ(json.ToDouble(), 3.14);

    s = L"3.14e-10";
    ASSERT_NO_THROW(i = _ParseNumber(json, s.begin(), s.end()));
    ASSERT_EQ(i, 8);
    ASSERT_EQ(json.ToDouble(), 3.14e-10);

    s = L"3.14e-10 ";
    ASSERT_NO_THROW(i = _ParseNumber(json, s.begin(), s.end()));
    ASSERT_EQ(i, 8);
    ASSERT_EQ(json.ToDouble(), 3.14e-10);

    s = L"3.14e-10, ";
    ASSERT_NO_THROW(i = _ParseNumber(json, s.begin(), s.end()));
    ASSERT_EQ(i, 8);
    ASSERT_EQ(json.ToDouble(), 3.14e-10);

    s = L"+3.14e-10 ";
    ASSERT_THROW(i = _ParseNumber(json, s.begin(), s.end()), ParseError);
    try {
        i = _ParseNumber(json, s.begin(), s.end());
    } catch (ParseError &e) {
        ASSERT_EQ(e.GetOffset(), 0);
    }

    s = L"3.14e-10a";
    ASSERT_THROW(i = _ParseNumber(json, s.begin(), s.end()), ParseError);
    try {
        i = _ParseNumber(json, s.begin(), s.end());
    } catch (ParseError &e) {
        ASSERT_EQ(e.GetOffset(), 8);
    }

    s = L"3.14e";
    ASSERT_THROW(i = _ParseNumber(json, s.begin(), s.end()), ParseError);
    try {
        i = _ParseNumber(json, s.begin(), s.end());
    } catch (ParseError &e) {
        ASSERT_EQ(e.GetOffset(), 5);
    }

    s = L"3.14e-";
    ASSERT_THROW(i = _ParseNumber(json, s.begin(), s.end()), ParseError);
    try {
        i = _ParseNumber(json, s.begin(), s.end());
    } catch (ParseError &e) {
        ASSERT_EQ(e.GetOffset(), 6);
    }

    s = L"00";
    ASSERT_THROW(i = _ParseNumber(json, s.begin(), s.end()), ParseError);
    try {
        i = _ParseNumber(json, s.begin(), s.end());
    } catch (ParseError &e) {
        ASSERT_EQ(e.GetOffset(), 1);
    }

    s = L"01";
    ASSERT_THROW(i = _ParseNumber(json, s.begin(), s.end()), ParseError);
    try {
        i = _ParseNumber(json, s.begin(), s.end());
    } catch (ParseError &e) {
        ASSERT_EQ(e.GetOffset(), 1);
    }
}


TEST(JsonTest, JsonParseString)
{
    Json json;
    string_type s;
    s = LR"("what")";
    string_type::difference_type i;
    ASSERT_NO_THROW(i = _ParseString(json, s.begin(), s.end()));
    ASSERT_EQ(json.GetStringRef(), L"what");
    ASSERT_EQ(i, 6);

    s = LR"("what\n")";
    ASSERT_NO_THROW(i = _ParseString(json, s.begin(), s.end()));
    ASSERT_EQ(json.GetStringRef(), L"what\n");
    ASSERT_EQ(i, 8);

    s = LR"("what\n\u000a")";
    ASSERT_NO_THROW(i = _ParseString(json, s.begin(), s.end()));
    ASSERT_EQ(json.GetStringRef(), L"what\n\n");
    ASSERT_EQ(i, 14);
    
    // Test string missing quote ".
    s = LR"("what)";
    ASSERT_THROW(i = _ParseString(json, s.begin(), s.end()), ParseError);
    try {
        i = _ParseString(json, s.begin(), s.end());
    } catch (ParseError &e) {
        ASSERT_EQ(e.GetOffset(), 5);
    }
}


TEST(JsonTest, JsonParseArray)
{
    Json json;
    string_type s;
    string_type::difference_type i;

    s = LR"(["test", "test2"])";
    ASSERT_NO_THROW(i = _ParseArray(json, s.begin(), s.end()));
    ASSERT_EQ(json[0].GetStringRef(), L"test");
    ASSERT_EQ(json[1].GetStringRef(), L"test2");

    s = LR"(["test", 1, null, true, false])";
    ASSERT_NO_THROW(i = _ParseArray(json, s.begin(), s.end()));
    ASSERT_EQ(json[0].GetStringRef(), L"test");
    ASSERT_EQ(json[1].ToDouble(), 1.);
    ASSERT_EQ(json[2].GetType(), JsonValueType::Null);
    ASSERT_EQ(json[3].GetType(), JsonValueType::True);
    ASSERT_EQ(json[4].GetType(), JsonValueType::False);
}


TEST(JsonTest, JsonParseObject)
{
    Json json;
    string_type s;
    string_type::difference_type i;

    s = LR"({"test": "test2"})";
    ASSERT_NO_THROW(i = _ParseObject(json, s.cbegin(), s.cend()));
    ASSERT_EQ(json["test"].GetStringRef(), L"test2");
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
    JsonValue *j = new JsonNull();
    double value;
    JsonValue *ja = new JsonArray{Json(3.0), Json(1.0)};
    Json json = {1., 2., "string3"};
    std::wcout << json << std::endl;
    json= L"test";
    std::wcout << json.GetStringRef() << std::endl;
    std::wcout << json << std::endl;
    std::wcout << json["test"] << std::endl;
    std::wcout << json["test1"] << std::endl;
    std::wcout << json << std::endl;
    json["test1"] = 1000.;
    std::wcout << json["test1"] << std::endl;
    std::wcout << json << std::endl;
    std::wstring json_str = LR"({"1.": 2, "nested": {"key": "value", "ok": true}, "pi": 3.14, "test": 0.3, "test1": 1000})";
    _ParseValue(json, json_str.begin(), json_str.end());
    std::wcout << json;
}