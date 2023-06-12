#include "json_reader.h"

namespace transport {

using namespace std::literals;

const json::Node& JsonReader::GetBaseRequestData() const {
    return data_.GetRoot().AsDict().at("base_requests"s);
}

const json::Node& JsonReader::GetStatRequestData() const {
    return data_.GetRoot().AsDict().at("stat_requests"s);
}

const json::Node& JsonReader::GetRenderSettingsData() const {
    return data_.GetRoot().AsDict().at("render_settings"s);
}

const json::Node& JsonReader::GetRoutingSettingsData() const {
    return data_.GetRoot().AsDict().at("routing_settings"s);
}

const json::Node& JsonReader::GetSerializationSettingsData() const {
    return data_.GetRoot().AsDict().at("serialization_settings"s);
}

void JsonReader::ImportData(Catalogue& db) const {
    StopsDist stops_distance; // Расстояния между остановками
    BusesInfo buses;          // Информация о маршрутах
    
    const json::Array& arr = GetBaseRequestData().AsArray();
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

// Вспомогательная функция для получения цветовой палитры
svg::Color ColorProcessing(const json::Node& color) {
    // Если цвет представлен в виде строки, то возвращаем его без изменений
    if (color.IsString()) {
        return color.AsString();
    }
    
    // Если массив из трех элементов - RGB
    if (color.AsArray().size() == 3) {
        return svg::Rgb(
            color.AsArray()[0].AsInt(),
            color.AsArray()[1].AsInt(),
            color.AsArray()[2].AsInt()
        );
    }
    
    // Иначе - RGBA
    return svg::Rgba(
        color.AsArray()[0].AsInt(),
        color.AsArray()[1].AsInt(),
        color.AsArray()[2].AsInt(),
        color.AsArray()[3].AsDouble()
    );
}

MapRenderer JsonReader::SetRenderSettings(const json::Node& render_settings) {
    RenderSettings result;
    
    if (render_settings.IsNull()) {
        return result;
    }
    
    const json::Dict& settings = render_settings.AsDict();
    
    // Получение ширины, высоты и отступа из настроек
    result.width = settings.at("width"s).AsDouble();
    result.height = settings.at("height"s).AsDouble();
    result.padding = settings.at("padding"s).AsDouble();
    
    // Получение толщины линии и радиуса остановки из настроек
    result.stop_radius = settings.at("stop_radius"s).AsDouble();
    result.line_width = settings.at("line_width"s).AsDouble();
    
    // Получение размера шрифта и смещения для подписей маршрутов из настроек
    result.bus_label_font_size = settings.at("bus_label_font_size"s).AsInt();
    result.bus_label_offset = svg::Point(
        settings.at("bus_label_offset"s).AsArray()[0].AsDouble(),
        settings.at("bus_label_offset"s).AsArray()[1].AsDouble()
    );
    
    // Получение размера шрифта и смещения для подписей остановок из настроек
    result.stop_label_font_size = settings.at("stop_label_font_size"s).AsInt();
    result.stop_label_offset = svg::Point(
        settings.at("stop_label_offset"s).AsArray()[0].AsDouble(),
        settings.at("stop_label_offset"s).AsArray()[1].AsDouble()
    );
    
    // Получение ширины и цвета подложки/фона из настроек
    result.underlayer_color = ColorProcessing(settings.at("underlayer_color"s));
    result.underlayer_width = settings.at("underlayer_width"s).AsDouble();
    
    // Получение палитры цветов из настроек
    for (const auto& color : settings.at("color_palette"s).AsArray()) {
        result.color_palette.push_back(ColorProcessing(color));
    }
    
    return result;
}

RouteSettings JsonReader::SetRouterSettings(const json::Node& router_settings) {
    RouteSettings result;
    
    result.bus_wait_time = router_settings.AsDict().at("bus_wait_time"s).AsInt();
    result.bus_velocity = router_settings.AsDict().at("bus_velocity"s).AsDouble();
    
    return result;
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