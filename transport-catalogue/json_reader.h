#pragma once

#include "json.h"
#include "json_builder.h"
#include "transport_catalogue.h"
#include "transport_router.h"
#include "map_renderer.h"
#include "domain.h"
#include "graph.h"
#include <vector>
#include <sstream>

using namespace std::literals;

const std::string BASE_DATA = "base_requests";
const std::string USER_REQUESTS = "stat_requests";
const std::string RENDER_SETTINGS = "render_settings";
const std::string ROUTING_SETTINGS = "routing_settings";

struct BusRouteJson {
    std::string bus_name;
    transport_catalogue::RouteType type;
    std::vector<std::string> route_stops;
};

struct RouteVistor {
    json::Dict& json;

    void operator()(const route::WaitResponse& resp) {
        json.emplace("type"s, resp.type);
        json.emplace("stop_name"s, resp.stop_name);
        json.emplace("time"s, resp.time);
    }
    void operator()(const route::BusResponse& resp) {
        json.emplace("type"s, resp.type);
        json.emplace("bus"s, resp.bus);
        json.emplace("span_count"s, resp.stop_count);
        json.emplace("time"s, resp.time);
    }
};

using BaseRequest = std::variant<std::monostate, transport_catalogue::StopWithDistances, BusRouteJson>;


class JsonReader {
public:
    explicit JsonReader(transport_catalogue::TransportCatalogue& tc) : transport_catalogue_(tc), transport_router_(tc, {}) {
    }

    size_t ReadJson(std::istream& input);
    size_t WriteJsonToStream(std::ostream& out);

    size_t ReadAndExecute(std::istream& input, std::ostream& out);

    [[nodiscard]] RendererSettings GetRendererSetting() const;


private:
    transport_catalogue::TransportCatalogue& transport_catalogue_;
    route::TransportRouter transport_router_;
    std::vector<json::Document> root_;
    std::vector<transport_catalogue::StopWithDistances> raw_stops_;
    std::vector<BusRouteJson> raw_buses_;
    graph::DirectedWeightedGraph<double> graph_;

    BaseRequest ParseAddDataNode(const json::Node& node) const;
    size_t ParseJsonToRawData();
    bool FillTransportCatalogue();
    json::Node ProcessOneUserRequestNode(const json::Node& user_request);
};

svg::Color ParseColor(const json::Node& node);
inline json::Node GetErrorNode(int id);
