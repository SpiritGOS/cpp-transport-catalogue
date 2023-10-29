#pragma once

#include <string>
#include <vector>

#include "json.h"

namespace json {

class Builder {

    class DictItemContext;
    class ArrayContext;
    class KeyContext;

    class Context {
    public:
        Context(Builder& builder) : builder_(builder) {}
        DictItemContext StartDict();
        ArrayContext StartArray();
        Builder& EndDict();
        Builder& EndArray();
        KeyContext Key(std::string key);
    protected:
        Builder& builder_;
    };

    class KeyContext : public Context {
    public:
        DictItemContext Value(Node::Value value);
        Builder& EndDict() = delete;
        Builder& EndArray() = delete;
        KeyContext Key(std::string key) = delete;
    };

    class DictItemContext : public Context {
    public:
        DictItemContext StartDict() = delete;
        ArrayContext StartArray() = delete;
        Builder& EndArray() = delete;
    };

    class ArrayContext : public Context {
    public:
        ArrayContext Value(Node::Value value);
        Builder& EndDict() = delete;
        KeyContext Key(std::string key) = delete;
    };
    
public:
    
    Builder() = default;
    DictItemContext StartDict();
    ArrayContext StartArray();
    KeyContext Key(std::string);
    Builder& Value(Node::Value);
    Builder& EndDict();
    Builder& EndArray();
    Node Build();
    ~Builder() = default;

private:

    bool IsEmpty();
    Node* GetCurrentNodePtr();
    Node* InsertNode(Node node);
    
    Node root_;
    std::vector<Node*> nodes_stack_;
};

}