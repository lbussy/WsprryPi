#include "../backend_http_guard.hpp"

#include <cassert>
#include <iostream>

int main() {
    const SupportRequestGuardSnapshot snapshot{
        true, "wsprrypi", {}, {{"192.168.50.10", "255.255.255.0"}}};
    const auto evaluate = [&](std::string method, std::string path,
                              std::string peer, std::string host,
                              std::optional<std::string> origin = std::nullopt) {
        return evaluate_backend_http_request(
            method, path, peer, host, origin, snapshot);
    };

    assert(evaluate("PUT", "/config", "127.0.0.1", "wsprrypi") ==
           BackendHttpGuardDecision::allowed);
    assert(evaluate("PATCH", "/config", "192.168.50.42", "wsprrypi:31415",
                    "http://wsprrypi:31415") == BackendHttpGuardDecision::allowed);
    assert(evaluate("POST", "/control/stop", "192.168.51.42", "wsprrypi") ==
           BackendHttpGuardDecision::rejected);
    assert(evaluate("POST", "/api/support-bundles", "192.168.50.42", "evil") ==
           BackendHttpGuardDecision::rejected);
    assert(evaluate("POST", "/api/network-safety", "192.168.51.42", "wsprrypi") ==
           BackendHttpGuardDecision::rejected);
    assert(evaluate("GET", "/api/network-safety", "192.168.50.42", "wsprrypi") ==
           BackendHttpGuardDecision::allowed);
    assert(evaluate("GET", "/api/support-bundles/job/download", "192.168.50.42",
                    "wsprrypi", "http://evil") == BackendHttpGuardDecision::rejected);
    assert(evaluate("GET", "/config", "203.0.113.4", "foreign") ==
           BackendHttpGuardDecision::rejected);
    assert(evaluate("GET", "/config", "192.168.50.42", "wsprrypi") ==
           BackendHttpGuardDecision::allowed);
    assert(evaluate("POST", "/future-operation", "127.0.0.1", "localhost") ==
           BackendHttpGuardDecision::allowed);
    assert(evaluate("POST", "/config/future-operation", "127.0.0.1", "localhost") ==
           BackendHttpGuardDecision::rejected);

    // Forwarded client identity is intentionally absent from the guard API and
    // therefore cannot turn this actual off-LAN peer into an allowed request.
    assert(evaluate("PUT", "/config", "203.0.113.4", "wsprrypi") ==
           BackendHttpGuardDecision::rejected);
    assert(evaluate_backend_http_request(
               "PUT", "/config", "203.0.113.4", "wsprrypi", std::nullopt,
               snapshot, PrivilegedNetworkMode::insecure_disabled) ==
           BackendHttpGuardDecision::allowed);
    assert(evaluate_backend_http_request(
               "PUT", "/config", "203.0.113.4", "evil", std::nullopt,
               snapshot, PrivilegedNetworkMode::insecure_disabled) ==
           BackendHttpGuardDecision::rejected);
    assert(evaluate_backend_http_request(
               "GET", "/config", "203.0.113.4", "wsprrypi", std::nullopt,
               snapshot, PrivilegedNetworkMode::insecure_disabled) ==
           BackendHttpGuardDecision::rejected);

    std::cout << "backend_http_guard_test: PASS\n";
}
