#include "stat_reader.h"

namespace navigation {
    namespace output {

        void ReadOutputRequests(std::istream &is, std::ostream& os, TransportCatalogue& tc) {
            std::string line;
            std::getline(is, line);
            std::size_t num_of_requests = std::stoi(line);
            for (std::size_t i = 0; i < num_of_requests; ++i) {
                std::getline(is, line);
                std::size_t command_end = line.find_first_of(' ');
                std::string command{line.c_str(), command_end};
                if (command == "Bus") {
                    std::string bus_name = line.substr(command_end + 1);
                    auto bus = tc.FindBus(bus_name);
                    if (bus.name == "") {
                        os << "Bus " << bus_name << ": not found" << std::endl;
                        continue;
                    }
                    auto bus_info = tc.GetBusInfo(bus);
                    os << std::setprecision(6);
                    os << "Bus " << bus_info.name << ": " << bus_info.stops_number << " stops on route, " << bus_info.uniq_stops_number << " unique stops, " << bus_info.actual_distance << " route length, " << bus_info.curvature << " curvature" << std::endl;
                }
                else {
                    std::string stop_name = line.substr(command_end + 1);
                    navigation::detail::StopBuses sbuses = tc.GetStopBuses(stop_name);
                    if (sbuses.stop_name == "") {
                        os << line << ": not found" << std::endl;
                    }
                    else if (sbuses.buses.empty()) {
                        os << line << ": no buses" << std::endl;
                    }
                    else {
                        os << line << ": buses";
                        for (auto bus : sbuses.buses) {
                            os << " " << bus->name;
                        }
                        os << std::endl;
                    }
                }
            }
        }
    }
}