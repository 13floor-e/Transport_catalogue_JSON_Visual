#include "json_reader.h"
#include "json_builder.h"
#include "request_handler.h"
#include "json.h"
#include <iostream>
#include "domain.h"
#include <algorithm>
#include "geo.h"
#include <cassert>
#include <sstream>
using namespace std;


void AddStopsInBaseRequests(TransportCatalogue& catalogue, const json::Dict& request_map) {

    // Этап 1. Добавляем все остановки.

        const std::string stop_name =
            request_map.at("name").AsString();

        if (stop_name.empty()) { // проверка на пустоту
            throw std::runtime_error(
                "Stop name must not be empty"
            );
        }

        if (catalogue.FindStop(stop_name) != nullptr) { // проверка на дубликаты
            throw std::runtime_error(
                "Duplicate stop: " + stop_name
            );
        }

        const double latitude =
            request_map.at("latitude").AsDouble();

        const double longitude =
            request_map.at("longitude").AsDouble();
        // проверки корректности расстояний
        if (!std::isfinite(latitude) ||
            !std::isfinite(longitude)) {
            throw std::runtime_error(
                "Invalid coordinates for stop: " + stop_name
            );
        }

        if (latitude < -90.0 || latitude > 90.0) {
            throw std::runtime_error(
                "Invalid latitude for stop: " + stop_name
            );
        }

        if (longitude < -180.0 || longitude > 180.0) {
            throw std::runtime_error(
                "Invalid longitude for stop: " + stop_name
            );
        }
        // конец проверок расстояний

        catalogue.AddStop(
            stop_name,
            geo::Coordinates{ latitude, longitude }
        );
    
}

void AddRouteDistancesInBaseRequests(TransportCatalogue& catalogue, const json::Dict& request_map) {

        const std::string stop_from_name =
            request_map.at("name").AsString();

        const StopPtr stop_from =
            catalogue.FindStop(stop_from_name);


        if (stop_from == nullptr) {
            throw std::runtime_error(
                "Stop not found: " + stop_from_name
            );
        }

        const auto distances_it =
            request_map.find("road_distances");

        const auto& distances =
            distances_it->second.AsMap();

        for (const auto& [stop_to_name, distance_node] : distances) {
            const StopPtr stop_to =
                catalogue.FindStop(stop_to_name);

            if (stop_from == stop_to) {
                throw std::runtime_error(
                    "Distance from stop to itself: " +
                    stop_from_name
                );
            }

            if (stop_to == nullptr) {
                throw std::runtime_error(
                    "Stop not found while reading road_distances: " +
                    stop_to_name
                );
            }

            const int distance = distance_node.AsInt();

            if (distance < 0) { // проверка на отрицательную дистанцию
                throw std::runtime_error(
                    "Negative distance from \"" +
                    stop_from_name + "\" to \"" +
                    stop_to_name + "\""
                );
            }

            catalogue.AddDistanceBetweenStops(
                stop_from,
                stop_to,
                distance
            );
        }
}

void AddBusesInBaseRequests(TransportCatalogue& catalogue, const json::Dict& request_map) {

        std::string bus_name =
            request_map.at("name").AsString();

        bool is_roundtrip =
            request_map.at("is_roundtrip").AsBool();

        const auto& stops_array =
            request_map.at("stops").AsArray();

        if (stops_array.empty()) { // проверка массива
            throw std::runtime_error(
                "Bus must contain at least one stop: " + bus_name
            );
        }

        vector<StopPtr> stop_pointers;
        stop_pointers.reserve(
            is_roundtrip
            ? stops_array.size()
            : stops_array.size() * 2 - 1
        );


        // Добавляем исходный порядок остановок.
        for (const auto& stop_node : stops_array) {
            const std::string stop_name =
                stop_node.AsString();

            if (stop_name.empty()) {
                throw std::runtime_error(
                    "Empty stop name in bus: " + bus_name
                );
            }

            const StopPtr stop =
                catalogue.FindStop(stop_name);

            if (stop == nullptr) {
                throw std::runtime_error(
                    "Could not find stop \"" +
                    stop_name + "\""
                );
            }

            stop_pointers.push_back(stop);
        }

        // проверка ввода автобусов
        if (bus_name.empty()) {
            throw std::runtime_error(
                "Bus name must not be empty"
            );
        }

        if (catalogue.FindBus(bus_name) != nullptr) {
            throw std::runtime_error(
                "Duplicate bus: " + bus_name
            );
        }

        catalogue.AddBus(
            std::move(bus_name),
            std::move(stop_pointers),
            is_roundtrip
        );
}

void JsonReader::ReadBaseRequests(
    const json::Document& doc,
    TransportCatalogue& catalogue
) {
    const auto& root = doc.GetRoot().AsMap();

    const auto base_requests_it = root.find("base_requests");

    if (base_requests_it == root.end()) {
        throw std::runtime_error(
            "JSON root does not contain key: base_requests"
        );
    }

    const auto& base_requests = base_requests_it->second.AsArray();

    for (const auto& request : base_requests) {
        const auto& request_map = request.AsMap();

        if (request_map.at("type").AsString() != "Stop") {
            continue;
        }

        const std::string stop_name =
            request_map.at("name").AsString();

        if (stop_name.empty()) { // проверка на пустоту
            throw std::runtime_error(
                "Stop name must not be empty"
            );
        }

        if (catalogue.FindStop(stop_name) != nullptr) { // проверка на дубликаты
            throw std::runtime_error(
                "Duplicate stop: " + stop_name
            );
        }

        AddStopsInBaseRequests(catalogue, request_map);
    }


    for (const auto& request : base_requests) {

        const auto& request_map = request.AsMap();

        if (request_map.at("type").AsString() != "Stop") {
            continue;
        }

        AddRouteDistancesInBaseRequests(catalogue, request_map);
    }
    
    for (const auto& request : base_requests) {
        const auto& request_map = request.AsMap();

        if (request_map.at("type").AsString() != "Bus") {
            continue;
        }

        AddBusesInBaseRequests(catalogue, request_map);
    }
}

void InsertBusStatInStatRequests(const TransportCatalogue& catalogue, const int id, const string& name, json::Array& responses) {
   auto route_stat = catalogue.GetRouteStat(name);
    
    if (route_stat) {
        // Используем Builder для построения JSON
        responses.push_back(
            json::Builder{}
                .StartDict()
                    .Key("request_id").Value(id)
                    .Key("stop_count").Value(static_cast<int>(route_stat->stops))
                    .Key("unique_stop_count").Value(static_cast<int>(route_stat->unique_stops))
                    .Key("route_length").Value(route_stat->real_length)
                    // Вычисляем кривизну прямо в Value
                    .Key("curvature").Value(route_stat->real_length * 1.0 / route_stat->geo_length)
                .EndDict()
                .Build()
        );
    } else {
        // Случай ошибки
        responses.push_back(
            json::Builder{}
                .StartDict()
                    .Key("request_id").Value(id)
                    .Key("error_message").Value("not found")
                .EndDict()
                .Build()
        );
    }
}

void InsertStopsInStatRequests(const TransportCatalogue& catalogue, const int id, const string& name, json::Array& responses) {
        const StopPtr stop = catalogue.FindStop(name);
    
    if (stop) {
        const auto& buses = catalogue.GetBusesByStop(name);
        
        // Создаем массив автобусов заранее, так как нам нужно отсортировать или просто собрать данные
        // Builder отлично работает и с промежуточными контейнерами, если логика сложная
        json::Array bus_array;
        if (buses) {
            for (const auto& bus_name : *buses) {
                bus_array.push_back(std::string(bus_name));
            }
        }

        // Теперь собираем итоговый JSON через Builder
        responses.push_back(
            json::Builder{}
                .StartDict()
                    .Key("request_id").Value(id)
                    .Key("buses").Value(bus_array) // Вставляем готовый массив
                .EndDict()
                .Build()
        );
    } else {
        responses.push_back(
            json::Builder{}
                .StartDict()
                    .Key("request_id").Value(id)
                    .Key("error_message").Value("not found")
                .EndDict()
                .Build()
        );
    }
}

void JsonReader::BuildMapRequest(const TransportCatalogue& cat, MapRenderer& map_, const int id, json::Array& responses) {
    svg::Document map = map_.BuildMapDocument(cat);
    std::ostringstream out;
    map.Render(out);
    std::string result = out.str();

    // 2. Формируем JSON ответ через Builder
    responses.push_back(
        json::Builder{}
            .StartDict()
                .Key("request_id").Value(id)
                .Key("map").Value(result) // Вставляем строку SVG как значение
            .EndDict()
            .Build()
    );
}

json::Node JsonReader::ReadStatRequests(const json::Document& doc, const TransportCatalogue& catalogue, MapRenderer& map_)
{
    json::Array responses;
    map_.SetRoute(catalogue);

    const auto& root = doc.GetRoot().AsMap();

    const auto& stat_requests = root.at("stat_requests").AsArray();

    for (const auto& request : stat_requests) {
        const auto& request_ = request.AsMap();
        const int id = request_.at("id").AsInt();
        const auto& request_type = request_.at("type").AsString();
        
        if (request_type == "Bus") {
            const auto& request_name = request_.at("name").AsString();
            InsertBusStatInStatRequests(catalogue, id, request_name, responses);
        }
        else if (request_type == "Stop") {
            const auto& request_name = request_.at("name").AsString();
            InsertStopsInStatRequests(catalogue, id, request_name, responses);

        } else if (request_type == "Map") {
            BuildMapRequest(catalogue, map_, id, responses);
        }

    }
    return responses;
}