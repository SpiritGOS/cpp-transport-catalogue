#include <iostream>
#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"

using namespace transport_catalogue;
using namespace std::literals;

int main() {
    TransportCatalogue tc;
    JsonReader reader(tc);
    reader.ReadJson_FillTransportCatalogue(std::cin);
    reader.QueryTC_WriteJsonToStream(std::cout);
//    RendererSettings rs = reader.GetRendererSetting();
//    MapRenderer mr(rs);
//    mr.RenderSvgMap(tc, std::cout);
}