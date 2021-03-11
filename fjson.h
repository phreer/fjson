#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <exception>
#include <initializer_list>
#include <memory>
#include <cassert>

namespace fjson {

enum class ValueType { 
    Number = 0, 
    Null = 1, 
    True = 2, 
    False = 3, 
    String = 4, 
    Array = 5, 
    Object = 6
};

const char *ValueTypeStrs[] = {
    "Number",
    "Null",
    "True",
    "False",
    "String",
    "Array",
    "Object"
};

using string_type = std::string;

inline const char* ValueTypeToStr(ValueType type)
{
    return ValueTypeStrs[static_cast<size_t>(type)];
} 

class JsonError: public std::exception
{
public:
    JsonError() = default;
    JsonError(const std::string &message): message_(message) {}
    JsonError(std::string &&message): message_(message) {}
    virtual const char* what() const noexcept
    {
        return message_.c_str();
    }
private:
    std::string message_;
};

class IncompatibleTypeError: public JsonError
{
public:
    IncompatibleTypeError() = default;
    IncompatibleTypeError(const std::string &message): JsonError(message) {}
    IncompatibleTypeError(std::string &&message): JsonError(message) {}
};

class IndexTypeError: public JsonError
{
public:
    IndexTypeError() = default;
    IndexTypeError(const std::string &message): JsonError(message) {}
    IndexTypeError(std::string &&message): JsonError(message) {}
};

class Json;

class JsonValue
{
public:
    // static JsonValue* parseFile(std::string filename);
    JsonValue(ValueType type): type_(type) {}
    virtual ~JsonValue() {}
    ValueType GetType() const { return type_; }
    bool isNumber() const { return type_ == ValueType::Number; }
    bool isNull() const { return type_ == ValueType::Null; }
    bool isTrue() const { return type_ == ValueType::True; }
    bool isFalse() const { return type_ == ValueType::False; }
    bool isString() const { return type_ == ValueType::String; }
    bool isArray() const { return type_ == ValueType::Array; }
    bool isObject() const { return type_ == ValueType::Object; }
    virtual size_t size() const
    {
        std::string message = "calling size() is incompatible for '";
        message += ValueTypeToStr(GetType()) + "'";
        throw IncompatibleTypeError(std::move(message));
    }
    // Note that functions toXXX() throw 'IncompatibleTypeError' when called by 
    // incompatable type. These functions try to convert 'JsonValue's to 
    // built-in types
    // For number
    virtual double toDouble() const
    {
        throw IncompatibleTypeError();
    }
    virtual bool toBool() const
    {
        throw IncompatibleTypeError();
    }
    // For string
    virtual std::string toString() const
    {
        throw IncompatibleTypeError();
    }
    virtual const std::string& getString() const
    {

    }
    // For object
    virtual Json& operator[] (const std::string &key)
    {
        throw IndexTypeError();
    }
    virtual const Json& operator[] (const std::string &key) const
    {
        throw IndexTypeError();
    }
    // For array
    virtual Json& operator[] (int index)
    {
        throw IndexTypeError();
    }
    virtual const Json& operator[] (int index) const
    {
        throw IndexTypeError();
    }
private:
    ValueType type_;
};

class JsonNull: public JsonValue
{
public:
    JsonNull(): JsonValue(ValueType::Null) {}
    ~JsonNull() {}
};

class JsonNumber: public JsonValue
{
public:
   JsonNumber(double value): JsonValue(ValueType::Number), value_(value) {};
   virtual double toDouble() const override
   {
       return value_;
   }
private:
    double value_;
};

class JsonString: public JsonValue
{
public:
    JsonString(const string_type &str): JsonValue(ValueType::String), value_(str) {}
    JsonString(string_type &&str): JsonValue(ValueType::String), value_(str) {}
    virtual string_type toString() const override
    {
        return value_;
    }
private:
    string_type value_;
};

class JsonArray: public JsonValue
{
public:
    JsonArray(const std::initializer_list<Json> &init_list): 
        JsonValue(ValueType::Array), values_(init_list) {}
    virtual Json& operator[] (int index) override
    {
        assert(index < values_.size() && -index <= static_cast<int>(values_.size()));
        size_t i = 0;
        if (index >= 0) i = static_cast<size_t>(index);
        else i = static_cast<size_t>(static_cast<int>(values_.size()) + index);
        return values_[i];
    }
    virtual size_t size() const override { return values_.size(); }
private:
    std::vector<Json> values_;
};

class JsonObject: public JsonValue
{
public:
    JsonObject();
    Json& operator[] (const string_type &key)
    {
        return kvalues_[key];
    }
    const Json& operator[] (const string_type &key) const
    {
        return kvalues_.at(key);
    }
private:
    std::map<string_type, Json> kvalues_;
};

class Json
{
public:
    Json() = delete;
    Json(double value)
    {
        json_value_ = std::make_shared<JsonNumber>(value);
    }
    Json(const std::initializer_list<Json> &init_list)
    {
        bool is_object = true;
        for (auto iter = init_list.begin(); iter < init_list.end(); ++iter) {
            if (!iter->isArray() || iter->size() != 2 ||
                    !(*iter)[0].isString()) {
                is_object = false;
                break;
            }
        }
        if (is_object) {
            json_value_ = std::make_shared<JsonObject>();
            for (auto iter = init_list.begin(); iter < init_list.end(); ++iter) {
                (*json_value_)[(*iter)[0]] = (*iter)[1];
            }
        } else {
            json_value_ = std::make_shared<JsonArray>(init_list);
        }
    }
    size_t size() const { return json_value_->size(); }
    ValueType GetType() const { return json_value_->GetType(); } 
    double toDouble() const { return json_value_->toDouble(); }
    std::string toString() const { return json_value_->toString(); }
    bool toBool() const { return json_value_->toBool(); }
    Json& operator[](const std::string &key)
    {
        return json_value_->operator[](key);
    }
    const Json& operator[](const std::string &key) const
    {
        return json_value_->operator[](key);
    }
    Json& operator [](int index)
    {
        return json_value_->operator[](index);
    }
    const Json& operator [](int index) const
    {
        return json_value_->operator[](index);
    }
    bool isNumber() const { return json_value_->isNumber(); }
    bool isNull() const { return json_value_->isNull(); }
    bool isTrue() const { return json_value_->isTrue(); }
    bool isFalse() const { return json_value_->isFalse(); }
    bool isString() const { return json_value_->isString(); }
    bool isArray() const { return json_value_->isArray(); }
    bool isObject() const { return json_value_->isObject(); }
private:
    std::shared_ptr<JsonValue> json_value_;
};

};