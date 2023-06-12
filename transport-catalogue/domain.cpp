#include "domain.h"

namespace transport {

Stop::Stop(const std::string& title, const geo::Coordinates& coordinates)
    : stop_title(title), coords(coordinates) {}

int Stop::GetStopsDistance(Stop* next) {
    if (stops_distances.count(next->stop_title)) {
        return stops_distances.at(next->stop_title);
    }
    else if (next->stops_distances.count(this->stop_title)) {
        return next->stops_distances.at(this->stop_title);
    }
    else {
        return 0;
    }
}

Bus::Bus(const std::string& number, std::vector<Stop*> stops, bool circle)
    : bus_number(number), stops(stops), is_circular(circle) {}

} // end of namespace transport