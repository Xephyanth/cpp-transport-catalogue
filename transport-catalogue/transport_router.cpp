#include "transport_router.h"

namespace transport {

// Конструктор класса
RouteShortest::RouteShortest(const Catalogue* db, const Bus* route)
    : forward_distance_(route->stops.size()), backward_distance_(route->stops.size()) 
{
    size_t forward_accum = 0;
    size_t backward_accum = 0;
    
    // Вычисление расстояния прямого и обратного пути между остановками
    forward_distance_[0] = 0;
    backward_distance_[0] = 0;
    
    for (size_t i = 0; i < route->stops.size() - 1; ++i) {
        forward_accum += db->GetStopsDistance(route->stops[i], route->stops[i + 1]);
        
        forward_distance_[i + 1] = forward_accum;
        
        if (!route->is_circular) {
            backward_accum += db->GetStopsDistance(route->stops[i + 1], route->stops[i]);
            
            backward_distance_[i + 1] = backward_accum;
        }
    }
}

// Вычисление расстояния между двумя остановками на маршруте
size_t RouteShortest::DistanceBetween(size_t index_from, size_t index_to) {
    if (index_from < index_to) {
        return forward_distance_[index_to] - forward_distance_[index_from];
    }
    else {
        return backward_distance_[index_from] - backward_distance_[index_to];
    }
}

// Перегрузка операторов 
RouteTime operator+(const RouteTime& lhs, const RouteTime& rhs) {
    return { lhs.stops_number + rhs.stops_number, lhs.waiting_time + rhs.waiting_time, lhs.travel_time + rhs.travel_time };
}

bool operator<(const RouteTime& lhs, const RouteTime& rhs) {
    return (lhs.waiting_time + lhs.travel_time < rhs.waiting_time + rhs.travel_time);
}

bool operator>(const RouteTime& lhs, const RouteTime& rhs) {
    return (lhs.waiting_time + lhs.travel_time > rhs.waiting_time + rhs.travel_time);
}

// Транспортный маршрутизатор
void TransportRouter::RouterBuilder(RouteSettings settings, const Catalogue* db) {
    // Преобразование км/ч в м/с
    settings_.bus_velocity = settings.bus_velocity / 3.6;
    // Преобразование минут в секунды
    settings_.bus_wait_time = settings.bus_wait_time * 60;
    
    db_ = db;
    
    auto* all_stops = db_->GetAllStops();
    graph_ = std::make_unique<graph::DirectedWeightedGraph<RouteTime>>(all_stops->size());
    graph::VertexId vertex_counter = 0;

    // Создание вершин графа для всех остановок
    for (auto [_, stop_ptr] : *all_stops) {
        graph_vertexes_.insert({ stop_ptr, vertex_counter++ });
    }

    // Построение ребра графа для всех маршрутов
    for (const auto& [_, route_ptr] : db->GetAllRoutes()) {
        const auto& stops = route_ptr->stops;
        RouteShortest distance_proc(db, route_ptr);
        
        for (int i = 0; i < stops.size() - 1; ++i) {
            for (int j = i + 1; j < stops.size(); ++j) {
                // Вычисление времени поездки между остановками
                RouteTime travel_dur(j - i, settings_.bus_wait_time,
                                          distance_proc.DistanceBetween(i, j) / settings_.bus_velocity
                );
                
                // Создание цепочки маршрутов
                RouteChain travel_unit{ stops[i], stops[j], route_ptr, travel_dur };
                
                // Добавление ребра в граф в прямом направлении
                graph_->AddEdge(graph::Edge<RouteTime>{graph_vertexes_[travel_unit.from], graph_vertexes_[travel_unit.to], travel_dur});
                graph_edges_.push_back(std::move(travel_unit));
                
                if (!route_ptr->is_circular) {
                    // Вычисление времени поездки в обратном направлении
                    RouteTime travel_dur(j - i, settings_.bus_wait_time,
                                              distance_proc.DistanceBetween(j, i) / settings_.bus_velocity
                    );
                    
                    // Создание цепочки маршруто в обратном направлении
                    RouteChain travel_unit{ stops[i], stops[j], route_ptr, travel_dur };
                    
                    // Добавление ребра в граф в обратном направлении
                    graph_->AddEdge(graph::Edge<RouteTime>{graph_vertexes_[travel_unit.to], graph_vertexes_[travel_unit.from], travel_dur});
                    graph_edges_.push_back(std::move(travel_unit));
                }
            }
        }
    }
    
    router_ = std::make_unique<graph::Router<RouteTime>>(*graph_);
}

// Поиск оптимального маршрута между двумя остановками
std::optional<std::vector<const RouteChain*>> TransportRouter::FindOptimalRoute(std::string_view from, std::string_view to) const {
    const Stop* stop_from = db_->FindStop(from);
    const Stop* stop_to = db_->FindStop(to);
    
    if (stop_from == nullptr || stop_to == nullptr) {
        return std::nullopt;
    }
    
    std::vector<const RouteChain*> result;
    if (stop_from == stop_to) {
        return result;
    }
    
    graph::VertexId vertex_from = graph_vertexes_.at(stop_from);
    graph::VertexId vertex_to = graph_vertexes_.at(stop_to);
    
    // Создание оптимального маршрутва с помощью поиска кратчайшего пути
    auto route = router_->BuildRoute(vertex_from, vertex_to);
    if (!route.has_value()) {
        return std::nullopt;
    }
    
    for (const auto& edge : route.value().edges) {
        result.push_back(&graph_edges_.at(edge));
    }
    
    return result;
}

} // end of namespace transport