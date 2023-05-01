#include "json_reader.h"
#include "request_handler.h"
#include "map_renderer.h"

#include <iostream>

using namespace std;

int main() {
    transport::Catalogue db;
    transport::MapRenderer renderer;
    
    transport::JsonReader input(std::cin, &db, &renderer);
    
    input.DatabaseRespond(std::cout);
}