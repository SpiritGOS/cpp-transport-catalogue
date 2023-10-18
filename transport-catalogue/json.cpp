#include "json.h"

#include <exception>

using namespace std;

namespace json {

namespace {

inline const size_t SKIP_NUM = 4;
inline const char SKIP_SYMBOLS[SKIP_NUM] = {' ', '\t', '\r', '\n'};

Node LoadNode(istream& input);

bool IsSkipSymbol(char c) {
    for (size_t i = 0; i < SKIP_NUM; ++i) {
        if (c == SKIP_SYMBOLS[i]) {
            return true;
        }
    }
    return false;
}

void FlushSkipSymbols(std::istream& strm) {
    char c;

    while (strm >> c) {
        if (!IsSkipSymbol(c)) {
            strm.putback(c);
            return;
        }
    }
}

Node LoadArray(istream& input) {
    Array result;

    char c;
    while (input >> c && c != ']') {
        if (c != ',') {
            input.putback(c);
        }
        result.push_back(LoadNode(input));
        FlushSkipSymbols(input);
    }
    if (c != ']') {
        throw ParsingError("Error loading array. Unexpected EOF.");
    }

    return Node(move(result));
}

Node LoadString(std::istream& input) {
    using namespace std::literals;

    auto it = std::istreambuf_iterator<char>(input);
    auto end = std::istreambuf_iterator<char>();
    std::string s;
    while (true) {
        if (it == end) {
            throw ParsingError("String parsing error");
        }
        const char ch = *it;
        if (ch == '"') {
            ++it;
            break;
        } else if (ch == '\\') {
            ++it;
            if (it == end) {
                throw ParsingError("String parsing error");
            }
            const char escaped_char = *(it);
            switch (escaped_char) {
                case 'n':
                    s.push_back('\n');
                    break;
                case 't':
                    s.push_back('\t');
                    break;
                case 'r':
                    s.push_back('\r');
                    break;
                case '"':
                    s.push_back('"');
                    break;
                case '\\':
                    s.push_back('\\');
                    break;
                default:
                    throw ParsingError("Unrecognized escape sequence \\"s +
                                       escaped_char);
            }
        } else if (ch == '\n' || ch == '\r') {
            throw ParsingError("Unexpected end of line"s);
        } else {
            s.push_back(ch);
        }
        ++it;
    }

    return Node(std::move(s));
}

Node LoadDict(istream& input) {
    Dict result;

    FlushSkipSymbols(input);
    char c;
    while (input >> c && c != '}') {
        if (c == ',') {
            FlushSkipSymbols(input);
            input >> c;
        }

        string key = LoadString(input).AsString();
        FlushSkipSymbols(input);
        input >> c;
        result.insert({move(key), LoadNode(input)});
        FlushSkipSymbols(input);
    }
    if (c != '}') {
        throw ParsingError("Error wile loading dictionary. Unexpected EOF.");
    }

    return Node(move(result));
}

Node LoadNumber(std::istream& input) {
    using namespace std::literals;

    std::string parsed_num;

    auto read_char = [&parsed_num, &input] {
        parsed_num += static_cast<char>(input.get());
        if (!input) {
            throw ParsingError("Failed to read number from stream"s);
        }
    };

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
    if (input.peek() == '0') {
        read_char();
    } else {
        read_digits();
    }

    bool is_int = true;
    if (input.peek() == '.') {
        read_char();
        read_digits();
        is_int = false;
    }

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
            try {
                return Node(std::stoi(parsed_num));
            } catch (...) {
            }
        }
        return Node(std::stod(parsed_num));
    } catch (...) {
        throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
    }
}

Node LoadBool(std::istream& input) {
    std::string check;
    check.reserve(5);
    char c;

    while (check.size() < 4 && input >> c) {
        check += c;
    }

    if (check.size() < 4) {
        throw ParsingError("Failed to read bool value. EOF!"s);
    }

    bool result;
    if (check == "true"s) {
        result = true;
    } else {
        if (!(input >> c)) {
            throw ParsingError("Failed to read bool value. EOF!"s);
        }
        check += c;
        if (check != "false"s) {
            cerr << "Data>>"s << check << "<<"s << endl;
            throw ParsingError(
                "Failed to read bool value. Incorrect symbols presented."s);
        }
        result = false;
    }

    return Node(result);
}

Node LoadNull(std::istream& input) {
    std::string check;
    check.reserve(5);
    char c;

    while (check.size() < 4 && input >> c) {
        check += c;
    }

    if (check.size() < 4) {
        throw ParsingError("Failed to read null value. EOF!"s);
    }
    if (check != "null"s) {
        throw ParsingError(
            "Failed to read null value. Incorrect symbols presented."s);
    }

    return Node(nullptr);
}

Node LoadNode(std::istream& input) {
    char c;

    FlushSkipSymbols(input);  // start of node reading

    input >> c;
    if (c == '[') {
        return LoadArray(input);
    } else if (c == '{') {
        return LoadDict(input);
    } else if (c == '"') {
        return LoadString(input);
    } else if (c == 't' || c == 'f') {
        input.putback(c);
        return LoadBool(input);
    } else if (c == 'n') {
        input.putback(c);
        return LoadNull(input);
    } else {
        input.putback(c);
        return LoadNumber(input);
    }
}

}  // namespace

const Array& Node::AsArray() const {
    if (!IsArray()) {
        throw std::logic_error("Node value is not array.");
    }
    return std::get<Array>(value_);
}

const Dict& Node::AsMap() const {
    if (!IsMap()) {
        throw std::logic_error("Node value is not map.");
    }
    return std::get<Dict>(value_);
}

int Node::AsInt() const {
    if (!IsInt()) {
        throw std::logic_error("Node value is not int.");
    }
    return std::get<int>(value_);
}

const string& Node::AsString() const {
    if (!IsString()) {
        throw std::logic_error("Node value is not string.");
    }
    return std::get<std::string>(value_);
}

bool Node::AsBool() const {
    if (!IsBool()) {
        throw std::logic_error("Node value is not bool.");
    }
    return std::get<bool>(value_);
}

double Node::AsDouble() const {
    if (!IsDouble()) {
        throw std::logic_error("Node value is not double and not int.");
    }
    if (IsInt()) {
        int res = std::get<int>(value_);
        return static_cast<double>(res);
    } else {
        return std::get<double>(value_);
    }
}

bool Node::IsInt() const { return std::holds_alternative<int>(value_); }

bool Node::IsDouble() const { return IsPureDouble() || IsInt(); }

bool Node::IsPureDouble() const {
    return std::holds_alternative<double>(value_);
}

bool Node::IsBool() const { return std::holds_alternative<bool>(value_); }

bool Node::IsString() const {
    return std::holds_alternative<std::string>(value_);
}

bool Node::IsNull() const {
    return std::holds_alternative<std::nullptr_t>(value_);
}

bool Node::IsArray() const { return std::holds_alternative<Array>(value_); }

bool Node::IsMap() const { return std::holds_alternative<Dict>(value_); }

bool Node::operator==(const Node& other) const {
    return this->value_ == other.value_;
}

bool Node::operator!=(const Node& other) const {
    return this->value_ != other.value_;
}

Document::Document(Node root) : root_(move(root)) {}

const Node& Document::GetRoot() const { return root_; }

bool Document::operator==(const Document& other) const {
    return this->GetRoot() == other.GetRoot();
}

bool Document::operator!=(const Document& other) const {
    return this->GetRoot() != other.GetRoot();
}

Document Load(istream& input) { return Document{LoadNode(input)}; }

void Print(const Document& doc, std::ostream& output) {
    PrintNode(doc.GetRoot(), output);
}

void PrintValue(std::nullptr_t, ostream& out) { out << "null"sv; }

void PrintValue(const string& str, ostream& out) {
    out << "\""sv;

    for (char c : str) {
        switch (c) {
            case '\"':
                out << "\\\""sv;
                break;
            case '\r':
                out << "\\r"sv;
                break;
            case '\n':
                out << "\\n"sv;
                break;
            case '\t':
                out << "\t"sv;
                break;
            case '\\':
                out << "\\\\"sv;
                break;
            default:
                out << c;
        }
    }

    out << "\""sv;
}

void PrintValue(bool val, std::ostream& out) {
    if (val) {
        out << "true"sv;
    } else {
        out << "false"sv;
    }
}

void PrintValue(const Array& arr, ostream& out) {
    out << "["sv;
    for (auto iter = arr.begin(); iter != arr.end(); ++iter) {
        PrintNode(*iter, out);
        if (std::next(iter) != arr.end()) {
            out << ", "sv;
        }
    }
    out << "]"sv;
}

void PrintValue(const Dict& dict, ostream& out) {
    out << "{"sv;
    for (auto iter = dict.begin(); iter != dict.end(); ++iter) {
        out << "\""sv << iter->first << "\":"sv;
        PrintNode(iter->second, out);
        if (std::next(iter) != dict.end()) {
            out << ", "sv;
        }
    }
    out << "}";
}

void PrintNode(const Node& node, ostream& out) {
    std::visit([&out](const auto& value) { PrintValue(value, out); },
               node.GetValue());
}

}  // namespace json