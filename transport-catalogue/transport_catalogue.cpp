#include "transport_catalogue.h"

#include <stdexcept>
#include <unordered_set>

using std::literals::string_literals::operator""s;

namespace transport {

// Добавление остановки
void Catalogue::AddStop(const std::string& stop_title, geo::Coordinates& coordinates) {
    // Создание новой остановки в списке всех остановок
    stops_.emplace_back(Stop{ stop_title, coordinates, {} });
    // Получение ссылки на созданный объект
    auto& new_stop = stops_.back();
    // Добавление остановку 
    all_stop_.emplace(stops_.back().stop_title, &new_stop);
}

// Добавление маршрута
void Catalogue::AddRoute(const std::string& number, const std::vector<std::string>& stops, bool circular) {
    // Создание нового автобуса в списке всех автобусов
    buses_.emplace_back(Bus{ number, stops, circular });
    // Получение ссылки на созданный объект
    auto& new_bus = buses_.back();
    // Добавление автобуса
    all_bus_.emplace(buses_.back().bus_number, &new_bus);
    
    // Цикл по остановкам - std::vector<std::string>& stops
    for (const auto& stop : stops) {
        // Цикл по всем остановкам - std::deque<Stop> stops_;
        for (auto& stop_ : stops_) {
            if (stop_.stop_title == stop) {
                stop_.buses_on_stop.emplace(number);
            }
        }
    }
}

// Поиск маршрута
const Bus* Catalogue::FindRoute(const std::string_view bus_number) const {
    return all_bus_.count(bus_number) ? all_bus_.at(bus_number) : nullptr;
}

// Поиск остановки
const Stop* Catalogue::FindStop(const std::string_view stop_title) const {
    return all_stop_.count(stop_title) ? all_stop_.at(stop_title) : nullptr;
}

// Получение информации о маршруте
const Route Catalogue::FindInformation(const std::string_view bus_number) {
    // Результирующий набор
    Route route_information_result;
    // Переменная для длины маршрута
    int route_length = 0.;
    // Переменная для географического расстояния
    double geo_route_length = 0.;
    // Поиск номера автобуса в списке
    const Bus* bus = FindRoute(bus_number);
    
    // Проверка наличия автобуса
    if (!bus) {
        throw std::invalid_argument("Bus number doesn't exist"s);
    }
    
    const auto& stops_count = bus->stops_on_route.size();
    
    if (stops_count == 0) {
        throw std::invalid_argument("The route has no stops"s);
    }
    
    // Вычисление кол-ва остановок на маршруте в зависимости от того, является он круговым или нет
    route_information_result.number_of_stops = bus->is_circular ? stops_count : stops_count * 2 - 1;
    
    for (size_t i = 0; i < stops_count - 1; ++i) {
        // Получение текущей и слеюущей остановки маршрута
        const Stop& current_stop = *all_stop_.at(bus->stops_on_route.at(i));
        const Stop& next_stop = *all_stop_.at(bus->stops_on_route.at(i + 1));
        
        // Вычисление длинны маршрута
        route_length += GetStopsDistance(current_stop, next_stop);
        geo_route_length += ComputeDistance(current_stop.coordinates, next_stop.coordinates);
        
        if (!bus->is_circular) {
            route_length += GetStopsDistance(next_stop, current_stop);
            geo_route_length += ComputeDistance(next_stop.coordinates, current_stop.coordinates);
        }
    }
    
    route_information_result.unique_number_of_stops = UniqueStopsFind(bus_number);
    route_information_result.length = route_length;
    route_information_result.curvature = route_length / geo_route_length;
    
    return route_information_result;
}

// Получение списка автобусов по остановке
const std::set<std::string> Catalogue::FindBusesByStop(const std::string_view stop_title) {
    return all_stop_.at(stop_title)->buses_on_stop;
}

// Поиск уникальных 
size_t Catalogue::UniqueStopsFind(const std::string_view bus_number) {
    std::unordered_set<std::string> unique_stops;
    
    for (const auto& stop : all_bus_.at(bus_number)->stops_on_route) {
        unique_stops.insert(stop);
    }
    
    return unique_stops.size();
}

// Установить расстояние между двумя остановками
void Catalogue::SetStopsDistance(const Stop& current, const Stop& next, int distance) {
    distance_between_stops[std::make_pair(&current, &next)] = distance;
}

// Получение расстояния между двумя остановками
int Catalogue::GetStopsDistance(const Stop& current, const Stop& next) const {
    auto it = distance_between_stops.find(std::make_pair(&current, &next));
    if (it != distance_between_stops.end()) {
        return it->second;
    }
    
    // Если расстояние от остановки `x` до остановки `y` не было найдено, поиск расстояния от `y` до `x`
    it = distance_between_stops.find(std::make_pair(&next, &current));
    if (it != distance_between_stops.end()) {
        return it->second;
    }
    
    // Если расстояние не было найдено ни в одном из направлений
    return 0;
}

} // end of namespace transport