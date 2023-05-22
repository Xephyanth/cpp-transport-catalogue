#pragma once

#include "geo.h"

#include <string>
#include <vector>

namespace transport {

struct Stop {
    // Название остановки
    std::string stop_title;
    // Координаты остановки
    geo::Coordinates coords;
    // Информация об остановках и расстояний между ними
    std::vector<std::pair<std::string, int>> stops_distances;
};

struct Bus {
    // Номер автобуса/маршрута
    std::string bus_number;
    // Список остановок на маршруте
    std::vector<const Stop*> stops;
    // Признак кругового маршрута
    bool is_circular;
};

struct BusRoute {
    // Название маршурта
    std::string_view name;
    // Количество остановок маршрута
    int stops;
    // Количество уникальных остановок
    int unique_stops;
    // Длина маршрута
    int route_length;
    // Извилистость маршрута
    double curvature;
};

} // end of namespace transport