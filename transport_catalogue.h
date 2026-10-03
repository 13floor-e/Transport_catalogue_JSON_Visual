#pragma once
#include "geo.h"
#include "domain.h"

#include <unordered_map>
#include <unordered_set>
#include <set>
#include <deque>
#include <string_view>
#include <vector>
#include <string>
#include <optional>
#include <map>

class TransportCatalogue {

public:
	struct PairHasher {

		template <typename First, typename Second>
		size_t operator()(const std::pair<First, Second>& obj) const {
			size_t first_hash = std::hash<First>()(obj.first);
			size_t second_hash = std::hash<Second>()(obj.second);
			// Комбинация хешей с использованием сдвига и XOR
			return first_hash ^ (second_hash << 1);
		}
	};



	void AddStop(const std::string& name, const geo::Coordinates& pos);
	void AddBus(const std::string& name, const std::vector<StopPtr>& stops, bool is_roundtrip);
	StopPtr FindStop(std::string_view bus_name) const;

	BusPtr FindBus(std::string_view bus_name) const;
	BusStat GetStat(BusPtr bus) const;
	std::optional<std::set<std::string_view>> GetBusesByStop(std::string_view stop) const;

	void AddDistanceBetweenStops(StopPtr stop1, StopPtr stop2, int distance);
	int GetDistanceBetweenStops(StopPtr stop1, StopPtr stop2) const;

	std::optional<Route> GetRouteStat(std::string_view name) const;

	const std::deque<Bus>& GetBuses() const;

	// метод возвращает список маршрутов
	const std::deque<Stop>& GetStops() const;

private:
	std::deque<Bus> bus_pool_;
	std::deque<Stop> stop_pool_;

	std::unordered_map<std::string_view, BusPtr> bus_by_name_;
	std::unordered_map<std::string_view, StopPtr> stop_by_name_;

	std::unordered_map<std::string, std::unordered_set<BusPtr>> bus_by_stop_;
	//Ук-ль на остановку и ук-ли на автобусы прошедшие чеоез неё

	std::unordered_map<std::pair<StopPtr, StopPtr>, int, PairHasher> distances_between_stops;

};