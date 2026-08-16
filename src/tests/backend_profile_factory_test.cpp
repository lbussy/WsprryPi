#include "backend_capabilities.hpp"
#include "arg_parser.hpp"
#include "config_handler.hpp"
#include "wspr_transmit.hpp"

#include <array>
#include <stdexcept>
#include <string>

namespace
{
bool compiled(wsprrypi::BackendKind backend)
{
    switch (backend)
    {
    case wsprrypi::BackendKind::RPI_CLOCK_GPIO:
        return WSPRRYPI_BACKEND_RPI_GPIO;
    case wsprrypi::BackendKind::RP1_GPCLK:
        return WSPRRYPI_BACKEND_RP1_GPCLK;
    case wsprrypi::BackendKind::SI5351:
        return WSPRRYPI_BACKEND_SI5351;
    case wsprrypi::BackendKind::SIMULATED:
        return WSPRRYPI_BACKEND_SIMULATED;
    }
    return false;
}
}

int main()
{
    WsprTransmitter transmitter;
    constexpr std::array backends{
        wsprrypi::BackendKind::RPI_CLOCK_GPIO,
        wsprrypi::BackendKind::RP1_GPCLK,
        wsprrypi::BackendKind::SI5351,
        wsprrypi::BackendKind::SIMULATED,
    };

    for (const auto backend : backends)
    {
        try
        {
            transmitter.selectBackend(backend);
            if (!compiled(backend))
                throw std::runtime_error("omitted backend selection unexpectedly succeeded");
        }
        catch (const std::invalid_argument &error)
        {
            if (compiled(backend))
                throw;
            const std::string message = error.what();
            if (message.find("unavailable in this build") == std::string::npos ||
                message.find(WSPRRYPI_COMPILED_BACKENDS) == std::string::npos)
            {
                throw std::runtime_error("omitted backend diagnostic lacks compiled capabilities");
            }
        }
    }

    constexpr std::array config_backends{
        TransmitBackendKind::GPIO,
        TransmitBackendKind::SI5351,
        TransmitBackendKind::SIMULATED,
    };
    for (const auto backend : config_backends)
    {
        if (transmit_backend_is_compiled(backend))
            continue;

        ArgParserConfig candidate;
        candidate.transmit_backend = backend;
        std::string error;
        if (validate_config_candidate(candidate, &error))
            throw std::runtime_error("omitted configuration backend unexpectedly validated");
        if (error != transmit_backend_unavailable_message(backend))
            throw std::runtime_error("omitted configuration backend diagnostic mismatch");
    }
}
