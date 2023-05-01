#pragma once

#include "map_renderer.h"
#include "transport_catalogue.h"

namespace transport {

class RequestHandler {
public:
    // Конструктор класса
    RequestHandler(const Catalogue& db, const MapRenderer& renderer);

    // Метод для получения статистики по автобусному маршруту
    std::optional<BusRoute> GetBusStat(const std::string_view& bus_name) const;
    
    // Метод для получения множества указателей на объекты Bus, которые останавливаются на остановке
    const std::unordered_set<const Bus*>* GetBusesByStop(const std::string_view& stop_name) const;
    
    // Метод для создания SVG-документа с картой маршрутов автобусов
    svg::Document RenderMap() const;

private:
    // Ссылки на объекты
    const Catalogue& db_;
    const MapRenderer& renderer_;
};

} // end of namespace transport