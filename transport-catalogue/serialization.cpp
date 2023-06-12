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

json::Node PointDeserialize(const serialize::Point& point) {
    return json::Node(json::Array{ point.x(), point.y() });
}

json::Node ColorDeserialize(const serialize::Color& color) {
    if (!color.name().empty()) {
        return json::Node(color.name());
    }
    else if (color.has_rgb()) {
        const serialize::RGB& rgb = color.rgb();
        
        return json::Node(json::Array{ rgb.red(), rgb.green(), rgb.blue() });
    }
    else if (color.has_rgba()) {
        const serialize::RGBA& rgba = color.rgba();
        
        return json::Node(json::Array{ rgba.red(), rgba.green(), rgba.blue(), rgba.opacity() });
    }
    else {
        return json::Node("none"s);
    }
}

json::Node ColorDeserialize(const google::protobuf::RepeatedPtrField<serialize::Color>& cv) {
    json::Array result;
    result.reserve(cv.size());
    
    for (const auto& c : cv) {
        result.emplace_back(ColorDeserialize(c));
    }
    
    return json::Node(std::move(result));
}

} // end of namespace

json::Node RenderSettingsDeserialize(const serialize::RenderSettings& rs) {
    return json::Builder{}.StartDict()
        .Key("width"s).Value( rs.width() )
        .Key("height"s).Value( rs.height() )
        .Key("padding"s).Value( rs.padding() )
        
        .Key("stop_radius"s).Value( rs.stop_radius() )
        .Key("line_width"s).Value( rs.line_width() )
        
        .Key("bus_label_font_size"s).Value( rs.bus_label_font_size() )
        .Key("bus_label_offset"s).Value( PointDeserialize(rs.bus_label_offset()) )
        
        .Key("stop_label_font_size"s).Value( rs.stop_label_font_size() )
        .Key("stop_label_offset"s).Value( PointDeserialize(rs.stop_label_offset()) )
        
        .Key("underlayer_color"s).Value( ColorDeserialize(rs.underlayer_color()) )
        .Key("underlayer_width"s).Value( rs.underlayer_width() )
        
        .Key("color_palette"s).Value( ColorDeserialize(rs.color_palette()) )
        .EndDict().Build();
}

json::Node RouterSettingsDeserialize(const serialize::Router& router) {
    return json::Builder{}.StartDict()
        .Key("bus_wait_time"s).Value( router.router_settings().bus_wait_time() )
        .Key("bus_velocity"s).Value( router.router_settings().bus_velocity() )
        .EndDict().Build();
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
    serialize::TransportCatalogue data;
    data.ParseFromIstream(&input);
    
    Catalogue db;
    MapRenderer renderer(RenderSettingsDeserialize( data.render_settings() ));
    Router router(RouterSettingsDeserialize( data.router() ));
    
    StopDeserialize(db, data);
    RouteDeserialize(db, data);
    
    return { std::move(db), std::move(renderer), std::move(router), GraphDeserialize(data.router()), VertexDeserialize(data.router()) };
}