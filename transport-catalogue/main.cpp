#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"
#include "serialization.h"

#include <fstream>
#include <iostream>

using namespace std::literals;

void PrintUsage(std::ostream& stream = std::cerr) {
    stream << "Usage: transport_catalogue [make_base|process_requests]\n"sv;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage();
        
        return 1;
    }

    const std::string_view mode(argv[1]);

    if (mode == "make_base"sv) {
        transport::Catalogue db;
        
        transport::JsonReader data(json::Load(std::cin));
        data.ImportData(db);
        
        transport::MapRenderer renderer(data.GetRenderSettings());
        transport::Router router(data.GetRoutingSettings(), db);
        
        std::ofstream fout(data.GetSerializationSettings().AsDict().at("file"s).AsString(), std::ios::binary);
        if (fout.is_open()) {
            SerializeDB(db, renderer, router, fout);
        }
        else {
            throw "File opening error";
        }
    }
    else if (mode == "process_requests"sv) {
        transport::JsonReader data(json::Load(std::cin));
        std::ifstream db_file(data.GetSerializationSettings().AsDict().at("file"s).AsString(), std::ios::binary);
        
        if (db_file) {
            auto [db, renderer, router, graph, vertex] = DeserializeDB(db_file);
            
            router.SetGraph(std::move(graph), std::move(vertex));
            transport::RequestHandler handler(db, renderer, router);
            handler.DatabaseRespond(data.GetStatRequest(), std::cout);
        }
        else {
            throw "File opening error";
        }
    }
    else {
        PrintUsage();
        
        return 1;
    }
}