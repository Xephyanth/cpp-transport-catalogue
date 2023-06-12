#pragma once

#include "geo.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <string_view>

namespace transport {

struct Stop {
    // Название остановки
    std::string stop_title;
    // Координаты остановки
    geo::Coordinates coords;
    // Информация об остановках и расстояний между ними
    std::unordered_map<std::string_view, int> stops_distances;
    
    Stop(const std::string& title, const geo::Coordinates& coordinates);
    // Получение информации о расстоянии между остановками
    int GetStopsDistance(Stop* next);
};

struct Bus {
    // Номер автобуса/маршрута
    std::string bus_number;
    // Список остановок на маршруте
    std::vector<Stop*> stops;
    // Признак кругового маршрута
    bool is_circular;
    // Конечная остановка маршрута
    Stop* final_stop = nullptr;
    
    Bus(const std::string& number, std::vector<Stop*> stops, bool circle);
};

struct BusRoute {
    std::vector<std::string_view> stops;
    std::string_view final_stop;
    bool is_roundtrip;
};

} // end of namespace transport