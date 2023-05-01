#include "map_renderer.h"

using namespace std::literals;

namespace transport {

bool IsZero(double value) {
	return std::abs(value) < EPSILON;
}

// Оператор, который проецирует координаты на сферу на плоскость с помощью заданных параметров
svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
	return { (coords.lng - min_lon_) * zoom_coeff_ + padding_, (max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
}

// Метод для установки настроек отрисовки
void MapRenderer::SetRenderSettings(RenderSettings& settings) {
	render_settings_ = std::move(settings);
}

svg::Document MapRenderer::GetSVG(const std::map<std::string_view, const Bus*>& routes) const {
	svg::Document result;
    
    // Создание словаря уникальных остановок, встречающихся на маршрутах
	std::map<std::string_view, const Stop*> unique_stops;
	for (const auto& [_, route] : routes) {
		for (const auto stop : route->stops) {
			unique_stops[stop->stop_title] = stop;
		}
	}

    // Получение координат каждой остановки
	std::vector<geo::Coordinates> coordinates;
	for (const auto [_, stop_ptr] : unique_stops) {
		coordinates.push_back(stop_ptr->coords);
	}
    // Проекция координат на сферу
	SphereProjector projector(coordinates.begin(), coordinates.end(), render_settings_.width, render_settings_.height, render_settings_.padding);

	size_t counter = 0;
    // Отрисовка линий маршрута на карте
	for (const auto& [_, route] : routes) {
        // Пропуск маршрута, если он не имеет остановок
		if (route->stops.empty()) {
			continue;
		}
        
        // Получение всех остановок маршрута в порядке движения автобуса
		std::vector<const Stop*> route_stops{ route->stops.begin(), route->stops.end() };
        // Добавление последней остановки, если маршрут не круговой
		if (!route->is_circular) {
			route_stops.insert(route_stops.end(), next(route->stops.rbegin()), route->stops.rend());
		}

        // Выбор цвета для маршрута из палитры цветов и отрисовка линии, соединяющей все остановки маршрута
		size_t color_index = counter % render_settings_.color_palette.size();

		svg::Polyline polyline;
		polyline.SetFillColor(svg::NoneColor)
			.SetStrokeColor(render_settings_.color_palette[color_index])
			.SetStrokeWidth(render_settings_.line_width)
			.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
			.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

		for (const auto stop : route_stops) {
			polyline.AddPoint(projector(stop->coords));
		}
        
        // Добавление линий в результирующий набор
		result.Add(polyline);
        
		++counter;
	}

	counter = 0;
    
    // Отрисовка подписей названий маршрутов
	for (const auto& [_, route] : routes) {
        // Пропуск маршрутов без остановок
		if (route->stops.empty()) {
			continue;
		}
        
		size_t color_index = counter % render_settings_.color_palette.size();
        
        // Элемент подложки
		svg::Text underlayer;
		underlayer.SetData(route->bus_number)
			.SetPosition(projector(route->stops.front()->coords))
			.SetOffset(render_settings_.bus_label_offset)
			.SetFillColor(render_settings_.underlayer_color)
			.SetStrokeColor(render_settings_.underlayer_color)
			.SetFontFamily("Verdana"s)
			.SetFontSize(render_settings_.bus_label_font_size)
			.SetFontWeight("bold"s)
			.SetStrokeWidth(render_settings_.underlayer_width)
			.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
			.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        // Элемент текста
		svg::Text text;
		text.SetData(route->bus_number)
			.SetPosition(projector(route->stops.front()->coords))
			.SetOffset(render_settings_.bus_label_offset)
			.SetFontSize(render_settings_.bus_label_font_size)
			.SetFontFamily("Verdana"s)
			.SetFontWeight("bold"s)
			.SetFillColor(render_settings_.color_palette[color_index]);

        // Добавляем текстовые элементы к результирующему набору
		result.Add(underlayer);
		result.Add(text);

        // Если маршрут не круговой и первая и последняя остановки не совпадают, 
        // добавляем текстовые элементы для последней остановки
		if (!route->is_circular && route->stops.front() != route->stops.back()) {
			result.Add(underlayer.SetPosition(projector(route->stops.back()->coords)));
			result.Add(text.SetPosition(projector(route->stops.back()->coords)));
		}
        
		++counter;
	}

    // Отрисовка точек с остановками
	for (const auto& [_, stop] : unique_stops) {
		svg::Circle stop_symbol;
        
        // Центр окружности и ее радиус
		stop_symbol.SetCenter(projector(stop->coords))
			.SetRadius(render_settings_.stop_radius)
			.SetFillColor("white"s);
        
		result.Add(stop_symbol);
	}

    // Отрисовка подписей остановок
	for (const auto& [_, stop] : unique_stops) {
        // Подложка подписи
		svg::Text underlayer;
		underlayer.SetData(stop->stop_title)
			.SetPosition(projector(stop->coords))
			.SetOffset(render_settings_.stop_label_offset)
			.SetFontSize(render_settings_.stop_label_font_size)
			.SetFontFamily("Verdana"s)
			.SetFillColor(render_settings_.underlayer_color)
			.SetStrokeColor(render_settings_.underlayer_color)
			.SetStrokeWidth(render_settings_.underlayer_width)
			.SetStrokeLineCap(svg::StrokeLineCap::ROUND)
			.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        // Текст подписи
		svg::Text text;
		text.SetData(stop->stop_title)
			.SetPosition(projector(stop->coords))
			.SetOffset(render_settings_.stop_label_offset)
			.SetFontSize(render_settings_.stop_label_font_size)
			.SetFontFamily("Verdana"s)
			.SetFillColor("black"s);

        // Добавление подписей к результирующему набору
		result.Add(underlayer);
		result.Add(text);
	}
    
	return result;
}

} // end of namespace transport