#pragma once
#include <string>
#include <vector>
#include "json.h"
#include "geo.h"

struct Stop {
    std::string name;
    geo::Coordinates position;
};
using StopPtr = const Stop*;
using StopMap = std::map<std::string, const Stop*>;


struct Bus {
	Bus(std::string n, std::vector<StopPtr> s, bool r)
		: name(std::move(n)), stops(std::move(s)), is_roundtrip(r) {
	}
	std::string name;
	std::vector<StopPtr> stops;
	bool is_roundtrip;
};

using BusPtr = const Bus*;

struct BusStat {
	size_t total_stops = 0;
	size_t unique_stops = 0;
	int route_length = 0;
	double geo_length = 0.;
	double curvature = 0.;
};

struct Route {
    size_t stops;
    size_t unique_stops;
    int real_length;
    double geo_length;
};


/*
 * В этом файле вы можете разместить классы/структуры, которые являются частью предметной области (domain)
 * вашего приложения и не зависят от транспортного справочника. Например, Автобусные маршруты и Остановки.
 *
 * Их можно было бы разместить и в transport_catalogue.h, однако вынесение их в отдельный
 * заголовочный файл может оказаться полезным, когда дело дойдёт до визуализации карты маршрутов:
 * визуализатор карты (map_renderer) можно будет сделать независящим от транспортного справочника.
 *
 * Если структура вашего приложения не позволяет так сделать, просто оставьте этот файл пустым.
 *
 */