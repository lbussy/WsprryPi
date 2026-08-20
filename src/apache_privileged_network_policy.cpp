#include "apache_privileged_network_policy.hpp"

#include <algorithm>
#include <arpa/inet.h>
#include <array>
#include <cstring>
#include <set>
#include <sstream>

namespace {

std::optional<unsigned int> prefix_length(
    const unsigned char *mask, std::size_t size) {
    unsigned int prefix = 0;
    bool saw_zero = false;
    for (std::size_t index = 0; index < size; ++index) {
        for (int bit = 7; bit >= 0; --bit) {
            const bool one = (mask[index] & (1U << bit)) != 0;
            if (one && saw_zero) return std::nullopt;
            if (one) ++prefix;
            else saw_zero = true;
        }
    }
    if (prefix == 0) return std::nullopt;
    return prefix;
}

std::optional<std::string> network_cidr(const SupportLocalNetwork &network) {
    std::array<unsigned char, 16> address{};
    std::array<unsigned char, 16> mask{};
    int family = AF_UNSPEC;
    std::size_t size = 0;
    if (inet_pton(AF_INET, network.address.c_str(), address.data()) == 1 &&
        inet_pton(AF_INET, network.netmask.c_str(), mask.data()) == 1) {
        family = AF_INET;
        size = 4;
    } else if (inet_pton(AF_INET6, network.address.c_str(), address.data()) == 1 &&
               inet_pton(AF_INET6, network.netmask.c_str(), mask.data()) == 1) {
        family = AF_INET6;
        size = 16;
    } else {
        return std::nullopt;
    }
    if (family == AF_INET) {
        if (address[0] == 0 || address[0] == 127 || address[0] >= 224)
            return std::nullopt;
    } else {
        in6_addr address6{};
        std::memcpy(&address6, address.data(), sizeof(address6));
        if (IN6_IS_ADDR_UNSPECIFIED(&address6) || IN6_IS_ADDR_LOOPBACK(&address6) ||
            IN6_IS_ADDR_MULTICAST(&address6) || IN6_IS_ADDR_V4MAPPED(&address6))
            return std::nullopt;
    }
    const auto prefix = prefix_length(mask.data(), size);
    if (!prefix) return std::nullopt;
    for (std::size_t index = 0; index < size; ++index)
        address[index] &= mask[index];
    char output[INET6_ADDRSTRLEN]{};
    if (!inet_ntop(family, address.data(), output, sizeof(output)))
        return std::nullopt;
    return std::string(output) + "/" + std::to_string(*prefix);
}

void append_peer_requirement(std::ostringstream &output, const std::set<std::string> &cidrs) {
    output << "    <RequireAny>\n"
           << "        Require local\n";
    for (const auto &cidr : cidrs)
        output << "        Require ip " << cidr << "\n";
    output << "    </RequireAny>\n";
}

} // namespace

ApachePrivilegedNetworkPolicyResult render_apache_privileged_network_policy(
    const std::vector<SupportLocalNetwork> &networks,
    PrivilegedNetworkMode mode) {
    if (mode == PrivilegedNetworkMode::insecure_disabled) {
        return {"# NETWORK SAFETY OFF\n"
                "# Peer/subnet restrictions are explicitly disabled.\n", {}};
    }
    std::set<std::string> cidrs;
    for (const auto &network : networks) {
        const auto cidr = network_cidr(network);
        if (!cidr)
            return {std::nullopt, "invalid eligible network"};
        cidrs.insert(*cidr);
    }
    if (cidrs.empty())
        return {std::nullopt, "no eligible network"};

    std::ostringstream output;
    output << "# Managed by WsprryPi. Manual edits will be replaced.\n"
           << "# Browser-peer policy uses only Apache's connection address.\n\n"
           << "<LocationMatch \"^/wsprrypi/config$\">\n"
           << "    <LimitExcept GET>\n";
    append_peer_requirement(output, cidrs);
    output << "    </LimitExcept>\n"
           << "</LocationMatch>\n\n"
           << "<LocationMatch \"^/wsprrypi/config/\">\n";
    append_peer_requirement(output, cidrs);
    output << "</LocationMatch>\n\n"
           << "<LocationMatch \"^/wsprrypi/control/stop(?:/|$)\">\n";
    append_peer_requirement(output, cidrs);
    output << "</LocationMatch>\n\n"
           << "<LocationMatch \"^/wsprrypi/api/support-bundles(?:/|$)\">\n";
    append_peer_requirement(output, cidrs);
    output << "</LocationMatch>\n\n"
           << "<LocationMatch \"^/wsprrypi/socket(?:/|$)\">\n";
    append_peer_requirement(output, cidrs);
    output << "</LocationMatch>\n";
    return {output.str(), {}};
}
