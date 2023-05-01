#include "json.h"

#include <cctype>
#include <cmath>
#include <unordered_map>

using namespace std::string_literals;

namespace json {

namespace {

//////////////////// Load Method's ////////////////////
    
std::string LoadLetters(std::istream& input) {
    std::string result;
    
    while (std::isalpha(static_cast<unsigned char>(input.peek()))) {
        result.push_back(static_cast<char>(input.get()));
    }

    return result;
}

Node LoadNode(std::istream& input);

Node LoadNull(std::istream& input) {
    if (auto value = LoadLetters(input); value == "null"s) {
        return Node{nullptr};
    }

    throw ParsingError("Not a null value"s);
}

Node LoadBool(std::istream& input) {
    std::string value = LoadLetters(input);
    
    if (value == "true"s) {
        return Node{true};
    }
    
    if (value == "false"s) {
        return Node{false};
    }

    throw ParsingError("Not a boolean value"s);
}

Node LoadArray(std::istream& input) {
    Array result;

    for (char symbol; input >> symbol && symbol != ']';) {
        if (symbol != ',')
            input.putback(symbol);

        result.emplace_back(LoadNode(input));

        if (input >> symbol) {
            if (symbol == ']') {
                break;
            }
            
            if (symbol != ',') {
                throw ParsingError("Parsing error in array data"s);
            }
        } else {
            throw ParsingError("Array parsing error"s);
        }
    }

    if (!input) {
        throw ParsingError("Incorrect format for Array Node parsing"s);
    }
    
    return Node{std::move(result)};
}

Node LoadNumber(std::istream& input) {
    std::string parsed_num;

    // Считывает в parsed_num очередной символ из input
    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        
        if (!input) {
            throw ParsingError("Failed to read number"s);
        }
    };

    // Считывает одну или более цифр в parsed_num из input
    auto read_digits = [&input, read_char] {
        if (!std::isdigit(input.peek())) {
            throw ParsingError("A digit is expected"s);
        }
        
        while (std::isdigit(input.peek())) {
            read_char();
        }
    };

    if (input.peek() == '-') {
        read_char();
    }
    
    // Парсим целую часть числа
    if (input.peek() == '0') {
        read_char();
        // После 0 в JSON не могут идти другие цифры
    } else {
        read_digits();
    }

    bool is_int = true;
    // Парсим дробную часть числа
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

    // Парсим экспоненциальную часть числа
    if (int ch = input.peek(); ch == 'e' || ch == 'E') {
        read_char();
        if (ch = input.peek(); ch == '+' || ch == '-') {
            read_char();
        }
        read_digits();
        is_int = false;
    }

    try {
        if (is_int) {
            // Сначала пробуем преобразовать строку в int
            try {
                return Node{std::stoi(parsed_num)};
            } catch (...) {
                // В случае неудачи, например, при переполнении,
                // код ниже попробует преобразовать строку в double
            }
        }
        return Node{std::stod(parsed_num)};
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadString(std::istream& input) {
    auto position = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();

    char current;
    char escape_symbol;

    std::string result;
    while (true) {
        if (position == end)
            throw ParsingError("Incorrect format for String Node parsing. Unexpected EOF"s);

        current = *position;

        if (current == '"') {
            ++position;
            break;
        }
        else if (current == '\\') {
            if (++position == end) {
                throw ParsingError("Incorrect input String for parsing. Part of escape current is missed"s);
            }
            
            escape_symbol = *position;
            switch (escape_symbol) {
                case 'n':
                    result.push_back('\n');
                    break;
                case 't':
                    result.push_back('\t');
                    break;
                case 'r':
                    result.push_back('\r');
                    break;
                case '"':
                    result.push_back('"');
                    break;
                case '\\':
                    result.push_back('\\');
                    break;
                default:
                    throw ParsingError("Incorrect input String for parsing. Unknown escape current \\"s +
                                       escape_symbol);
            }
        } else if (current == '\n' || current == '\r') {
            throw ParsingError("Incorrect input String for parsing. Unexpected EOF"s);
        } else {
            result.push_back(current);
        }

        ++position;
    }
    return Node{std::move(result)};
}

Node LoadDict(std::istream& input) {
    Dict result;

    for (char symbol; input >> symbol && symbol != '}';) {
        if (symbol == '"') {
            std::string key = LoadString(input).AsString();
            
            if (result.count(key) > 0) {
                throw ParsingError("Key "s + key + " is already exists in the Dict"s);
            }
            
            if (input >> symbol && symbol != ':') {
                throw ParsingError("Expected ':' symbol after key)"s);
            }
            
            result.emplace(std::move(key), LoadNode(input));
        } else if (symbol != ',') {
            throw ParsingError("Data must be separated by ',' symbol)"s);
        }
    }

    if (!input)
        throw ParsingError("Incorrect format for Dict Node parsing"s);

    return Node{std::move(result)};
}

Node LoadNode(std::istream& input) {
    char symbol;
    if (!(input >> symbol))
        throw ParsingError("Incorrect format for Node parsing. Unexpected EOF"s);

    if (symbol == 'n') {
        input.putback(symbol);
        return LoadNull(input);
    } else if (symbol == 't' || symbol == 'f') {
        input.putback(symbol);
        return LoadBool(input);
    } else if (symbol == '[') {
        return LoadArray(input);
    } else if (symbol == '{') {
        return LoadDict(input);
    } else if (symbol == '"') {
        return LoadString(input);
    } else {
        input.putback(symbol);
        return LoadNumber(input);
    }
}

}  // namespace

//////////////////// IsType Method's ////////////////////

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(*this);
}
    
bool Node::IsBool() const {
    return std::holds_alternative<bool>(*this);
}
    
bool Node::IsInt() const {
    return std::holds_alternative<int>(*this);
}
    
bool Node::IsDouble() const {
    return std::holds_alternative<double>(*this) || std::holds_alternative<int>(*this);
}
    
bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(*this);
}
    
bool Node::IsString() const {
    return std::holds_alternative<std::string>(*this);
}
    
bool Node::IsArray() const {
    return std::holds_alternative<Array>(*this);
}
    
bool Node::IsMap() const {
    return std::holds_alternative<Dict>(*this);
}

//////////////////// AsType Method's ////////////////////

const Node::Value& Node::GetValue() const {
    return *this;
}

bool Node::AsBool() const {
    if (auto* value = std::get_if<bool>(this)) {
        return *value;
    }

    throw std::logic_error("Impossible to parse node as Boolean"s);
}

int Node::AsInt() const {
    if (auto* value = std::get_if<int>(this)) {
        return *value;
    }
    
    throw std::logic_error("Impossible to parse node as Int "s);
}

double Node::AsDouble() const {
    if (auto* value = std::get_if<double>(this)) {
        return *value;
    }

    if (auto* value = std::get_if<int>(this)) {
        return static_cast<double>(*value);
    }

    throw std::logic_error("Impossible to parse node as Double "s);
}

const std::string& Node::AsString() const {
    if (auto* value = std::get_if<std::string>(this)) {
        return *value;
    }
    
    throw std::logic_error("Impossible to parse node as String"s);
}

const Array& Node::AsArray() const {
    if (auto* value = std::get_if<Array>(this)) {
        return *value;
    }
    
    throw std::logic_error("Impossible to parse node as Array"s);
}

const Dict& Node::AsMap() const {
    if (auto* value = std::get_if<Dict>(this)) {
        return *value;
    }
    
    throw std::logic_error("Impossible to parse node as Dict"s);
}

//////////////////// overload operators ////////////////////
    
bool operator==(const Node& left, const Node& right) {
    return left.GetValue() == right.GetValue();
}

bool operator!=(const Node& left, const Node& right) {
    return !(left == right);
}

//////////////////// Document ////////////////////
    
Document::Document(Node root) : root_(std::move(root)) {}

const Node& Document::GetRoot() const {
    return root_;
}

bool operator==(const Document& left, const Document& right) {
    return left.GetRoot() == right.GetRoot();
}

bool operator!=(const Document& left, const Document& right) {
    return !(left == right);
}

Document Load(std::istream& input) {
    return Document{LoadNode(input)};
}

//////////////////// Output ////////////////////

void NodeOutputPrinter::operator() (std::nullptr_t) {
    out << "null"s;
}
    
void NodeOutputPrinter::operator() (const Array& value) {
    out << "["s;
    for (auto iter = value.begin(); iter < value.end(); ++iter) {
        std::visit(NodeOutputPrinter{ out }, iter->GetValue());
        if (value.end() - iter > 1) {
            out << ", "s;
        }
    }
    out << "]"s;
}
    
void NodeOutputPrinter::operator() (const Dict& value) {
    out << "{"s;
    for (auto iter = value.begin(); iter != value.end(); ++iter) {
        out << '"' << iter->first << '"' << ':';
        std::visit(NodeOutputPrinter{ out }, iter->second.GetValue());
        if (iter != prev(value.end())) {
            out << ", "s;
        }
    }
    out << "}";
}
    
void NodeOutputPrinter::operator() (bool value) {
    out << std::boolalpha << value;
}
    
void NodeOutputPrinter::operator() (int value) {
    out << value;
}
    
void NodeOutputPrinter::operator() (double value) {
    out << value;
}
    
void NodeOutputPrinter::operator() (const std::string& value) {
    out << "\""s;
    for (const auto& ch : value) {
        switch (ch) {
        case '"':
            out << "\\\""s;
            break;
        case '\\':
            out << "\\\\"s;
            break;
        case '\n':
            out << "\\n"s;
            break;
        case '\r':
            out << "\\r"s;
            break;
        case '\t':
            out << "\t"s;
            break;
        default:
            out << ch;
        }
    }
    out << "\""s;
}

void Print(const Document& doc, std::ostream& output) {
    std::visit(NodeOutputPrinter{ output }, doc.GetRoot().GetValue());
}

}  // namespace json