#include "serialization.h"

using namespace std::literals;

/*
*   Сериализация
*/

void SerializeDB(const Catalogue& db, const MapRenderer& renderer, const Router& router, std::ostream& output)
{
    serialize::TransportCatalogue data;
    
    for (const auto& [_, stop] : db.GetSortedAllStops()) {
        *data.add_stop() = db.StopsSerialize(stop);
    }
    
    for (const auto& [_, bus] : db.GetSortedAllBuses()) {
        *data.add_bus() = db.BusesSerialize(bus);
    }
    
    *data.mutable_render_settings() = renderer.RenderSettingSerialize(renderer.GetRenderSettings());
    *data.mutable_router() = router.RouterSerialize(router);
    
    data.SerializeToOstream(&output);
}

/*
*   Десериализация
*/

void DistancesDeserialize(Catalogue& db, const serialize::TransportCatalogue& data) {
    for (size_t i = 0; i < data.stop_size(); ++i) {
        const serialize::Stop& stop = data.stop(i);
        Stop* from = db.FindStop(stop.name());
        
        for (size_t j = 0; j < stop.near_stop_size(); ++j) {
            db.SetStopsDistance(from, db.FindStop(stop.near_stop(j)), stop.distance(j));
        }
    }
}

void StopDeserialize(Catalogue& db, const serialize::TransportCatalogue& data) {
    for (size_t i = 0; i < data.stop_size(); ++i) {
        const serialize::Stop& stop = data.stop(i);
        
        db.AddStop(stop.name(), { stop.coordinate(0), stop.coordinate(1) });
    }
    
    DistancesDeserialize(db, data);
}

void RouteDeserialize(Catalogue& db, const serialize::TransportCatalogue& data) {
    for (size_t i = 0; i < data.bus_size(); ++i) {
        const serialize::Bus& bus_i = data.bus(i);
        std::vector<Stop*> stops(bus_i.stop_size());
        for (size_t j = 0; j < stops.size(); ++j) {
            stops[j] = db.FindStop(bus_i.stop(j));
        }
        
        db.AddRoute(bus_i.name(), stops, bus_i.is_circle());
        if (!bus_i.final_stop().empty()) {
            Bus* bus = db.FindRoute(bus_i.name());
            bus->final_stop = db.FindStop(bus_i.final_stop());
        }
    }
}

namespace {

svg::Point PointDeserialize(const serialize::Point& point) {
    return svg::Point({ point.x(), point.y() });
}

svg::Color ColorDeserialize(const serialize::Color& color) {
    if (!color.name().empty()) {
        return svg::Color(color.name());
    }
    else if (color.has_rgb()) {
        const serialize::RGB& rgb = color.rgb();
        
        return svg::Rgb({ static_cast<uint8_t>(rgb.red()),
                          static_cast<uint8_t>(rgb.green()),
                          static_cast<uint8_t>(rgb.blue())
                        });
    }
    else if (color.has_rgba()) {
        const serialize::RGBA& rgba = color.rgba();
        
        return svg::Rgba({ static_cast<uint8_t>(rgba.red()),
                           static_cast<uint8_t>(rgba.green()),
                           static_cast<uint8_t>(rgba.blue()),
                           rgba.opacity()
                        });
    }
    else {
        return svg::Color("none"s);
    }
}

std::vector<svg::Color> ColorDeserialize(const google::protobuf::RepeatedPtrField<serialize::Color>& cv) {
    std::vector<svg::Color> result;
    result.reserve(cv.size());
    
    for (const auto& c : cv) {
        result.emplace_back(ColorDeserialize(c));
    }
    
    return std::move(result);
}

} // end of namespace

MapRenderer RenderSettingsDeserialize(const serialize::RenderSettings& rs) {
    RenderSettings result;
    
    result.width = rs.width();
    result.height = rs.height();
    result.padding = rs.padding();
    
    result.stop_radius = rs.stop_radius();
    result.line_width = rs.line_width();
    
    result.bus_label_font_size = rs.bus_label_font_size();
    result.bus_label_offset = PointDeserialize(rs.bus_label_offset());
    
    result.stop_label_font_size = rs.stop_label_font_size();
    result.stop_label_offset = PointDeserialize(rs.stop_label_offset());
    
    result.underlayer_color = ColorDeserialize(rs.underlayer_color());
    result.underlayer_width = rs.underlayer_width();
    
    result.color_palette = ColorDeserialize(rs.color_palette());
    
    return result;
}

Router RouterSettingsDeserialize(const serialize::Router& router) {
    RouteSettings result;
    
    result.bus_wait_time = router.router_settings().bus_wait_time();
    result.bus_velocity = router.router_settings().bus_velocity();
    
    return result;
}

graph::DirectedWeightedGraph<double> GraphDeserialize(const serialize::Router& router) {
    const serialize::Graph& g = router.graph();
    std::vector<graph::Edge<double>> edges(g.edge_size());
    std::vector<std::vector<graph::EdgeId>> incidence_lists(g.vertex_size());
    
    for (size_t i = 0; i < edges.size(); ++i) {
        const serialize::Edge& e = g.edge(i);
        edges[i] = { e.name(), static_cast<size_t>(e.quality()),
        static_cast<size_t>(e.from()), static_cast<size_t>(e.to()), e.weight() };
    }
    
    for (size_t i = 0; i < incidence_lists.size(); ++i) {
        const serialize::Vertex& v = g.vertex(i);
        incidence_lists[i].reserve(v.edge_id_size());
        
        for (const auto& id : v.edge_id()) {
            incidence_lists[i].push_back(id);
        }
    }
    
    return graph::DirectedWeightedGraph<double>(edges, incidence_lists);
}

std::map<std::string, graph::VertexId> VertexDeserialize(const serialize::Router& router) {
    std::map<std::string, graph::VertexId> result;
    
    for (const auto& s : router.stop_id()) {
        result[s.name()] = s.id();
    }
    
    return result;
}

DeserializeData DeserializeDB(std::istream& input) {
    Catalogue db;
    
    serialize::TransportCatalogue data;
    data.ParseFromIstream(&input);
    
    MapRenderer renderer(RenderSettingsDeserialize( data.render_settings() ));
    Router router(RouterSettingsDeserialize( data.router() ));
    
    StopDeserialize(db, data);
    RouteDeserialize(db, data);
    
    return { std::move(db), std::move(renderer), std::move(router), GraphDeserialize(data.router()), VertexDeserialize(data.router()) };
}