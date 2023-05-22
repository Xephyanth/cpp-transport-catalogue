#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <unordered_map>
#include <vector>
#include <memory>

namespace transport {

// Структура, описывающая настройки маршрута
struct RouteSettings {
    // Время ожидания автобуса на остановке
    int bus_wait_time;
    // Скорость движения автобуса
    double bus_velocity;
};

// Структура, хранящая время пути между двумя остановками
struct RouteTime {
    RouteTime() = default;
    
    RouteTime(int stops_number, double waiting_time, double travel_time) 
        : stops_number(stops_number), waiting_time(waiting_time), travel_time(travel_time) {}
    
    // Количество промежуточных остановок
    int stops_number;
    // Время ожидания
    double waiting_time;
    // Время пути
    double travel_time;
};

// Перегрузка операторов
RouteTime operator+(const RouteTime& lhs, const RouteTime& rhs);

bool operator<(const RouteTime& lhs, const RouteTime& rhs);
bool operator>(const RouteTime& lhs, const RouteTime& rhs);

// Структура, описывающая цепочку маршрута от одной остановки до другой
struct RouteChain {
    const Stop* from;
    const Stop* to;
    const Bus* route;
    RouteTime route_time;
};

// Класс для нахождения кратчайшего пути между остановками
class RouteShortest {
public:
    RouteShortest(const Catalogue* catalogue, const Bus* route);
    
    // Расстояние между двумя вершинами графа/остановками
    size_t DistanceBetween(size_t index_from, size_t index_to);
    
private:
    // Расстояние до каждой остановки в прямом направлении
    std::vector<size_t> forward_distance_;
    // Расстояние до каждой остановки в обратном направлении
    std::vector<size_t> backward_distance_;
};

class TransportRouter {
public:
    TransportRouter() = default;
    
    void RouterBuilder(RouteSettings settings, const Catalogue* db);
    
    // Поиск оптимального маршрута
    std::optional<std::vector<const RouteChain*>> FindOptimalRoute(std::string_view current, std::string_view next) const;
    
private:
    // Настройки маршрута
    RouteSettings settings_;
    
    const Catalogue* db_;
    
    // Граф маршрутов
    std::unique_ptr<graph::DirectedWeightedGraph<RouteTime>> graph_;
    // Соответствие остановок и вершин графа
    std::unordered_map<const Stop*, graph::VertexId> graph_vertexes_;
    // Ребра графа/цепочки маршрутов
    std::vector<RouteChain> graph_edges_;
    
    std::unique_ptr<graph::Router<RouteTime>> router_;
};

} // end of namespace transport