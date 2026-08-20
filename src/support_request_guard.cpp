#include "support_request_guard.hpp"
#include "privileged_network_policy.hpp"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <unistd.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>
#include <set>

namespace {
struct ParsedAddress { int family = AF_UNSPEC; std::array<unsigned char, 16> bytes{}; };
struct ParsedHost { std::string identity; std::optional<unsigned short> port; };

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    while (!value.empty() && value.back() == '.') value.pop_back();
    return value;
}

std::optional<unsigned short> strict_port(const std::string &value) {
    if (value.empty() || !std::all_of(value.begin(), value.end(), [](unsigned char c) { return c >= '0' && c <= '9'; })) return std::nullopt;
    unsigned short number = 0;
    for (const unsigned char c : value) {
        const unsigned short digit = static_cast<unsigned short>(c - '0');
        if (number > 6553 || (number == 6553 && digit > 5)) return std::nullopt;
        number = static_cast<unsigned short>(number * 10 + digit);
    }
    return number == 0 ? std::nullopt : std::optional<unsigned short>(static_cast<unsigned short>(number));
}

std::optional<ParsedAddress> address(const std::string &text) {
    ParsedAddress result;
    if (inet_pton(AF_INET, text.c_str(), result.bytes.data()) == 1) { result.family = AF_INET; return result; }
    if (inet_pton(AF_INET6, text.c_str(), result.bytes.data()) == 1) {
        // Normalize IPv4-mapped IPv6 peers and host literals.
        static constexpr unsigned char prefix[] = {0,0,0,0,0,0,0,0,0,0,0xff,0xff};
        if (std::memcmp(result.bytes.data(), prefix, sizeof(prefix)) == 0) {
            std::memmove(result.bytes.data(), result.bytes.data() + 12, 4); result.family = AF_INET;
        } else result.family = AF_INET6;
        return result;
    }
    return std::nullopt;
}

bool loopback(const ParsedAddress &value) {
    if (value.family == AF_INET) return value.bytes[0] == 127;
    static constexpr unsigned char loop[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
    return value.family == AF_INET6 && std::memcmp(value.bytes.data(), loop, sizeof(loop)) == 0;
}
bool prohibited(const ParsedAddress &value) {
    if (value.family == AF_INET) return value.bytes[0] == 0 || (value.bytes[0] & 0xf0) == 0xe0;
    static constexpr unsigned char zero[16] = {}; return value.family != AF_INET6 ||
        std::memcmp(value.bytes.data(), zero, 16) == 0 || value.bytes[0] == 0xff;
}
bool same_subnet(const ParsedAddress &peer, const ParsedAddress &network, const ParsedAddress &mask) {
    if (peer.family != network.family || network.family != mask.family) return false;
    const size_t length = peer.family == AF_INET ? 4 : 16;
    for (size_t index = 0; index < length; ++index)
        if ((peer.bytes[index] & mask.bytes[index]) != (network.bytes[index] & mask.bytes[index])) return false;
    return true;
}

std::optional<ParsedHost> host(const std::string &source) {
    if (source.empty() || source.find_first_of("/@?#") != std::string::npos) return std::nullopt;
    std::string name; std::optional<unsigned short> port;
    if (source.front() == '[') {
        const auto close = source.find(']'); if (close == std::string::npos) return std::nullopt;
        name = source.substr(1, close - 1); std::array<unsigned char, 16> literal{}; if (inet_pton(AF_INET6, name.c_str(), literal.data()) != 1) return std::nullopt;
        if (close + 1 < source.size()) { if (source[close + 1] != ':') return std::nullopt; port = strict_port(source.substr(close + 2)); if (!port) return std::nullopt; }
    } else {
        if (source.find(':') != source.rfind(':')) return std::nullopt;
        const auto colon = source.rfind(':'); name = colon == std::string::npos ? source : source.substr(0, colon);
        if (colon != std::string::npos) { port = strict_port(source.substr(colon + 1)); if (!port) return std::nullopt; }
    }
    if (name.empty() || name.find_first_of(" []\\") != std::string::npos) return std::nullopt;
    return ParsedHost{lower(name), port};
}

bool approved_host(const ParsedHost &candidate, const SupportRequestGuardSnapshot &snapshot) {
    const auto parsed = address(candidate.identity);
    if (parsed && loopback(*parsed)) return true;
    std::set<std::string> names{"localhost"};
    if (!snapshot.hostname.empty()) { const auto name = lower(snapshot.hostname); if (!name.empty()) { names.insert(name); if (name.ends_with(".local")) names.insert(name.substr(0, name.size() - 6)); else names.insert(name + ".local"); } }
    for (const auto &name : snapshot.configured_hostnames) { const auto normalized = lower(name); if (!normalized.empty()) names.insert(normalized); }
    if (names.contains(candidate.identity)) return true;
    if (parsed) {
        for (const auto &network : snapshot.networks) {
            const auto assigned = address(network.address);
            const size_t length = parsed->family == AF_INET ? 4 : 16;
            if (assigned && parsed->family == assigned->family && std::memcmp(parsed->bytes.data(), assigned->bytes.data(), length) == 0) return true;
        }
    }
    return false;
}

bool same_host_identity(const ParsedHost &left, const ParsedHost &right) {
    const auto left_address = address(left.identity);
    const auto right_address = address(right.identity);
    if (left_address || right_address) {
        if (!left_address || !right_address || left_address->family != right_address->family) return false;
        const size_t length = left_address->family == AF_INET ? 4 : 16;
        return std::memcmp(left_address->bytes.data(), right_address->bytes.data(), length) == 0;
    }
    return left.identity == right.identity;
}

std::optional<ParsedHost> origin_host(const std::string &origin, bool &https) {
    if (origin == "null") return std::nullopt;
    const auto scheme_end = origin.find("://"); if (scheme_end == std::string::npos) return std::nullopt;
    const auto scheme = lower(origin.substr(0, scheme_end)); if (scheme != "http" && scheme != "https") return std::nullopt;
    https = scheme == "https"; const auto authority = origin.substr(scheme_end + 3);
    if (authority.empty() || authority.find_first_of("/?#") != std::string::npos) return std::nullopt;
    return host(authority);
}

PrivilegedInterfaceKind physical_interface_kind(const std::string &name) {
    const auto begins = [&](std::string_view prefix) { return name.starts_with(prefix); };
    if (begins("wlan") || begins("wl")) return PrivilegedInterfaceKind::wifi;
    if (begins("eth") || begins("en")) return PrivilegedInterfaceKind::ethernet;
    return PrivilegedInterfaceKind::other;
}

bool excluded_interface_name(const std::string &name) {
    static constexpr std::array<std::string_view, 14> prefixes = {
        "lo", "tun", "tap", "utun", "wg", "tailscale", "zt", "docker",
        "veth", "br", "virbr", "cni", "podman", "awdl"};
    return std::any_of(prefixes.begin(), prefixes.end(),
                       [&](std::string_view prefix) { return name.starts_with(prefix); });
}
}

SupportRequestGuard::SupportRequestGuard(SupportRequestGuardSnapshot snapshot) : snapshot_(std::move(snapshot)) {}

SupportRequestGuardResult SupportRequestGuard::evaluate(const std::string &peer_address, const std::string &host_header, const std::optional<std::string> &origin_header, bool enforce_peer) const {
    const auto peer = address(peer_address);
    if (!peer || prohibited(*peer)) return {SupportRequestGuardDecision::rejected_peer};
    if (enforce_peer && !loopback(*peer)) {
        if (!snapshot_.discovery_succeeded) return {SupportRequestGuardDecision::interface_discovery_unavailable};
        bool matched = false;
        for (const auto &item : snapshot_.networks) { const auto network = address(item.address); const auto mask = address(item.netmask); if (network && mask && same_subnet(*peer, *network, *mask)) { matched = true; break; } }
        if (!matched) return {SupportRequestGuardDecision::rejected_peer};
    }
    const auto request_host = host(host_header); if (!request_host || !approved_host(*request_host, snapshot_)) return {SupportRequestGuardDecision::rejected_host};
    if (!origin_header) return {SupportRequestGuardDecision::allowed};
    bool https = false; const auto origin = origin_host(*origin_header, https);
    if (!origin || !approved_host(*origin, snapshot_) || !same_host_identity(*origin, *request_host)) return {SupportRequestGuardDecision::rejected_origin};
    // Explicit ports must match. With no Host port, permit the scheme default (80 or 443).
    const auto expected = request_host->port.value_or(https ? 443 : 80);
    if (origin->port.value_or(https ? 443 : 80) != expected) return {SupportRequestGuardDecision::rejected_origin};
    return {SupportRequestGuardDecision::allowed};
}

SupportRequestGuardSnapshot SupportRequestGuard::discover_local_networks() {
    SupportRequestGuardSnapshot result; char hostname[256]{};
    if (gethostname(hostname, sizeof(hostname) - 1) == 0) result.hostname = hostname;
    ifaddrs *interfaces = nullptr; if (getifaddrs(&interfaces) != 0) return result;
    for (auto *item = interfaces; item != nullptr; item = item->ifa_next) {
        if (!item->ifa_name || !item->ifa_addr || !item->ifa_netmask ||
            item->ifa_addr->sa_family != item->ifa_netmask->sa_family) continue;
        const std::string name = item->ifa_name;
        if (excluded_interface_name(name)) continue;
        char buffer[INET6_ADDRSTRLEN]{};
        std::string address_text;
        std::string mask_text;
        if (item->ifa_addr->sa_family == AF_INET) {
            const auto *a = reinterpret_cast<sockaddr_in *>(item->ifa_addr); const auto *m = reinterpret_cast<sockaddr_in *>(item->ifa_netmask);
            if (inet_ntop(AF_INET, &a->sin_addr, buffer, sizeof(buffer))) { address_text = buffer; if (inet_ntop(AF_INET, &m->sin_addr, buffer, sizeof(buffer))) mask_text = buffer; }
        } else if (item->ifa_addr->sa_family == AF_INET6) {
            const auto *a = reinterpret_cast<sockaddr_in6 *>(item->ifa_addr); const auto *m = reinterpret_cast<sockaddr_in6 *>(item->ifa_netmask);
            if (inet_ntop(AF_INET6, &a->sin6_addr, buffer, sizeof(buffer))) { address_text = buffer; if (inet_ntop(AF_INET6, &m->sin6_addr, buffer, sizeof(buffer))) mask_text = buffer; }
        }
        const PrivilegedInterfaceCandidate candidate{
            (item->ifa_flags & IFF_UP) != 0,
            physical_interface_kind(name),
            (item->ifa_flags & IFF_LOOPBACK) != 0,
            (item->ifa_flags & IFF_POINTOPOINT) != 0,
            false, false, false, false,
            address_text, mask_text};
        if (is_eligible_privileged_interface(candidate))
            result.networks.push_back({address_text, mask_text});
    }
    freeifaddrs(interfaces);
    result.discovery_succeeded = !result.networks.empty();
    return result;
}
