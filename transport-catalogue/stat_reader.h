#pragma once

#include <iostream>
#include <iomanip>
#include "transport_catalogue.h"

namespace navigation {
    namespace output {
        void ReadOutputRequests(std::istream &is, std::ostream& os, navigation::TransportCatalogue& tc);
    }
}