#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

enum class PrivilegedOperationClass {
    read_only,
    protected_operation,
    reject
};

enum class PrivilegedNetworkMode {
    enforced,
    insecure_disabled
};

struct PrivilegedNetworkModeParseResult {
    PrivilegedNetworkMode mode = PrivilegedNetworkMode::enforced;
    bool valid = false;
    bool missing = false;
};

enum class PrivilegedInterfaceKind {
    ethernet,
    wifi,
    other
};

struct PrivilegedInterfaceCandidate {
    bool active = false;
    PrivilegedInterfaceKind kind = PrivilegedInterfaceKind::other;
    bool loopback = false;
    bool point_to_point = false;
    bool tunnel = false;
    bool vpn = false;
    bool container = false;
    bool software_bridge = false;
    std::string address;
    std::string netmask;
};

[[nodiscard]] PrivilegedOperationClass classify_privileged_http_operation(
    std::string_view method, std::string_view path);
[[nodiscard]] PrivilegedOperationClass classify_privileged_websocket_command(
    std::string_view command);
[[nodiscard]] PrivilegedOperationClass classify_privileged_websocket_protocol_operation(
    std::string_view operation);
[[nodiscard]] PrivilegedNetworkModeParseResult parse_privileged_network_mode(
    const std::optional<std::string> &value);
[[nodiscard]] bool is_eligible_privileged_interface(
    const PrivilegedInterfaceCandidate &candidate);
[[nodiscard]] bool has_eligible_privileged_subnet(
    const std::vector<PrivilegedInterfaceCandidate> &candidates);
[[nodiscard]] bool is_forwarded_client_identity_header(std::string_view name);
