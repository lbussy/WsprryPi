// SPDX-License-Identifier: MIT
#include "rp1_gpclk_linux_provider.hpp"
#include "rp1_gpclk_uapi.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace
{
constexpr std::uint64_t kAdministrativeCapabilities =
    RP1_GPCLK_CAP_STABLE_STATE | RP1_GPCLK_CAP_ROUTE_IDENTITY |
    RP1_GPCLK_CAP_COMPAT_IDENTITY | RP1_GPCLK_CAP_CLEANUP_FAULT_LATCH;

const char* completionName(wsprrypi::Rp1GpclkCompletionState value)
{
    switch (value)
    {
    case wsprrypi::Rp1GpclkCompletionState::idle: return "idle";
    case wsprrypi::Rp1GpclkCompletionState::running: return "running";
    case wsprrypi::Rp1GpclkCompletionState::draining: return "draining";
    case wsprrypi::Rp1GpclkCompletionState::complete: return "complete";
    case wsprrypi::Rp1GpclkCompletionState::failed: return "failed";
    case wsprrypi::Rp1GpclkCompletionState::dead: return "dead";
    }
    return "unknown";
}
}

int main(int argc, char** argv)
{
    if (argc != 2 || (std::string(argv[1]) != "4" && std::string(argv[1]) != "20"))
    {
        std::cerr << "usage: rp1_gpclk_admin_probe 4|20\n";
        return EXIT_FAILURE;
    }
    const std::uint32_t route = std::string(argv[1]) == "4"
        ? RP1_GPCLK_ROUTE_GPIO4 : RP1_GPCLK_ROUTE_GPIO20;
    wsprrypi::Rp1GpclkPosixIo io;
    wsprrypi::Rp1GpclkLinuxProvider provider(io);
    wsprrypi::Rp1GpclkProviderIdentity identity;
    std::string error;
    if (!provider.query(route, kAdministrativeCapabilities, false, identity, error))
    {
        std::cerr << "query=failed\nerror=" << error << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "query=ok\nroute=" << identity.route
              << "\ncompatibility_state=" << identity.compatibility_state
              << "\ncompatibility_reason=" << identity.compatibility_reason
              << "\ncapabilities=" << identity.capabilities
              << "\nlive_eligible="
              << ((identity.capabilities & RP1_GPCLK_CAP_LIVE_ELIGIBLE) ? "yes" : "no")
              << "\nmodule_id=" << identity.module_id
              << "\nbuild_id=" << identity.build_id
              << "\ncompatibility_id=" << identity.compatibility_id << '\n';

    if (!provider.acquire(route, kAdministrativeCapabilities, error))
    {
        std::cout << "acquire=rejected\nerror=" << error << '\n';
        return EXIT_SUCCESS;
    }
    wsprrypi::Rp1GpclkProviderEventState state;
    std::cout << "acquire=ok\n";
    if (provider.getState(0, state, error))
        std::cout << "state=" << completionName(state.completion)
                  << "\ngeneration=0\nterminal_reason=" << state.terminal_reason << '\n';
    else
        std::cout << "state=rejected\nstate_error=" << error << '\n';
    if (!provider.release(error))
    {
        std::cerr << "release=failed\nerror=" << error << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "release=ok\n";
    return EXIT_SUCCESS;
}
