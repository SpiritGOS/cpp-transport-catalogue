#include "transport_catalogue.h"

#include <iomanip>
#include <iostream>
#include <numeric>
#include <set>
#include <utility>

namespace transport_catalogue {

void TransportCatalogue::AddStop(const std::string& name,
                                 const geo::Coordinates coords) {
    const Stop stop{name, coords};

    AddStop(stop);
}

void TransportCatalogue::AddStop(const Stop& stop) {
    if (stops_index_.count(stop.stop_name) > 0) return;

    const Stop* ptr = &stops_.emplace_back(stop);

    std::string_view stop_name(ptr->stop_name);
    stops_index_.emplace(stop_name, ptr);
}

std::pair<bool, const Stop&> TransportCatalogue::FindStop(
    const std::string_view name) const {
    const auto iter = stops_index_.find(name);
    if (iter == stops_index_.end()) return {false, EMPTY_STOP};

    return {true, *iter->second};
}

bool TransportCatalogue::AddBus(const BusRoute& bus_route) {
    const auto iter = routes_index_.find(bus_route.bus_name);
    if (iter != routes_index_.end()) return false;

    const BusRoute* ptr = &bus_routes_.emplace_back(bus_route);

    std::string_view bus_name(ptr->bus_name);
    routes_index_.emplace(bus_name, ptr);

    for (const Stop* stop : ptr->route_stops) {
        stop_and_buses_[stop->stop_name].insert(bus_name);
    }

    return true;
}

const BusRoute& TransportCatalogue::FindBus(std::string_view name) {
    const auto iter = routes_index_.find(name);
    if (iter == routes_index_.end()) return EMPTY_BUS_ROUTE;

    return *iter->second;
}

BusInfo TransportCatalogue::GetBusInfo(std::string_view bus_name) const {
    BusInfo result;
    result.type = RouteType::NOT_SET;

    auto iter = routes_index_.find(bus_name);
    if (iter == routes_index_.end()) return result;

    const BusRoute& route = *iter->second;

    std::set<const Stop*> unique_stops(route.route_stops.begin(),
                                       route.route_stops.end());
    result.unique_stops = unique_stops.size();

    double length_geo = 0.0;
    size_t length_meters = 0;
    for (auto first = route.route_stops.begin();
         first != route.route_stops.end(); ++first) {
        auto second = std::next(first);
        if (second == route.route_stops.end()) break;

        length_geo +=
            ComputeDistance((**first).coordinates, (**second).coordinates);

        length_meters +=
            GetDistanceBetweenStops((**first).stop_name, (**second).stop_name);
        if (route.type == RouteType::RETURN_ROUTE) {
            length_meters += GetDistanceBetweenStops((**second).stop_name,
                                                     (**first).stop_name);
        }
    }

    result.stops_number = route.route_stops.size();
    if (route.type == RouteType::RETURN_ROUTE) {
        result.stops_number *= 2;
        result.stops_number -= 1;
        length_geo *= 2;
    }
    result.route_length = length_meters;
    result.curvature = static_cast<double>(length_meters) / length_geo;
    result.bus_name = route.bus_name;
    result.type = route.type;

    return result;
}

const std::set<std::string_view>& TransportCatalogue::GetBusesForStop(
    std::string_view stop) const {
    const auto iter = stop_and_buses_.find(stop);

    if (iter == stop_and_buses_.end()) {
        return EMPTY_BUS_ROUTE_SET;
    }

    return iter->second;
}

bool TransportCatalogue::SetDistanceBetweenStops(std::string_view stop,
                                                 std::string_view other_stop,
                                                 int dist) {
    auto iter_stop = stops_index_.find(stop);
    auto iter_other = stops_index_.find(other_stop);
    if (iter_stop == stops_index_.end() || iter_other == stops_index_.end())
        return false;

    StopsPointers direct{};
    direct.stop = iter_stop->second;
    direct.other = iter_other->second;
    stops_distance_index_[direct] = dist;

    StopsPointers reverse{};
    reverse.stop = direct.other;
    reverse.other = direct.stop;
    auto iter_rev = stops_distance_index_.find(reverse);
    if (iter_rev == stops_distance_index_.end()) {
        stops_distance_index_[reverse] = dist;
    }

    return true;
}

int TransportCatalogue::GetDistanceBetweenStops(
    std::string_view stop, std::string_view other_stop) const {
    const auto iter_stop = stops_index_.find(stop);
    const auto iter_other = stops_index_.find(other_stop);
    if (iter_stop == stops_index_.end() || iter_other == stops_index_.end())
        return -1;

    StopsPointers direct{};
    direct.stop = iter_stop->second;
    direct.other = iter_other->second;

    auto iter_dist = stops_distance_index_.find(direct);
    if (iter_dist == stops_distance_index_.end()) return -1;

    return iter_dist->second;
}

const std::map<std::string_view, const BusRoute*>
TransportCatalogue::GetAllRoutesIndex() const {
    std::map<std::string_view, const BusRoute*> result(routes_index_.begin(),
                                                       routes_index_.end());
    return result;
}

const std::map<std::string_view, const Stop*>
TransportCatalogue::GetAllStopsIndex() const {
    std::map<std::string_view, const Stop*> result(stops_index_.begin(),
                                                   stops_index_.end());
    return result;
}

}  // namespace transport_catalogue
