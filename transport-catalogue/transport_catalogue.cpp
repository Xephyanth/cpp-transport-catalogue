#include "transport_catalogue.h"

namespace transport {

// Метод добавления новой остановки в каталог
void Catalogue::AddStop(const std::string_view title, geo::Coordinates coords, const std::vector<std::pair<std::string, int>>& stops_distance) {
    // Создание и добавление новой остановки
    const auto& stop = stops_.emplace_back(Stop{ std::string(title), coords,  stops_distance });
    // Добавление указателя на новую остановку
    all_stops_.insert({ stop.stop_title, &stop });
    // Создание и добавление пустого списка автобусов для каждой остановки
    buses_by_stop_[&stop];
}

// Метод добавления новой остановки в каталог
void Catalogue::AddRoute(const std::string_view number, const std::vector<std::string_view>& stops, bool circular) {
    // Создаем вектор указателей на остановки для маршрута
    std::vector<const Stop*> route_stops;
    route_stops.reserve(stops.size());
    
    // Добавляем новый маршрут в список маршрутов
    auto& route = buses_.emplace_back(Bus{ std::string(number), route_stops, circular });
    // Добавляем маршрут в список всех маршрутов по номеру
    all_buses_.insert({ route.bus_number, &route });
    
    // Для каждой остановки на маршруте
    for (const auto& stop : stops) {
        // Находим указатель на остановку в каталоге
        const auto* stop_ptr = FindStop(stop);
        // Добавляем остановку в список остановок маршрута
        route.stops.push_back(stop_ptr);
        // Добавляем маршрут в список маршрутов, проходящих через эту остановку
        buses_by_stop_[stop_ptr].insert(FindRoute(number));
    }
}

// Метод поиска автобусного маршрута по номеру
const Bus* Catalogue::FindRoute(const std::string_view number) const {
    // Ищем маршрут в списке, используя номер в качестве ключа
    const auto it = all_buses_.find(number);
    
    if (it == all_buses_.end()) {
        return nullptr;
    } else { 
        return it->second;
    }
}

// Метод поиска остановки по названию
const Stop* Catalogue::FindStop(const std::string_view title) const {
    // Ищем остановку в списке, используя название в качестве ключа
    const auto it = all_stops_.find(title);
    
    if (it == all_stops_.end()) {
        return nullptr;
    } else {
        return it->second;
    }
}

// Метод возвращает указатель, который содержит указатели на все объекты
const std::unordered_set<const Bus*>* Catalogue::GetStopInfo(const std::string_view title) const {
    if (FindStop(title)) {
        return &buses_by_stop_.at(FindStop(title));
    } else {
        return nullptr;
    }
}
    
// Метод установления расстояния между остановками
void Catalogue::SetStopsDistance(const Stop* current, const Stop* next, int distance) {
    // Создание пары остановок, состоящей из текущей и следующей остановки
    auto stops_pair = std::make_pair(current, next);
    // Установка расстояния между текущей и следующей остановкой
    distance_between_stops_[stops_pair] = distance;
}

// Метод получения расстояния между остановками
size_t Catalogue::GetStopsDistance(const Stop* current, const Stop* next) const {
    // Создание пары из текущей и следующей остановок
    auto stops_pair = std::make_pair(current, next);
    
    // Поиск расстояния от `x` до `y`
    if (distance_between_stops_.find(stops_pair) != distance_between_stops_.end()) {
        return distance_between_stops_.at(stops_pair);
    }
    
    // Поиск расстояния от `y` до `x`
    return GetStopsDistance(next, current);
}

// Метод возвращает список всех маршрутов
const std::map <std::string_view, const Bus*> Catalogue::GetAllRoutes() const {
    std::map<std::string_view, const Bus*> result;
    
    for (const auto& route : buses_) {
        result[route.bus_number] = &route;
    }
    
    return result;
}

const std::unordered_map<std::string_view, const Stop*>* Catalogue::GetAllStops() const {
    return &all_stops_;
}

// Метод возвращает ионфрмацию о маршруте
const std::optional<BusRoute> Catalogue::GetRouteInfo(const std::string_view bus_name) const {
    BusRoute result;
    
    // Получение указателя на объект маршрута по его названию
    const auto* route = FindRoute(bus_name);
    
    // Если маршрут не найден, возврат пустого объекта optional
    if (route == nullptr) {
        return {};
    }

    // Список остановок маршрута
    std::vector<const Stop*> stops_local{ route->stops.begin(), route->stops.end() };
    // Множество уникальных остановок
    std::unordered_set<const Stop*> unique_stops;

    // Добавление всех остановок в множество уникальных остановок
    std::for_each(stops_local.begin(), stops_local.end(),
                  [&unique_stops](const auto& stop) {
                      unique_stops.emplace(stop);
                  });

    // Если маршрут не кольцевой, добавляется обратный путь в список остановок
    if (!route->is_circular) {
        stops_local.insert(stops_local.end(), next(route->stops.rbegin()), route->stops.rend());
    }

    
    double road_route_length = 0;
    double geo_route_length = 0;
    // Вычисление длины маршрута как сумма расстояний между остановками
    for (auto it = stops_local.begin(); it != std::prev(stops_local.end()); ++it) {
        road_route_length += ComputeDistance((*it)->coords, (*(it + 1))->coords);
        geo_route_length += GetStopsDistance(*it, *(it + 1));
    }

    result.name = route->bus_number;
    result.stops = int(stops_local.size());
    result.unique_stops = int(unique_stops.size());
    result.route_length = geo_route_length;
    result.curvature = geo_route_length / road_route_length;
    
    return result;
}

} // end of namesapce transport