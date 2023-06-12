#pragma once

#include "json_builder.h"
#include "transport_catalogue.h"
#include "graph.h"
#include "router.h"

#include <optional>

namespace transport {

// Структура, описывающая настройки маршрута
struct RouteSettings {
    // Время ожидания автобуса на остановке
    int bus_wait_time;
    // Скорость движения автобуса
    double bus_velocity;
};

// Объявление синонимов
using GraphData = graph::DirectedWeightedGraph<double>;
using VertexData = std::map<std::string, graph::VertexId>;
    
class Router {
public:
    Router() = default;

    // Конструктор, принимающий узел с настройками
    Router(const RouteSettings& settings);
    // Конструктор, принимающий узел с настройками и каталог
    Router(const RouteSettings& settings, const Catalogue& db);
    // Конструктор, принимающий узел с настройками, граф и идентификаторы остановок
    Router(const RouteSettings& settings, GraphData graph, VertexData vertex); 

    // Установка графа и вершин остановок
    void SetGraph(GraphData&& graph, VertexData&& vertex);

    // Построение графа на основе каталога
    const GraphData& BuildGraph(const Catalogue& db);

    // Получение массива элементов ребер графа
    json::Node GetEdgesItems(const std::vector<graph::EdgeId>& edges) const;

    // Получение информации о маршруте от текущей остановки до следующей
    std::optional<graph::Router<double>::RouteInfo> GetRouteInfo(const Stop* current, const Stop* next) const;

    // Получение количества вершин в графе
    size_t GetGraphVertexCount();

    // Получение идентификаторов остановок
    const VertexData& GetStopsVertex() const;

    // Получение графа
    const GraphData& GetGraph() const;

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
    RouteSettings route_settings_;
    
    // Граф
    GraphData graph_; 
    // Идентификаторы остановок и их вершины
    VertexData stops_vertex_; 
    // Указатель на объект класса Router
    graph::Router<double>* router_ = nullptr; 
};

} // end of namespace transport