#pragma once

#include "map_renderer.h"
#include "transport_catalogue.h"
#include "transport_router.h"

#include <utility>

namespace transport {

class RequestHandler {
public:
    // Конструктор класса
    RequestHandler(const Catalogue& db, const MapRenderer& renderer, const Router& router);
    
    // Формирование ответа от базы данных транспортного каталога
    void DatabaseRespond(const json::Node& doc, std::ostream& output);
    
    // Метод для создания SVG-документа с картой маршрутов автобусов
    svg::Document RenderMap() const;

private:
    // Формирование информации о маршруте
    json::Node BusRespond(const json::Dict& request);
    // Формирование информации об остановке
    json::Node StopRespond(const json::Dict& request);
    // Формирование информации о визуализации
    json::Node MapImageRespond(const json::Dict& request);
    // Формирование информации о быстром/оптимальном пути
    json::Node RouteRespond(const json::Dict& request);
    
    // Ссылки на объекты
    const Catalogue& db_;
    const MapRenderer& renderer_;
    const Router& router_;
};

} // end of namespace transport