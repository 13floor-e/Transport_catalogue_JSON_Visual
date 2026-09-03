#include "map_renderer.h"
#include "svg.h"

#include <iomanip>
#include <iostream>
#include <cstdio>
#include <cassert>
#include <sstream>
#include <set>
using namespace std;

void MapRenderer::SortBuses(deque<Bus>& buses) {
    sort(buses.begin(), buses.end(), [](Bus lhs, Bus rhs) {
        return lhs.name < rhs.name;
        });
}
void MapRenderer::SortStops(deque<Stop>& stops) {
    sort(stops.begin(), stops.end(), [](Stop lhs, Stop rhs) {
        return lhs.name < rhs.name;
        });
}

vector<StopPtr> MapRenderer::SearchingEndStops(BusPtr bus) const {
    vector<StopPtr> result;

    if (bus->stops.empty()) {
        return result;
    }

    result.push_back(bus->stops.front());

    if (!bus->is_roundtrip && bus->stops.size() > 1) {
        if (bus->stops.front() != bus->stops.back()){
             result.push_back(bus->stops.back());
        }
    }

    return result;
}

svg::Color MapRenderer::ColorConvert(const json::Node& node)
{
    if (node.IsString()) {
        return node.AsString();
    }

    assert(node.IsArray());
    json::Array color_arr = node.AsArray();
    if (color_arr.size() == 3) {
        return svg::Rgb{ static_cast<uint8_t>(color_arr[0].AsInt()),static_cast<uint8_t>(color_arr[1].AsInt()),static_cast<uint8_t>(color_arr[2].AsInt()) };
    }

    return svg::Rgba{ static_cast<uint8_t>(color_arr[0].AsInt()),static_cast<uint8_t>(color_arr[1].AsInt()),static_cast<uint8_t>(color_arr[2].AsInt()),color_arr[3].AsDouble() };
}


string MapRenderer::GetColorString(const svg::Color& color) {
    if (auto* str = get_if<std::string>(&color)) {
        return *str;  // Строка — возвращаем без изменений
    }
    std::ostringstream oss;
    if (auto* rgb = std::get_if<svg::Rgb>(&color)) {
        // RGB: [r, g, b] → rgb(r,g,b)
        return "rgb(" +
            to_string(static_cast<int>(rgb->red)) + "," +
            to_string(static_cast<int>(rgb->green)) + "," +
            to_string(static_cast<int>(rgb->blue)) + ")";
    }

    if (auto* rgba = std::get_if<svg::Rgba>(&color)) {
        // RGBA: [r, g, b, a] → rgba(r,g,b,a)
        oss << "rgba("
            << static_cast<int>(rgba->red) << ","
            << static_cast<int>(rgba->green) << ","
            << static_cast<int>(rgba->blue) << ","
            << rgba->opacity << ")";
        return oss.str();
    }

    throw invalid_argument("Unsupported color array size");
}
void MapRenderer::SetStops(StopPtr stop)
{
    stops_.push_back(*stop);
}

void MapRenderer::SetBus(BusPtr bus) {
    buses_.push_back(*bus);
}

void MapRenderer::SetRoute(const TransportCatalogue& cat) {
    buses_ = cat.GetBuses();
    stops_ = cat.GetStops();

    coordinates_.clear();
    coordinates_.reserve(stops_.size());
    for (const auto& stop : stops_) {
        coordinates_.push_back(stop.position);
    }
    SortBuses(buses_);
    SortStops(stops_);
}

void MapRenderer::CreatePolyline(svg::Color color, vector<svg::Point> points, svg::ObjectContainer& doc) const {
    svg::Polyline polyline;
    polyline.SetStrokeColor(color)
        .SetFillColor("none")
        .SetStrokeWidth(settings_.line_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
    for (const auto& point : points) {
        polyline.AddPoint(point);
    }

    doc.Add(move(polyline)); // линии
}

void MapRenderer::CreateBusNameLabel(
    const Bus& bus,
    StopPtr stop,
    SphereProjector& projector,
    RenderSettings& settings,
    svg::ObjectContainer& doc,
    svg::Color bus_color
) {
    svg::Point pixel_pos = projector(stop->position);

    svg::Text label_underlayer_1;

    label_underlayer_1.SetPosition(pixel_pos)
        .SetOffset(settings.bus_label_offset)
        .SetFontSize(settings.bus_label_font_size)
        .SetFontFamily("Verdana")
        .SetFontWeight("bold")
        .SetData(bus.name)
        .SetFillColor(settings.underlayer_color)
        .SetStrokeColor(settings.underlayer_color)
        .SetStrokeWidth(settings.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    // Подложка
    doc.Add(move(label_underlayer_1));

    svg::Text label_text_1;

    label_text_1.SetPosition(pixel_pos)
        .SetOffset(settings.bus_label_offset)
        .SetFontSize(settings.bus_label_font_size)
        .SetFontFamily("Verdana")
        .SetFontWeight("bold")
        //.SetStrokeWidth(settings.underlayer_width)
        .SetData(bus.name)
        .SetFillColor(bus_color);

    // Основная надпись
    doc.Add(move(label_text_1));
}

void MapRenderer::AddCircles(svg::ObjectContainer& container, const TransportCatalogue& cat, SphereProjector& proj, RenderSettings& settings, vector<StopPtr> v)
{
    for (auto& stop : v) {
        svg::Point pixel_pos = proj(stop->position);
        svg::Circle circle;
        circle.SetCenter(pixel_pos);
        circle.SetRadius(settings.stop_radius);
        circle.SetColor("white");

        container.Add(move(circle));
    }

}

void MapRenderer::RenderStopNamesOnCircles( // имена остановок на кругах
    StopPtr stop,
    SphereProjector& projector,
    RenderSettings& settings,
    svg::ObjectContainer& doc)
{

    svg::Point pix_pos = projector(stop->position);

    svg::Text label_underlayer_2;
    label_underlayer_2.SetPosition(pix_pos)
        .SetOffset(settings.stop_label_offset)
        .SetFontSize(settings.stop_label_font_size)
        .SetFontFamily("Verdana")
        .SetData(stop->name)
        .SetFillColor(settings.underlayer_color)
        .SetStrokeColor(settings.underlayer_color)
        .SetStrokeWidth(settings.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    doc.Add(move(label_underlayer_2));

    svg::Text stop_name_text;
    stop_name_text.SetPosition(pix_pos)
        .SetOffset(settings.stop_label_offset)
        .SetFontSize(settings.stop_label_font_size)
        .SetFontFamily("Verdana")
        .SetData(stop->name)
        .SetFillColor("black");

    doc.Add(move(stop_name_text));
}

void MapRenderer::SetSettings(const RenderSettings& settings)
{
    settings_ = settings;
}

MapRenderer::RenderSettings MapRenderer::ParseRenderSettingsFromJson(const json::Node node)
{
    MapRenderer render;

    const auto& renderSettingsNode = node.AsMap();

    MapRenderer::RenderSettings settings;

    settings.width = renderSettingsNode.at("width").AsDouble();
    settings.height = renderSettingsNode.at("height").AsDouble();
    settings.padding = renderSettingsNode.at("padding").AsDouble();
    settings.line_width = renderSettingsNode.at("line_width").AsDouble();
    settings.stop_radius = renderSettingsNode.at("stop_radius").AsDouble();
    settings.bus_label_font_size = renderSettingsNode.at("bus_label_font_size").AsInt();
    settings.stop_label_font_size = renderSettingsNode.at("stop_label_font_size").AsInt();

    if (renderSettingsNode.count("underlayer_color")) {
        settings.underlayer_color = ColorConvert(renderSettingsNode.at("underlayer_color"));
    }
    else {
        settings.underlayer_color = "none";
    }

    settings.underlayer_width = renderSettingsNode.at("underlayer_width").AsDouble();
    // Для массивов (например, bus_label_offset)
    const auto& offsetNode = renderSettingsNode.at("bus_label_offset").AsArray();

    settings.bus_label_offset.x = offsetNode[0].AsDouble();
    settings.bus_label_offset.y = offsetNode[1].AsDouble();

    const auto& offsetNode_ = renderSettingsNode.at("stop_label_offset").AsArray();
    settings.stop_label_offset.x = offsetNode_[0].AsDouble();
    settings.stop_label_offset.y = offsetNode_[1].AsDouble();

    const auto& color_palette = renderSettingsNode.at("color_palette").AsArray();

    for (const auto& item : color_palette) {
        settings.color_palette.push_back(ColorConvert(item));
    }

    render.SetSettings(settings);
    return settings;

}

vector<StopPtr> MapRenderer::GetUniqueStops(const deque<Bus>& all_buses) {
    vector<StopPtr> all_stops;

    // 1. Собираем все остановки из всех автобусов
    for (const auto& bus : all_buses) {
        for (const auto* stop : bus.stops) {
            all_stops.push_back(stop);
        }
    }

    // 2. Сортируем по имени (это обязательно для unique)
    sort(all_stops.begin(), all_stops.end(), [](const StopPtr lhs, const StopPtr rhs) {
        return lhs->name < rhs->name;
        });

    // 3. Убираем дубликаты
    auto last = unique(all_stops.begin(), all_stops.end(), [](const StopPtr lhs, const StopPtr rhs) {
        return lhs->name == rhs->name;
        });

    // 4. Возвращаем очищенный вектор
    all_stops.erase(last, all_stops.end());

    return all_stops;
}

svg::Color MapRenderer::GetCurrentColor(size_t color_index) { // получаем текущий цвет
    const auto& raw_color =
        settings_.color_palette[color_index %
        settings_.color_palette.size()];

    svg::Color current_color = GetColorString(raw_color);
    return current_color;
}


void MapRenderer::RenderMap(const TransportCatalogue& cat)
{
    svg::Document doc;
    vector<geo::Coordinates> all_coordinates;
    SetRoute(cat);
    vector<StopPtr> sorted_bus_stops;

    all_coordinates.reserve(stops_.size());

    for (const auto& bus : buses_) {
        for (const auto* stop : bus.stops) {
            all_coordinates.push_back(stop->position);
        }
    }
    sorted_bus_stops = GetUniqueStops(buses_);

    if (all_coordinates.empty()) {
        cerr << "Coordinates are empty!\n";
        return;
    }

    SphereProjector projector(
        all_coordinates.begin(),
        all_coordinates.end(),
        settings_.width,
        settings_.height,
        settings_.padding
    );

    size_t color_index = 0;
    // Первый проход: только маршруты
    for (const auto& bus : buses_) {
        if (bus.stops.empty()) { continue; }

        vector<svg::Point> points;
        //points.reserve(bus.stops.size()+1);

        svg::Color current_color = GetCurrentColor(color_index);

        for (const auto* stop : bus.stops) {
            svg::Point point = projector(stop->position);
            if (std::isfinite(point.x) && std::isfinite(point.y)) {
                points.push_back(move(point));
            }
        }

        if (!bus.is_roundtrip) {
            // Проверяем, что есть хотя бы одна точка
            if (!bus.stops.empty()) {
                points.push_back(projector(bus.stops.front()->position));
            }
        }

        if (!points.empty()) {
            CreatePolyline(move(current_color), move(points), doc);
        }

        ++color_index;
    }

    // Второй проход: только подписи
    color_index = 0;

    for (auto& bus : buses_) {
        if (bus.stops.empty()) {
            continue;
        }

        svg::Color current_color = GetCurrentColor(color_index);

        // ищем конечные остановки
        auto end_points = SearchingEndStops(&bus);
        //MakeUnique(end_points);
        for (StopPtr& stop : end_points) { // устанавливаем на конечную остановку
            CreateBusNameLabel(
                bus,
                stop,
                projector,
                settings_,
                doc,
                current_color);
        }

        ++color_index;
    }
    AddCircles(doc, cat, projector, settings_, sorted_bus_stops);

    for (StopPtr& stop : sorted_bus_stops) {
        RenderStopNamesOnCircles(
            move(stop),
            projector,
            settings_,
            doc);
    }
    doc.Render(cout);
}

