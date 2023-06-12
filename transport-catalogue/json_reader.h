#pragma once

#include "json.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "transport_catalogue.h"

namespace transport {

class JsonReader {
public:
    JsonReader(json::Document data) : data_(data) {}
    
    // Возвращает узел "base_requests" из JSON-документа
    const json::Node& GetBaseRequestData() const;
    // Возвращает узел "stat_requests"
    const json::Node& GetStatRequestData() const;
    // Возвращает узел "render_settings"
    const json::Node& GetRenderSettingsData() const;
    // Возвращает узел "routing_settings"
    const json::Node& GetRoutingSettingsData() const;
    // Возвращает узел "serialization_settings"
    const json::Node& GetSerializationSettingsData() const;

    // Заполнение базы данных из JSON-документа
    void ImportData(Catalogue& db) const;

    // Метод устанавливает настройки визуализации
    MapRenderer SetRenderSettings(const json::Node& render_settings);
    // Метод устанавливает настройки маршрутизатора
    RouteSettings SetRouterSettings(const json::Node& router_settings);
    
private:
    // JSON-документ, содержащий все входные данные
    json::Document data_;
    
    // Объявление синонимов
    using StopsDist = std::unordered_map<std::string_view, std::unordered_map<std::string_view, int>>;
    using BusesInfo = std::unordered_map<std::string_view, BusRoute>;

    // Метод обработки всех остановок из запроса и добавления в базу данных
    void ParseStop(Catalogue& db, const json::Dict& request, StopsDist& stops_distance) const;
    // Метод устанавливает расстояния между остановками
    void SetStopsDistances(Catalogue& db, const StopsDist& stops_distance) const;
    // Метод обработки всех маршрутов из запроса
    void ParseBus(const json::Dict& request, BusesInfo& buses) const;
    // Метод добавляет маршруты автобусов в базу данных
    void BusesAdd(Catalogue& db, const BusesInfo& buses) const;
    // Метод устанавливает конечные остановки для каждого маршрута
    void SetFinalStop(Catalogue& db, const BusesInfo& buses) const;
};
    
} // end of namespace transport