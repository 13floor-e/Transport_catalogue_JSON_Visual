#pragma once
#include "map_renderer.h"
#include "request_handler.h"
#include "transport_catalogue.h"
#include "json.h"
/*
 * Здесь можно разместить код наполнения транспортного справочника данными из JSON,
 * а также код обработки запросов к базе и формирование массива ответов в формате JSON
 */
class Jsonreader {
public:
	json::Node Processes(const json::Document& doc, const TransportCatalogue& catalogue);
	void JsonReaderStdin(const json::Document& doc, TransportCatalogue& catalogue);
	//void PrintSettings(const json::Document& doc, MapRenderer& render);
	//json::Node SettingsResponcess();
};


