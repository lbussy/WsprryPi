#include "../band_lookup.hpp"
#include "../json.hpp"
#include "../wspr_band_catalog_response.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
void require(bool condition, const std::string &message)
{
    if (!condition)
    {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}
}

int main()
{
    const BandLookup lookup;
    const auto response = nlohmann::json::parse(
        build_wspr_band_catalog_response_json(lookup, 1500.0));

    require(response.at("bands").size() == 17,
            "compatibility band catalog must retain one effective preset per band");
    require(response.at("presets").size() == 19,
            "complete preset catalog must contain the bare catalog and two qualified presets");

    const auto &legacy = response.at("presets").at(17);
    require(legacy.at("preset") == "60m:legacy" &&
                legacy.at("band") == "60m" &&
                legacy.at("dial_frequency_hz") == 5287200 &&
                legacy.at("existing_common") == true,
            "60m:legacy response must identify the retained convention");

    const auto &wrc15 = response.at("presets").at(18);
    require(wrc15.at("preset") == "60m:wrc15" &&
                wrc15.at("band") == "60m" &&
                wrc15.at("dial_frequency_hz") == 5364700 &&
                wrc15.at("existing_common") == false,
            "60m:wrc15 response must retain separate preset and band identities");

    std::cout << "WSPR preset catalog response tests passed.\n";
    return 0;
}
