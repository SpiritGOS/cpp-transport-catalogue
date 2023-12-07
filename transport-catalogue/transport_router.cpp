#include "transport_router.h"

namespace route {
	TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& tc, RoutingSettings settings) : catalogue_(tc), settings_(settings)
	{
		BuildVerticesForStops(catalogue_.GetAllStops());
		BuildRoutesGraph(catalogue_.GetAllBuses());
		router_ = std::make_unique<Router>(*routes_);
	}

	std::optional<ResponseData> TransportRouter::BuildRoute(std::string_view from, std::string_view to) const
	{
		std::optional<ResponseData> result;
		graph::VertexId from_id = stop_to_vertex_.at(from).start;
		graph::VertexId to_id = stop_to_vertex_.at(to).start;

		auto route = router_->BuildRoute(from_id, to_id);
		if (route.has_value()) {
			result.emplace(ResponseData{});
			result->total_time = route->weight;
			for (auto edge_id : route->edges) {
				graph::Edge<Weight> edge = routes_->GetEdge(edge_id);
				result->items.emplace_back(edge_to_response_.at(edge));
			}
		}
		return result;
	}

	void TransportRouter::SetSettings(RoutingSettings settings)
	{
		settings_ = settings;
	}

	void TransportRouter::Update()
	{
		BuildVerticesForStops(catalogue_.GetAllStops());
		BuildRoutesGraph(catalogue_.GetAllBuses());
		router_ = std::make_unique<Router>(*routes_);
	}

	void TransportRouter::BuildVerticesForStops(const std::set<std::string_view>& stops)
	{
		graph::VertexId start{ 0 };
		graph::VertexId end{ 1 };

		stop_to_vertex_.reserve(stops.size());

		for (std::string_view stop_name : stops) {
			stop_to_vertex_.emplace(stop_name, StopVertices{ start, end });
			start += 2;
			end += 2;
		}
	}

	void TransportRouter::AddBusRouteEdges(const transport_catalogue::BusRoute& bus_info)
	{
		auto get_time = [this](std::string_view from, std::string_view to) -> double {
			auto result = catalogue_.GetDistanceBetweenStops(from, to);
			return result != -1 ? static_cast<double>(result) / (settings_.bus_velocity / 60.0 * 1000.0) : static_cast<double>(catalogue_.GetDistanceBetweenStops(to, from)) / (settings_.bus_velocity / 60.0 * 1000.0);
			};
		std::unordered_map<std::pair<std::string_view, std::string_view>, Info, PairHasher> distances;
		auto set_info = [&](auto begin, auto end) {
			auto previos = begin;
			for (auto it = begin; it != end; ++it) {
				double cumulative_time = 0;
				previos = it;
				for (auto to = std::next(it); to != end; ++to) {
					cumulative_time += get_time((*previos)->stop_name, (*to)->stop_name);
					auto stops_pair = std::make_pair(std::string_view((*it)->stop_name), std::string_view((*to)->stop_name));
					if (distances.count(stops_pair)) {
						distances[stops_pair] = distances[stops_pair].time < cumulative_time ? distances[stops_pair] : Info{ cumulative_time, static_cast<int>(std::distance(it, to)) };
					}
					else {
						distances.emplace(stops_pair, Info{ cumulative_time, static_cast<int>(std::distance(it, to)) });
					}
					previos = to;
				}
			}
			};
		if (bus_info.type == transport_catalogue::RouteType::RETURN_ROUTE) {
			set_info(bus_info.route_stops.begin(), bus_info.route_stops.end());
			set_info(bus_info.route_stops.rbegin(), bus_info.route_stops.rend());
		}
		else if (bus_info.type == transport_catalogue::RouteType::CIRCLE_ROUTE) {
			set_info(bus_info.route_stops.begin(), bus_info.route_stops.end());
		}

		for (const auto& [route, info] : distances) {
			auto from = stop_to_vertex_[route.first].end;
			auto to = stop_to_vertex_[route.second].start;

			auto edge = graph::Edge<Weight>{ from, to, info.time };

			routes_->AddEdge(edge);
			edge_to_response_.emplace(edge, BusResponse(info.time, bus_info.bus_name, info.stops_count));
		}
	}

	void TransportRouter::BuildRoutesGraph(const std::deque<transport_catalogue::BusRoute>& buses)
	{
		routes_ = std::make_unique<Graph>(stop_to_vertex_.size() * 2);

		auto wait_time = static_cast<double>(settings_.bus_wait_time);
		auto stop_edge = graph::Edge<Weight>{};

		for (auto& [stop_name, stop_vertices] : stop_to_vertex_) {
			stop_edge = graph::Edge<Weight>{ stop_vertices.start, stop_vertices.end, wait_time };
			routes_->AddEdge(stop_edge);
			edge_to_response_.emplace(stop_edge, WaitResponse(wait_time, std::string{ stop_name }));
		}
		for (const auto& bus : buses) {
			AddBusRouteEdges(bus);
		}
	}


}