#pragma once

#include "transport_catalogue.h"

namespace transport {

void DataReader(std::istream& input, Catalogue& catalogue);

namespace processing {

// Обработка строки запроса добавления остановки
std::tuple<std::string, geo::Coordinates, std::string> ParseStop(std::string& requests);

// Обработка строки запроса добавления маршрута
std::tuple<std::string, std::vector<std::string>, bool> ParseRoute(std::string& requests);

// Обработка строки запроса добавления расстояния между остановками
void ParseDistance(const std::string& stop_title, std::string& requests, Catalogue& catalogue);

} // end of namespace processing

} // end of namespace transport