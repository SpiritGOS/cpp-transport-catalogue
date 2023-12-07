#include "input_reader.h"

namespace transport_catalogue {
    namespace input {
        void ReadInputRequests(std::istream& is, TransportCatalogue& tc) {
            std::vector<Request> query;
            std::string line;
            std::getline(is, line);
            std::size_t num_of_requests = std::stoi(line);
            for (std::size_t i = 0; i < num_of_requests; ++i) {
                std::getline(is, line);
                std::size_t command_end = line.find_first_of(' ');
                std::string command = line.substr(0, command_end);
                std::string args = line.substr(command_end + 1, line.size() - command_end);
                if (command == "Stop"sv) {
                    if (line.find(',') != line.rfind(',')) {
                        query.push_back({ 0, command, args });
                        query.push_back({ 1, command, args });
                    } else {
                        query.push_back({ 0, command, args });
                    }
                } else {
                    query.push_back({ 2, command, args });
                }
            }
            std::sort(query.begin(), query.end(), [](Request& lhs, Request& rhs) {
                return lhs.priority < rhs.priority;
                });
            ProcessInputRequests(tc, query);
        }

        void ProcessInputRequests(TransportCatalogue& tc, std::vector<Request>& query)
        {
            for (Request& request : query) {
                std::size_t delimiter = request.args.find(':');
                std::string name = request.args.substr(0, delimiter);
                std::string args = request.args.substr(delimiter + 2, request.args.size() - delimiter);
                if (request.priority == 0) {
                    delimiter = args.find(',');
                    tc.AddStop(name, std::stod(args.substr(0, delimiter)), std::stod(args.substr(delimiter + 1, args.find(delimiter, ','))));
                } else if (request.priority == 1) {
                    delimiter = args.find(',', args.find(',') +1);
                    std::size_t next_delimiter = args.find(',', delimiter + 1);
                    do {
                        std::size_t m_pos = args.find('m', delimiter + 1);
                        double distance = std::stod(args.substr(delimiter+2, m_pos));
                        std::string stop = args.substr(m_pos + 5, next_delimiter - m_pos - 5);
                        tc.SetStopsDistance(name, stop, distance);
                        delimiter = next_delimiter;
                        next_delimiter = args.find(',', delimiter + 1);
                    } while (delimiter != args.npos);
                } else {
                    std::string delimiter_str = args.find(" - ") != args.npos ? " - " : " > ";
                    bool type = args.find(" - ") != args.npos;
                    std::vector<std::string> stops;
                    delimiter = args.find(delimiter_str);
                    while (delimiter != args.npos) {
                        std::string stop = args.substr(0, delimiter);
                        stops.push_back(stop);
                        args = args.substr(delimiter + 3);
                        delimiter = args.find(delimiter_str);
                    }
                    stops.push_back(args);
                    tc.AddBus(name, stops, type);
                }
            }
        }
    }
}
