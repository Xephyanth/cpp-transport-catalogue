#pragma once

#include "json.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "transport_router.h"

namespace transport {
 
class JsonReader {
public:
    explicit JsonReader(std::istream& input, Catalogue* db, MapRenderer* renderer, TransportRouter* router);

    // Формирование ответа от базы данных транспортного каталога
    void DatabaseRespond(std::ostream& output);
    
    // Формирование информации о маршруте
    json::Dict BusRespond(RequestHandler& handler, const json::Node& request);
    // Формирование информации об остановке
    json::Dict StopRespond(RequestHandler& handler, const json::Node& request);
    // Формирование информации о визуализации
    json::Dict MapImageRespond(RequestHandler& handler, const json::Node& request);
    // Формирование информации о быстром/оптимальном пути
    json::Dict RouteRespond(RequestHandler& handler, const json::Node& request);
    
    // Получение данных визуализации
    RenderSettings RendererDataProcessing(const json::Dict& settings);
    // Получение цветовой палитры
    svg::Color ColorProcessing(const json::Node& color);
    
private:
    json::Document input_;
    Catalogue* db_;
    MapRenderer* renderer_;
    TransportRouter* router_;
};

} // end of namespace transport