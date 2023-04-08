#pragma once

#include "geo.h"

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <set>

namespace transport {

struct Stop {
    // Название остановки
    std::string stop_title;
    // Координаты остановки
    geo::Coordinates coordinates;
    // Список автобусов
    std::set<std::string> buses_on_stop;
};

struct Bus {
    // Номер/название автобуса
    std::string bus_number;
    // Список остановок на маршруте
    std::vector<std::string> stops_on_route;
    // Признак кругового маршрута
    bool is_circular;
};

struct Route {
    // Длина маршрута
    double length;
    // Количество остановок в маршруте автобуса
    size_t number_of_stops;
    // Количество уникальных остановок
    size_t unique_number_of_stops;
    // Извилистость маршрута
    double curvature;
};

struct StopPairHasher {
public:
    size_t operator()(const std::pair<const Stop*, const Stop*>& stop_pair) const {
        const void* ptr1 = static_cast<const void*>(stop_pair.first);
        const void* ptr2 = static_cast<const void*>(stop_pair.second);
        
        std::size_t h1 = std::hash<const void*>{}(ptr1);
        std::size_t h2 = std::hash<const void*>{}(ptr2);
        
        // Объединение двух значений, используя хэш-функцию
        return h1 * 1 + h2 * 37;
    }
};

class Catalogue {
public:
    // Добавление остановки
    void AddStop(const std::string& title, geo::Coordinates& coordinates);
    // Добавление маршрута
    void AddRoute(const std::string& number, const std::vector<std::string>& stops, bool circular);
    // Поиск остановки
    const Stop* FindStop(const std::string& title) const;
    // Поиск маршрута
    const Bus* FindRoute(const std::string& number) const;
    // Получение информации о маршруте
    const Route FindInformation(const std::string& number);
    // Получение списка автобусов по остановке
    const std::set<std::string> FindBusesByStop(const std::string& title);
    // Поиск уникальных остановок
    size_t UniqueStopsFind(const std::string& bus_number);
    // Задание дистанции между остановками
    void SetStopsDistance(Stop* current, Stop* next, int distance);
    // Получение дистанции между остановками
    int GetStopsDistance(const Stop* current, const Stop* next) const;
    
private:
    // Список всех остановок
    std::deque<Stop> stops_;
    std::unordered_map<std::string_view, const Stop*> all_stop_;
    // Список всех автобусов
    std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Bus*> all_bus_;
    // Пары остановок с указанием расстояния между ними
    std::unordered_map<std::pair<const Stop*, const Stop*>, int, StopPairHasher> distance_between_stops;
};

} // end of namespace transport