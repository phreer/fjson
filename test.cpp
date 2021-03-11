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
        std::cout << e.what() << std::endl;
    }
    JsonValue *ja = new JsonArray{Json(3.0), Json(1.0)};
    for (size_t i = 0; i < ja->size(); ++i)
    {
        (*ja)[i] = 12.;
        std::cout << (*ja)[i].ToDouble() << std::endl;
    }
    Json json = {1., 2., "string3"};
    std::cout << json << std::endl;
    json = "test";
    std::cout << json.GetStringRef() << std::endl;
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
    std::cout << json << std::endl;
    std::cout << json["test"] << std::endl;
    std::cout << json["test1"] << std::endl;
}