#pragma once

#include <optional>
#include <string>
#include <vector>

struct SupportLocalNetwork {
    std::string address;
    std::string netmask;
};

struct SupportRequestGuardSnapshot {
    bool discovery_succeeded = false;
    std::string hostname;
    std::vector<std::string> configured_hostnames;
    std::vector<SupportLocalNetwork> networks;
};

enum class SupportRequestGuardDecision {
    allowed,
    rejected_peer,
    rejected_host,
    rejected_origin,
    interface_discovery_unavailable
};

struct SupportRequestGuardResult {
    SupportRequestGuardDecision decision;
    [[nodiscard]] bool allowed() const noexcept { return decision == SupportRequestGuardDecision::allowed; }
};

class SupportRequestGuard {
public:
    explicit SupportRequestGuard(SupportRequestGuardSnapshot snapshot);

    [[nodiscard]] SupportRequestGuardResult evaluate(
        const std::string &peer_address,
        const std::string &host_header,
        const std::optional<std::string> &origin_header,
        bool enforce_peer = true) const;

    [[nodiscard]] static SupportRequestGuardSnapshot discover_local_networks();

private:
    SupportRequestGuardSnapshot snapshot_;
};
