#include <iostream>
#include <exception>
#include "fjson.h"

using namespace fjson;
int main()
{
    JsonValue *j = new JsonNull();
    double value;
    try {
        value = j->toDouble();
    } catch (std::exception &e) {
        std::cout << e.what() << std::endl;
    }
    JsonValue *ja = new JsonArray{Json(3.0), Json(1.0)};
    for (size_t i = 0; i < ja->size(); ++i)
    {
        (*ja)[i] = 12.;
        std::cout << (*ja)[i].toDouble() << std::endl;
    }

}