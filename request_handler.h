#pragma once
#include <optional>
#include <unordered_set>

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "svg.h"

/*
 * Здесь можно было бы разместить код обработчика запросов к базе, содержащего логику, которую не
 * хотелось бы помещать ни в transport_catalogue, ни в json reader.
 *
 * В качестве источника для идей предлагаем взглянуть на нашу версию обработчика запросов.
 * Вы можете реализовать обработку запросов способом, который удобнее вам.
 *
 * Если вы затрудняетесь выбрать, что можно было бы поместить в этот файл,
 * можете оставить его пустым.
 */

 // Класс RequestHandler играет роль Фасада, упрощающего взаимодействие JSON reader-а
 // с другими подсистемами приложения.
 // См. паттерн проектирования Фасад: https://ru.wikipedia.org/wiki/Фасад_(шаблон_проектирования)

class RequestHandler : public TransportCatalogue{
public:
    // MapRenderer понадобится в следующей части итогового проекта
    RequestHandler(TransportCatalogue& db,MapRenderer& renderer, 
    std::unordered_map<std::string, std::unordered_set<BusPtr>> bus_by_stop,
    std::unordered_map<std::string, BusPtr> bus_by_name,
    std::unordered_map<std::string, StopPtr> stop_by_name)
        : db_(db), renderer_(renderer), bus_by_stop_(bus_by_stop)
        , bus_by_name_(bus_by_name) , stop_by_name_(stop_by_name) {}

    //RequestHandler(TransportCatalogue& db, MapRenderer& render)
    //   : db_(db) renderer_(render){}
    
    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusStat> GetBusStat(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через
    const std::unordered_set<BusPtr>& GetBusesByStop(const std::string_view& stop_name) const;

    // Этот метод будет нужен в следующей части итогового проекта
    //svg::Document RenderMap() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    TransportCatalogue& db_;
    MapRenderer& renderer_;
    std::unordered_map<std::string, std::unordered_set<BusPtr>> bus_by_stop_;
    std::unordered_map<std::string, BusPtr> bus_by_name_;
    std::unordered_map<std::string, StopPtr> stop_by_name_;
    
};
//#pragma once
//#include <optional>
//#include <unordered_map>
//#include <unordered_set>
//#include <string_view>
//
//#include "transport_catalogue.h"
//#include "svg.h"
//
//class RequestHandler {
//public:
//    std::optional<TransportCatalogue::BusStat> GetBusStat(std::string_view bus_name) const;
//    const std::unordered_set<TransportCatalogue::BusPtr>& GetBusesByStop(std::string_view stop_name) const;
//    svg::Document RenderMap() const;
//
//private:
//    const TransportCatalogue& db_;
//    std::unordered_map<std::string_view, std::unordered_set<TransportCatalogue::BusPtr>> bus_by_stop_;
//};