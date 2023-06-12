#include "transport_catalogue.h"

#include <string>
#include <vector>
#include <algorithm>

namespace transport {

using namespace std::literals;

// Метод добавления новой остановки в каталог
void Catalogue::AddStop(const std::string_view title, const geo::Coordinates& coordinates) {
    // Добавление новой остановки
    stops_.push_back(Stop( std::string(title), coordinates ));
    // Указатель на новую остановку
    Stop* stop = &stops_.back();
    buses_by_stop_[stop->stop_title];
    // Добавление пустого списка автобусов для каждой остановки
    all_stops_[stop->stop_title] = stop;
}

// Метод добавления новой остановки в каталог
void Catalogue::AddRoute(const std::string_view number, const std::vector<Stop*>& stops, bool circular) {
    // Добавляем новый маршрут в список маршрутов
    buses_.push_back(Bus( std::string(number), stops, circular ));
    
    // Указатель на новую остановку
    Bus* bus = &buses_.back();
    // Для каждой остановки на маршруте
    for (const Stop* stop : stops) {
        // Добавляем маршрут в список маршрутов, проходящих через эту остановку
        buses_by_stop_[stop->stop_title][bus->bus_number] = bus;
    }
    
    // Добавляем маршрут в список всех маршрутов по номеру
    all_buses_[bus->bus_number] = bus;
}


// Метод поиска остановки по названию
Stop* Catalogue::FindStop(const std::string_view title) {
    return all_stops_.count(title) ? all_stops_.at(title) : nullptr;
}
const Stop* Catalogue::FindStop(const std::string_view title) const {
    return all_stops_.count(title) ? all_stops_.at(title) : nullptr;
}

// Метод поиска автобусного маршрута по номеру
Bus* Catalogue::FindRoute(const std::string_view number) {
    return all_buses_.count(number) ? all_buses_.at(number) : nullptr;
}
const Bus* Catalogue::FindRoute(const std::string_view number) const {
    return all_buses_.count(number) ? all_buses_.at(number) : nullptr;
}

// Метод возвращает ионфрмацию о маршруте
const std::map<std::string_view, Bus*> Catalogue::GetRouteInfo(const std::string_view number) const {
    return buses_by_stop_.at(number);
}

// Метод установления расстояния между остановками
void Catalogue::SetStopsDistance(Stop* current, Stop* next, int distance) {
    current->stops_distances[next->stop_title] = distance;
}

// Метод возвращает отсортированный список всех маршрутов
const std::map<std::string_view, Bus*>& Catalogue::GetSortedAllBuses() const {
    return all_buses_;
}
    
// Метод возвращает отсортированный список всех остановок
const std::map<std::string_view, Stop*>& Catalogue::GetSortedAllStops() const {
    return all_stops_;
}

//
serialize::Stop Catalogue::StopsSerialize(const Stop* stop) const {
    serialize::Stop result;
    
    result.set_name(stop->stop_title);
    result.add_coordinate(stop->coords.lat);
    result.add_coordinate(stop->coords.lng);
    
    for (const auto& [n, d] : stop->stops_distances) {
        result.add_near_stop(static_cast<std::string>(n));
        result.add_distance(d);
    }
    
    return result;
}

serialize::Bus Catalogue::BusesSerialize(const Bus* bus) const {
    serialize::Bus result;
    
    result.set_name(bus->bus_number);
    
    for (const auto& s : bus->stops) {
        result.add_stop(s->stop_title);
    }
    
    result.set_is_circle(bus->is_circular);
    
    if (bus->final_stop) {
        result.set_final_stop(bus->final_stop->stop_title);
    }
    
    return result;
}

} // end of namespace transport