#pragma once

#include <iostream>
#include <map>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace json {

class Node;

using Dict = std::map<std::string, Node>;
using Array = std::vector<Node>;

Node LoadNumber(std::istream& input);
Node LoadString(std::istream& input);

class ParsingError : public std::runtime_error {
   public:
    using runtime_error::runtime_error;
};

class Node {
   public:
    using Value = std::variant<std::nullptr_t, int, double, std::string, bool,
                               Array, Dict>;

    template <typename ValueType>
    Node(ValueType value) : value_(value) {}

    Node() : value_(nullptr) {}

    const Value& GetValue() const { return value_; }

    bool IsInt() const;
    bool IsDouble() const;
    bool IsPureDouble() const;
    bool IsBool() const;
    bool IsString() const;
    bool IsNull() const;
    bool IsArray() const;
    bool IsMap() const;

    int AsInt() const;
    bool AsBool() const;
    double AsDouble() const;
    const std::string& AsString() const;
    const Array& AsArray() const;
    const Dict& AsMap() const;

    bool operator==(const Node& other) const;
    bool operator!=(const Node& other) const;

   private:
    Value value_;
};

class Document {
   public:
    explicit Document(Node root);

    const Node& GetRoot() const;

    bool operator==(const Document& other) const;
    bool operator!=(const Document& other) const;

   private:
    Node root_;
};

Document Load(std::istream& input);

void Print(const Document& doc, std::ostream& output);

template <typename Value>
void PrintValue(const Value& value, std::ostream& out) {
    out << value;
}

void PrintValue(const std::string& str, std::ostream& out);
void PrintValue(std::nullptr_t, std::ostream& out);
void PrintValue(bool val, std::ostream& out);
void PrintValue(const Array& arr, std::ostream& out);
void PrintValue(const Dict& dict, std::ostream& out);

void PrintNode(const Node& node, std::ostream& out);

}  // namespace json