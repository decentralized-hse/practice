#include "json_converter.h"
#include "ron64.h"

#include <sstream>
#include <stdexcept>
#include <cctype>

////////////////////////////////////////////////////////////////////////////////

namespace NBason {

////////////////////////////////////////////////////////////////////////////////

namespace {

class TJsonParser {
public:
    TJsonParser(const std::string& json)
        : Json_(json)
        , Pos_(0)
    { }

    TBasonRecord Parse()
    {
        SkipWhitespace();
        return ParseValue("");
    }

private:
    const std::string& Json_;
    size_t Pos_;

    void SkipWhitespace()
    {
        while (Pos_ < Json_.size() && std::isspace(Json_[Pos_])) {
            ++Pos_;
        }
    }

    char Peek()
    {
        SkipWhitespace();
        if (Pos_ >= Json_.size()) {
            throw std::runtime_error("Unexpected end of JSON");
        }
        return Json_[Pos_];
    }

    char Next()
    {
        SkipWhitespace();
        if (Pos_ >= Json_.size()) {
            throw std::runtime_error("Unexpected end of JSON");
        }
        return Json_[Pos_++];
    }

    void Expect(char ch)
    {
        if (Next() != ch) {
            throw std::runtime_error(std::string("Expected '") + ch + "'");
        }
    }

    std::string ParseString()
    {
        Expect('"');
        std::string result;
        
        while (Pos_ < Json_.size() && Json_[Pos_] != '"') {
            if (Json_[Pos_] == '\\') {
                ++Pos_;
                if (Pos_ >= Json_.size()) {
                    throw std::runtime_error("Unterminated string escape");
                }
                char escaped = Json_[Pos_++];
                switch (escaped) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'b': result += '\b'; break;
                    case 'f': result += '\f'; break;
                    case 'n': result += '\n'; break;
                    case 'r': result += '\r'; break;
                    case 't': result += '\t'; break;
                    case 'u':
                        // Unicode escape - simplified handling
                        throw std::runtime_error("Unicode escapes not fully supported");
                    default:
                        throw std::runtime_error("Invalid escape sequence");
                }
            } else {
                result += Json_[Pos_++];
            }
        }
        
        Expect('"');
        return result;
    }

    std::string ParseNumber()
    {
        size_t start = Pos_;
        
        if (Json_[Pos_] == '-') {
            ++Pos_;
        }
        
        if (Pos_ >= Json_.size() || !std::isdigit(Json_[Pos_])) {
            throw std::runtime_error("Invalid number");
        }
        
        while (Pos_ < Json_.size() && 
               (std::isdigit(Json_[Pos_]) || Json_[Pos_] == '.' || 
                Json_[Pos_] == 'e' || Json_[Pos_] == 'E' || 
                Json_[Pos_] == '+' || Json_[Pos_] == '-')) {
            ++Pos_;
        }
        
        return Json_.substr(start, Pos_ - start);
    }

    TBasonRecord ParseValue(const std::string& key)
    {
        char ch = Peek();
        
        if (ch == '"') {
            return TBasonRecord(EBasonType::String, key, ParseString());
        } else if (ch == '{') {
            return ParseObject(key);
        } else if (ch == '[') {
            return ParseArray(key);
        } else if (ch == 't' || ch == 'f' || ch == 'n') {
            return ParseLiteral(key);
        } else if (ch == '-' || std::isdigit(ch)) {
            return TBasonRecord(EBasonType::Number, key, ParseNumber());
        }
        
        throw std::runtime_error("Unexpected character in JSON");
    }

    TBasonRecord ParseLiteral(const std::string& key)
    {
        if (Json_.substr(Pos_, 4) == "true") {
            Pos_ += 4;
            return TBasonRecord(EBasonType::Boolean, key, "true");
        } else if (Json_.substr(Pos_, 5) == "false") {
            Pos_ += 5;
            return TBasonRecord(EBasonType::Boolean, key, "false");
        } else if (Json_.substr(Pos_, 4) == "null") {
            Pos_ += 4;
            return TBasonRecord(EBasonType::Boolean, key, "");  // null = empty value
        }
        
        throw std::runtime_error("Invalid literal");
    }

    TBasonRecord ParseObject(const std::string& key)
    {
        TBasonRecord record(EBasonType::Object, key, "");
        Expect('{');
        
        SkipWhitespace();
        if (Peek() == '}') {
            Next();
            return record;
        }
        
        while (true) {
            std::string fieldName = ParseString();
            Expect(':');
            auto value = ParseValue(fieldName);
            record.Children.push_back(std::move(value));
            
            SkipWhitespace();
            if (Peek() == '}') {
                Next();
                break;
            }
            Expect(',');
        }
        
        return record;
    }

    TBasonRecord ParseArray(const std::string& key)
    {
        TBasonRecord record(EBasonType::Array, key, "");
        Expect('[');
        
        SkipWhitespace();
        if (Peek() == ']') {
            Next();
            return record;
        }
        
        size_t index = 0;
        while (true) {
            std::string indexKey = EncodeRon64(index++);
            auto value = ParseValue(indexKey);
            record.Children.push_back(std::move(value));
            
            SkipWhitespace();
            if (Peek() == ']') {
                Next();
                break;
            }
            Expect(',');
        }
        
        return record;
    }
};

void RecordToJson(const TBasonRecord& record, std::ostringstream& out)
{
    switch (record.Type) {
        case EBasonType::String:
            out << '"';
            for (char ch : record.Value) {
                switch (ch) {
                    case '"': out << "\\\""; break;
                    case '\\': out << "\\\\"; break;
                    case '\b': out << "\\b"; break;
                    case '\f': out << "\\f"; break;
                    case '\n': out << "\\n"; break;
                    case '\r': out << "\\r"; break;
                    case '\t': out << "\\t"; break;
                    default:
                        if (ch < 32) {
                            // Control character - escape as \uXXXX
                            out << "\\u00" << std::hex << (int)(unsigned char)ch << std::dec;
                        } else {
                            out << ch;
                        }
                }
            }
            out << '"';
            break;

        case EBasonType::Number:
            out << record.Value;
            break;

        case EBasonType::Boolean:
            if (record.Value.empty()) {
                out << "null";
            } else {
                out << record.Value;
            }
            break;

        case EBasonType::Object:
            out << '{';
            for (size_t i = 0; i < record.Children.size(); ++i) {
                if (i > 0) out << ',';
                out << '"' << record.Children[i].Key << "\":";
                RecordToJson(record.Children[i], out);
            }
            out << '}';
            break;

        case EBasonType::Array:
            out << '[';
            for (size_t i = 0; i < record.Children.size(); ++i) {
                if (i > 0) out << ',';
                RecordToJson(record.Children[i], out);
            }
            out << ']';
            break;
    }
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

std::vector<uint8_t> JsonToBason(const std::string& json)
{
    TJsonParser parser(json);
    TBasonRecord record = parser.Parse();
    return EncodeBason(record);
}

std::string BasonToJson(const uint8_t* data, size_t len)
{
    auto [record, _] = DecodeBason(data, len);
    
    std::ostringstream out;
    RecordToJson(record, out);
    return out.str();
}

////////////////////////////////////////////////////////////////////////////////

} // namespace NBason
