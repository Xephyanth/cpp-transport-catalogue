#include "request_handler.h"

#include <utility>
#include <sstream>
#include <unordered_set>

namespace transport {

using namespace std::literals;

// Конструктор класса
RequestHandler::RequestHandler(const Catalogue& db, const MapRenderer& renderer, const Router& router)
    : db_(db), renderer_(renderer), router_(router) {}

// Метод для формирование ответа
void RequestHandler::DatabaseRespond(const json::Node& doc, std::ostream& output) {
    json::Array output_array;
    
    const json::Array& arr = doc.AsArray();
    output_array.reserve(arr.size());
    
    // Цикл по массиву запросов
    for (auto& request_node : arr) {
        const json::Dict& request = request_node.AsDict();
        const std::string& type = request.at("type"s).AsString();
        
        // Вызов функции вывода информации об объекте
        // Если тип запроса - Маршрут
        if (type == "Bus"s) {
            output_array.push_back(BusRespond(request));
        }
        
        // Если тип запроса - Остановка
        if (type == "Stop"s) {
            output_array.push_back(StopRespond(request));
        }
        
        // Если тип запроса - Визуализация
        if (type == "Map"s) {
            output_array.push_back(MapImageRespond(request));
        }
        
        // Если тип запроса - Маршрут между двумя остановками
        if (type == "Route"s) {
            output_array.push_back(RouteRespond(request));
        }
    }
    
    // Вывод полученного результата
    json::Print(json::Document(json::Node(std::move(output_array))), output);
}

// Возвращает SVG-документ, представляющий карту маршрутов
svg::Document RequestHandler::RenderMap() const {
    return renderer_.GetSVG(db_.GetSortedAllBuses());
}

// Возвращает ответ на запрос с информацией автобусного маршрута
json::Node RequestHandler::BusRespond(const json::Dict& request) {
    // Получение идентификатора запроса
    int id = request.at("id"s).AsInt();
    // Получение номера маршрута
    const std::string& bus_numb = request.at("name"s).AsString();
    
    // Получение информации об автобусе
    if (const Bus* bus = db_.FindRoute(bus_numb)) {
        int stops_count = bus->stops.size();
        int distance = 0;
        double straight_distance = 0.0;
        
        for (int i = 1; i < stops_count; ++i) {
            distance += bus->stops[i - 1]->GetStopsDistance(bus->stops[i]);
            straight_distance += geo::ComputeDistance(bus->stops[i - 1]->coords, bus->stops[i]->coords);
        }
        
        double curvature = distance / straight_distance;
        std::unordered_set<std::string> unique_stops_set;
        
        for (transport::Stop* s : bus->stops) {
            unique_stops_set.emplace(s->stop_title);
        }
        
        int unique_stops = unique_stops_set.size();
        
        // Добавление информации к ответу
        return json::Builder{}.StartDict()
            .Key("route_length"s).Value(distance)
            .Key("unique_stop_count"s).Value(unique_stops)
            .Key("stop_count"s).Value(stops_count)
            .Key("curvature"s).Value(curvature)
            .Key("request_id"s).Value(id)
            .EndDict().Build();
    }
    else { // Если информация о автобусе не найдена
        return json::Builder{}.StartDict()
            .Key("error_message"s).Value("not found"s)
            .Key("request_id"s).Value(id)
            .EndDict().Build();
    }
}

// Возвращает ответ на запрос с информацией транспортной остановки
json::Node RequestHandler::StopRespond(const json::Dict& request) {
    // Получение идентификатора запроса
    int id = request.at("id"s).AsInt();
    // Получение названия остановки
    const std::string& title = request.at("name"s).AsString();
    
    if (const Stop* stop = db_.FindStop(title)) {
        // Массив для хранения номеров маршрутов
        json::Array buses_array;
        const auto& buses_on_stop = db_.GetRouteInfo(stop->stop_title);
        buses_array.reserve(buses_on_stop.size());
        
        // Заполнение массива номерами маршрутов
        for (auto& [_, bus] : buses_on_stop) {
            buses_array.push_back(bus->bus_number);
        }
        
        // Добавление информации к ответу
        return json::Builder{}.StartDict()
            .Key("buses"s).Value(std::move(buses_array))
            .Key("request_id"s).Value(id)
            .EndDict().Build();
    }
    else { // Если информация об остановке не найдена
        return json::Builder{}.StartDict()
            .Key("error_message"s).Value("not found"s)
            .Key("request_id"s).Value(id)
            .EndDict().Build();
    }
}

json::Node RequestHandler::MapImageRespond(const json::Dict& request) {
    // Получение идентификатора запроса
    int id = request.at("id"s).AsInt();
    
    // Вывод SVG-изображения карты в поток
    std::ostringstream strm;
    svg::Document map = RenderMap();
    map.Render(strm);
    
    return json::Builder{}.StartDict()
        .Key("request_id"s).Value(id)
        .Key("map"s).Value(strm.str())
        .EndDict().Build();
}

json::Node RequestHandler::RouteRespond(const json::Dict& request) {
    // Получение идентификатора запроса
    int id = request.at("id"s).AsInt();
    
    const std::string& name_from = request.at("from"s).AsString();
    const std::string& name_to = request.at("to"s).AsString();
    
    // Цикл по каждому элементу маршрута
    if (const Stop* stop_from = db_.FindStop(name_from)) {
        if (const Stop* stop_to = db_.FindStop(name_to)) {
            if (auto ri = router_.GetRouteInfo(stop_from, stop_to)) {
                auto [wieght, edges] = ri.value();
                
                return json::Builder{}.StartDict()
                    .Key("items"s).Value(std::move(router_.GetEdgesItems(edges)))
                    .Key("total_time"s).Value(wieght)
                    .Key("request_id"s).Value(id)
                    .EndDict().Build();
            }
        }
    }
    
    return json::Builder{}.StartDict()
        .Key("error_message"s).Value("not found"s)
        .Key("request_id"s).Value(id)
        .EndDict().Build();
}

} // end of namespace transport