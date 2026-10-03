#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <variant>

namespace json {

    class Node;
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;
    using Value = std::variant<std::nullptr_t, int, double, std::string, bool, Array, Dict>;

    // Эта ошибка должна выбрасываться при ошибках парсинга JSON
    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class Node {
    public:
        Node() : value_(nullptr) {}

        Node(Value value) : value_(std::move(value)) {}
        Node(Dict value) : value_(std::move(value)) {}
        Node(Array value) : value_(std::move(value)) {}
        Node(std::nullptr_t) : value_(nullptr) {}
        Node(int value) : value_(value) {}
        Node(double value) : value_(value) {}
        Node(std::string value) : value_(std::move(value)) {}
        Node(bool value) : value_(value) {}

        //Node(std::vector<std::pair<double,double>> pair_v) : pair_vector(std::move(pair_v)) {}

        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsBool() const;
        bool IsString() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;

        int AsInt() const;
        double AsDouble() const;
        bool AsBool() const;
        const std::string& AsString() const;
        const Array& AsArray() const;
        const Dict& AsMap() const;

        const Value& GetValue() const {
            return value_;
        }

        Value& GetValue() {
            return value_;
        }
        Array& AsArray() {
            if (!IsArray()) {
                throw std::logic_error("Node does not contain a Array value!");
            }
            return std::get<Array>(value_);
        }

        bool operator==(const Node& other) const {
            return value_ == other.value_;
        }
        bool operator!=(const Node& other) const {
            return !(*this == other);
        }

    private:
        Value value_;
    };

    class Document {
    public:
        //Document() = default;

        Document(Node root) : root_(std::move(root)) {}

        const Node& GetRoot() const;

        bool operator==(const Document& other) const {
            return this->root_ == other.root_;
        }
        bool operator!=(const Document& other) const {
            return !(*this == other);
        }

    private:
        Node root_;
    };

    Document Load(std::istream& input);

    void Print(const Document& doc, std::ostream& output);

}  // namespace json