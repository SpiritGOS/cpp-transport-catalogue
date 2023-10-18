#pragma once

#include <vector>
#include <istream>
#include <algorithm>
#include <string>
#include <deque>

#include "transport_catalogue.h"

using namespace std::literals;
namespace navigation {
    namespace input {
        struct Request {
            int priority = 0;
            std::string command;
            std::string args;
        };

        void ReadInputRequests(std::istream &is, navigation::TransportCatalogue& tc);
        void ProcessInputRequests(navigation::TransportCatalogue& tc, std::vector<Request>& query);
    }
}