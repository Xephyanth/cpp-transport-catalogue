#pragma once

#include <fstream>
#include <iostream>
#include <string>

#include "json_reader.h"
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

using namespace transport;

/*
*   Сериализация
*/

void SerializeDB(const Catalogue& db, const MapRenderer& renderer, const Router& router, std::ostream& output);

/*
*   Десериализация
*/

using DeserializeData = std::tuple<Catalogue, MapRenderer, Router, graph::DirectedWeightedGraph<double>, std::map<std::string, graph::VertexId>>;

DeserializeData DeserializeDB(std::istream& input);