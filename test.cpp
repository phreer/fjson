#include <iostream>
#include <exception>
#include "fjson.h"

using namespace fjson;
int main()
{
    JsonValue *j = new JsonNull();
    double value;
    try {
        value = j->ToDouble();
    } catch (std::exception &e) {
        std::wcout << e.what() << std::endl;
    }
    JsonValue *ja = new JsonArray{Json(3.0), Json(1.0)};
    for (size_t i = 0; i < ja->size(); ++i)
    {
        (*ja)[i] = 12.;
        std::wcout << (*ja)[i].ToDouble() << std::endl;
    }
    Json json = {1., 2., "string3"};
    std::wcout << json << std::endl;
    json= L"test";
    std::wcout << json.GetStringRef() << std::endl;
    json = {
        {"test", 0.2 + 0.1}, 
        {"pi", 3.14}, 
        {"nested", {
                {"key", "value"}, 
                {"ok", true}
            }
        }, 
        {"1." , 2. }
    };
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