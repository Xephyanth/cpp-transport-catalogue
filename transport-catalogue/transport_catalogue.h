#pragma once

#include "domain.h"

#include <deque>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <string_view>
#include <algorithm>
#include <optional>

namespace transport {

namespace detail {

struct StopPairHasher {
public:
	size_t operator() (const std::pair<const Stop*, const Stop*> stop_pair) const {
		const void* ptr1 = static_cast<const void*>(stop_pair.first);
        const void* ptr2 = static_cast<const void*>(stop_pair.second);

        std::size_t h1 = std::hash<const void*>{}(ptr1);
        std::size_t h2 = std::hash<const void*>{}(ptr2);

        // Объединение двух значений, используя хэш-функцию
        return h1 + h2 * multiplier;
	}
    
private:
    int multiplier = 31;
};

} // end of namescape detail

class Catalogue {
public:
    // Добавление остановки
	void AddStop(std::string_view title, geo::Coordinates coords, const std::vector<std::pair<std::string, int>>& stops_distance);
    // Добавление маршрута
	void AddRoute(std::string_view number, const std::vector<std::string_view>& stops, bool circular);
    
    // Поиск остановки
	const Stop* FindStop(std::string_view title) const;
    // Поиск маршрута
    const Bus* FindRoute(std::string_view number) const;
    
    // Получение информации об остановке
	const std::unordered_set<const Bus*>* GetStopInfo(std::string_view title) const;
    // Получение информации о маршруте
	const std::optional<BusRoute> GetRouteInfo(std::string_view number) const;
    
    // Задание дистанции между остановками
	void SetStopsDistance(const Stop* current, const Stop* next, int distance);
    // Получение дистанции между остановками
	size_t GetStopsDistance(const Stop* current, const Stop* next) const;
    
	const std::map<std::string_view, const Bus*> GetAllRoutes() const;

private:
    // Список всех остановок
	std::deque<Stop> stops_;
    std::unordered_map<std::string_view, const Stop*> all_stops_;
    // Список всех автобусов
	std::deque<Bus> buses_;
    std::unordered_map<std::string_view, const Bus*> all_buses_;
	
    // Пары остановок с указанием расстояния между ними
	std::unordered_map<std::pair<const Stop*, const Stop*>, double, detail::StopPairHasher> distance_between_stops_;
    
	// Список автобусов, проходящих через остановку
	std::unordered_map<const Stop*, std::unordered_set<const Bus*>> buses_by_stop_;
};

} // end of namesapce transport