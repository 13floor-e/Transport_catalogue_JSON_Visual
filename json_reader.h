#pragma once
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "json.h"


class MapRenderer;

class JsonReader {
public:
	json::Node ReadStatRequests(const json::Document& doc, const TransportCatalogue& catalogue, MapRenderer& map_);
	void ReadBaseRequests(const json::Document& doc, TransportCatalogue& catalogue);
	void BuildMapRequest(const TransportCatalogue& cat, MapRenderer& map_, const int id, json::Array& responses);
	friend MapRenderer;
};


