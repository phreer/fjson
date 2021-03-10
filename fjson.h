#include <iostream>
#include <vector>
#include <string>
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

inline const char* ValueTypeToStr(ValueType type)
{
    return ValueTypeStrs[static_cast<size_t>(type)];
} 
class ConvertError: public std::exception
{
public:
    ConvertError(ValueType src_type, ValueType dst_type):
        src_type_(src_type), dst_type_(dst_type)
    {
        message_ = std::string("conversion from '") + ValueTypeToStr(src_type)
            + "' to '" + ValueTypeToStr(dst_type) + "' is invalid";
    }
    virtual const char* what() const noexcept
    {
        return message_.c_str();
    }
private:
    ValueType src_type_, dst_type_;
    std::string message_;
};

class IndexError: public std::exception
{
public:
    IndexError(ValueType type): type_(type)
    {
        message_ = std::string("indexing '") + ValueTypeToStr(type) 
            + " ' is invalid";
    }
    virtual const char* what() const noexcept override {
        return message_.c_str();
    }
private:
    ValueType type_;
    std::string message_;
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
        throw "size() not implemented";
    }
    // Note that functions toXXX() throw 'ConvertError' when called by 
    // incompatable type. These functions try to convert 'JsonValue's to 
    // built-in types
    // For number
    virtual double toDouble() const
    {
        throw ConvertError(GetType(), ValueType::Number);
    }
    virtual bool toBool() const
    {
        throw ConvertError(GetType(), ValueType::True);
    }
    // For string
    virtual std::string toString() const
    {
        throw ConvertError(GetType(), ValueType::String);
    }
    // For object
    virtual Json& operator [] (const std::string &key)
    {
        throw IndexError(GetType());
    }
    virtual const Json& operator [] (const std::string &key) const
    {
        throw IndexError(GetType());
    }
    // For array
    virtual Json& operator [] (int index)
    {
        throw IndexError(GetType());
    }
    virtual const Json& operator [] (int index) const
    {
        throw IndexError(GetType());
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

class JsonArray: public JsonValue
{
public:
    JsonArray(const std::initializer_list<Json> &init_list): 
        JsonValue(ValueType::Array), values_(init_list) {}
    virtual Json& operator[] (int index) override
    {
        std::cout << index << std::endl;
        assert(index < values_.size() && -index <= values_.size());
        return values_[index];
    }
    virtual size_t size() const override { return values_.size(); }
private:
    std::vector<Json> values_;
};

class Json
{
public:
    Json() = delete;
    Json(double value)
    {
        json_value_ = std::make_shared<JsonNumber>(value);
    }
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
private:
    std::shared_ptr<JsonValue> json_value_;
};

};