#include "request_handler.h"
#include "transport_catalogue.h"
#include "domain.h"
const std::unordered_set<BusPtr>& RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {

    static const std::unordered_set<BusPtr> empty_set;
    auto it = bus_by_stop_.find(std::string(stop_name));
    if (it != bus_by_stop_.end()) {
        return it->second;
    }
    return empty_set;
}

//svg::Document RequestHandler::RenderMap() const
//{
//    svg::Document res;
//    //const std::deque<Bus> buses = db_.GetBuses();
//    return renderer_.RenderMap(res);
//}

std::optional<BusStat> RequestHandler::GetBusStat(const std::string_view& bus_name) const
{
    
    auto it = bus_by_name_.find(std::string(bus_name));
    if (it == bus_by_name_.end()) {
        return std::nullopt;
    }

    BusPtr bus = it->second;
    if (!bus) {
        return std::nullopt;
    }

    BusStat stat{};
    stat.route_length = 0;

    std::unordered_set<StopPtr> seen;

    StopPtr prev = nullptr;

    int unique_stop_count = 0, stop_count = 0;

    for (auto stop : bus->stops) {
        ++stop_count;

        if (seen.insert(stop).second) {
            ++unique_stop_count;
        }

        if (prev) {
            // Длина по дорогам
            stat.route_length += GetDistanceBetweenStops(prev, stop);

            // Географическая длина
            stat.geo_length += geo::ComputeDistance(prev->position, stop->position);
        }
        prev = stop;
    }

    if (stat.geo_length > 0.0) {
        stat.curvature = static_cast<double>(stat.route_length) / stat.geo_length;
    }

    return stat;
}