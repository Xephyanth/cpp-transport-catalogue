#pragma once

#include "map_renderer.h"
#include "transport_router.h"

namespace transport {

class RequestHandler {
public:
    // Конструктор класса
    RequestHandler(const Catalogue& db, const MapRenderer& renderer, const TransportRouter& router);

    // Метод для получения статистики по автобусному маршруту
    std::optional<BusRoute> GetBusStat(const std::string_view& bus_name) const;
    
    // Метод для получения множества указателей на объекты Bus, которые останавливаются на остановке
    const std::unordered_set<const Bus*>* GetBusesByStop(const std::string_view& stop_name) const;
    
    // Метод для создания SVG-документа с картой маршрутов автобусов
    svg::Document RenderMap() const;
    
    // Метод для получения оптимального маршрута между двумя остановками
    std::optional<std::vector<const RouteChain*>> GetOptimalRoute(std::string_view current, std::string_view next) const;

private:
    // Ссылки на объекты
    const Catalogue& db_;
    const MapRenderer& renderer_;
    const TransportRouter& router_;
};

} // end of namespace transport