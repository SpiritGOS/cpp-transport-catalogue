#include "json_builder.h"

#include <string>

namespace json {

using namespace std::literals;

bool Builder::IsEmpty() { return nodes_stack_.size() == 0; }

Node* Builder::GetCurrentNodePtr() {
    if (IsEmpty()) {
        throw std::logic_error("stack empty"s);
    }
    return nodes_stack_.back();
}

Builder::KeyContext Builder::Key(std::string key) {
    if (GetCurrentNodePtr()->IsMap()) {
        Dict& current_node = const_cast<Dict&>(GetCurrentNodePtr()->AsMap());
        current_node.emplace(key, key);
        nodes_stack_.push_back(&current_node[key]);
    } else {
        throw std::logic_error("Key() called not in context"s);
    }
    return {*this};
}

Node* Builder::InsertNode(Node node) {
    if (IsEmpty() && root_.IsNull()) {
        root_ = Node(node);
        return &root_;
    } else if (GetCurrentNodePtr()->IsArray()) {
        Array& current_node =
            const_cast<Array&>(GetCurrentNodePtr()->AsArray());
        current_node.push_back(node);
        return &current_node.back();
    } else if (GetCurrentNodePtr()->IsString()) {
        const std::string key = GetCurrentNodePtr()->AsString();
        nodes_stack_.pop_back();
        if (GetCurrentNodePtr()->IsMap()) {
            Dict& current_node =
                const_cast<Dict&>(GetCurrentNodePtr()->AsMap());
            current_node[key] = node;
            return &current_node[key];
        } else {
            throw std::logic_error("Insert Value not for the key"s);
        }
    } else {
        throw std::logic_error("Insert Value not in context"s);
    }
}

Builder& Builder::Value(Node::Value value) {
    InsertNode(value);
    return *this;
}

Builder::DictItemContext Builder::StartDict() {
    nodes_stack_.push_back(InsertNode(Dict{}));
    return {*this};
}

Builder::ArrayContext Builder::StartArray() {
    nodes_stack_.push_back(InsertNode(Array{}));
    return {*this};
}

Builder& Builder::EndDict() {
    if (GetCurrentNodePtr()->IsMap()) {
        nodes_stack_.pop_back();
    } else {
        throw std::logic_error("EndDict() called not in context"s);
    }
    return *this;
}

Builder& Builder::EndArray() {
    if (GetCurrentNodePtr()->IsArray()) {
        nodes_stack_.pop_back();
    } else {
        throw std::logic_error("EndArray() called not in context"s);
    }
    return *this;
}

Node Builder::Build() {
    if (root_.IsNull() || !IsEmpty()) {
        throw std::logic_error(
            "Build() after creating or not for ready objects"s);
    }
    return root_;
}

Builder::DictItemContext Builder::Context::StartDict() {
    builder_.StartDict();
    return {builder_};
}

Builder::ArrayContext Builder::Context::StartArray() {
    builder_.StartArray();
    return {builder_};
}

Builder& Builder::Context::EndDict() {
    builder_.EndDict();
    return builder_;
}

Builder& Builder::Context::EndArray() {
    builder_.EndArray();
    return builder_;
}

Builder::KeyContext Builder::Context::Key(std::string key) {
    builder_.Key(key);
    return {builder_};
}

Builder::DictItemContext Builder::KeyContext::Value(Node::Value value) {
    builder_.Value(value);
    return {builder_};
}

Builder::ArrayContext Builder::ArrayContext::Value(Node::Value value) {
    builder_.Value(value);
    return {builder_};
}

}  // namespace json
