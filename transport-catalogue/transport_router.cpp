#include "transport_router.h"

#include <string>
#include <string_view>
#include <map>
#include <utility>
#include <vector>
#include <algorithm>

namespace transport {

using namespace std::literals;

Router::Router(const RouteSettings& settings) : route_settings_(settings) {}

// Перегруженный конструктор для построения графов на основе каталога 
Router::Router(const RouteSettings& settings, const Catalogue& db) : route_settings_(settings) {
    BuildGraph(db);
}

// Перегруженный конструктор для построения графов и создания нового объекта
Router::Router(const RouteSettings& settings, GraphData graph, VertexData vertex)
    : route_settings_(settings), graph_(graph), stops_vertex_(vertex)
{
    router_ = new graph::Router<double>(graph_);
}

// Метод устанавливает новый граф и карту остановок
void Router::SetGraph(GraphData&& graph, VertexData&& vertex) {
    graph_ = std::move(graph);
    stops_vertex_ = std::move(vertex);
    router_ = new graph::Router<double>(graph_);
}

// Метод строит граф маршрутов на основе транспортного каталога
const GraphData& Router::BuildGraph(const Catalogue& db) {
    const std::map<std::string_view, Stop*>& all_stops = db.GetSortedAllStops();
    const std::map<std::string_view, Bus*>& all_buses = db.GetSortedAllBuses();
    
    // Создание графа с удвоенным количеством вершин
    GraphData stops_graph(all_stops.size() * 2);
    VertexData stop_vertex;
    graph::VertexId vertex_id = 0;
    
    // Добавление вершин и ребра, связанных с остановками
    for (const auto& [_, stop] : all_stops) {
        stop_vertex[stop->stop_title] = vertex_id;
        stops_graph.AddEdge({ stop->stop_title, 0,
                              vertex_id, ++vertex_id,
                              static_cast<double>(route_settings_.bus_wait_time)
                           });
        ++vertex_id;
    }
    stops_vertex_ = std::move(stop_vertex);

    // Добавление вершин и ребра, связанных с автобусами
    std::for_each(all_buses.begin(), all_buses.end(),
             [&stops_graph, this](const auto& item) {
                 const auto& bus = item.second;
                 const std::vector<Stop*>& stops = bus->stops;
                 size_t stops_count = stops.size();
                 
                 for (size_t i = 0; i < stops_count; ++i) {
                     for (size_t j = i + 1; j < stops_count; ++j) {
                         const Stop* stop_from = stops[i];
                         const Stop* stop_to = stops[j];
                         int dist_sum = 0;
                         
                         // Вычисление суммарного расстояния между остановками
                         for (size_t k = i + 1; k <= j; ++k) {
                             dist_sum += stops[k - 1]->GetStopsDistance(stops[k]);
                         }
                         
                         // Добавление ребра графа для автобуса
                         stops_graph.AddEdge({ bus->bus_number, j - i, stops_vertex_.at(stop_from->stop_title) + 1,
                                               stops_vertex_.at(stop_to->stop_title),
                                               static_cast<double>(dist_sum) / (route_settings_.bus_velocity * (100.0 / 6.0)) }
                                            );
                         
                         // Если автобус не является кольцевым и достигнута конечная остановка,
                         // прерываем добавление ребер
                         if (!bus->is_circular && stop_to == bus->final_stop && j == stops_count / 2) {
                             break;
                         }
                     }
                 }
             });

    graph_ = std::move(stops_graph);
    router_ = new graph::Router<double>(graph_);
    
    return graph_;
}

// Метод преобразует ребра графа в элементы массива для JSON
json::Node Router::GetEdgesItems(const std::vector<graph::EdgeId>& edges) const {
    json::Builder builder;
    auto arrayContext = builder.StartArray();
    
    // Создание массива элементов ребер графа для передачи в формат JSON
    for (auto& edge_id : edges) {
        const graph::Edge<double>& edge = graph_.GetEdge(edge_id);
        
        if (edge.quality == 0) {
            auto dictContext = arrayContext.StartDict();
            dictContext.Key("stop_name"s).Value(static_cast<std::string>(edge.name));
            dictContext.Key("time"s).Value(edge.weight);
            dictContext.Key("type"s).Value("Wait"s);
            dictContext.EndDict();
        }
        else {
            auto dictContext = arrayContext.StartDict();
            dictContext.Key("bus"s).Value(static_cast<std::string>(edge.name));
            dictContext.Key("span_count"s).Value(static_cast<int>(edge.quality));
            dictContext.Key("time"s).Value(edge.weight);
            dictContext.Key("type"s).Value("Bus"s);
            dictContext.EndDict();
        }
    }
    
    return arrayContext.EndArray().Build();
}

// Метод возвращает информацию о маршруте между остановками
std::optional<graph::Router<double>::RouteInfo> Router::GetRouteInfo(const Stop* current, const Stop* next) const {
    return router_->BuildRoute(stops_vertex_.at(current->stop_title), stops_vertex_.at(next->stop_title));
}

// Метод возвращает количество вершин в графе
size_t Router::GetGraphVertexCount() {
    return graph_.GetVertexCount();
}

// Метод возвращает константную ссылку на объект и позволяет получить доступ к построенному графу
const GraphData& Router::GetGraph() const {
    return graph_;
}

// Метод возвращает соответствия идентификаторов остановок и их названий
const VertexData& Router::GetStopsVertex() const {
    return stops_vertex_;
}

// Метод возвращает объект, содержащий настройки маршрутизатора
json::Node Router::GetSettings() const {
    return json::Builder{}.StartDict()
        .Key("bus_wait_time"s).Value(route_settings_.bus_wait_time)
        .Key("bus_velocity"s).Value(route_settings_.bus_velocity)
        .EndDict().Build();
}

/*
*   Сериализация
*/

serialize::RouterSettings Router::RouterSettingSerialize(const json::Node& router_settings) const {
    const json::Dict& rs_map = router_settings.AsDict();
    
    serialize::RouterSettings result;
    
    result.set_bus_wait_time(rs_map.at("bus_wait_time"s).AsInt());
    result.set_bus_velocity(rs_map.at("bus_velocity"s).AsDouble());
    
    return result;
}

serialize::Graph GraphSerialize(const GraphData& g) {
    serialize::Graph result;
    
    size_t vertex_count = g.GetVertexCount();
    size_t edge_count = g.GetEdgeCount();
    
    for (size_t i = 0; i < edge_count; ++i) {
        serialize::Edge s_edge;
        
        const graph::Edge<double>& edge = g.GetEdge(i);
        
        s_edge.set_name(edge.name);
        s_edge.set_quality(edge.quality);
        s_edge.set_from(edge.from);
        s_edge.set_to(edge.to);
        s_edge.set_weight(edge.weight);
        
        *result.add_edge() = s_edge;
    }
    for (size_t i = 0; i < vertex_count; ++i) {
        serialize::Vertex vertex;
        
        for (const auto& edge_id : g.GetIncidentEdges(i)) {
            vertex.add_edge_id(edge_id);
        }
        
        *result.add_vertex() = vertex;
    }
    
    return result;
}

serialize::Router Router::RouterSerialize(const Router& router) const {
    serialize::Router result;
    
    *result.mutable_router_settings() = RouterSettingSerialize(router.GetSettings());
    *result.mutable_graph() = GraphSerialize(router.GetGraph());
    
    for (const auto& [name, id] : router.GetStopsVertex()) {
        serialize::StopId si;
        
        si.set_name(name);
        si.set_id(id);
        
        *result.add_stop_id() = si;
    }
    
    return result;
}

} // end of namespace transport