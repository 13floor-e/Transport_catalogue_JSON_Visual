#include "transport_catalogue.h"
#include "domain.h"
#include <optional>
#include <unordered_set>
#include <iostream>
#include "request_handler.h"

const std::deque<Bus>& TransportCatalogue::GetBuses() const {
    return bus_pool_;
}

// метод возвращает список маршрутов
const std::deque<Stop>& TransportCatalogue::GetStops() const {
    return stop_pool_;
}

void TransportCatalogue::AddBus(const std::string& name, const std::vector<StopPtr>& stops, bool is_roundtrip) {
    if (bus_by_name_.find(name) != bus_by_name_.end()) {
        throw std::invalid_argument("Bus already exists: " + name);
    }

    if (name.empty()) {
        throw std::invalid_argument(
            "Bus name must not be empty"
        );
    }

    if (stops.empty()) {
        throw std::invalid_argument(
            "Bus must contain at least one stop"
        );
    }

    bus_pool_.emplace_back(Bus{ name, stops, is_roundtrip });

    BusPtr added_ptr = &bus_pool_.back();

    for (StopPtr stop : added_ptr->stops) {
        if (stop != nullptr) {
            bus_by_stop_[stop->name].insert(added_ptr);
        }
    }

    bus_by_name_.emplace(
        added_ptr->name,
        added_ptr
    );
}

void TransportCatalogue::AddStop(const std::string& name, const geo::Coordinates& pos) {
    if (stop_by_name_.find(name) != stop_by_name_.end()) {
        throw std::invalid_argument("Stop already exists: " + name);
    }

    if (name.empty()) {
        throw std::invalid_argument(
            "Stop name must not be empty"
        );
    }

    stop_pool_.emplace_back(Stop{ name, pos });

    StopPtr added_ptr = &stop_pool_.back();

    stop_by_name_.emplace(
        added_ptr->name,
        added_ptr
    );
}

StopPtr TransportCatalogue::FindStop(std::string_view bus_name) const {

    auto iter = stop_by_name_.find(bus_name);

    if (iter == stop_by_name_.end()) {
        return nullptr;
    }
    else {
        return iter->second; // first - ключ
    }
}


int TransportCatalogue::GetDistanceBetweenStops(StopPtr stop1, StopPtr stop2) const
{
    bool false_stop_1 = stop1 == nullptr;
    bool false_stop_2 = stop2 == nullptr;

    if (!false_stop_1 && !false_stop_2) {

        if (auto it = distances_between_stops.find({ stop1, stop2 }); it != distances_between_stops.end()) {
            return it->second;
        }
        if (auto it = distances_between_stops.find({ stop2, stop1 }); it != distances_between_stops.end()) {
            return it->second;
        }
    }

    return -1;
}
std::optional<Route> TransportCatalogue::GetRouteStat(std::string_view name) const {

    //    auto it = bus_by_name_.find(name);
    //    if (it == bus_by_name_.end())
    //        return std::nullopt;
    //
    //    size_t stops_count = it->second->stops.size();
    //    size_t unique_stops = std::unordered_set<StopPtr>(it->second->stops.begin(), it->second->stops.end()).size();
    //    double geograph_length = 0;
    //    int real_length = 0;
    //    for (size_t i(0); i < it->second->stops.size() - 1; ++i) {
    //        geograph_length += geo::ComputeDistance({ it->second->stops[i]->position.lat, it->second->stops[i]->position.lng },
    //            { it->second->stops[i + 1]->position.lat, it->second->stops[i + 1]->position.lng });
    //        real_length += GetDistanceBetweenStops(it->second->stops[i], it->second->stops[i + 1]);
    //    }
    //    return Route{ stops_count, unique_stops, real_length, geograph_length };
    //
    //}
    auto it = bus_by_name_.find(name);

     if (it == bus_by_name_.end()) {
         return std::nullopt;
     }

     const Bus& bus = *it->second;

     if (bus.stops.empty()) {
         return Route{ 0, 0, 0, 0.0 };
     }

     const std::size_t unique_stops =
         std::unordered_set<StopPtr>(
             bus.stops.begin(),
             bus.stops.end()
         ).size();

     std::size_t stops_count = bus.stops.size();
     double geograph_length = 0.0;
     int real_length = 0;

     auto add_segment = [&](StopPtr from, StopPtr to) {
         geograph_length += geo::ComputeDistance(
             from->position,
             to->position
         );

         real_length += GetDistanceBetweenStops(from, to);
         };

     // Прямой путь
     for (std::size_t i = 0; i + 1 < bus.stops.size(); ++i) {
         add_segment(bus.stops[i], bus.stops[i + 1]);
     }

     // Обратный путь некольцевого маршрута
     if (!bus.is_roundtrip) {
         stops_count = bus.stops.size() * 2 - 1;

         for (std::size_t i = bus.stops.size() - 1; i > 0; --i) {
             add_segment(bus.stops[i], bus.stops[i - 1]);
         }
     }

     return Route{
         stops_count,
         unique_stops,
         real_length,
         geograph_length
     };
}

BusStat TransportCatalogue::GetStat(BusPtr bus) const {
BusStat stat{};
std::unordered_set<StopPtr> seen_stops;  // для уникальных остановок
StopPtr prev_stop = nullptr;

for (auto stop : bus->stops) {
    ++stat.total_stops;

    // Считаем уникальные остановки
    if (seen_stops.count(stop) == 0) {
        ++stat.unique_stops;
        seen_stops.insert(stop);
    }

    // Вычисляем длину маршрута
    if (prev_stop) {
        stat.geo_length += ComputeDistance(prev_stop->position, stop->position);
        stat.route_length += GetDistanceBetweenStops(prev_stop, stop);
    }
    prev_stop = stop;
}
stat.curvature = stat.route_length / stat.geo_length;
return stat;
}

std::optional<std::set<std::string_view>> TransportCatalogue::GetBusesByStop(std::string_view stop) const {
    std::set<std::string_view> result;

for (auto& bus : bus_pool_) {
    auto it = std::find_if(bus.stops.begin(), bus.stops.end(), [stop](StopPtr item) { // поиск остановки для каждого автобуса
        return item->name == stop;
        });
    if (it != bus.stops.end()) // Если автобусы есть
        result.insert(bus.name);
}

return result.size() ? std::optional<std::set<std::string_view>>(std::move(result)) : std::nullopt;
}

void TransportCatalogue::AddDistanceBetweenStops(StopPtr stop1, StopPtr stop2, int distance)
{
    if (!stop1 || !stop2) return;

    if(stop1 == nullptr || stop2 == nullptr) {
        throw std::invalid_argument(
            "Distance endpoints must not be null"
        );
    }

    if (distance < 0) {
        throw std::invalid_argument(
            "Distance must not be negative"
        );
    }

    std::string_view stop_name1 = stop1->name;
    std::string_view stop_name2 = stop2->name;

    if (FindStop(stop_name1) && FindStop(stop_name2)) {
        distances_between_stops[{stop1, stop2}] = distance;
    }
}


BusPtr TransportCatalogue::FindBus(std::string_view bus_name) const {
    auto iter = bus_by_name_.find(bus_name);

    if (iter == bus_by_name_.end()) {
        return nullptr;
    }
    else {
        return iter->second; // first - ключ
    }
}
