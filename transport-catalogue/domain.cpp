#include "domain.h"
#include <iomanip>


namespace transport_catalogue {

    std::ostream& operator<<(std::ostream& os, const BusInfo& bus_info) {
        using namespace std::literals;
        double length = bus_info.route_length;

        os  << "Bus "s << bus_info.bus_name << ": "s << bus_info.stops_number << " stops on route, "s
            << bus_info.unique_stops << " unique stops, "s << std::setprecision(6) << length << " route length, "s
            << std::setprecision(6) << bus_info.curvature << " curvature"s << std::endl;

        return os;
    }


}
