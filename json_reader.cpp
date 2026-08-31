#include "json_reader.h"
#include "request_handler.h"
#include "json.h"
#include <iostream>
#include "domain.h"
#include <algorithm>
#include "geo.h"
#include <cassert>
using namespace std;


//
//void Jsonreader::JsonReaderStdin(const json::Document& doc, TransportCatalogue& catalogue)
//{
//    const auto& root = doc.GetRoot();
//
//    const auto& root_map = root.AsMap();
//
//    auto it = root_map.find("base_requests");
//
//
//    if (it == root_map.end()) {
//        throw std::runtime_error("JSON root does not contain key: base_requests");
//    }
//
//    const auto& base_requests = it->second.AsArray();
//
//    for (const auto& request : base_requests) {
//        const auto& request_node = request.AsMap();
//
//        if (request_node.at("type").AsString() == "Stop") {
//            string stop_name = request_node.at("name").AsString();
//            double latitude = request_node.at("latitude").AsDouble();
//            double longitude = request_node.at("longitude").AsDouble();
//            catalogue.AddStop(stop_name, { latitude, longitude });
//        }
//    }
//
//    for (const auto& request : base_requests) {
//        const auto& request_node = request.AsMap();
//
//        if (request_node.at("type").AsString() != "Stop") {
//            continue;
//        }
//
//        string stop_from = request_node.at("name").AsString();
//        const StopPtr stop_from_ptr = catalogue.FindStop(stop_from);
//        if (!stop_from_ptr) {
//            throw runtime_error("stop_from not found: " + stop_from);
//        }
//
//        // Если в JSON нет road_distances — просто пропускаем
//        if (request_node.count("road_distances") == 0) {
//            continue;
//        }
//
//        const auto& distances_node = request_node.at("road_distances").AsMap();
//        for (const auto& [stop_to_name_node, distance_node] : distances_node) {
//
//            int distance = distance_node.AsInt(); // int / AsInt()
//
//            const StopPtr stop_to_ptr = catalogue.FindStop(stop_to_name_node);
//            if (!stop_to_ptr) {
//
//                throw runtime_error("stop_to not found while reading road_distances: " + stop_to_name_node);
//
//            }
//
//            catalogue.AddDistanceBetweenStops(stop_from_ptr, stop_to_ptr, distance);
//        }
//    }
//
//    for (const auto& request : base_requests) {
//        const auto& request_node = request.AsMap();
//
//        if (request_node.at("type").AsString() == "Bus") {
//
//            // 1. Читаем базовые данные
//            std::string bus_name = request_node.at("name").AsString();
//            bool is_roundtrip = request_node.at("is_roundtrip").AsBool();
//
//            const auto& stops_array = request_node.at("stops").AsArray();
//            std::vector<std::string> stop_names;
//            //stop_names.reserve(!is_roundtrip ? stops_array.size()*2 : stops_array.size());
//
//            for (const auto& stop_name_node : stops_array) {
//                StopPtr stop = catalogue.FindStop(stop_name_node.AsString());
//                if (!stop) {
//                    throw runtime_error("Could not find stop \"" + stop_name_node.AsString() + "\"");
//                }
//                stop_names.push_back(stop_name_node.AsString());
//            }
//
//            std::vector<StopPtr> stop_pointers;
//            stop_pointers.reserve(!is_roundtrip ? stops_array.size() * 2 : stops_array.size());
//
//            for (const auto& name : stop_names) {
//                StopPtr stop = catalogue.FindStop(name);
//                if (!stop) {
//                    throw runtime_error("Could not find stop \"" + name + "\"");
//                }
//                stop_pointers.push_back(std::move(stop));
//            }
//
//            if (!is_roundtrip) {
//                // Берем текущий размер (например, 3: A, B, C)
//                try {
//                    stop_pointers.insert(stop_pointers.end(), next(stop_pointers.rbegin()), stop_pointers.rend());
//                }
//                catch (out_of_range& e) {
//                    cerr << e.what() << endl;
//                }
//            }
//
//            catalogue.AddBus(std::move(bus_name), std::move(stop_pointers), is_roundtrip);
//        }
//    }
//}


void Jsonreader::JsonReaderStdin(
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

    // Этап 1. Добавляем все остановки.
    
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

    // Этап 2. Добавляем дорожные расстояния.
    for (const auto& request : base_requests) {
        const auto& request_map = request.AsMap();

        if (request_map.at("type").AsString() != "Stop") {
            continue;
        }

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

        if (distances_it == request_map.end()) {
            continue;
        }

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

    // Этап 3. Добавляем автобусы.
    for (const auto& request : base_requests) {
        const auto& request_map = request.AsMap();

        if (request_map.at("type").AsString() != "Bus") {
            continue;
        }

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

        std::vector<StopPtr> stop_pointers;
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

        // Для некольцевого маршрута добавляем обратный путь,

        //if (!is_roundtrip && stop_pointers.size() > 1) {
        //    for (
        //        int index =
        //        static_cast<int>(stop_pointers.size()) - 2;
        //        index >= 0;
        //        --index
        //        ) {
        //        stop_pointers.push_back(stop_pointers[index]);
        //    }
        //}
        
        //if (!is_roundtrip) {
        //        // Берем текущий размер (например, 3: A, B, C)
        //        try {
        //            stop_pointers.insert(stop_pointers.end(), next(stop_pointers.rbegin()), stop_pointers.rend());
        //        }
        //        catch (out_of_range& e) {
        //            cerr << e.what() << endl;
        //        }
        //    }


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
}


json::Node Jsonreader::Processes(const json::Document& doc, const TransportCatalogue& catalogue)
{
    json::Array responses;
    const auto& root = doc.GetRoot().AsMap();

    const auto& stat_requests = root.at("stat_requests").AsArray();

    for (const auto& request : stat_requests) {
        const auto& request_ = request.AsMap();
        const int id = request_.at("id").AsInt();
        const auto& request_type = request_.at("type").AsString();
        const auto& request_name = request_.at("name").AsString();

        if (request_type == "Bus") {
            auto route_stat = catalogue.GetRouteStat(request_name);
            if (route_stat) {
                json::Dict response;
                response.insert({ "curvature"s, route_stat->real_length * 1.0 / route_stat->geo_length });
                response.insert({ "request_id"s, id });
                response.insert({ "route_length"s, route_stat->real_length });
                response.insert({ "stop_count"s, static_cast<int>(route_stat->stops) });
                response.insert({ "unique_stop_count"s, static_cast<int>(route_stat->unique_stops) });
                responses.push_back(response);
            }
            else {
                json::Dict response;
                response.insert({ "request_id"s, id });
                response.insert({ "error_message"s, "not found"s });
                responses.push_back(response);
            }

        }
        else if (request_type == "Stop") {
            const StopPtr stop = catalogue.FindStop(request_name);
            if (stop) {
                json::Dict response;
                json::Array bus_array;
                const auto& buses = catalogue.GetBusesByStop(request_name);
                if (buses) {
                    for (const auto& bus_name : *buses) {
                        bus_array.push_back(string(bus_name));
                    }
                }

                // response.insert({})
                response.insert({ "buses"s, bus_array });
                response.insert({ "request_id"s, id });
                responses.push_back(response);
            }
            else {
                json::Dict response;
                response.insert({ "request_id"s, id });
                response.insert({ "error_message"s, "not found"s });
                responses.push_back(response);
            }

        }

    }
    return responses;
}