#include "domain.h"

//Stop CreateStopFromJson(const json::Node& node)
//{
//    const auto& dict = node.AsMap();
//
//    if (dict.at("type").AsString() != "Stop") { // проверяем тип Stop
//        throw std::runtime_error("Invalid stop type");
//    }
//
//    std::string name = dict.at("name").AsString();
//    double lat = dict.at("latitude").AsDouble();
//    double lng = dict.at("longitude").AsDouble();
//
//    return Stop(std::move(name), geo::Coordinates{ lat, lng });
//}
//
//Bus CreateBusFromJson(const json::Node& node, const StopMap& stops)
//{
//    const auto& dict = node.AsMap();
//    if (dict.at("type").AsString() != "Bus") {
//        throw std::runtime_error("Invalid bus type");
//    }
//
//    std::string name = dict.at("name").AsString();
//    const json::Array& stops_array = dict.at("stops").AsArray();
//
//    bool is_roundtrip = dict.at("is_roundtrip").AsBool(); // проверяем кольцевой ли маршрут
//
//    std::vector<StopPtr> stop_pointers; // вектор указателей на остановки
//
//    for (const auto& stop_name : stops_array) {
//        auto stop_it = stops.find(stop_name.AsString());
//        if (stop_it == stops.end()) {
//            throw std::runtime_error("Stop not found: " + stop_name.AsString());
//        }
//        stop_pointers.push_back(stop_it->second);
//    }
//
//    StopPtr start_stop = stop_pointers[0];
//    StopPtr end_stop = is_roundtrip ? stop_pointers[0] : stop_pointers.back();
//
//    return Bus()
//}
