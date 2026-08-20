#pragma once

#include "support_request_guard.hpp"

#include <optional>
#include <string>
#include <vector>

struct ApachePrivilegedNetworkPolicyResult {
    std::optional<std::string> configuration;
    std::string error;

    [[nodiscard]] bool valid() const noexcept { return configuration.has_value(); }
};

[[nodiscard]] ApachePrivilegedNetworkPolicyResult
render_apache_privileged_network_policy(
    const std::vector<SupportLocalNetwork> &networks);
