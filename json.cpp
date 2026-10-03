#include "json.h"

using namespace std;
using namespace std::literals;

namespace json {

    namespace {

        string GetString(istream& input) {
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
                }
                else if (ch == '\\') {
                    ++it;
                    if (it == end) {
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
                    //  \\, \n, \t, \r, \"
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
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r') {
                    throw ParsingError("Unexpected end of line"s);
                }
                else {
                    s.push_back(ch);
                }
                ++it;
            }

            return s;
        }

        Node LoadNode(istream& input, bool is_first = false);

        Node LoadNumber(istream& input) {
            string parsed_num;

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
            // Целая часть числа
            if (input.peek() == '0') {
                read_char();
            }
            else {
                read_digits();
            }

            bool is_int = true;
            // Дробная часть числа
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
                    // Сначала пробуем преобразовать строку в int
                    try {
                        int num = std::stoi(parsed_num);
                        return { num };
                    }
                    catch (...) {
                    }
                }
                double num = std::stod(parsed_num);
                return { num };
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }

            return {};
        }

        Node LoadBool(istream& input) {
            string parsed_str{ "true"s };
            bool result{ true };

            char ch = input.get();
            if (ch == 'f') {
                parsed_str = "false"s;
                result = false;
            }

            for (size_t i = 1; i < parsed_str.size(); ++i) {
                ch = input.get();
                if (!input || ch != parsed_str[i]) {
                    throw ParsingError("Failed to read bool from stream"s);
                }
            }
            ch = input.get();
            if (input) {
                if (isalpha(ch) || isdigit(ch)) {
                    throw ParsingError("Failed to read bool from stream"s);
                }
                else {
                    input.putback(ch);
                }
            }

            return { result };
        }

        Node LoadNull(istream& input) {
            string parsed_str{ "null"s };
            for (size_t i = 0; i < parsed_str.size(); ++i) {
                char ch = input.get();
                if (!input || ch != parsed_str[i]) {
                    throw ParsingError("Failed to read null from stream"s);
                }
            }
            char ch = input.get();
            if (input) {
                if (isalpha(ch) || isdigit(ch)) {
                    throw ParsingError("Failed to read bool from stream"s);
                }
                else {
                    input.putback(ch);
                }
            }

            return {};
        }

        Node LoadString(istream& input) {
            return { GetString(input) };
        }

        Node LoadArray(std::istream& input) {
            Array result;

            input >> std::ws;                 // после '['
            if (!input) throw ParsingError("Failed to read Array");

            if (input.peek() == ']') {       // []
                input.get();
                return Node{ std::move(result) };
            }

            while (true) {
                result.push_back(LoadNode(input));

                input >> std::ws;
                if (!input) throw ParsingError("Failed to read Array");

                char sep = static_cast<char>(input.get()); // ',' или ']'
                if (sep == ',') {
                    input >> std::ws;
                    continue;
                }
                if (sep == ']') {
                    break;
                }
                throw ParsingError("Failed to read Array");
            }

            return Node{ std::move(result) };
        }

        Node LoadDict(std::istream& input) {
            Dict result;

            input >> std::ws;
            if (input.peek() == '}') { // {}
                input.get();
                return Node{ std::move(result) };
            }

            while (true) {
                input >> std::ws;
                if (input.get() != '"') throw ParsingError("Failed to read Dict"s);

                std::string key = GetString(input);

                input >> std::ws;
                if (input.get() != ':') throw ParsingError("Failed to read Dict"s);

                Node val = LoadNode(input);
                result.emplace(std::move(key), std::move(val));

                input >> std::ws;
                char sep = static_cast<char>(input.get());
                if (sep == ',') continue;
                if (sep == '}') break;

                throw ParsingError("Failed to read Dict"s);
            }

            return Node{ std::move(result) };
        }

        Node LoadNode(istream& input, bool is_first) {
            char ch;
            input >> ch;

            if (isdigit(ch) || ch == '-') {
                input.putback(ch);
                return LoadNumber(input);
            }
            else if (ch == 't' || ch == 'f') {
                input.putback(ch);
                return LoadBool(input);
            }
            else if (ch == 'n') {
                input.putback(ch);
                return LoadNull(input);
            }
            else if (ch == '"') {
                return LoadString(input);
            }
            else if (ch == '[') {
                return LoadArray(input);
            }
            else if (ch == '{') {
                return LoadDict(input);
            }
            else {
                if (is_first && !isspace(ch)) {
                    throw ParsingError("Failed to read Node from stream"s);
                }
                else {
                    input.putback(ch);
                }
            }

            return {};
        }

    }  // namespace

    bool Node::IsInt() const {
        return std::holds_alternative<int>(value_);
    }

    bool Node::IsDouble() const {
        if (std::holds_alternative<double>(value_)) {
            return true;
        }
        else {
            return IsInt();
        }
    }

    bool Node::IsPureDouble() const {
        return std::holds_alternative<double>(value_);
    }

    bool Node::IsBool() const {
        return std::holds_alternative<bool>(value_);
    }

    bool Node::IsString() const {
        return std::holds_alternative<std::string>(value_);
    }

    bool Node::IsNull() const {
        return std::holds_alternative<std::nullptr_t>(value_);
    }

    bool Node::IsArray() const {
        return std::holds_alternative<Array>(value_);
    }

    bool Node::IsMap() const {
        return std::holds_alternative<Dict>(value_);
    }

    int Node::AsInt() const {
        if (!IsInt()) {
            throw std::logic_error("Node does not contain a int value!"s);
        }
        return std::get<int>(value_);
    }

    double Node::AsDouble() const {
        if (IsPureDouble()) {
            return std::get<double>(value_);
        }
        else if (!IsInt()) {
            throw std::logic_error("Node does not contain a double value!"s);
        }
        return static_cast<double>(std::get<int>(value_));
    }

    bool Node::AsBool() const {
        if (!IsBool()) {
            throw std::logic_error("Node does not contain a bool value!"s);
        }
        return std::get<bool>(value_);
    }

    const string& Node::AsString() const {
        if (!IsString()) {
            throw std::logic_error("Node does not contain a string value!"s);
        }
        return std::get<std::string>(value_);
    }

    const Array& Node::AsArray() const {
        if (!IsArray()) {
            throw std::logic_error("Node does not contain a Array value!"s);
        }
        return std::get<Array>(value_);
    }

    const Dict& Node::AsMap() const {
        if (!IsMap()) {
            throw std::logic_error("Node does not contain a Dict value!"s);
        }
        return std::get<Dict>(value_);
    }

    const Node& Document::GetRoot() const {
        return root_;
    }

    Document Load(istream& input) {
        return Document{ LoadNode(input, true) };
    }

    struct PrintContext {
        ostream& out;
        int indent_step{ 4 };
        int indent{ 0 };

        void PrintIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        PrintContext Indented() {
            return { out, indent_step, indent + indent_step };
        }
    };

    void PrintValue(std::nullptr_t, PrintContext& ctx) {
        ctx.out << "null"sv;
    }

    void PrintValue(int value, PrintContext& ctx) {
        ctx.out << value;
    }

    void PrintValue(double value, PrintContext& ctx) {
        ctx.out << value;
    }

    void PrintValue(const string& value, PrintContext& ctx) {
        ctx.out << "\""sv;
        for (char ch : value) {
            switch (ch) {
            case '\r': {
                ctx.out << "\\r"sv;
                break;
            }
            case '\n': {
                ctx.out << "\\n"sv;
                break;
            }
            case '\t': {
                ctx.out << "\\t"sv;
                break;
            }
            case '\\':
            case '\"': {
                ctx.out << "\\"sv << ch;
                break;
            }
            default: {
                ctx.out << ch;
            }
            }
        }
        ctx.out << "\""sv;
    }

    void PrintValue(bool value, PrintContext& ctx) {
        if (value) {
            ctx.out << "true"sv;
        }
        else {
            ctx.out << "false"sv;
        }
    }

    void PrintNode(const Node& node, PrintContext& ctx);

    void PrintValue(const Array& value, PrintContext& ctx) {
        ctx.PrintIndent();
        ctx.out << "["sv;
        bool is_first{ true };

        for (const auto& node : value) {
            if (is_first) {
                is_first = false;
            }
            else {
                ctx.out << ", "sv;
            }
            PrintNode(node, ctx);
        }

        ctx.out << "]"sv;
    }

    void PrintValue(const Dict& value, PrintContext& ctx) {
        ctx.PrintIndent();
        ctx.out << "{"sv << endl;
        bool is_first{ true };

        PrintContext content = ctx.Indented();
        for (const auto& [key, node] : value) {
            if (is_first) {
                is_first = false;
            }
            else {
                content.out << ", "sv << endl;
            }
            content.PrintIndent();
            content.out << "\""sv << key << "\" : "sv;
            PrintNode(node, content);
        }

        ctx.out << endl << "}"sv;
    }

    void PrintNode(const Node& node, PrintContext& ctx) {
        visit([&ctx](const auto& value) { PrintValue(value, ctx); }, node.GetValue());
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintContext ctx{ output, 4, 0 };
        PrintNode(doc.GetRoot(), ctx);
    }

}  // namespace json