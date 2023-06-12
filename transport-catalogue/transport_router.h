#pragma once

#include "json_builder.h"
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

#include <optional>

namespace transport {

class Router {
public:
    Router() = default;

    // Конструктор, принимающий узел с настройками
    Router(const json::Node& settings);
    // Конструктор, принимающий узел с настройками и каталог
    Router(const json::Node& settings, const Catalogue& db);
    // Конструктор, принимающий узел с настройками, граф и идентификаторы остановок
    Router(const json::Node& settings, graph::DirectedWeightedGraph<double> graph, std::map<std::string, graph::VertexId> vertex); 

    // Установка графа и вершин остановок
    void SetGraph(graph::DirectedWeightedGraph<double>&& graph, std::map<std::string, graph::VertexId>&& vertex);

    // Построение графа на основе каталога
    const graph::DirectedWeightedGraph<double>& BuildGraph(const Catalogue& db);

    // Получение массива элементов ребер графа
    json::Node GetEdgesItems(const std::vector<graph::EdgeId>& edges) const;

    // Получение информации о маршруте от текущей остановки до следующей
    std::optional<graph::Router<double>::RouteInfo> GetRouteInfo(const Stop* current, const Stop* next) const;

    // Получение количества вершин в графе
    size_t GetGraphVertexCount();

    // Получение идентификаторов остановок
    const std::map<std::string, graph::VertexId>& GetStopsVertex() const;

    // Получение графа
    const graph::DirectedWeightedGraph<double>& GetGraph() const;

    // Получение настроек
    json::Node GetSettings() const;

    // Методы для сериализации данных
    serialize::RouterSettings RouterSettingSerialize(const json::Node& router_settings) const;
    
    serialize::Router RouterSerialize(const transport::Router& router) const;
    
    // Деструктор
    ~Router() {
        delete router_;
    }

private:
    // Установка настроек
    void SetSettings(const json::Node& settings_node);
    
    // Время ожидания автобуса
    int bus_wait_time_ = 0;
    // Скорость автобуса
    double bus_velocity_ = 0;
    
    // Граф
    graph::DirectedWeightedGraph<double> graph_; 
    // Идентификаторы остановок и их вершины
    std::map<std::string, graph::VertexId> stops_vertex_; 
    // Указатель на объект класса Router
    graph::Router<double>* router_ = nullptr; 
};

} // end of namespace transport