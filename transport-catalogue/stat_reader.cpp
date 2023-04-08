#include "stat_reader.h"

#include <iostream>
#include <iomanip>

using std::literals::string_literals::operator""s;

namespace transport {

void DataOutput(std::ostream& output, Catalogue& catalogue) {
    size_t number_of_requests = 0;
    std::cin >> number_of_requests;
    
    for (size_t i = 0; i < number_of_requests; ++i) {
        std::string keyword;
        std::cin >> keyword;
        
        std::string request;
        std::getline(std::cin, request);
        
        // Вывод информации о маршруте
        if (keyword == "Bus"s) {
            std::string route_number = request.substr(1, request.npos);
            
            if (!catalogue.FindRoute(route_number)) {
                output << "Bus "s << route_number << ": not found"s << std::endl;
                continue;
            }
            
            const auto& route_info = catalogue.FindInformation(route_number);
            output << "Bus "s << route_number << ": "s
                   << route_info.number_of_stops << " stops on route, "s
                   << route_info.unique_number_of_stops << " unique stops, "s
                   << std::setprecision(6) << route_info.length << " route length, "s 
                   << route_info.curvature << " curvature"s << std::endl;
        }
        // Вывод информации об остановке
        else if (keyword == "Stop"s) {
            std::string stop_title = request.substr(1, request.npos);
            
            if (!catalogue.FindStop(stop_title)) {
                output << "Stop "s << stop_title << ": not found"s << std::endl;
                continue;
            }
            
            std::set<std::string> buses_on_stop = catalogue.FindBusesByStop(stop_title);
            
            if (buses_on_stop.empty()) {
                output << "Stop "s << stop_title << ": no buses"s << std::endl;
            } else {
                output << "Stop "s << stop_title << ": buses "s;
                for (const auto& bus : buses_on_stop) {
                    output << bus << " "s;
                }
                output << std::endl;
            }
        }
    }
}

} // end of namespace transport