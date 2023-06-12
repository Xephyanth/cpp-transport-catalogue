#define _USE_MATH_DEFINES
#include "geo.h"

#include <cmath>

namespace geo {

double ComputeDistance(Coordinates from, Coordinates to) {
    if (from == to) {
        return 0;
    }
    
    static const double dr = M_PI / 180.;
    static const int earth_radius = 6371000;
    
    return std::acos(std::sin(from.lat * dr) * std::sin(to.lat * dr) +
                std::cos(from.lat * dr) * std::cos(to.lat * dr) *
                std::cos(std::abs(from.lng - to.lng) * dr)) * earth_radius;
}

} // end of namespace geo