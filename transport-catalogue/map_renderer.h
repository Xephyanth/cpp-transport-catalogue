#pragma once

#include "domain.h"
#include "svg.h"

#include <map>
#include <set>
#include <algorithm>

namespace transport {

struct RenderSettings {
    double width;
    double height;
    double padding;
    
    double line_width;
    double stop_radius;
    
    int bus_label_font_size;
    svg::Point bus_label_offset;
    
    int stop_label_font_size;
    svg::Point stop_label_offset;
    
    svg::Color underlayer_color;
    double underlayer_width;
    
    std::vector<svg::Color> color_palette;
};

inline const double EPSILON = 1e-6;

bool IsZero(double value);

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end, double max_width,
        double max_height, double padding)
        : padding_(padding) {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
            return lhs.lng < rhs.lng;
                });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
            return lhs.lat < rhs.lat;
                });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(geo::Coordinates coords) const;

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

class MapRenderer {
public:
    // Метод для установки настроек отрисовки
    void SetRenderSettings(RenderSettings& settings);

    // Метод для получения SVG-документа на основе карты маршрутов
    svg::Document GetSVG(const std::map<std::string_view, const Bus*>& routes) const;
    
    // Вспомогательная функция для проецирования автобусных маршрутов
    void FillBuses(const std::map<std::string_view, const Bus*>& routes, SphereProjector projector, svg::Document& result) const;
    // Вспомогательная функция для проецирования автобусных остановок
    void FillStops(std::map<std::string_view, const Stop*> unique_stops, SphereProjector projector, svg::Document& result) const;

private:
    // Настройки отрисовки карты
    RenderSettings render_settings_;
};

} // end of namespace transport