#pragma once

#include <vector>
#include <istream>
#include <algorithm>
#include <string>
#include <deque>

#include "transport_catalogue.h"

using namespace std::literals;
namespace transport_catalogue {
    namespace input {
        struct Request {
            int priority = 0;
            std::string command;
            std::string args;
        };

        void ReadInputRequests(std::istream &is, transport_catalogue::TransportCatalogue& tc);
        void ProcessInputRequests(transport_catalogue::TransportCatalogue& tc, std::vector<Request>& query);
    }
}