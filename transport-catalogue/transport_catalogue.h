#pragma once

#include "domain.h"

#include <deque>
#include <vector>
#include <string>
#include <string_view>
#include <unordered_map>
#include <map>

#include <transport_catalogue.pb.h>

namespace transport {

class Catalogue {
public:
    // Добавление остановки
    void AddStop(const std::string_view title, const geo::Coordinates& coordinates);
    // Добавление маршрута
    void AddRoute(const std::string_view number, const std::vector<Stop*>& stops, bool circular);
    
    // Поиск остановки
    Stop* FindStop(const std::string_view title);
    const Stop* FindStop(const std::string_view title) const;
    // Поиск маршрута
    Bus* FindRoute(const std::string_view number);
    const Bus* FindRoute(const std::string_view number) const;
    
    // Получение информации о маршруте
    const std::map<std::string_view, Bus*> GetRouteInfo(const std::string_view number) const;
    
    // Задание дистанции между остановками
    void SetStopsDistance(Stop* current, Stop* next, int distance);
    
    // Получение отсортированных списков
    const std::map <std::string_view, Bus*>& GetSortedAllBuses() const;
    const std::map <std::string_view, Stop*>& GetSortedAllStops() const;

    // Сериализация данных каталога
    serialize::Stop StopsSerialize(const Stop* stop) const;
    serialize::Bus BusesSerialize(const Bus* bus) const;
    
private:
    // Список всех остановок
    std::deque<Stop> stops_;
    std::map<std::string_view, Stop*> all_stops_;
    // Список всех автобусов
    std::deque<Bus> buses_;
    std::map<std::string_view, Bus*> all_buses_;
    
    // Список автобусов, проходящих через остановку
    std::unordered_map <std::string_view, std::map<std::string_view, Bus*>> buses_by_stop_;
};

} // end of namespace transport