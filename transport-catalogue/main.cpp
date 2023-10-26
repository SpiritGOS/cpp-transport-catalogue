#include <iostream>
#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"

using namespace transport_catalogue;
using namespace std::literals;

int main() {
    TransportCatalogue tc;
    JsonReader reader(tc);
    reader.ReadAndExecute(std::cin, std::cout);
}