#include "json_reader.h"

using namespace std::literals;

size_t JsonReader::ReadJson(std::istream& input) {
    size_t result = 0;

    try {
        json::Document doc = json::Load(input);
        if (doc.GetRoot().IsMap()) {
            result = doc.GetRoot().AsMap().size();
            root_.emplace_back(std::move(doc));
        }
    } catch (const json::ParsingError& e) {
        std::cerr << e.what() << std::endl;
    }

    return result;
}

size_t JsonReader::ReadAndExecute(std::istream& input, std::ostream& out) {
    if (!ReadJson(input)) {
        return 0;
    }
    size_t result = ParseJsonToRawData();
    
    FillTransportCatalogue();
    WriteJsonToStream(out);
    return result;
}

size_t JsonReader::ParseJsonToRawData() {
    size_t result = 0;

    const json::Node& root_node = root_.back().GetRoot();
    if (!root_node.IsMap()) {
        throw json::ParsingError(
            "Error reading JSON data for database filling.");
    }
    const json::Dict& dict = root_node.AsMap();
    auto iter = dict.find(BASE_DATA);
    if (iter == dict.end() || !(iter->second.IsArray())) {
        throw json::ParsingError(
            "Error reading JSON data for database filling.");
    }

    const json::Array& nodes = iter->second.AsArray();
    for (const auto& node : nodes) {
        using namespace transport_catalogue;
        BaseRequest data = std::move(ParseAddDataNode(node));
        if (auto* stop = std::get_if<StopWithDistances>(&data)) {
            raw_stops_.emplace_back(std::move(*stop));
        } else if (auto* bus = std::get_if<BusRouteJson>(&data)) {
            raw_buses_.emplace_back(std::move(*bus));
        } else {
            throw json::ParsingError(
                "Error reading JSON data for database filling.");
        }
        ++result;
    }

    return result;
}

BaseRequest JsonReader::ParseAddDataNode(const json::Node& node) const {
    using namespace transport_catalogue;
    if (!node.IsMap()) return {};
    const json::Dict& dict = node.AsMap();
    const auto type_i = dict.find("type"s);
    if (type_i == dict.end()) return {};
    if (type_i->second == "Stop"s) {
        StopWithDistances stop;

        if (const auto name_i = dict.find("name"s);
            name_i != dict.end() && name_i->second.IsString()) {
            stop.stop_name = name_i->second.AsString();
        } else
            return {};

        if (const auto lat_i = dict.find("latitude"s);
            lat_i != dict.end() && lat_i->second.IsDouble()) {
            stop.coordinates.lat = lat_i->second.AsDouble();
        } else
            return {};

        if (const auto lng_i = dict.find("longitude"s);
            lng_i != dict.end() && lng_i->second.IsDouble()) {
            stop.coordinates.lng = lng_i->second.AsDouble();
        } else
            return {};

        const auto dist_i = dict.find("road_distances"s);
        if (dist_i != dict.end() && !(dist_i->second.IsMap())) return {};
        for (const auto& [other_name, other_dist] : dist_i->second.AsMap()) {
            if (!other_dist.IsInt()) return {};
            stop.distances.emplace_back(StopDistanceData{
                other_name, static_cast<size_t>(other_dist.AsInt())});
        }
        return {stop};

    } else if (type_i->second == "Bus"s) {
        BusRouteJson route;

        if (const auto name_i = dict.find("name"s);
            name_i != dict.end() && name_i->second.IsString()) {
            route.bus_name = name_i->second.AsString();
        } else
            return {};
        if (const auto route_i = dict.find("is_roundtrip"s);
            route_i != dict.end() && route_i->second.IsBool()) {
            route.type = route_i->second.AsBool() ? RouteType::CIRCLE_ROUTE
                                                  : RouteType::RETURN_ROUTE;
        } else
            return {};

        const auto stops_i = dict.find("stops"s);
        if (stops_i != dict.end() && !(stops_i->second.IsArray())) return {};
        for (const auto& stop_name : stops_i->second.AsArray()) {
            if (!stop_name.IsString()) return {};
            route.route_stops.emplace_back(stop_name.AsString());
        }

        return {route};

    } else {
        return {};
    }
}

bool JsonReader::FillTransportCatalogue() {
    for (const auto& stop : raw_stops_) {
        transport_catalogue_.AddStop(stop);
    }

    for (const auto& stop : raw_stops_) {
        for (const auto& [other, distance] : stop.distances) {
            bool pairOK = transport_catalogue_.SetDistanceBetweenStops(
                stop.stop_name, other, distance);
            if (!pairOK) {
                std::cerr << "ERROR while adding distance to stop pair of "s
                          << stop.stop_name << " and "s << other << "."
                          << std::endl;
            }
        }
    }

    for (auto& route : raw_buses_) {

        if (route.route_stops.size() < 2) {
            std::cerr << "Error while adding bus routes for bus: "s
                      << route.bus_name
                      << ". Number of stops must be at least 2." << std::endl;
            continue;
        }
        transport_catalogue::BusRoute br;
        br.bus_name = std::move(route.bus_name);
        br.type = route.type;
        for (auto& route_stop : route.route_stops) {
            br.route_stops.emplace_back(
                &(transport_catalogue_.FindStop(route_stop).second));
        }

        transport_catalogue_.AddBus(br);
    }

    route::RoutingSettings settings;
    const auto& root_node = root_.back().GetRoot();
    const json::Dict& dict = root_node.AsMap();
    auto iter = dict.find(ROUTING_SETTINGS);
    if (iter == dict.end() || !iter->second.IsMap()) {
        throw json::ParsingError("Error reading json data with routing settings.");
    }
    auto routing_settings_map = iter->second.AsMap();
    if (const auto bus_wait_time = routing_settings_map.find("bus_wait_time"s);
        bus_wait_time != routing_settings_map.end() && bus_wait_time->second.IsInt()) {
        settings.bus_wait_time = bus_wait_time->second.AsInt();
    } else {
        throw json::ParsingError("Error reading json data with routing settings.");
    }

    if (const auto bus_velocity = routing_settings_map.find("bus_velocity"s);
        bus_velocity != routing_settings_map.end() && bus_velocity->second.IsInt()) {
        settings.bus_velocity = bus_velocity->second.AsInt();
    } else {
        throw json::ParsingError("Error reading json data with routing settings.");
    }
    transport_router_.SetSettings(settings);

    return true;
}

size_t JsonReader::WriteJsonToStream(std::ostream& out) {
    const auto& root_node = root_.back().GetRoot();
    if (!root_node.IsMap()) {
        throw json::ParsingError(
            "Error reading JSON data with user requests to database.");
    }

    const json::Dict& dict = root_node.AsMap();
    auto iter = dict.find(USER_REQUESTS);
    if (iter == dict.end() || !(iter->second.IsArray())) {
        throw json::ParsingError(
            "Error reading JSON data with user requests to database.");
    }

    json::Builder builder;
    builder.StartArray();
    for (const json::Node& node : iter->second.AsArray()) {
        if (!node.IsMap()) {
            throw json::ParsingError(
                "Error reading JSON data with user requests to database. One "
                "of nodes is not a dictionary.");
        }
        builder.Value(std::move(ProcessOneUserRequestNode(node)));
    }
    json::Node res_node = builder.EndArray().Build();
    json::PrintNode(res_node, out);

    return res_node.AsArray().size();
}

json::Node JsonReader::ProcessOneUserRequestNode(
    const json::Node& user_request) {
    using namespace transport_catalogue;

    if (!user_request.IsMap()) {
        throw json::ParsingError(
            "Error reading JSON data with user requests to database. One of "
            "nodes is not a dictionary.");
    }
    const json::Dict& request_fields = user_request.AsMap();

    int id = -1;
    if (const auto id_i = request_fields.find("id"s);
        id_i != request_fields.end() && id_i->second.IsInt()) {
        id = id_i->second.AsInt();
    } else
        throw json::ParsingError(
            "Error reading JSON data with user requests to database. One of "
            "node's fields is crippled.");

    const auto type_i = request_fields.find("type"s);
    if (type_i == request_fields.end() || !(type_i->second.IsString())) {
        throw json::ParsingError(
            "Error reading JSON data with user requests to database. One of "
            "node's fields is crippled.");
    }
    std::string type = type_i->second.AsString();

    if (type == "Map"s) {
        RendererSettings rs = GetRendererSetting();
        MapRenderer mr(rs);

        std::ostringstream stream;
        mr.RenderSvgMap(transport_catalogue_, stream);

        json::Dict result;
        result.emplace("request_id"s, id);
        result.emplace("map"s, std::move(stream.str()));

        return {result};
    }
    if (type == "Route"s) {
        const auto from_i = request_fields.find("from"s);
        const auto to_i = request_fields.find("to"s);
        std::string from, to;
        if (from_i == request_fields.end() || !from_i->second.IsString() || to_i == request_fields.end() || !to_i->second.IsString()) {
            throw json::ParsingError("Error reading JSON data with user requests to database. One of node's fields is crippled.");
        }

        from = from_i->second.AsString();
        to = to_i->second.AsString();

        transport_router_.Update();
        auto route = transport_router_.BuildRoute(from, to);
        json::Dict result;

        result.emplace("request_id"s, id);
        if (route.has_value()) {
            result.emplace("total_time"s, route.value().total_time);
            json::Array items;
            for (const auto& part : route.value().items) {
                json::Dict node;
                std::visit(RouteVistor{node}, part);
                items.push_back(node);
            }
            result.emplace("items"s, std::move(items));
        }
        else {
            result.emplace("error_message"s, "not found"s);
        }

        return { result };
    }
    std::string name;
    if (const auto name_i = request_fields.find("name"s);
        name_i != request_fields.end() && name_i->second.IsString()) {
        name = name_i->second.AsString();
    } else
        throw json::ParsingError("Error reading JSON data with user requests to database. One of node's fields is crippled.");

    if (type == "Bus"s) {
        BusInfo bi = transport_catalogue_.GetBusInfo(name);
        if (bi.type == RouteType::NOT_SET) {
            return GetErrorNode(id);
        }
        json::Dict result;
        result.emplace("request_id"s, id);
        result.emplace("curvature"s, bi.curvature);
        result.emplace("route_length"s, static_cast<int>(bi.route_length));
        result.emplace("stop_count"s, static_cast<int>(bi.stops_number));
        result.emplace("unique_stop_count"s, static_cast<int>(bi.unique_stops));

        return {result};
    }

    if (type == "Stop"s) {
        if (!transport_catalogue_.FindStop(name).first) {
            return GetErrorNode(id);
        }
        json::Dict result;
        json::Array buses;
        const std::set<std::string_view>& bus_routes =
            transport_catalogue_.GetBusesForStop(name);
        for (auto bus_route : bus_routes) {
            buses.emplace_back(std::move(std::string{bus_route}));
        }
        result.emplace("request_id"s, id);
        result.emplace("buses"s, buses);

        return {result};
    }

    throw json::ParsingError(
        "Error reading JSON data with user requests to database. Node's type "
        "field contains invalid data.");
}


RendererSettings JsonReader::GetRendererSetting() const {
    const auto& root_node = root_.back().GetRoot();
    if (!root_node.IsMap()) {
        throw json::ParsingError(
            "Error reading JSON data with render settings.");
    }

    const json::Dict& dict = root_node.AsMap();
    auto iter = dict.find(RENDER_SETTINGS);
    if (iter == dict.end() || !(iter->second.IsMap())) {
        throw json::ParsingError(
            "Error reading JSON data with render settings..");
    }

    RendererSettings settings;
    const json::Dict render_settings = iter->second.AsMap();
    if (const auto width_i = render_settings.find("width"s);
        width_i != render_settings.end() && width_i->second.IsDouble()) {
        settings.width = width_i->second.AsDouble();
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, width data.");

    if (const auto height_i = render_settings.find("height"s);
        height_i != render_settings.end() && height_i->second.IsDouble()) {
        settings.height = height_i->second.AsDouble();
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, height data.");

    if (const auto padd_i = render_settings.find("padding"s);
        padd_i != render_settings.end() && padd_i->second.IsDouble()) {
        settings.padding = padd_i->second.AsDouble();
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, padding data.");

    if (const auto field_iter = render_settings.find("line_width"s);
        field_iter != render_settings.end() && field_iter->second.IsDouble()) {
        settings.line_width = field_iter->second.AsDouble();
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, line width data.");

    if (const auto field_iter = render_settings.find("stop_radius"s);
        field_iter != render_settings.end() && field_iter->second.IsDouble()) {
        settings.stop_radius = field_iter->second.AsDouble();
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, stop radius data.");

    if (const auto field_iter = render_settings.find("bus_label_font_size"s);
        field_iter != render_settings.end() && field_iter->second.IsInt()) {
        settings.bus_label_font_size = field_iter->second.AsInt();
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, bus label font size data.");

    if (const auto field_iter = render_settings.find("bus_label_offset"s);
        field_iter != render_settings.end() && field_iter->second.IsArray()) {
        json::Array arr = field_iter->second.AsArray();
        if (arr.size() != 2)
            throw json::ParsingError(
                "Error while parsing renderer settings, bus label font offset "
                "data.");
        settings.bus_label_offset.x = arr[0].AsDouble();
        settings.bus_label_offset.y = arr[1].AsDouble();
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, bus label font offset "
            "data.");

    if (const auto field_iter = render_settings.find("stop_label_font_size"s);
        field_iter != render_settings.end() && field_iter->second.IsInt()) {
        settings.stop_label_font_size = field_iter->second.AsInt();
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, stop label font size "
            "data.");

    if (const auto field_iter = render_settings.find("stop_label_offset"s);
        field_iter != render_settings.end() && field_iter->second.IsArray()) {
        json::Array arr = field_iter->second.AsArray();
        if (arr.size() != 2)
            throw json::ParsingError(
                "Error while parsing renderer settings, stop label font offset "
                "data.");
        settings.stop_label_offset.x = arr[0].AsDouble();
        settings.stop_label_offset.y = arr[1].AsDouble();
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, stop label font offset "
            "data.");

    if (const auto field_iter = render_settings.find("underlayer_color"s);
        field_iter != render_settings.end()) {
        svg::Color color = ParseColor(field_iter->second);
        if (std::holds_alternative<std::monostate>(color)) {
            throw json::ParsingError(
                "Error while parsing renderer settings, underlayer color "
                "data.");
        }

        settings.underlayer_color = color;
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, underlayer color data.");

    if (const auto field_iter = render_settings.find("underlayer_width"s);
        field_iter != render_settings.end() && field_iter->second.IsDouble()) {
        settings.underlayer_width = field_iter->second.AsDouble();
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, underlayer width data.");

    if (const auto field_iter = render_settings.find("color_palette"s);
        field_iter != render_settings.end() && field_iter->second.IsArray()) {
        json::Array arr = field_iter->second.AsArray();
        for (const auto& color_node : arr) {
            svg::Color color = ParseColor(color_node);
            if (std::holds_alternative<std::monostate>(color)) {
                throw json::ParsingError(
                    "Error while parsing renderer settings, color palette "
                    "data.");
            }
            settings.color_palette.emplace_back(color);
        }
    } else
        throw json::ParsingError(
            "Error while parsing renderer settings, color palette data.");

    return settings;
}

svg::Color ParseColor(const json::Node& node) {
    if (node.IsString()) {
        return {node.AsString()};
    }

    if (node.IsArray()) {
        json::Array arr = node.AsArray();
        if (arr.size() == 3) {
            uint8_t red = arr[0].AsInt();
            uint8_t green = arr[1].AsInt();
            uint8_t blue = arr[2].AsInt();

            return {svg::Rgb(red, green, blue)};
        }
        if (arr.size() == 4) {
            uint8_t red = arr[0].AsInt();
            uint8_t green = arr[1].AsInt();
            uint8_t blue = arr[2].AsInt();
            double opacity = arr[3].AsDouble();

            return {svg::Rgba(red, green, blue, opacity)};
        }
    }

    return {};
}

inline json::Node GetErrorNode(int id) {
    return json::Builder().StartDict().Key("request_id"s).Value(id)
                .Key("error_message"s).Value("not found"s).EndDict()
                .Build();
}