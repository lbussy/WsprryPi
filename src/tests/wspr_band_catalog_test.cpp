#include "../wspr_band_catalog_response.hpp"
#include "../wspr_band_lookup.hpp"
#include "../config_handler.hpp"
#include "../version.hpp"

#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include "../json.hpp"

namespace
{
    void require(bool condition, const std::string &message)
    {
        if (!condition)
        {
            std::cerr << "FAIL: " << message << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    struct ExpectedBand
    {
        const char *name;
        std::int64_t dial_frequency_hz;
    };

    const std::vector<ExpectedBand> expected_bands = {
        {"2200m", 136000}, {"630m", 474200}, {"160m", 1836600},
        {"80m", 3568600}, {"60m", 5287200}, {"40m", 7038600},
        {"30m", 10138700}, {"22m", 13551500}, {"20m", 14095600},
        {"17m", 18104600}, {"15m", 21094600}, {"12m", 24924600},
        {"10m", 28124600}, {"6m", 50293000}, {"4m", 70091000},
        {"2m", 144489000},
    };

    void require_catalog_response(
        const nlohmann::json &response,
        std::int64_t expected_offset_hz)
    {
        require(response.at("command") == "wspr_band_catalog",
                "catalog response must identify wspr_band_catalog");
        require(response.at("status") == "ok",
                "catalog response must report success");
        require(response.at("audio_offset_hz").is_number_integer(),
                "audio_offset_hz must be integral JSON");
        require(response.at("audio_offset_hz") == expected_offset_hz,
                "catalog response must report the requested audio offset");
        require(response.at("bands").is_array(),
                "catalog response must contain a bands array");
        require(response.at("bands").size() == expected_bands.size(),
                "catalog response must contain exactly the canonical display bands");

        for (std::size_t index = 0; index < expected_bands.size(); ++index)
        {
            const auto &actual = response.at("bands").at(index);
            const auto &expected = expected_bands.at(index);
            require(actual.at("band") == expected.name,
                    "catalog response order must use canonical display bands");
            require(actual.at("dial_frequency_hz").is_number_integer() &&
                        actual.at("tone_frequency_hz").is_number_integer(),
                    "catalog response frequencies must be integral JSON");
            require(actual.at("dial_frequency_hz") == expected.dial_frequency_hz,
                    std::string(expected.name) +
                        " dial frequency must match the backend definition");
            require(actual.at("tone_frequency_hz") ==
                        expected.dial_frequency_hz + expected_offset_hz,
                    std::string(expected.name) +
                        " tone frequency must apply the offset exactly once");
        }
    }

    void require_rejected_offset(
        const WSPRBandLookup &lookup,
        double offset_hz,
        const std::string &description)
    {
        bool rejected = false;
        try
        {
            (void)build_wspr_band_catalog_response_json(lookup, offset_hz);
        }
        catch (const std::runtime_error &)
        {
            rejected = true;
        }
        require(rejected, "catalog response must reject " + description);
    }
}

int main()
{
    WSPRBandLookup lookup;
    const auto catalog = lookup.canonical_wspr_band_catalog();
    require(catalog.size() == expected_bands.size(),
            "lookup catalog must expose exactly 16 canonical display bands");
    for (std::size_t index = 0; index < expected_bands.size(); ++index)
    {
        require(catalog.at(index).band == expected_bands.at(index).name,
                "lookup catalog must preserve canonical display order");
        require(catalog.at(index).dial_frequency_hz ==
                    static_cast<std::uint64_t>(expected_bands.at(index).dial_frequency_hz),
                "lookup catalog dial frequency must match the authoritative definition");
    }
    for (const auto &entry : catalog)
    {
        require(entry.band != "lf" && entry.band != "mf",
                "lookup catalog must not expose alias display rows");
    }

    require(std::get<double>(lookup.lookup(std::string("lf"))) == 136000.0,
            "lf lookup alias must remain accepted");
    require(std::get<double>(lookup.lookup(std::string("mf"))) == 474200.0,
            "mf lookup alias must remain accepted");
    require(lookup.parse_string_to_frequency("22m") == 13551500.0,
            "22m parser behavior must remain unchanged");

    const auto default_response = nlohmann::json::parse(
        build_wspr_band_catalog_response_json(lookup, 1500.0));
    require_catalog_response(default_response, 1500);
    require(default_response.at("bands").at(7).at("dial_frequency_hz") == 13551500,
            "22m catalog dial frequency must be 13,551,500 Hz");
    require(default_response.at("bands").at(8).at("tone_frequency_hz") == 14097100,
            "20m default catalog tone frequency must be 14,097,100 Hz");

    const auto zero_offset_response = nlohmann::json::parse(
        build_wspr_band_catalog_response_json(lookup, 0.0));
    require_catalog_response(zero_offset_response, 0);

    const auto non_default_response = nlohmann::json::parse(
        build_wspr_band_catalog_response_json(lookup, 2750.0));
    require_catalog_response(non_default_response, 2750);
    require(non_default_response.at("bands").at(8).at("tone_frequency_hz") == 14098350,
            "non-default offset must be reflected exactly once in the catalog tone frequency");

    require_rejected_offset(lookup, -1500.0, "negative integral offsets");
    require_rejected_offset(lookup, -1500.5, "negative fractional offsets");
    require_rejected_offset(lookup, 1500.5, "positive fractional offsets");
    require_rejected_offset(
        lookup,
        std::numeric_limits<double>::quiet_NaN(),
        "NaN offsets");
    require_rejected_offset(
        lookup,
        std::numeric_limits<double>::infinity(),
        "positive infinite offsets");
    require_rejected_offset(
        lookup,
        -std::numeric_limits<double>::infinity(),
        "negative infinite offsets");
    require_rejected_offset(
        lookup,
        static_cast<double>(std::numeric_limits<std::int64_t>::max()),
        "offsets outside the safely representable integral range");
    require_rejected_offset(
        lookup,
        std::nextafter(
            static_cast<double>(std::numeric_limits<std::int64_t>::max()),
            0.0),
        "offsets that overflow derived tone frequencies");

    const std::int64_t highest_dial_frequency_hz =
        static_cast<std::int64_t>(catalog.back().dial_frequency_hz);
    const std::int64_t mathematical_safe_offset_limit =
        std::numeric_limits<std::int64_t>::max() - highest_dial_frequency_hz;
    const auto offset_is_accepted = [&lookup](double offset_hz) {
        try
        {
            (void)build_wspr_band_catalog_response_json(lookup, offset_hz);
            return true;
        }
        catch (const std::runtime_error &)
        {
            return false;
        }
    };

    // The integer boundary may round either direction when represented as a
    // double.  Move toward zero only if the initial representation is rejected,
    // then advance to the greatest adjacent representable accepted value.
    constexpr int kMaximumBoundarySearchSteps = 4;
    double nearby_safe_offset =
        static_cast<double>(mathematical_safe_offset_limit);
    int downward_steps = 0;
    while (!offset_is_accepted(nearby_safe_offset) &&
           downward_steps < kMaximumBoundarySearchSteps)
    {
        nearby_safe_offset = std::nextafter(nearby_safe_offset, 0.0);
        ++downward_steps;
    }
    require(offset_is_accepted(nearby_safe_offset),
            "boundary search must find an accepted representable catalog offset");

    int upward_steps = 0;
    for (;;)
    {
        const double next_offset = std::nextafter(
            nearby_safe_offset,
            std::numeric_limits<double>::infinity());
        if (!offset_is_accepted(next_offset))
        {
            break;
        }
        require(upward_steps < kMaximumBoundarySearchSteps,
                "boundary search must reach the greatest accepted offset within four ULP steps");
        nearby_safe_offset = next_offset;
        ++upward_steps;
    }

    require(std::isfinite(nearby_safe_offset) &&
                std::trunc(nearby_safe_offset) == nearby_safe_offset &&
                nearby_safe_offset >= 0.0 &&
                nearby_safe_offset <
                    static_cast<double>(std::numeric_limits<std::int64_t>::max()),
            "nearby safe offset must satisfy the production integral conversion contract");
    const std::int64_t nearby_safe_offset_hz =
        static_cast<std::int64_t>(nearby_safe_offset);
    require(nearby_safe_offset_hz <= mathematical_safe_offset_limit,
            "nearby safe offset must not overflow the highest catalog dial frequency");
    const auto nearby_safe_response = nlohmann::json::parse(
        build_wspr_band_catalog_response_json(lookup, nearby_safe_offset));
    require_catalog_response(nearby_safe_response, nearby_safe_offset_hz);
    const auto &highest_band = nearby_safe_response.at("bands").back();
    require(highest_band.at("tone_frequency_hz") <=
                std::numeric_limits<std::int64_t>::max() &&
                highest_band.at("tone_frequency_hz") ==
                    highest_dial_frequency_hz + nearby_safe_offset_hz,
            "nearby safe offset must produce an exact in-range tone for the highest band");
    const double unsafe_offset = std::nextafter(
        nearby_safe_offset,
        std::numeric_limits<double>::infinity());
    require(unsafe_offset > nearby_safe_offset &&
                std::nextafter(
                    nearby_safe_offset,
                    std::numeric_limits<double>::infinity()) == unsafe_offset,
            "unsafe offset must be the immediate representable neighbor above the safe offset");
    require(std::isfinite(unsafe_offset) && unsafe_offset >= 0.0 &&
                std::trunc(unsafe_offset) == unsafe_offset &&
                unsafe_offset <
                    static_cast<double>(std::numeric_limits<std::int64_t>::max()),
            "adjacent unsafe offset must reach tone addition rather than another validation failure");
    require_rejected_offset(
        lookup,
        unsafe_offset,
        "the immediate representable offset above the tone-addition limit");

    const ArgParserConfig original_config = config;
    const nlohmann::json original_json = jConfig;
    set_patch_all_from_web_runtime_apply_suppressed_for_test(true);
    set_raspberry_pi_generation_override_for_test(4);
    init_default_config();
    config.use_ini = false;
    config_to_json();
    require(current_wspr_audio_offset_hz() == WSPR_AUDIO_OFFSET_HZ,
            "default configuration must publish the default catalog offset");

    PreparedConfigCandidate reloaded_candidate;
    reloaded_candidate.valid = true;
    reloaded_candidate.normalized_config = config;
    reloaded_candidate.normalized_config.wspr_audio_offset_hz = 2750.0;
    reloaded_candidate.normalized_config.wspr.audio_offset_hz = 2750.0;
    reloaded_candidate.normalized_json = jConfig;
    commit_config_candidate(reloaded_candidate);
    require(current_wspr_audio_offset_hz() == 2750.0,
            "accepted reload configuration must publish its catalog offset");
    require_catalog_response(
        nlohmann::json::parse(build_wspr_band_catalog_response_json(
            lookup,
            current_wspr_audio_offset_hz())),
        2750);

    ArgParserConfig runtime_copy_source = config;
    runtime_copy_source.wspr_audio_offset_hz = 3000.0;
    runtime_copy_source.wspr.audio_offset_hz = 3000.0;
    ArgParserConfig local_copy_target = config;
    copy_runtime_config(runtime_copy_source, local_copy_target);
    require(local_copy_target.wspr.audio_offset_hz == 3000.0 &&
                current_wspr_audio_offset_hz() == 2750.0,
            "non-global runtime copies must not publish the catalog offset snapshot");
    copy_runtime_config(runtime_copy_source, config);
    require(config.wspr.audio_offset_hz == 3000.0 &&
                current_wspr_audio_offset_hz() == 3000.0,
            "global runtime copies must publish the catalog offset snapshot");

    const nlohmann::json before_rejected_patch = jConfig;
    bool rejected_web_update = false;
    try
    {
        patch_all_from_web({{"CW", {{"Start Second", 60}}}});
    }
    catch (const std::exception &)
    {
        rejected_web_update = true;
    }
    require(rejected_web_update,
            "invalid CW start seconds must be rejected before web configuration commit");
    require(config.wspr.audio_offset_hz == 3000.0 &&
                current_wspr_audio_offset_hz() == 3000.0 &&
                jConfig == before_rejected_patch,
            "rejected web updates must leave the published catalog offset unchanged");

    patch_all_from_web({{"WSPR", {{"Frequency", "20m"}}}});
    require(current_wspr_audio_offset_hz() == WSPR_AUDIO_OFFSET_HZ,
            "accepted web configuration update must republish the catalog offset");
    copy_runtime_config(original_config, config);
    jConfig = original_json;
    require(current_wspr_audio_offset_hz() == original_config.wspr.audio_offset_hz,
            "test cleanup must restore the published catalog offset snapshot");
    clear_raspberry_pi_generation_override_for_test();
    set_patch_all_from_web_runtime_apply_suppressed_for_test(false);

    std::cout << "wspr band catalog test passed" << std::endl;
    return EXIT_SUCCESS;
}
