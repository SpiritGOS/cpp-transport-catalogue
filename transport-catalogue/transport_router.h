#pragma once

#include <variant>
#include <memory>

#include "domain.h"
#include "router.h"
#include "transport_catalogue.h"

namespace route {

	struct RoutingSettings {
		short bus_wait_time;
		short bus_velocity;
	};

	struct WaitResponse {
		double time{ 0. };
		std::string type{ "Wait" };
		std::string stop_name;
		WaitResponse(double time, std::string stop_name) : time(time), stop_name(stop_name) {}
	};

	struct BusResponse {
		double time{ 0. };
		std::string type{ "Bus" };
		std::string bus;
		short stop_count = 0;
		BusResponse(double time, std::string bus, short stop_count) : time(time), bus(bus), stop_count(stop_count) {}
	};

	using Response = std::variant<WaitResponse, BusResponse>;

	struct ResponseData {
		double total_time{ 0. };
		std::vector<Response> items;
	};

	struct PairHasher {
		size_t operator()(const std::pair<std::string_view, std::string_view> pair) const {
			return 37 * std::hash<std::string_view>{}(pair.first) + 37 * 37 * std::hash<std::string_view>{}(pair.second);
		}
	};

	struct Info {
		double time{ 0 };
		int stops_count{ 0 };
	};

	class TransportRouter {
	public:
		using Weight = double;
		using Graph = graph::DirectedWeightedGraph<Weight>;
		using Router = graph::Router<Weight>;

		struct StopVertices {
			graph::VertexId start{ 0 };
			graph::VertexId end{ 0 };
		};

		struct EdgeHash {
			size_t operator()(const graph::Edge<Weight>& edge) const {
				return 41 * std::hash<size_t>{}(edge.from) + 41 * 41 * std::hash<size_t>{}(edge.to) + 41 * 41 * 41 * std::hash<double>{}(edge.weight);
			}
		};

		struct EdgeEqual {
			bool operator()(const graph::Edge<Weight>& lhs, const graph::Edge<Weight>& rhs) const {
				return lhs.from == rhs.from && lhs.to == rhs.to && lhs.weight == rhs.weight;
			}
		};
	
		using EdgeToResponseStorage = std::unordered_map<graph::Edge<Weight>, Response, EdgeHash, EdgeEqual>;

		TransportRouter(const transport_catalogue::TransportCatalogue& tc, RoutingSettings settings);
		[[nodiscard]] std::optional<ResponseData> BuildRoute(std::string_view from, std::string_view to) const;
	private:
		void BuildVerticesForStops(const std::set<std::string_view>& stops);
		void AddBusRouteEdges(const transport_catalogue::BusRoute& bus_info);
		void BuildRoutesGraph(const std::deque<transport_catalogue::BusRoute>& buses);


		const transport_catalogue::TransportCatalogue& catalogue_;
		RoutingSettings settings_;

		std::unordered_map<std::string_view, StopVertices> stop_to_vertex_;
		EdgeToResponseStorage edge_to_response_;

		std::unique_ptr<Graph> routes_{ nullptr };
		std::unique_ptr<Router> router_{ nullptr };
	};
}