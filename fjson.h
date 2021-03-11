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
    bool IsNumber() const { return type_ == ValueType::Number; }
    bool IsNull() const { return type_ == ValueType::Null; }
    bool IsTrue() const { return type_ == ValueType::True; }
    bool IsFalse() const { return type_ == ValueType::False; }
    bool IsString() const { return type_ == ValueType::String; }
    bool IsArray() const { return type_ == ValueType::Array; }
    bool IsObject() const { return type_ == ValueType::Object; }
    virtual size_t size() const
    {
        std::string message = "calling size() is incompatible for '";
        message += std::string(ValueTypeToStr(GetType())) + "'";
        throw IncompatibleTypeError(std::move(message));
    }
    // Note that functions toXXX() throw 'IncompatibleTypeError' when called by 
    // incompatable type. These functions try to convert 'JsonValue's to 
    // built-in types
    // For number
    virtual double ToDouble() const
    {
        throw IncompatibleTypeError();
    }
    virtual bool ToBool() const
    {
        throw IncompatibleTypeError();
    }
    // For string
    virtual string_type ToString() const
    {
        throw IncompatibleTypeError();
    }
    virtual string_type& GetStringRef()
    {
        throw IncompatibleTypeError();
    }
    virtual const string_type& GetStringRef() const
    {
        throw IncompatibleTypeError();
    }

    // For object
    virtual Json& operator[] (const string_type &key)
    {
        throw IndexTypeError();
    }
    virtual Json& operator[] (string_type &&key)
    {
        throw IndexTypeError();
    }
    virtual const Json& operator[] (const string_type &key) const
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
   JsonNumber(double value): JsonValue(ValueType::Number), value_(value) {}
   virtual double ToDouble() const override
   {
       return value_;
   }
private:
    double value_;
};

class JsonTrue: public JsonValue
{
public:
    JsonTrue(): JsonValue(ValueType::True) {}
    ~JsonTrue() {}
};

class JsonFalse: public JsonValue
{
public:
    JsonFalse(): JsonValue(ValueType::False) {}
    ~JsonFalse() {}
};

class JsonString: public JsonValue
{
public:
    JsonString(const string_type &str): JsonValue(ValueType::String), value_(str) {}
    JsonString(string_type &&str): JsonValue(ValueType::String), value_(str) {}
    virtual string_type ToString() const override
    {
        return value_;
    }
    virtual const string_type& GetStringRef() const override
    {
        return value_;
    }
    virtual string_type& GetStringRef() override
    {
        return value_;
    }
private:
    string_type value_;
};


class JsonArray: public JsonValue
{
public:
    using container_type = std::vector<Json>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

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
    iterator begin()
    {
        return values_.begin();
    }
    iterator end()
    {
        return values_.end();
    }
    const_iterator cbegin()
    {
        return values_.cbegin();
    }
    const_iterator cend()
    {
        return values_.cend();
    }
private:
    container_type values_;
};

class JsonObject: public JsonValue
{
public:
    using container_type = std::map<string_type, Json>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;

    JsonObject(): JsonValue(ValueType::Object) {}

    virtual Json& operator[] (const string_type &key) override
    {
        return kvalues_[key];
    }
    virtual  Json& operator[] (string_type &&key) override
    {
        return kvalues_[std::move(key)];
    }
    virtual const Json& operator[] (const string_type &key) const override
    {
        return kvalues_.at(key);
    }
    iterator begin()
    {
        return kvalues_.begin();
    }
    iterator end()
    {
        return kvalues_.end();
    }
    const_iterator cbegin() const
    {
        return kvalues_.cbegin();
    }
    const_iterator cend() const
    {
        return kvalues_.cend();
    }

private:
    container_type kvalues_;
};


class Json
{
public:
    Json() {}
    Json(double value)
    {
        json_value_ = std::make_shared<JsonNumber>(value);
    }
    Json(const string_type &str)
    {
        json_value_ = std::make_shared<JsonString>(str);
    }
    Json(string_type &&str)
    {
        json_value_ = std::make_shared<JsonString>(std::move(str));
    }
    Json(const char *src): Json(string_type(src)) {}
    Json(const std::initializer_list<Json> &init_list)
    {
        bool is_object = true;
        for (auto iter = init_list.begin(); iter < init_list.end(); ++iter) {
            if (!iter->IsArray() || iter->size() != 2 ||
                    !(*iter)[0].IsString()) {
                is_object = false;
                break;
            }
        }
        if (is_object) {
            json_value_ = std::make_shared<JsonObject>();
            for (auto iter = init_list.begin(); iter < init_list.end(); ++iter) {
                (*this)[(*iter)[0]] = (*iter)[1];
            }
        } else {
            json_value_ = std::make_shared<JsonArray>(init_list);
        }
    }
    size_t size() const { return json_value_->size(); }
    ValueType GetType() const { return json_value_->GetType(); } 
    double ToDouble() const { return json_value_->ToDouble(); }
    bool ToBool() const { return json_value_->ToBool(); }

    // For string
    string_type ToString() const { return json_value_->ToString(); }
    string_type& GetStringRef() { return json_value_->GetStringRef(); }
    const string_type& GetStringRef() const { return json_value_->GetStringRef(); }
    Json& operator[] (const string_type &key)
    {
        return json_value_->operator[](key);
    }
    Json& operator[] (string_type &&key)
    {
        return json_value_->operator[](std::move(key));
    }
    const Json& operator[] (const string_type &key) const
    {
        return json_value_->operator[](key);
    }

    Json& operator[] (const char *key)
    {
        return json_value_->operator[](string_type(key));
    }
    const Json& operator[] (const char *key) const
    {
        return json_value_->operator[](string_type(key));
    }
    Json& operator[] (const Json &key)
    {
        return json_value_->operator[](key.GetStringRef());
    }
    Json& operator[] (Json &&key)
    {
        return json_value_->operator[](std::move(key.GetStringRef()));
    }
    const Json& operator[] (const Json &key) const
    {
        return json_value_->operator[](key.GetStringRef());
    }

    Json& operator[] (int index)
    {
        return json_value_->operator[](index);
    }
    const Json& operator[] (int index) const
    {
        return json_value_->operator[](index);
    }
    bool IsNumber() const { return json_value_->IsNumber(); }
    bool IsNull() const { return json_value_->IsNull(); }
    bool IsTrue() const { return json_value_->IsTrue(); }
    bool IsFalse() const { return json_value_->IsFalse(); }
    bool IsString() const { return json_value_->IsString(); }
    bool IsArray() const { return json_value_->IsArray(); }
    bool IsObject() const { return json_value_->IsObject(); }
    
    friend std::ostream& operator<< (std::ostream &o, const Json &json);
    
    // TODO: add iterator to iterate the elements
    class object_iterator
    {
    public:
        using self_type = object_iterator;
    private:

    };

    class const_object_iterator
    {

    };
    
    class array_iterator
    {

    };

    class const_array_iterator
    {
        
    };
    
private:
    std::shared_ptr<JsonValue> json_value_;
};


std::ostream & operator<< (std::ostream &o, const Json &json)
{
    switch(json.GetType()) {
    case ValueType::Null: 
        o << "Null";
        break;
    case ValueType::True:
        o << "true";
        break;
    case ValueType::False:
        o << "false";
        break;
    case ValueType::Number:
        o << json.ToDouble();
        break;
    case ValueType::String:
        o << "\"" << json.GetStringRef() << "\"";
        break;
    case ValueType::Array: {
        o << "[";
        auto array_json = std::dynamic_pointer_cast<JsonArray>(json.json_value_);
        for (auto iter = array_json->cbegin(), end = array_json->cend(); 
                iter != end; ++iter) {
            if (iter != array_json->cbegin()) o << ", ";
            o << *iter;
        }
        o << "]";
        break;
    }
    case ValueType::Object: {
        o << "{";
        auto object_json = std::dynamic_pointer_cast<JsonObject>(json.json_value_);
        for (auto iter = object_json->cbegin(), end = object_json->cend(); 
                iter != end; ++iter) {
            if (iter != object_json->cbegin()) o << ", ";
            o << "\"" << iter->first << "\": " << iter->second;
        }
        o << "}";
        break;
    }
    }
    return o;
}
};