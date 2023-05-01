#include "json_reader.h"

#include <vector>
#include <string>
#include <sstream>

using namespace std::literals;

namespace transport {

JsonReader::JsonReader(std::istream& input, Catalogue* db, MapRenderer* renderer) : input_(json::Load(input)), db_(db), renderer_(renderer) {
	std::vector<std::string> stop_titles;
    
    // Обработка и сохранение всех транспортных остановок
	for (const auto& base_requests : input_.GetRoot().AsMap().at("base_requests"s).AsArray()) {
		if (base_requests.AsMap().at("type"s) == "Stop"s) {
            // Получение информации
			auto data = base_requests.AsMap();
            
			auto title = data.at("name"s).AsString();
			auto geo_lat = data.at("latitude"s).AsDouble();
			auto geo_lng = data.at("longitude"s).AsDouble();
			auto distance = data.at("road_distances"s).AsMap();
            
			stop_titles.push_back(title);
            
            // Формирование вектора пар, которая содержит название остановки и расстояние до неё
			std::vector<std::pair<std::string, int>> route_stops;
			for (const auto& [stop_name, distance] : distance) {
				route_stops.push_back(std::make_pair(stop_name, distance.AsInt()));
			}
            
            // Добавление остановки в каталог
			db_->AddStop(title, { geo_lat, geo_lng }, route_stops);
		}
	}

    // Обработка и добавление расстояний между остановками
	for (auto stop : stop_titles) {
		auto* current = db_->FindStop(stop);
        
		for (auto& [next, distance] : current->stops_distances) {
			db_->SetStopsDistance(current, db_->FindStop(next), distance);
		}
	}
    
    // Добавление автобусных маршрутов
    for (const auto& base_requests : input_.GetRoot().AsMap().at("base_requests"s).AsArray()) {
		if (base_requests.AsMap().at("type"s) == "Bus"s) {
            // Получение информации
			auto data = base_requests.AsMap();
            
			const auto& name = data.at("name"s).AsString();
            
            // Остановки маршрута
			std::vector<std::string_view> stops;
			for (const auto& stop : data.at("stops"s).AsArray()) {
				stops.push_back(stop.AsString());
			}
            
            // Добавление маршрута в каталог
			db_->AddRoute(name, stops, data.at("is_roundtrip"s).AsBool());
		}
	}
}

void JsonReader::DatabaseRespond(std::ostream& output) {
    RequestHandler handler(*db_, *renderer_);
	json::Array respond;
    
    // Цикл по массиву запросов внутри поля stat_requests корневого узла
	for (const json::Node& request : input_.GetRoot().AsMap().at("stat_requests"s).AsArray()) {
		std::string type = request.AsMap().at("type"s).AsString();
        
        // Вызов функции вывода информации об объекте
        // Если тип запроса - Маршрут
		if (type == "Bus"s) {
			respond.push_back(std::move(BusRespond(handler, request)));
		}
        
        // Если тип запроса - Остановка
		if (type == "Stop"s) {
			respond.push_back(std::move(StopRespond(handler, request)));
		}
        
        // Если тип запроса - Визуализация
		if (type == "Map"s) {
			respond.push_back(std::move(MapImageRespond(handler, request)));
		}
	}
    
	const auto result = json::Document(std::move(respond));
    
    // Вывод полученного результата
	json::Print(result, output);
}

// Возвращает ответ на запрос с информацией автобусного маршрута
json::Dict JsonReader::BusRespond(RequestHandler& handler, const json::Node& request) {
    json::Dict respond;

    // Получение информации об автобусе
    auto bus_info = handler.GetBusStat(request.AsMap().at("name"s).AsString());
    // Получение идентификатора запроса
    int request_id = request.AsMap().at("id"s).AsInt();
    
    // Если информация о автобусе найдена
    if (bus_info.has_value()) {
        // Добавление к ответу информации
		respond.emplace("curvature"s, bus_info.value().curvature);
        respond.emplace("request_id"s, request_id);
        respond.emplace("route_length"s, bus_info.value().route_length);
        respond.emplace("stop_count"s, static_cast<int>(bus_info.value().stops));
        respond.emplace("unique_stop_count"s, static_cast<int>(bus_info.value().unique_stops));
	} else { // Если информация о автобусе не найдена
		respond.emplace("request_id"s, request_id);
		respond.emplace("error_message"s, "not found"s);
	}
    
    return respond;
}

// Возвращает ответ на запрос с информацией транспортной остановки
json::Dict JsonReader::StopRespond(RequestHandler& handler, const json::Node& request) {
	// Получаем информацию о маршрутах, проходящих через остановку
	auto stop_info = handler.GetBusesByStop(request.AsMap().at("name"s).AsString());
    // Получение идентификатора запроса
	int request_id = request.AsMap().at("id"s).AsInt();
    
    json::Dict respond;

    // Если информация о маршрутах была найдена
    if (stop_info) {
		respond.emplace("request_id"s, request_id);

        // Множество для хранения номеров маршрутов
        std::set<std::string> buses;
        // Добавление номеров маршрутов
        for (auto* bus : *stop_info) {
            buses.insert(bus->bus_number);
        }

        // Массив для хранения номеров маршрутов
        json::Array buses_array;
        // Заполнение массива номерами маршрутов
        for (auto& bus : buses) {
            buses_array.push_back(bus);
        }

        respond.emplace("buses"s, std::move(buses_array));
	} else { // Если информация об остановке не найдена
		respond.emplace("request_id"s, request_id);
		respond.emplace("error_message"s, "not found"s);
	}

    return respond;
}

json::Dict JsonReader::MapImageRespond(RequestHandler& handler, const json::Node& request) {
    // Обработка настроек визуализации
	RenderSettings render_settings = RendererDataProcessing(input_.GetRoot().AsMap().at("render_settings"s).AsMap());
    
    // Получение идентификатора запроса
	int request_id = request.AsMap().at("id"s).AsInt();
    
	renderer_->SetRenderSettings(render_settings);
    
    // Вывод SVG-изображения карты в поток
	std::stringstream stream;
	handler.RenderMap().Render(stream);
    
    json::Dict respond;

    respond.emplace("request_id"s, request_id);
    // Извлечение строки из потока и сохранение в качестве ключа
    respond.emplace("map"s, stream.str());

    return respond;
}

RenderSettings JsonReader::RendererDataProcessing(const json::Dict& settings) {
	RenderSettings renderer;
    
    // Получение ширины, высоты и отступа из настроек
    renderer.width = settings.at("width"s).AsDouble();
	renderer.height = settings.at("height"s).AsDouble();
	renderer.padding = settings.at("padding"s).AsDouble();
    
	// Получение толщины линии и радиуса остановки из настроек
    renderer.line_width = settings.at("line_width"s).AsDouble();
	renderer.stop_radius = settings.at("stop_radius"s).AsDouble();
    
    // Получение размера шрифта и смещения для подписей маршрутов из настроек
	renderer.bus_label_font_size = settings.at("bus_label_font_size"s).AsInt();
	renderer.bus_label_offset = svg::Point(settings.at("bus_label_offset"s).AsArray()[0].AsDouble(),
		settings.at("bus_label_offset"s).AsArray()[1].AsDouble());
    
    // Получение размера шрифта и смещения для подписей остановок из настроек
	renderer.stop_label_font_size = settings.at("stop_label_font_size"s).AsInt();
	renderer.stop_label_offset = svg::Point(settings.at("stop_label_offset"s).AsArray()[0].AsDouble(),
		settings.at("stop_label_offset"s).AsArray()[1].AsDouble());
    
    // Получение ширины и цвета подложки/фона из настроек
	renderer.underlayer_width = settings.at("underlayer_width"s).AsDouble();
	renderer.underlayer_color = ColorProcessing(settings.at("underlayer_color"s));
    
    // Получение палитры цветов из настроек
	for (const auto& color : settings.at("color_palette"s).AsArray()) {
		renderer.color_palette.push_back(ColorProcessing(color));
	}
    
	return renderer;
}

svg::Color JsonReader::ColorProcessing(const json::Node& color) {
    // Если цвет представлен в виде строки, то возвращаем его без изменений
	if (color.IsString()) {
		return color.AsString();
	}
    
    // Если массив из трех элементов - RGB
	if (color.AsArray().size() == 3) {
		return svg::Rgb(color.AsArray()[0].AsInt(), color.AsArray()[1].AsInt(), color.AsArray()[2].AsInt());
	}
    
    // Иначе - RGBA
	return svg::Rgba(color.AsArray()[0].AsInt(), color.AsArray()[1].AsInt(), color.AsArray()[2].AsInt(), color.AsArray()[3].AsDouble());
}

} // end of namespace transport