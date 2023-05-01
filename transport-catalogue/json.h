#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json {

class Node;

using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

class ParsingError : public std::runtime_error {
public:
    using runtime_error::runtime_error;
};

class Node final : private std::variant<std::nullptr_t, bool, int, double, std::string, Dict, Array> {
public:
    using variant::variant;
    using Value = variant;


    bool IsNull() const;
    bool IsBool() const;
    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsString() const;
    bool IsArray() const;
    bool IsMap() const;

    const Value& GetValue() const;
    
    bool AsBool() const;
    int AsInt() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    friend bool operator==(const Node& left, const Node& right);
    friend bool operator!=(const Node& left, const Node& right);
};

class Document {
public:
    Document() = default;
    
    explicit Document(Node root);

    const Node& GetRoot() const;

    friend bool operator==(const Document& left, const Document& right);
    friend bool operator!=(const Document& left, const Document& right);

private: 
    Node root_;
};

Document Load(std::istream& input);

struct NodeOutputPrinter {
    std::ostream& out;
    
    void operator() (std::nullptr_t);
    void operator() (const Array& value);
    void operator() (const Dict& value);
    void operator() (bool value);
    void operator() (int value);
    void operator() (double value);
    void operator() (const std::string& value);
};
    
void Print(const Document& doc, std::ostream& output);

}  // namespace json