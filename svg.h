#pragma once

#include <optional>
#include <cstdint>
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <variant>

namespace svg {

    struct Rgb {
        Rgb() = default;
        Rgb(uint8_t r, uint8_t g, uint8_t b)
            : red(r)
            , green(g)
            , blue(b) {
        }
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    struct Rgba {
        Rgba() = default;
        Rgba(uint8_t r, uint8_t g, uint8_t b, double op)
            : red(r)
            , green(g)
            , blue(b)
            , opacity(op)
        {
        }
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 0.0;
    };

    using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;
    inline const Color NoneColor{};


    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    std::ostream& operator<<(std::ostream& out, StrokeLineCap type);
    std::ostream& operator<<(std::ostream& out, StrokeLineJoin type);
    std::ostream& operator<<(std::ostream& out, const Color color);



    template <typename Owner>
    class PathProps {
    protected:
        ~PathProps() = default;
    public:
        Owner& SetFillColor(Color color) {
            color_ = std::move(color);
            return AsOwner();
        }

        Owner& SetStrokeColor(Color color) {
            stroke_color_ = move(color);
            return AsOwner();
        }

        Owner& SetStrokeWidth(double width) {
            width_ = width;
            return AsOwner();
        }

        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            cap_ = line_cap;
            return AsOwner();
        }

        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            join_ = line_join;
            return AsOwner();
        }
        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (color_) {
                out << " fill=\""sv << color_.value() << "\""sv;
            }
            if (stroke_color_) {
                out << " stroke=\""sv << stroke_color_.value() << "\""sv;
            }
            if (width_) {
                out << " stroke-width=\""sv << width_.value() << "\""sv;
            }
            if (cap_) {
                out << " stroke-linecap=\""sv << cap_.value() << "\""sv;
            }
            if (join_) {
                out << " stroke-linejoin=\""sv << join_.value() << "\""sv;
            }
        }
    private:
        Owner& AsOwner() {
            // static_cast безопасно преобразует *this к Owner&,
            return static_cast<Owner&>(*this);
        }
        std::optional<Color> color_;
        std::optional<Color> stroke_color_;
        std::optional<double> width_;
        std::optional<StrokeLineCap> cap_;
        std::optional<StrokeLineJoin> join_;
    };


    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }
        double x = 0;
        double y = 0;
    };

    /*
     * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
     * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
     */
    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    /*
     * Абстрактный базовый класс Object служит для унифицированного хранения
     * конкретных тегов SVG-документа
     * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
     */
    class Object {
    public:
        inline void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;
    };

    class ObjectContainer {
    public:
        // Универсальный метод добавления: принимает ЛЮБОЙ объект, наследующий Object
        template <typename Shape>
        void Add(Shape object) {
            // Внутри он превращает любой объект в unique_ptr<Object>
            AddPtr(std::make_unique<Shape>(std::move(object)));
        }

        // Чисто виртуальный метод: каждый наследник должен решить, КАК именно добавлять объекты
        virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

    protected:
        ~ObjectContainer() = default; // Защита от случайного удаления через указатель на базовый класс
    };

    class Circle final : public Object, public PathProps<Circle> {
    public:
        ~Circle() = default;

        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);
        Circle& SetColor(svg::Color color);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_;
        double radius_ = 1.0;

        double width_ = 0.0;
        StrokeLineCap line_cap_;
        StrokeLineJoin line_join_;

        svg::Color color_;
    };

    class Polyline final : public Object {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);

        Polyline& SetFillColor(Color color) {
            color_ = std::move(color);
            return AsOwner();
        }

        Polyline& SetStrokeColor(Color color) {
            stroke_color_ = move(color);
            return AsOwner();
        }

        Polyline& SetStrokeWidth(double width) {
            width_ = width;
            return AsOwner();
        }

        Polyline& SetStrokeLineCap(StrokeLineCap line_cap) {
            cap_ = line_cap;
            return AsOwner();
        }

        Polyline& SetStrokeLineJoin(StrokeLineJoin line_join) {
            join_ = line_join;
            return AsOwner();
        }

    private:
        virtual void RenderObject(const RenderContext& context) const;
        std::vector<Point> points_;

        Polyline& AsOwner() {
            // static_cast безопасно преобразует *this к Owner&,
            return static_cast<Polyline&>(*this);
        }

        Color color_; // optional
        Color stroke_color_; // optional<Color>
        double width_;
        StrokeLineCap cap_;
        StrokeLineJoin join_;
    };

    /*
     * Класс Text моделирует элемент <text> для отображения текста
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
     */
    class Text : public Object, public PathProps<Text>{
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text& SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text& SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text& SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text& SetData(std::string data);

    private:
        virtual void RenderObject(const RenderContext& context) const;

        Text& AsOwner() {
            // static_cast безопасно преобразует *this к Owner&,
            return static_cast<Text&>(*this);
        }

        Point pos_ = { 0,0 };
        Point offset_ = { 0,0 };
        uint32_t font_size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_;
    };

    class Document : public ObjectContainer{
    public:

        Document() = default;
        /*
         Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
         Пример использования:
         Document doc;
         doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
        */
        template <typename Shape>
        void Add(Shape object) {

            static_assert(std::is_base_of<Object, Shape>::value, "Shape must inherit from Object");

            objects_.emplace_back(std::make_unique<Shape>(std::move(object)));
        }

        // Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(std::unique_ptr<Object>&& obj);

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const; // inline

        // Прочие методы и данные, необходимые для реализации класса Document
    private:
        std::vector<std::unique_ptr<Object>> objects_;
    };



}  // namespace svg