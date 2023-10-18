#include "request_handler.h"

std::optional<BusInfo> RequestHandler::GetBusStat(const std::string_view &bus_name) const {
    BusInfo bi = db_.GetBusInfo(bus_name);

    if (bi.type == RouteType::NOT_SET) {
        return {};
    }

    return {bi};
}

const std::set<std::string_view> &RequestHandler::GetBusesByStop(const std::string_view &stop_name) const {
    return db_.GetBusesForStop(stop_name);
}

