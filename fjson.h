// Author: Phree Liu

#ifndef __FJSON_H__
#define __FJSON_H__

#include <iostream>
#include <codecvt>
#include <locale>
#include <vector>
#include <string>
#include <map>
#include <exception>
#include <initializer_list>
#include <memory>
#include <cassert>

namespace fjson {

enum class JsonValueType { 
    Number = 0, 
    Null = 1, 
    True = 2, 
    False = 3, 
    String = 4, 
    Array = 5, 
    Object = 6, 
    InvalidValue = 7, 
};

const char *ValueTypeStrs[] = {
    "Number",
    "Null",
    "True",
    "False",
    "String",
    "Array",
    "Object", 
    "InvalidValue"
};

using string_type = std::wstring;
using charT = string_type::value_type;

inline const char* ValueTypeToStr(JsonValueType type)
{
    return ValueTypeStrs[static_cast<size_t>(type)];
} 

class JsonError: public std::exception
{
public:
    JsonError() = default;
    JsonError(const std::string &message): message_(message) {}
    JsonError(std::string &&message): message_(std::move(message)) {}
    virtual const char* what() const noexcept
    {
        return message_.c_str();
    }
protected:
    std::string GetMessage() const { return message_; }
private:
    std::string message_;
};

class IncompatibleTypeError: public JsonError
{
public:
    IncompatibleTypeError() = default;
    IncompatibleTypeError(const std::string &message): JsonError(message) {}
    IncompatibleTypeError(std::string &&message): JsonError(std::move(message)) {}
};

class IndexTypeError: public JsonError
{
public:
    IndexTypeError() = default;
    IndexTypeError(const std::string &message): JsonError(message) {}
    IndexTypeError(std::string &&message): JsonError(std::move(message)) {}
};

class ParseError: public JsonError
{
public:
    ParseError() = default;
    ParseError(const std::string &message, 
               string_type::difference_type offset, 
               const string_type &processed_string):
            JsonError(message), 
            offset_(offset), 
            processed_string_(processed_string)
    {}
    ParseError(std::string &&message, 
               string_type::difference_type offset, 
               string_type &&processed_string):
            JsonError(std::move(message)), 
            offset_(offset), 
            processed_string_(std::move(processed_string))
    {}
    string_type::difference_type GetOffset() const { return offset_; }
    const string_type& GetProcessedString() const { return processed_string_; }
private:
    string_type::difference_type offset_;
    string_type processed_string_;
};


class Json;

class JsonValue
{
public:
    using array_container_type = std::vector<Json>;
    using object_container_type = std::map<string_type, Json>;
    using size_type = array_container_type::size_type;

    // static JsonValue* parseFile(std::string filename);
    JsonValue(JsonValueType type): type_(type) {}

    virtual ~JsonValue() {}
    JsonValueType GetType() const { return type_; }
    bool IsNumber() const { return type_ == JsonValueType::Number; }
    bool IsNull() const { return type_ == JsonValueType::Null; }
    bool IsTrue() const { return type_ == JsonValueType::True; }
    bool IsFalse() const { return type_ == JsonValueType::False; }
    bool IsString() const { return type_ == JsonValueType::String; }
    bool IsArray() const { return type_ == JsonValueType::Array; }
    bool IsObject() const { return type_ == JsonValueType::Object; }
    bool IsValid() const { return type_ != JsonValueType::InvalidValue; }
    
private:
    JsonValueType type_;
};

class JsonNull: public JsonValue
{
public:
    JsonNull(): JsonValue(JsonValueType::Null) {}
    ~JsonNull() {}
};

class JsonNumber: public JsonValue
{
public:
    JsonNumber(): JsonValue(JsonValueType::Number), value_(0.) {}
    JsonNumber(double value): JsonValue(JsonValueType::Number), value_(value) {}
    double ToDouble() const
    {
        return value_;
    }
private:
    double value_;
};

class JsonTrue: public JsonValue
{
public:
    JsonTrue(): JsonValue(JsonValueType::True) {}
    ~JsonTrue() {}
    operator bool() const { return true; }
};

class JsonFalse: public JsonValue
{
public:
    JsonFalse(): JsonValue(JsonValueType::False) {}
    ~JsonFalse() {}
    operator bool() const { return false; }
};

class JsonString: public JsonValue, public string_type
{
public:
    JsonString(const string_type &str):
            JsonValue(JsonValueType::String), string_type(str) {}
    JsonString(string_type &&str): 
            JsonValue(JsonValueType::String), string_type(std::move(str)) {}
    string_type ToString() const
    {
        return string_type(this->c_str());
    }
    const string_type& GetStringRef() const
    {
        return *this;
    }
    string_type& GetStringRef()
    {
        return *this;
    }
};


class JsonArray: public JsonValue, public std::vector<Json>
{
public:
    using container_type = std::vector<Json>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;
    using size_type = container_type::size_type;
    using difference_type = container_type::difference_type;

    JsonArray(): JsonValue(JsonValueType::Array) {}
    JsonArray(const std::initializer_list<Json> &init_list): 
        JsonValue(JsonValueType::Array), std::vector<Json>(init_list) {}
};


class JsonObject: public JsonValue, public std::map<string_type, Json>
{
public:
    using container_type = std::map<string_type, Json>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;
    using size_type = container_type::size_type;
    using difference_type = container_type::difference_type;

    JsonObject(): JsonValue(JsonValueType::Object) {}
};


class JsonInvalidValue: public JsonValue
{
public:
    JsonInvalidValue(): JsonValue(JsonValueType::InvalidValue) {}
    ~JsonInvalidValue() {}
};


class Json
{
public:
    using object_container_type = JsonObject::container_type;
    using array_container_type = JsonArray::container_type;
    using size_type = array_container_type::size_type;
    Json()
    {
        json_value_ = std::make_shared<JsonInvalidValue>();
    }
    Json(JsonValueType type)
    {
        switch(type) {
        case JsonValueType::Number: {
            json_value_ = std::make_shared<JsonNumber>();
            break;
        }
        case JsonValueType::False: {
            json_value_ = std::make_shared<JsonFalse>();
            break;
        }
        case JsonValueType::True: {
            json_value_ = std::make_shared<JsonTrue>();
            break;
        }
        case JsonValueType::Null: {
            json_value_ = std::make_shared<JsonNull>();
            break;
        }
        case JsonValueType::Array: {
            json_value_ = std::make_shared<JsonArray>();
            break;
        }
        case JsonValueType::Object: {
            json_value_ = std::make_shared<JsonObject>();
            break;
        }
        default:
            json_value_ = std::make_shared<JsonInvalidValue>();
        }
    }
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
    Json(const char *src)
    {
        std::wstring_convert<std::codecvt_utf8_utf16<charT> > converter;
        string_type ws = converter.from_bytes(src);
        json_value_ = std::make_shared<JsonString>(ws);
    }
    Json(const string_type::value_type *src): Json(string_type(src)) {}
    Json(bool value)
    {
        if (value) json_value_ = std::make_shared<JsonTrue>();
        else json_value_ = std::make_shared<JsonFalse>();
    }
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
    size_type size() const
    {
        if (this->IsArray())
        {
            auto p = std::static_pointer_cast<JsonArray>(json_value_);
            return p->size();
        } else if (this->IsObject())
        {
            auto p = std::static_pointer_cast<JsonObject>(json_value_);
            return p->size();
        }
        std::string message = std::string("calling size() on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IncompatibleTypeError(std::move(message));
    }
    JsonValueType GetType() const { return json_value_->GetType(); } 
    double ToDouble() const
    {
        if (this->IsNumber()) {
            auto p = std::static_pointer_cast<JsonNumber>(json_value_);
            return p->ToDouble();
        }
        std::string message = std::string("calling ToDouble() on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IncompatibleTypeError(std::move(message));
    }
    bool ToBool() const {
        if (this->IsTrue()) return true;
        if (this->IsFalse()) return false;
        std::string message = std::string("calling ToBool() on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IncompatibleTypeError(std::move(message));
    }
    void resize(size_type n)
    {
        if (this->IsArray()) {
            auto p = std::static_pointer_cast<JsonArray>(json_value_);
            return p->resize(n);
        } else if (this->IsString()) {
            auto p = std::static_pointer_cast<JsonString>(json_value_);
            return p->resize(n);
        }
        std::string message = std::string("calling resize() on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IncompatibleTypeError(std::move(message));
    }
    // For string
    string_type ToString() const
    {
        if (this->IsString()) {
            auto p = std::static_pointer_cast<JsonString>(json_value_);
            return p->ToString();
        }
        std::string message = std::string("calling ToString() on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IncompatibleTypeError(std::move(message));
    }

    string_type& GetStringRef()
    {
        if (this->IsString()) {
            auto p = std::static_pointer_cast<JsonString>(json_value_);
            return p->GetStringRef();
        }
        std::string message = std::string("calling GetStringRef() on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IncompatibleTypeError(std::move(message));
    }

    const string_type& GetStringRef() const
    {
        if (this->IsString()) {
            auto p = std::static_pointer_cast<JsonString>(json_value_);
            return p->GetStringRef();
        }
        std::string message = std::string("calling GetStringRef() on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IncompatibleTypeError(std::move(message));
    }

    Json& operator[] (const string_type &key)
    {
        if (this->IsObject()) {
            auto p = std::static_pointer_cast<JsonObject>(json_value_);
            return (*p)[key];
        }
        std::string message = std::string("indexing with string on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IndexTypeError(std::move(message));
    }

    Json& operator[] (string_type &&key)
    {
        if (this->IsObject()) {
            auto p = std::static_pointer_cast<JsonObject>(json_value_);
            return (*p)[std::move(key)];
        }
        std::string message = std::string("indexing with string on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IndexTypeError(std::move(message));
    }


    const Json& operator[] (const string_type &key) const
    {
        if (this->IsObject()) {
            auto p = std::static_pointer_cast<JsonObject>(json_value_);
            return (*p)[key];
        }
        std::string message = std::string("indexing with string on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IndexTypeError(std::move(message));
    }


    Json& operator[] (const Json &key)
    {
        if (!key.IsString())
        {
            throw IndexTypeError("indexing with unsupported type");
        }
        if (this->IsObject()) {
            auto p = std::static_pointer_cast<JsonObject>(json_value_);
            return (*p)[key.GetStringRef()];
        }
        std::string message = std::string("indexing with string on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IndexTypeError(std::move(message));
    }
    Json& operator[] (Json &&key)
    {
        if (!key.IsString())
        {
            throw IndexTypeError("indexing with unsupported type");
        }
        if (this->IsObject()) {
            auto p = std::static_pointer_cast<JsonObject>(json_value_);
            return (*p)[std::move(key.GetStringRef())];
        }
        std::string message = std::string("indexing with string on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IndexTypeError(std::move(message));
    }
    const Json& operator[] (const Json &key) const
    {
        if (!key.IsString())
        {
            throw IndexTypeError("indexing with unsupported type");
        }
        if (this->IsObject()) {
            auto p = std::static_pointer_cast<JsonObject>(json_value_);
            return (*p)[key.GetStringRef()];
        }
        std::string message = std::string("indexing with string on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IndexTypeError(std::move(message));
    }

    Json& operator[] (int index)
    {
        if (this->IsArray()) {
            auto p = std::static_pointer_cast<JsonArray>(json_value_);
            return (*p)[index];
        }
        std::string message = std::string("indexing with integer on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IndexTypeError(std::move(message));
    }
    const Json& operator[] (int index) const
    {
        if (this->IsArray()) {
            auto p = std::static_pointer_cast<JsonArray>(json_value_);
            return (*p)[index];
        }
        std::string message = std::string("indexing with integer on ") + 
                ValueTypeToStr(GetType()) + " is invalid";
        throw IndexTypeError(std::move(message));
    }
    
    bool IsNumber() const { return json_value_->IsNumber(); }
    bool IsNull() const { return json_value_->IsNull(); }
    bool IsTrue() const { return json_value_->IsTrue(); }
    bool IsFalse() const { return json_value_->IsFalse(); }
    bool IsString() const { return json_value_->IsString(); }
    bool IsArray() const { return json_value_->IsArray(); }
    bool IsObject() const { return json_value_->IsObject(); }
    bool IsValid() const { return json_value_->IsValid(); }
    
    friend std::wostream& operator<< (std::wostream &o, const Json &json);
    
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
    
    static Json Parse(const string_type &str);
private:
    std::shared_ptr<JsonValue> json_value_;
};


std::wostream & operator<< (std::wostream &o, const Json &json)
{
    switch(json.GetType()) {
    case JsonValueType::Null: 
        o << "Null";
        break;
    case JsonValueType::True:
        o << "true";
        break;
    case JsonValueType::False:
        o << "false";
        break;
    case JsonValueType::Number:
        o << json.ToDouble();
        break;
    case JsonValueType::String:
        o << "\"" << json.GetStringRef() << "\"";
        break;
    case JsonValueType::Array: {
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
    case JsonValueType::Object: {
        o << "{";
        auto object_json = std::dynamic_pointer_cast<JsonObject>(json.json_value_);
        size_t count = 0;
        for (auto iter = object_json->cbegin(), end = object_json->cend(); 
                iter != end; ++iter) {
            if (!iter->second.IsValid()) break;
            if (count) o << ", ";
            o << "\"" << iter->first << "\": " << iter->second;
            ++count;
        }
        o << "}";
        break;
    }
    default:
        break;
    }
    return o;
}


bool IsWhitespace(string_type::value_type c)
{
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}


bool IsControlChar(string_type::value_type c)
{
    return (c < 0x1f) || (c == 0x7f) || ((0x80 < c) && c < 0x9f);
}


constexpr bool IsDigit(charT c) {
    return '0' <= c && c <= '9';
}


string_type::difference_type _ParseValue(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end);


string_type::difference_type _ParseArray(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end);


string_type::difference_type _ParseString(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end);


string_type::difference_type _ParseObject(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end);


string_type::difference_type _ParseArray(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end)
{
    enum Status {WAIT_LBRACKET, WAIT_RBRACKET, COMPLETED};
    Status status = WAIT_LBRACKET;
    auto iter = begin;
    while (iter < end) {
        // Skip whitespace
        if (IsWhitespace(*iter)) {
            ++iter;
            break;
        }

        if (status == WAIT_LBRACKET) {
            switch (*iter) {
            case '[': {
                string_type::difference_type i;
                status = WAIT_RBRACKET;
                json = Json(JsonValueType::Array);
                json.resize(1);
                try {
                    i = _ParseValue(json[0], iter+1, end);
                } catch (ParseError &e) {
                    throw ParseError("invalid json array", 
                                     iter - begin + i, 
                                     string_type(iter+i, end));
                }
                iter += i + 1;
                continue;
            }
            default: {
                goto complete;
            }
            }
        } else if (status == WAIT_RBRACKET) {
            switch (*iter) {
            case ']': {
                status = COMPLETED;
                goto complete;
            }
            case ',': {
                string_type::difference_type i;
                json.resize(json.size() + 1);
                try {
                    i = _ParseValue(json[json.size() - 1], iter+1, end);
                } catch (ParseError &e) {
                    throw ParseError("invalid json array", 
                                     iter - begin + i + 1,
                                     string_type(iter, end));
                }
                iter += i + 1;
                continue;
            }
            }
        } else {
            goto complete;
        }
next_iter:
        ++iter;
    }
complete:
    if (status == COMPLETED) {
        return iter - begin;
    } else {
        throw ParseError("invalid json array", 
                         iter - begin, 
                         string_type(begin, end));
    }
}


// If any error occurred when parsing, an ParseError exception will be thrown.
// If the parsing process completes without error, then the position of the 
// final character, i.e., '}', will be returned.
string_type::difference_type _ParseObject(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end)
{
    enum Status {WAIT_LBRACE, WAIT_STRING1, WAIT_STRING2, WAIT_COLON, 
                 WAIT_RBRACE_COMMA, COMPLETED};
    Status status = WAIT_LBRACE;
    auto iter = begin;
    Json key, value;
    json = Json(JsonValueType::Object);
    while (iter < end) {
        if (IsWhitespace(*iter)) {
            goto next_iter;
        }
        if (status == WAIT_LBRACE) {
            if (*iter == '{') {
                status = WAIT_STRING1;
                goto next_iter;
            } else {
                goto complete;
            }
        } else if (status == WAIT_STRING1) {
            // Empty object
            if (*iter == '}') {
                status = COMPLETED;
                goto next_iter;
            }
            string_type::difference_type i;
            try {
                i = _ParseString(key, iter, end);
            } catch (ParseError &e) {
                throw ParseError("invalid json object", 
                                 iter - begin + e.GetOffset(),
                                 string_type(begin, end));
            }
            status = WAIT_COLON;
            iter += i;
            continue;
        } else if (status == WAIT_STRING2) {
            string_type::difference_type i;
            try {
                i = _ParseString(key, iter, end);
            } catch (ParseError &e) {
                throw ParseError("invalid json object", 
                                 iter - begin + e.GetOffset(),
                                 string_type(begin, end));
            }
            status = WAIT_COLON;
            iter += i;
            continue;
        } else if (status == WAIT_COLON) {
            if (*iter == ':') {
                string_type::difference_type i;
                ++iter;
                try {
                    i = _ParseValue(value, iter, end);
                } catch (ParseError &e) {
                    throw ParseError("invalid json object", 
                                     iter - begin + e.GetOffset(), 
                                     string_type(begin, end));
                }
                status = WAIT_RBRACE_COMMA;
                iter += i;
                continue;
            }
            goto complete;
        } else if (status == WAIT_RBRACE_COMMA) {
            if (*iter == ',') {
                status == WAIT_STRING2;
            } else if (*iter == '}') {
                status = COMPLETED;
            }
            goto next_iter;
        } else {
            break;
        }
next_iter:
        ++iter;
    }
complete:
    if (status != COMPLETED) {
        throw ParseError("invalid json object", 
                         iter - begin, 
                         string_type(begin, end));
    }
    return iter - begin;
}



string_type::difference_type _ParseString(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end)
{
    enum Status {WAIT_QUOT1, WAIT_QUOT2, BACKSLASH_PENDING, COMPLETED};
    Status status = WAIT_QUOT1;
    auto iter = begin;
    string_type result;
    while (iter < end) {
        if (status == WAIT_QUOT1) {
            if (IsWhitespace(*iter)) {
                goto next_iter;
            }
            switch (*iter) {
            case '\"': {
                status = WAIT_QUOT2;
                goto next_iter;
            }
            default:
                goto complete;
            }
        } else if (status == WAIT_QUOT2) {
            if (IsControlChar(*iter)) {
                goto complete;
            }
            switch (*iter) {
            case '\"': {
                status = COMPLETED;
                goto next_iter;
            }
            case '\\': {
                status = BACKSLASH_PENDING;
                goto next_iter;
            }
            default:
                result.push_back(*iter);
            }
        } else if (status == BACKSLASH_PENDING) {
            switch (*iter) {
            case '\\':
            case '/':
            case '\"': {
                result.push_back(*iter);
                break;
            }
            case 'b': {
                result.push_back(L'\b');
                break;
            }
            case 'f': {
                result.push_back(L'\f');
                break;
            }
            case 'n': {
                result.push_back(L'\n');
                break;
            }
            case 'r': {
                result.push_back(L'\r');
                break;
            }
            case 't': {
                result.push_back(L'\t');
                break;
            }
            case 'u': {
                using charT = string_type::value_type;
                charT c;
                try {
                    c = static_cast<charT>(
                            stoi(string_type(iter+1, iter+5), nullptr, 16));
                } catch (std::invalid_argument &e) {
                    throw ParseError("invalid json string.", 
                                     iter - begin, 
                                     string_type(begin, end));
                } 
                iter += 4;
                result.push_back(c);
                break;
            }
            default: {
                goto complete;
            }
            }
            status = WAIT_QUOT2;
        } else if (status == COMPLETED) {
            break;
        }
next_iter:
        ++iter;
    }
complete:
    if (status == COMPLETED) {
        json = Json(std::move(result));
        return iter - begin;
    } else {
        throw ParseError("invalid json string", 
                         iter - begin, 
                         string_type(begin, end));
    }
}


string_type::difference_type _ParseNumber(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end)
{
    enum Status {START, WAIT_DIGIT1, WAIT_DIGIT2, FRACTION, WAIT_FRACTION_DIGIT, 
            WAIT_FRACTION_DIGIT_END, WAIT_E_DIGIT, WAIT_E_SIGN, WAIT_E_DIGIT_END, 
            COMPLETED, BAD};
    Status status = START;
    auto iter = begin;
    double result;
    string_type s;
    while (iter < end) {
        if (status == START) {
            if (IsWhitespace(*iter)) {
                goto next_iter;
            } else if (*iter == '-') {
                status = WAIT_DIGIT1;
                goto add_char;
            } else if (IsDigit(*iter)) {
                status = WAIT_DIGIT1;
                continue;
            } else {
                status = BAD;
                goto complete;
            }
        } else if (status == WAIT_DIGIT1) {
            if (*iter == '0') {
                status = FRACTION;
                goto add_char;
            } else if (IsDigit(*iter)) {
                status = WAIT_DIGIT2;
                goto add_char;
            } else {
                status = BAD;
                goto complete;
            }
        } else if (status == WAIT_DIGIT2) {
            if (IsDigit(*iter)) {
                goto add_char;
            } else if (*iter == '.') {
                status = WAIT_FRACTION_DIGIT;
                goto add_char;
            } else if (*iter == 'e' || *iter == 'E') {
                status = WAIT_E_SIGN;
                goto add_char;
            } else if (IsWhitespace(*iter)) {
                status = COMPLETED;
                goto next_iter;
            } else {
                goto complete;
            }
        } else if (status == FRACTION) {
            if (*iter == '.') {
                status = WAIT_FRACTION_DIGIT;
                goto next_iter;
            } else if (*iter == 'e' || *iter == 'E') {
                status = WAIT_E_SIGN;
                goto next_iter;
            } if (IsWhitespace(*iter)) {
                status = COMPLETED;
                goto next_iter;
            } else {
                status = BAD;
                goto complete;
            }
        } else if (status == WAIT_FRACTION_DIGIT) {
            if (IsDigit(*iter)) {
                status = WAIT_FRACTION_DIGIT_END;
                goto add_char;
            } else {
                status = BAD;
                goto complete;
            } 
        } else if (status == WAIT_FRACTION_DIGIT_END) {
            if (IsDigit(*iter)) {
                goto add_char;
            } else if (*iter == 'e' || *iter == 'E') {
                status = WAIT_E_SIGN;
                goto add_char;
            } else if (IsWhitespace(*iter) || *iter == ',') {
                status = COMPLETED;
                goto complete;
            } else {
                status = BAD;
                goto complete;
            }
        } else if (status == WAIT_E_SIGN) {
            if (IsDigit(*iter)) {
                status = WAIT_E_DIGIT;
                continue;
            } else if (*iter == '-' || *iter == '+') {
                status = WAIT_E_DIGIT;
                goto add_char;
            } else {
                status = BAD;
                goto complete;
            }
        } else if (status == WAIT_E_DIGIT) {
            if (IsDigit(*iter)) {
                status = WAIT_E_DIGIT_END;
                goto add_char;
            } else {
                status = BAD;
                goto complete;
            }
        } else if (status == WAIT_E_DIGIT_END) {
            if (IsDigit(*iter)) {
                goto add_char;
            } else if (IsWhitespace(*iter) || *iter == ',') {
                status = COMPLETED;
                goto complete;
            } else {
                status = BAD;
                goto complete;
            }
        } else {
            break;
        }
add_char:
        s.push_back(*iter);
next_iter:
        iter++;
    }
complete:
    switch (status)
    {
    case COMPLETED:
    case FRACTION:
    case WAIT_DIGIT2: 
    case WAIT_FRACTION_DIGIT_END:
    case WAIT_E_DIGIT_END:
        json = std::stod(s); 
        return iter - begin;
    default:
        throw ParseError("invalid json number", 
                         iter - begin, 
                         string_type(begin, end));
    }
}


string_type::difference_type _ParseValue(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end)
{
    auto iter = begin;
    bool completed = false;
    while (iter < end) {
        if (IsWhitespace(*iter)) {
            goto next_iter;
        }
        switch (*iter) {
        case '{': {
            string_type::difference_type i;
            try {
                i = _ParseObject(json, iter, end);
                completed = true;
            } catch (ParseError &e) {
                i = e.GetOffset();
            }
            iter += i;
            goto complete;
        }
        case '[': {
            string_type::difference_type i;
            try {
                i = _ParseArray(json, iter, end);
                completed = true;
            } catch (ParseError &e) {
                i = e.GetOffset();
            }
            iter += i;
            goto complete;
        }
        case '\"': {
            string_type::difference_type i;
            try {
                i = _ParseString(json, iter, end);
                completed = true;
            } catch (ParseError &e) {
                i = e.GetOffset();
            }
            iter += i;
            goto complete;
        }
        case 't':
            if (string_type(iter+1, iter+4) == L"rue") {
                json = Json(true);
                completed = true;
                break;
            } else {
                goto complete;
            }
        case 'f':
            if (string_type(iter+1, iter+5) == L"alse") {
                json = Json(false);
                completed = true;
                break;
            } else {
                goto complete;
            }
        case 'n':
            if (string_type(iter+1, iter+4) == L"ull") {
                json = Json(JsonValueType::Null);
                completed = true;
                break;
            } else{
                goto complete;
            }
        case '1': case '2': case '3': case '4': case '5': case '6': case '7':
        case '8': case '9': case '0': case '-': {
            string_type::difference_type i;
            try {
                i = _ParseNumber(json, iter, end);
                completed = true;
            } catch (ParseError &e) {
                i = e.GetOffset();
            }
            iter += i;
            goto complete;
        }
        }
        if (completed) break;
next_iter:
        ++iter;
    }
complete:
    if (!completed) {
        throw ParseError("invalid json value", 
                         iter - begin, 
                         string_type(begin, end));
    }
    return iter - begin;
}
};

#endif // __FJSON_H__