#pragma once
#include <string>
#include "geo.h"
#include <vector>
#include <set>

namespace transport_catalogue {

    struct Stop {
        std::string stop_name;
        geo::Coordinates coordinates;
    };

    struct StopDistanceData {
        std::string other_stop_name;
        size_t distance;
    };

    struct StopWithDistances : Stop {
        std::vector<StopDistanceData> distances;
    };

    const Stop EMPTY_STOP{};

    enum RouteType {
        NOT_SET,
        CIRCLE_ROUTE,
        RETURN_ROUTE
    };

    struct BusRoute {
        std::string bus_name;
        RouteType type;
        std::vector<const Stop *> route_stops;
    };

    const BusRoute EMPTY_BUS_ROUTE{};
    const std::set<std::string_view> EMPTY_BUS_ROUTE_SET{};

    struct BusInfo {
        std::string_view bus_name;
        RouteType type;
        size_t stops_number;
        size_t unique_stops;
        size_t route_length;
        double curvature;
    };

    std::ostream &operator<<(std::ostream &os, const BusInfo &bus_info);

}