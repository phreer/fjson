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
               string_type::difference_type offset):
            JsonError(message), offset_(offset) {}
    ParseError(std::string &&message, 
               string_type::difference_type offset): 
            JsonError(std::move(message)) {}
    string_type::difference_type GetOffset() const { return offset_; }
private:
    string_type::difference_type offset_;
};


class Json;

class JsonValue
{
public:
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
   JsonNumber(double value): JsonValue(JsonValueType::Number), value_(value) {}
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
    JsonTrue(): JsonValue(JsonValueType::True) {}
    ~JsonTrue() {}
};

class JsonFalse: public JsonValue
{
public:
    JsonFalse(): JsonValue(JsonValueType::False) {}
    ~JsonFalse() {}
};

class JsonString: public JsonValue
{
public:
    JsonString(const string_type &str): JsonValue(JsonValueType::String), value_(str) {}
    JsonString(string_type &&str): JsonValue(JsonValueType::String), value_(str) {}
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
        JsonValue(JsonValueType::Array), values_(init_list) {}
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

    JsonObject(): JsonValue(JsonValueType::Object) {}

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


class JsonInvalidValue: public JsonValue
{
public:
    JsonInvalidValue(): JsonValue(JsonValueType::InvalidValue) {}
    ~JsonInvalidValue() {}
};


class Json
{
public:
    Json()
    {
        json_value_ = std::make_shared<JsonInvalidValue>();
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
    size_t size() const { return json_value_->size(); }
    JsonValueType GetType() const { return json_value_->GetType(); } 
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

    Json& operator[] (const charT *key)
    {
        return json_value_->operator[](string_type(key));
    }
    const Json& operator[] (const charT *key) const
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
    
    static Json parse(const string_type &str);
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


string_type::difference_type _ParseValue(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end) throw (ParseError);


string_type::difference_type _ParseArray(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end) throw (ParseError);


string_type::difference_type _ParseString(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end) throw (ParseError);


string_type::difference_type _ParseObject(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end) throw (ParseError);


string_type::difference_type _ParseArray(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end) throw (ParseError)
{
    enum Status {WAIT_LBRAKET, WAIT_RBRAKET, COMPLETED};
    Status status = WAIT_LBRAKET;
    auto iter = begin;
    size_t count = 0;
    while (iter < end) {
        // Skip whitespace
        if (IsWhitespace(*iter)) {
            ++iter;
            break;
        }

        if (status == WAIT_LBRAKET) {
            switch (*iter) {
            case '[': {
                status = WAIT_RBRAKET;
                break;
            }
            default: {
                goto complete;
            }
            }
        } else if (status == WAIT_RBRAKET) {
            switch (*iter) {
            case ']': {
                status = COMPLETED;
                goto complete;
            }
            case ',': {
                string_type::difference_type i;
                try {
                    i = _ParseValue(json[count], iter+1, end);
                } catch (ParseError &e) {
                    throw ParseError("invalid json string", iter - begin + i + 1);
                }
            }
            }
        } else {
            goto complete;
        }
        ++iter;
    }
complete:
    if (status == COMPLETED) {
        return iter - begin;
    }
}


// If any error occurred when parsing, an ParseError exception will be thrown.
// If the parsing process completes without error, then the position of the 
// final character, i.e., '}', will be returned.
string_type::difference_type _ParseObject(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end) throw (ParseError)
{
    enum Status {WAIT_LBRACE, WAIT_STRING1, WAIT_STRING2, WAIT_COLON, 
                 WAIT_RBRACE_COMMA, COMPLETED};
    Status status = WAIT_LBRACE;
    auto iter = begin;
    Json key, value;
    while (iter < end) {
        if (IsWhitespace(*iter)) {
            goto next_loop;
        }
        if (status == WAIT_LBRACE) {
            if (*iter == '{') {
                status = WAIT_STRING1;
                goto next_loop;
            } else {
                goto complete;
            }
        } else if (status == WAIT_STRING1) {
            // Empty object
            if (*iter == '}') {
                status = COMPLETED;
                goto next_loop;
            }
            string_type::difference_type i;
            try {
                i = _ParseString(key, iter, end);
            } catch (ParseError &e) {
                throw ParseError("invalid json string", iter - begin + e.GetOffset());
            }
            status = WAIT_COLON;
            iter += i;
            continue;
        } else if (status == WAIT_STRING2) {
            string_type::difference_type i;
            try {
                i = _ParseString(key, iter, end);
            } catch (ParseError &e) {
                throw ParseError("invalid json string", iter - begin + e.GetOffset());
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
                    throw ParseError("invalid json string", iter - begin + e.GetOffset());
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
            goto next_loop;
        } else {
            break;
        }
next_loop:
        ++iter;
    }
complete:
    if (status != COMPLETED) {
        throw ParseError("invalid json string", iter - begin);
    }
    return iter - begin;
}



string_type::difference_type _ParseString(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end) throw (ParseError)
{
    enum Status {WAIT_QUOT1, WAIT_QUOT2, BACKSLASH_PENDING, COMPLETED};
    Status status = WAIT_QUOT1;
    auto iter = begin;
    string_type result;
    while (iter < end) {
        if (IsWhitespace(*iter)) {
            goto next_loop;
        }

        if (status == WAIT_QUOT1) {
            switch (*iter) {
            case '\"': {
                status = WAIT_QUOT2;
                goto next_loop;
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
                goto next_loop;
            }
            case '\\': {
                status = BACKSLASH_PENDING;
                goto next_loop;
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
                    throw ParseError("invalid json string.", iter - begin);
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
next_loop:
        ++iter;
    }
complete:
    if (status == COMPLETED) {
        json = Json(result);
        return iter - begin;
    } else {
        throw ParseError("invalid json string", iter - begin);
    }
}


string_type::difference_type _ParseValue(Json &json, 
        typename string_type::const_iterator begin, 
        typename string_type::const_iterator end) throw (ParseError)
{
    auto iter = begin;
    bool completed = false;
    while (iter < end) {
        if (IsWhitespace(*iter)) {
            goto next_loop;
        }
        switch (*iter) {
        case '{': {
            string_type::difference_type i;
            try {
                i = _ParseObject(json, iter, end);
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
                json = Json(false);
                completed = true;
                break;
            } else{
                goto complete;
            }
        }
        if (completed) break;
next_loop:
        ++iter;
    }
complete:
    if (!completed) {
        throw ParseError("invalid json string", iter - begin);
    }
    return iter - begin;
}
};