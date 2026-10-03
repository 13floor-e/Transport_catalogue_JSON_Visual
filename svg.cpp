#include "svg.h"
#include "map_renderer.h"

#include <iostream>
#include <format>
#include <iomanip> // optional
using namespace std;
namespace svg {


    using namespace std::literals;
    inline std::ostream& operator<<(std::ostream& out, StrokeLineCap type) {
        switch (type) {
        case StrokeLineCap::BUTT: out << "butt"; break;
        case StrokeLineCap::ROUND: out << "round"; break;
        case StrokeLineCap::SQUARE: out << "square";break;
        default: break;
        }
        return out;
    }
    inline std::ostream& operator<<(ostream& out, StrokeLineJoin type) {
        switch (type) {
        case StrokeLineJoin::ARCS: out << "arcs"; break;
        case StrokeLineJoin::BEVEL: out << "bevel";break;
        case StrokeLineJoin::MITER: out << "miter"; break;
        case StrokeLineJoin::MITER_CLIP: out << "miter-clip"; break;
        case StrokeLineJoin::ROUND: out << "round"; break;
        default: break;
        }
        return out;
    }

    string RemoveTrailingZeros(string value) {
        const auto dot_pos = value.find('.');

        // Если дробной части нет
        if (dot_pos == std::string::npos) {
            return value;
        }

        // Удаляем нули справа
        value.erase(value.find_last_not_of('0') + 1);

        // Если после удаления нулей осталась точка — удаляем её
        if (!value.empty() && value.back() == '.') {
            value.pop_back();
        }

        return value;
    }

    std::string FormatNumber(double value) {
        return RemoveTrailingZeros(format("{:.6g}", value));
    }

    inline void PrintColor(ostream& os, const std::monostate&) {
        os << "none";
    }

    inline void PrintColor(ostream& os, const Rgb& color) {
        os << std::format("rgb({},{},{})", color.red, color.green, color.blue);
    }

    inline void PrintColor(std::ostream& os, const Rgba& color) {
        os << format("rgba({},{},{},{})", color.red, color.green, color.blue, FormatNumber(color.opacity));
    }

    inline void PrintColor(std::ostream& os, const std::string& color) {
        os << color;
    }

    std::ostream& operator<<(std::ostream& out, const Color color)
    {
        std::visit([&out](const auto& color) {
            PrintColor(out, color);
            }, color);
        return out;
    }


    inline void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }


    void HtmlEncodeString(std::ostream& out, std::string_view sv) { // html в виде строки
        for (char c : sv) {
            switch (c) {
            case '"':  out << "&quot;";  break;
            case '<':  out << "&lt;";   break;
            case '>':  out << "&gt;";   break;
            case '&':  out << "&amp;";  break;
            case '\'': out << "&apos;"; break;
            default:   out.put(c);       break;
            }
        }
    }

    template <typename T>
    inline void RenderValue(std::ostream& out, const T& value) {
        out << value;
    }

    template <>
    inline void RenderValue<std::string>(std::ostream& out, const std::string& s) {
        HtmlEncodeString(out, s);
    }


    // --------- Polyline ----------
    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }
    void Polyline::RenderObject(const RenderContext& context) const {
        context.RenderIndent();
        auto& out = context.out;

        out << "<polyline points=\"";
        for (size_t i = 0; i < points_.size(); ++i) {
            out << points_[i].x << "," << points_[i].y;
            if (i + 1 != points_.size()) {
                out << " ";
            }
        }
        out << "\"";

        // fill
        out << " fill=\"" << color_ << "\"";

        // stroke
        out << " stroke=\"" << stroke_color_ << "\"";
        out << " stroke-width=\"" << width_ << "\"";

        // linecap/linejoi
        out << " stroke-linecap=\"" << StrokeLineCap(cap_) << "\"";
        out << " stroke-linejoin=\"" << StrokeLineJoin(join_) << "\"";

        out << "/>\n";
    }

    void Document::AddPtr(std::unique_ptr<Object>&& obj)
    {
        objects_.emplace_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const
    {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" << std::endl;// вывод

        RenderContext context(out, 4);

        //std::cout << objects_[10];

        for (const auto& obj : objects_) { // проход
            obj->Render(context.Indented());
        }

        out << "</svg>" << std::endl; // вывод
    }

    Text& Text::SetPosition(Point pos)
    {
        pos_ = std::move(pos);
        return *this;

    }

    //void HtmlEncodeString(std::ostream& out, std::string_view sv) {
    //    for (char c : sv) {
    //        switch (c) {
    //        case '"':  out << "&quot;";  break;
    //        case '<':  out << "&lt;";   break;
    //        case '>':  out << "&gt;";   break;
    //        case '&':  out << "&amp;";  break;
    //        case '\'': out << "&apos;"; break;
    //        default:   out.put(c);       break;
    //        }
    //    }
    //}

    //template <typename T>
    //inline void RenderValue(std::ostream& out, const T& value) {
    //    out << value;
    //}

    //template <>
    //inline void RenderValue<std::string>(std::ostream& out, const std::string& s) {
    //    HtmlEncodeString(out, s);
    //}

    //void Text::RenderObject(const RenderContext& context) const {

    //    auto& out = context.out;
    //    out << "<text x=\"" << pos_.x << "\" y=\"" << pos_.y
    //        << "\" dx=\"" << offset_.x << "\" dy=\"" << offset_.y
    //        << "\" font-size=\"" << font_size_ << "\""
    //        << (font_family_.empty() ? "" : " font-family=\"" + font_family_ + "\"")
    //        << (font_weight_.empty() ? "" : " font-weight=\"" + font_weight_ + "\"");
    //    out << ">";
    //    RenderValue(out, data_); // Функция для коррект
    //    out << "</text>" << std::endl;
    //}
    // -----------------Circle-------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    Circle& Circle::SetColor(svg::Color color) {
        color_ = color;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const { // вывод
        context.RenderIndent();
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\" fill=\""sv << color_ << "\" "sv;
        out << "/>"sv;
    }

    // -----------------Text---------------------

    void Text::RenderObject(const RenderContext& context) const {
    context.out << "<text";

    RenderAttrs(context.out);

    
    context.out << " x=\"" << pos_.x << "\"";
    context.out << " y=\"" << pos_.y << "\"";

    context.out << " dx=\"" << offset_.x << "\"";
    context.out << " dy=\"" << offset_.y << "\"";


    context.out << " font-size=\"" << font_size_ << "\"";

    if (!font_family_.empty()) {
        context.out << " font-family=\"" << font_family_ << "\"";
    }

    
    if (!font_weight_.empty()) {
        context.out << " font-weight=\"" << font_weight_ << "\"";
    }

    context.out << ">";
    HtmlEncodeString(context.out, data_);
    context.out << "</text>";
    }

    Text& Text::SetOffset(Point offset)
    {
        offset_ = std::move(offset);
        return AsOwner();
    }

    Text& Text::SetFontSize(uint32_t size)
    {
        font_size_ = size;
        return AsOwner();
    }

    Text& Text::SetFontFamily(std::string font_family)
    {
        font_family_ = std::move(font_family);
        return AsOwner();
    }

    Text& Text::SetFontWeight(std::string font_weight)
    {
        font_weight_ = std::move(font_weight);
        return AsOwner();
    }

    Text& Text::SetData(std::string data)
    {
        data_ = std::move(data);
        return AsOwner();
    }


}  // namespace svg