#pragma once
#include "svg.h"
#include "geo.h"
#include "transport_catalogue.h"
#include <algorithm>
#include <cmath>

inline const double EPSILON = 1e-6;
inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}
class SphereProjector { // масштаб, пикс.координаты, отступы
public:
    // points_begin и points_end задают начало и конец интервала элементов geo::Coordinates
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
        double max_width, double max_height, double padding)
        : padding_(padding) //
    {
        // Если точки поверхности сферы не заданы, вычислять нечего
        if (points_begin == points_end) {
            return;
        }

        // Находим точки с минимальной и максимальной долготой
        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        // Находим точки с минимальной и максимальной широтой
        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        // Вычисляем коэффициент масштабирования вдоль координаты x
        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        // Вычисляем коэффициент масштабирования вдоль координаты y
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            // Коэффициенты масштабирования по ширине и высоте ненулевые,
            // берём минимальный из них
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
            // Коэффициент масштабирования по ширине ненулевой, используем его
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            // Коэффициент масштабирования по высоте ненулевой, используем его
            zoom_coeff_ = *height_zoom;
        }
    }

    // Проецирует широту и долготу в координаты внутри SVG-изображения
    svg::Point operator()(geo::Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer
{
    // using Color = std::variant<std::monostate, std::string, svg::Rgb, Rgba>;
public:

    struct RenderSettings {
    public:
        double zoom_coeff_ = 0;

        double width = 0.0;
        double height = 0.0;
        double line_width = 0.0;
        double stop_radius = 0.0;
        int bus_label_font_size = 0;
        svg::Point bus_label_offset; // vector double

        std::string font_family; // font_family из Text

        int stop_label_font_size = 0;
        svg::Point stop_label_offset; // vector double

        svg::Color underlayer_color = "none";
        //= svg::Rgba(255, 255, 255, 0.85);
        double underlayer_width = 0.0;
        std::vector<svg::Color> color_palette;

        double min_lat_ = 0, max_lat_ = 0, max_lng = 0, min_lng = 0, padding = 0;
    };

    struct StopPixelPosition {
    public:
        double x;
        double y;
    };

    MapRenderer(const RenderSettings& settings, const std::deque<Bus>& buses, const std::deque<Stop>& stops, const std::vector<geo::Coordinates>& coordinates)
        : settings_(settings), buses_(buses), stops_(stops), coordinates_(coordinates) {
    }
    MapRenderer() = default;
    MapRenderer(RenderSettings& settings) : settings_(settings) {}
public:
    // цвет
    static svg::Color ColorConvert(const json::Node& node);
    std::string GetColorString(const svg::Color& color);

    // установка значений
    void SetSettings(const RenderSettings& settings);
    void SetStops(StopPtr stop);
    void SetBus(BusPtr bus);
    void SetRoute(const TransportCatalogue& cat);
    std::vector<StopPtr> GetUniqueStops(const std::deque<Bus>& all_buses);
    std::vector<StopPtr> SearchingEndStops(BusPtr bus) const;

    // Парсинг настроек
    static MapRenderer::RenderSettings ParseRenderSettingsFromJson(const json::Node node);

    // текст
    void CreateBusNameLabel(
        const Bus& bus,
        StopPtr stop,
        SphereProjector& projector,
        RenderSettings& settings,
        svg::ObjectContainer& doc,
        svg::Color bus_color
    );
    // линия
    void CreatePolyline(svg::Color color, std::vector<svg::Point> points, svg::ObjectContainer& doc) const;
    // круги
    void AddCircles(svg::ObjectContainer& container, const TransportCatalogue& cat, SphereProjector& proj, RenderSettings& settings, std::vector<StopPtr> v);
    // названия остановок на карте
    void RenderStopNamesOnCircles(
        StopPtr stop,
        SphereProjector& projector,
        RenderSettings& settings,
        svg::ObjectContainer& doc);
    // сортировки
    void SortBuses(std::deque<Bus>& buses);
    void SortStops(std::deque<Stop>& stops);

    svg::Color GetCurrentColor(size_t color_index);
    //рендер
    void RenderMap(const TransportCatalogue& cat);
private:
    RenderSettings settings_;
    std::deque<Bus> buses_;
    std::deque<Stop> stops_;
    std::vector<geo::Coordinates> coordinates_;

    // svg::Text route_text;

    // void Draw(svg::ObjectContainer& container) const override;
};


