#include "map_renderer.h"

#include <vector>

namespace transport {

using namespace std::literals;

// SphereProjector

bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

// Оператор, который проецирует координаты на сферу на плоскость с помощью заданных параметров
svg::Point SphereProjector::operator()(geo::Coordinates coords) const {
    return { (coords.lng - min_lon_) * zoom_coeff_ + padding_, (max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
}

// MapRenderer

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

MapRenderer::MapRenderer(const json::Node& render_settings) {
    if (render_settings.IsNull()) {
        return;
    }
    
    const json::Dict& settings = render_settings.AsDict();
    
    // Получение ширины, высоты и отступа из настроек
    render_settings_.width = settings.at("width"s).AsDouble();
    render_settings_.height = settings.at("height"s).AsDouble();
    render_settings_.padding = settings.at("padding"s).AsDouble();
    
    // Получение толщины линии и радиуса остановки из настроек
    render_settings_.stop_radius = settings.at("stop_radius"s).AsDouble();
    render_settings_.line_width = settings.at("line_width"s).AsDouble();
    
    // Получение размера шрифта и смещения для подписей маршрутов из настроек
    render_settings_.bus_label_font_size = settings.at("bus_label_font_size"s).AsInt();
    render_settings_.bus_label_offset = svg::Point(
        settings.at("bus_label_offset"s).AsArray()[0].AsDouble(),
        settings.at("bus_label_offset"s).AsArray()[1].AsDouble()
    );
    
    // Получение размера шрифта и смещения для подписей остановок из настроек
    render_settings_.stop_label_font_size = settings.at("stop_label_font_size"s).AsInt();
    render_settings_.stop_label_offset = svg::Point(
        settings.at("stop_label_offset"s).AsArray()[0].AsDouble(),
        settings.at("stop_label_offset"s).AsArray()[1].AsDouble()
    );
    
    // Получение ширины и цвета подложки/фона из настроек
    render_settings_.underlayer_color = ColorProcessing(settings.at("underlayer_color"s));
    render_settings_.underlayer_width = settings.at("underlayer_width"s).AsDouble();
    
    // Получение палитры цветов из настроек
    for (const auto& color : settings.at("color_palette"s).AsArray()) {
        render_settings_.color_palette.push_back(ColorProcessing(color));
    }
}

svg::Document MapRenderer::GetSVG(const std::map<std::string_view, Bus*>& buses) const {
    std::map<std::string_view, Stop*> all_stops;
    std::vector<geo::Coordinates> all_coords;
    svg::Document result;
    
    for (const auto& [bus_name, bus_ptr] : buses) {
        if (bus_ptr->stops.size() == 0) {
            continue;
        }
        
        for (const auto& stop : bus_ptr->stops) {
            all_stops[stop->stop_title] = stop;
            all_coords.push_back(stop->coords);
        }
    }
    
    SphereProjector sp(all_coords.begin(), all_coords.end(), render_settings_.width, render_settings_.height, render_settings_.padding);
    
    for (const auto& line : GetBusLines(buses, sp)) {
        result.Add(line);
    }
    
    for (const auto& text : GetBusLabels(buses, sp)) {
        result.Add(text);
    }
    
    for (const auto& circle : GetStopCircles(all_stops, sp)) {
        result.Add(circle);
    }
    
    for (const auto& text : GetStopLabels(all_stops, sp)) {
        result.Add(text);
    }
    
    return result;
}

std::vector<svg::Polyline> MapRenderer::GetBusLines(const std::map<std::string_view, Bus*>& buses, const SphereProjector& sp) const {
    std::vector<svg::Polyline> result;
    unsigned color_num = 0;
    
    for (auto& [_, bus] : buses) {
        if (bus->stops.size() == 0) {
            continue;
        }
        
        svg::Polyline line;
        std::vector<geo::Coordinates> points;
        for (auto stop : bus->stops) {
            line.AddPoint(sp(stop->coords));
        }
        
        line.SetFillColor("none"s);
        line.SetStrokeColor(render_settings_.color_palette[color_num]);
        line.SetStrokeWidth(render_settings_.line_width);
        line.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        line.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        if (color_num < (render_settings_.color_palette.size() - 1)) {
            ++color_num;
        }
        else {
            color_num = 0;
        }
        
        result.push_back(line);
    }
    
    return result;
}

std::vector<svg::Text> MapRenderer::GetBusLabels(const std::map<std::string_view, Bus*>& buses, const SphereProjector& sp) const {
    std::vector<svg::Text> result;
    unsigned color_num = 0;
    
    for (auto& [_, bus] : buses) {
        if (bus->stops.size() == 0) {
            continue;
        }
        
        svg::Text text_underlayer;
        svg::Text text;
        
        text_underlayer.SetData(bus->bus_number);
        text.SetData(bus->bus_number);
        text.SetFillColor(render_settings_.color_palette[color_num]);
        if (color_num < (render_settings_.color_palette.size() - 1)) {
            ++color_num;
        }
        else {
            color_num = 0;
        }
        
        text_underlayer.SetFillColor(render_settings_.underlayer_color);
        text_underlayer.SetStrokeColor(render_settings_.underlayer_color);
        text.SetFontFamily("Verdana"s);
        text_underlayer.SetFontFamily("Verdana"s);
        text.SetFontSize(render_settings_.bus_label_font_size);
        text_underlayer.SetFontSize(render_settings_.bus_label_font_size);
        text.SetFontWeight("bold"s);
        text_underlayer.SetFontWeight("bold"s);
        text.SetOffset(render_settings_.bus_label_offset);
        text_underlayer.SetOffset(render_settings_.bus_label_offset);
        text_underlayer.SetStrokeWidth(render_settings_.underlayer_width);
        text_underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        text_underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        text.SetPosition(sp(bus->stops[0]->coords));
        text_underlayer.SetPosition(sp(bus->stops[0]->coords));
        
        result.push_back(text_underlayer);
        result.push_back(text);
        
        if ((!bus->is_circular) && (bus->final_stop) && (bus->final_stop->stop_title != bus->stops[0]->stop_title)) {
            svg::Text text2 = text;
            svg::Text text2_underlayer = text_underlayer;
            
            text2.SetPosition(sp(bus->final_stop->coords));
            text2_underlayer.SetPosition(sp(bus->final_stop->coords));
            
            result.push_back(text2_underlayer);
            result.push_back(text2);
        }
    }
    
    return result;
}

std::vector<svg::Text> MapRenderer::GetStopLabels(const std::map<std::string_view, Stop*>& stops, const SphereProjector& sp) const {
    std::vector<svg::Text> result;
    
    for (auto& [_, stop] : stops) {
        svg::Text text, text_underlayer;
        
        text.SetPosition(sp(stop->coords));
        text.SetOffset(render_settings_.stop_label_offset);
        text.SetFontSize(render_settings_.stop_label_font_size);
        text.SetFontFamily("Verdana"s);
        text.SetData(stop->stop_title);
        text.SetFillColor("black"s);
        
        text_underlayer.SetPosition(sp(stop->coords));
        text_underlayer.SetOffset(render_settings_.stop_label_offset);
        text_underlayer.SetFontSize(render_settings_.stop_label_font_size);
        text_underlayer.SetFontFamily("Verdana"s);
        text_underlayer.SetData(stop->stop_title);
        text_underlayer.SetFillColor(render_settings_.underlayer_color);
        text_underlayer.SetStrokeColor(render_settings_.underlayer_color);
        text_underlayer.SetStrokeWidth(render_settings_.underlayer_width);
        text_underlayer.SetStrokeLineCap(svg::StrokeLineCap::ROUND);
        text_underlayer.SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        
        result.push_back(text_underlayer);
        result.push_back(text);
    }
    
    return result;
}

std::vector<svg::Circle> MapRenderer::GetStopCircles(const std::map<std::string_view, Stop*>& stops, const SphereProjector& sp) const {
    std::vector<svg::Circle> result;
    
    for (auto& [_, stop] : stops) {
        svg::Circle circle;
        
        circle.SetCenter(sp(stop->coords));
        circle.SetRadius(render_settings_.stop_radius);
        circle.SetFillColor("white"s);
        
        result.push_back(circle);
    }
    
    return result;
}

/*
*   Сериализация
*/

// Метод получает состояние объектов для сериализации
const RenderSettings& MapRenderer::GetRenderSettings() const {
    return render_settings_;
}

namespace {

serialize::Point PointSerialize(const svg::Point& point) {
    serialize::Point result;
    
    result.set_x( point.x );
    result.set_y( point.y );
    
    return result;
}

serialize::Color ColorSerialize(const svg::Color& color) {
    serialize::Color result;
    
    if (std::holds_alternative<std::string>(color)) {
        result.set_name(std::get<std::string>(color));
    }
    else {
        bool is_rgb = std::holds_alternative<svg::Rgb>(color);
        
        if (is_rgb) {
            svg::Rgb rgb = std::get<svg::Rgb>(color);
            
            result.mutable_rgb()->set_red(rgb.red);
            result.mutable_rgb()->set_green(rgb.green);
            result.mutable_rgb()->set_blue(rgb.blue);
        }
        else {
            svg::Rgba rgba = std::get<svg::Rgba>(color);
            
            result.mutable_rgba()->set_red(rgba.red);
            result.mutable_rgba()->set_green(rgba.green);
            result.mutable_rgba()->set_blue(rgba.blue);
            result.mutable_rgba()->set_opacity(rgba.opacity);
        }
    }
    
    return result;
}

} // end of namespace

serialize::RenderSettings MapRenderer::RenderSettingSerialize(const RenderSettings& settings) const {
    serialize::RenderSettings result;
    
    result.set_width(settings.width);
    result.set_height(settings.height);
    result.set_padding(settings.padding);
    
    result.set_line_width(settings.line_width);
    result.set_stop_radius(settings.stop_radius);
    
    result.set_bus_label_font_size(settings.bus_label_font_size);
    *result.mutable_bus_label_offset() = PointSerialize(settings.bus_label_offset);
    
    result.set_stop_label_font_size(settings.stop_label_font_size);
    *result.mutable_stop_label_offset() = PointSerialize(settings.stop_label_offset);
    
    *result.mutable_underlayer_color() = ColorSerialize(settings.underlayer_color);
    result.set_underlayer_width(settings.underlayer_width);
    
    for (const auto& c : settings.color_palette) {
        *result.add_color_palette() = ColorSerialize(c);
    }
    
    return result;
}

} // end of namespace transport