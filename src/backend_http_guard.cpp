#include "backend_http_guard.hpp"

#include "privileged_network_policy.hpp"

BackendHttpGuardDecision evaluate_backend_http_request(
    std::string_view method,
    std::string_view path,
    const std::string &peer_address,
    const std::string &host_header,
    const std::optional<std::string> &origin_header,
    const SupportRequestGuardSnapshot &snapshot) {
    const auto classification = classify_privileged_http_operation(method, path);
    if (classification == PrivilegedOperationClass::reject) {
        const bool protected_candidate =
            path == "/config" || path.starts_with("/config/") ||
            path == "/control" || path.starts_with("/control/") ||
            path == "/api/support-bundles" ||
            path.starts_with("/api/support-bundles/");
        if (protected_candidate) return BackendHttpGuardDecision::rejected;
    }
    return SupportRequestGuard(snapshot).evaluate(
               peer_address, host_header, origin_header).allowed()
               ? BackendHttpGuardDecision::allowed
               : BackendHttpGuardDecision::rejected;
}
