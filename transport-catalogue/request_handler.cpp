#include "request_handler.h"

namespace transport {

// Конструктор класса
RequestHandler::RequestHandler(const Catalogue& db, const MapRenderer& renderer, const TransportRouter& router)
    : db_(db), renderer_(renderer), router_(router) {}

// Возвращает информацию автобусного маршрута, либо std::optional, если маршрут не существует
std::optional<BusRoute> RequestHandler::GetBusStat(const std::string_view& bus_name) const {
    return db_.GetRouteInfo(bus_name);
}

// Возвращает указатель на список автобусов, проходящих через остановку
const std::unordered_set<const Bus*>* RequestHandler::GetBusesByStop(const std::string_view& stop_name) const {
    return db_.GetStopInfo(stop_name);
}

// Возвращает SVG-документ, представляющий карту маршрутов
svg::Document RequestHandler::RenderMap() const {
    return renderer_.GetSVG(db_.GetAllRoutes());
}

// Возвращает оптимальный маршрут между двумя остановками
std::optional<std::vector<const RouteChain*>> RequestHandler::GetOptimalRoute(std::string_view current, std::string_view next) const {
    return router_.FindOptimalRoute(current, next);
}

} // end of namespace transport