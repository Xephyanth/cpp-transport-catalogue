#pragma once

#include "json.h"
#include "transport_catalogue.h"

namespace transport {

class JsonReader {
public:
    JsonReader(json::Document data) : data_(data) {}

    // Возвращает узел "base_requests" из JSON-документа
    const json::Node& GetBaseRequest() const;
    // Возвращает узел "stat_requests"
    const json::Node& GetStatRequest() const;
    // Возвращает узел "render_settings"
    const json::Node& GetRenderSettings() const;
    // Возвращает узел "routing_settings"
    const json::Node& GetRoutingSettings() const;
    // Возвращает узел "serialization_settings"
    const json::Node& GetSerializationSettings() const;

    // Заполнение базы данных из JSON-документа
    void ImportData(Catalogue& db) const;

private:
    // JSON-документ, содержащий все входные данные
    json::Document data_;

    // Вспомогательные типы данных
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