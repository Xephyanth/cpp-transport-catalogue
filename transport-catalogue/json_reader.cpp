#include "json_reader.h"

namespace transport {

using namespace std::literals;

const json::Node& JsonReader::GetBaseRequest() const {
    return data_.GetRoot().AsDict().at("base_requests"s);
}

const json::Node& JsonReader::GetStatRequest() const {
    return data_.GetRoot().AsDict().at("stat_requests"s);
}

const json::Node& JsonReader::GetRenderSettings() const {
    return data_.GetRoot().AsDict().at("render_settings"s);
}

const json::Node& JsonReader::GetRoutingSettings() const {
    return data_.GetRoot().AsDict().at("routing_settings"s);
}

const json::Node& JsonReader::GetSerializationSettings() const {
    return data_.GetRoot().AsDict().at("serialization_settings"s);
}

void JsonReader::ImportData(Catalogue& db) const {
    StopsDist stops_distance; // Расстояния между остановками
    BusesInfo buses;          // Информация о маршрутах
    
    const json::Array& arr = GetBaseRequest().AsArray();
    for (const auto& request_node : arr) {
        const json::Dict& request = request_node.AsDict();
        // Извлечение типа запроса
        const std::string& type = request.at("type"s).AsString();
        
        // Обработка запроса типа Остановка
        if (type == "Stop"s) {
            ParseStop(db, request, stops_distance);
        }
        // Обработка запроса типа Маршрут
        if (type == "Bus"s) {
            ParseBus(request, buses);
        }
    }
    
    
    SetStopsDistances(db, stops_distance); // Установка расстояния между остановками
    BusesAdd(db, buses);                   // Добавление маршрутов
    SetFinalStop(db, buses);               // Установка конечных остановок
}

void JsonReader::ParseStop(Catalogue& db, const json::Dict& request, StopsDist& stops_distance) const {
    // Извлечение названия остановки и координат
    const auto& stop_name = request.at("name"s).AsString();
    geo::Coordinates coords = { request.at("latitude"s).AsDouble(), request.at("longitude"s).AsDouble() };
    
    // Добавление остановки в БД
    db.AddStop(stop_name, coords);
    
    // Извлечение информации о расстояниях
    const json::Dict& near_stops = request.at("road_distances"s).AsDict();
    for (const auto& [next_stop, dist] : near_stops) {
        // Добавление расстояний до указанных ближайших остановки
        stops_distance[stop_name][next_stop] = dist.AsInt();
    }
}

void JsonReader::SetStopsDistances(Catalogue& db, const StopsDist& stops_distance) const {
    for (const auto& [stop, near_stops] : stops_distance) {
        for (const auto& [stop_name, dist] : near_stops) {
            // Сохранение расстояний между остановками в БД
            db.SetStopsDistance(db.FindStop(stop), db.FindStop(stop_name), dist);
        }
    }
}

void JsonReader::ParseBus(const json::Dict& request, BusesInfo& buses) const {
    // Извлечение названия маршрута и его остановок
    const std::string& bus_name = request.at("name"s).AsString();
    const json::Array& bus_stops = request.at("stops"s).AsArray();
    
    // Получение кол-ва остановок
    size_t stops_count = bus_stops.size();
    // Признак кругового маршрута
    bool is_roundtrip = request.at("is_roundtrip"s).AsBool();
    buses[bus_name].is_roundtrip = is_roundtrip;
    
    // Получение списка остановок маршрута
    auto& stops = buses[bus_name].stops;
    if (stops_count > 0) {
        // В зависимости от типа маршрута резервируется память для списка остановок 
        stops.reserve(is_roundtrip ? stops_count : stops_count * 2);
    }
    
    // Цикл по всем остановкам
    for (size_t i = 0; i < bus_stops.size(); ++i) {
        // Добавление остановки
        stops.push_back(bus_stops[i].AsString());
        
        // Если достигли последней остановки
        if (i == bus_stops.size() - 1) {
            // И если маршрут не круговой
            if (!is_roundtrip) {
                // Устанавка последней остановки
                buses[bus_name].final_stop = bus_stops[i].AsString();
                
                // Список остановок в обратном порядке для обратного маршрута
                for (int j = stops.size() - 2; j >= 0; --j) {
                    stops.push_back(stops[j]);
                }
            }
            else {
                // Установка последней остановки для кругового маршрута
                buses[bus_name].final_stop = bus_stops[0].AsString();
            }
        }
    }
}

void JsonReader::BusesAdd(Catalogue& db, const BusesInfo& buses) const {
    for (const auto& [name, info] : buses) {
        std::vector<Stop*> all_stops;
        const auto& stops = info.stops;
        all_stops.reserve(stops.size());
        
        for (const auto& stop : stops) {
            all_stops.push_back(db.FindStop(stop));
        }
        
        // Добавление маршрута в БД
        db.AddRoute(name, all_stops, info.is_roundtrip);
    }
}

void JsonReader::SetFinalStop(Catalogue& db, const BusesInfo& buses) const {
    // Задает конечную остановку для маршрута в БД
    for (auto& [bus_name, info] : buses) {
        if (Bus* bus = db.FindRoute(bus_name)) {
            if (Stop* stop = db.FindStop(info.final_stop)) {
                bus->final_stop = stop;
            }
        }
    }
}

} // end of namespace transport