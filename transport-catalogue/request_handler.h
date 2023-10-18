#pragma once

#include "transport_catalogue.h"
#include "svg.h"
#include <set>
#include <string_view>

using namespace transport_catalogue;

class RequestHandler {
public:
    RequestHandler(const TransportCatalogue& db)
        : db_(db) {
    }

    // Возвращает информацию о маршруте (запрос Bus)
    std::optional<BusInfo> GetBusStat(const std::string_view& bus_name) const;

    // Возвращает маршруты, проходящие через
    const std::set<std::string_view>& GetBusesByStop(const std::string_view& stop_name) const;

    // Этот метод будет нужен в следующей части итогового проекта
    svg::Document RenderMap() const;

private:
    // RequestHandler использует агрегацию объектов "Транспортный Справочник" и "Визуализатор Карты"
    const TransportCatalogue& db_;
    //const renderer::MapRenderer& renderer_;
};
