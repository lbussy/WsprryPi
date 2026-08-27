/* SPDX-License-Identifier: MIT */
#include "rp1_gpclk_development_policy.hpp"
#include <mutex>

namespace
{
using D = wsprrypi::Rp1GpclkDevelopmentDenial;
using R = wsprrypi::Rp1GpclkDevelopmentDecision;

R deny(D reason, const char* code, const char* explanation) noexcept
{
    return {false, reason, code, explanation,
        "Experimental development use only; this is not qualification or normal product eligibility."};
}

bool known_route(std::uint32_t route) noexcept
{
    return route == wsprrypi::kRp1GpclkDevelopmentRouteGpio4 ||
        route == wsprrypi::kRp1GpclkDevelopmentRouteGpio20;
}

const char* expected_compatibility(std::uint32_t route) noexcept
{
    return route == wsprrypi::kRp1GpclkDevelopmentRouteGpio4
        ? wsprrypi::kRp1GpclkDevelopmentGpio4Compatibility.data()
        : route == wsprrypi::kRp1GpclkDevelopmentRouteGpio20
            ? wsprrypi::kRp1GpclkDevelopmentGpio20Compatibility.data() : "";
}

std::mutex authorization_mutex;
std::optional<wsprrypi::Rp1GpclkDevelopmentPolicyInputs> armed_operation;
} // namespace

namespace wsprrypi
{
std::optional<Rp1GpclkProviderIdentity>
rp1GpclkExpectedDevelopmentIdentity(std::uint32_t route)
{
    if (!known_route(route))
        return std::nullopt;
    Rp1GpclkProviderIdentity identity;
    identity.abi_min = identity.abi_max = kRp1GpclkDevelopmentUapiAbi;
    identity.route = route;
    identity.compatibility_state = kRp1GpclkDevelopmentCompatibilityExperimental;
    identity.module_id = kRp1GpclkDevelopmentModuleId;
    identity.build_id = kRp1GpclkDevelopmentModuleVersion;
    identity.compatibility_id = expected_compatibility(route);
    return identity;
}

std::string rp1GpclkDevelopmentIdentityBinding(
    const Rp1GpclkProviderIdentity& identity)
{
    return identity.module_id + "|" + identity.build_id + "|" +
        identity.compatibility_id + "|" + std::to_string(identity.abi_min) +
        "|" + std::to_string(identity.abi_max) + "|" +
        std::to_string(identity.route) + "|" +
        std::to_string(identity.compatibility_state);
}

void armRp1GpclkDevelopmentOperation(Rp1GpclkDevelopmentPolicyInputs inputs)
{
    std::lock_guard<std::mutex> lock(authorization_mutex);
    armed_operation = std::move(inputs);
}

std::optional<Rp1GpclkDevelopmentPolicyInputs>
consumeRp1GpclkDevelopmentOperation(
    const std::string& operation_id,
    std::uint32_t requested_route,
    const Rp1GpclkProviderIdentity& identity)
{
    std::lock_guard<std::mutex> lock(authorization_mutex);
    if (!armed_operation)
        return std::nullopt;
    auto result = std::move(*armed_operation);
    armed_operation.reset();
    if (result.operation_id != operation_id ||
        result.requested_route != requested_route)
        return std::nullopt;
    result.identity = identity;
    result.module_route = identity.route;
    return result;
}

void invalidateRp1GpclkDevelopmentOperation() noexcept
{
    std::lock_guard<std::mutex> lock(authorization_mutex);
    armed_operation.reset();
}

bool rp1GpclkDevelopmentOperationArmedForRoute(
    std::uint32_t requested_route) noexcept
{
    std::lock_guard<std::mutex> lock(authorization_mutex);
    return armed_operation &&
        armed_operation->development_testing_enabled &&
        armed_operation->rp1_backend_selected &&
        known_route(requested_route) &&
        armed_operation->requested_route == requested_route;
}

Rp1GpclkDevelopmentDecision decideRp1GpclkDevelopmentUse(
    const Rp1GpclkDevelopmentPolicyInputs& i) noexcept
{
    if (!i.development_testing_enabled)
        return deny(D::development_testing_disabled, "development-testing-disabled", "RP1 GPCLK development testing requires a deliberate operation-scoped enablement.");
    if (!i.rp1_backend_selected)
        return deny(D::unsupported_backend, "unsupported-backend", "The operation is not using the RP1 GPCLK backend.");
    if (!known_route(i.requested_route) || !known_route(i.persisted_route))
        return deny(D::invalid_route, "invalid-route", "An explicit GPIO4 or GPIO20 route is required.");
    if (i.active_route_count == 0)
        return deny(D::zero_route_topology, "zero-route-topology", "No active RP1 GPCLK route was established.");
    if (i.active_route_count != 1)
        return deny(D::ambiguous_topology, "ambiguous-topology", "Exactly one RP1 GPCLK route must be active.");
    if (i.requested_route != i.persisted_route || i.persisted_route != i.configured_route ||
        i.configured_route != i.active_route || i.active_route != i.module_route)
        return deny(D::configured_active_mismatch, "configured-active-mismatch", "Requested, saved, boot-configured, active, and module-reported routes must agree exactly.");
    if (i.identity.module_id.empty() || i.identity.build_id.empty() || i.identity.compatibility_id.empty())
        return deny(D::unknown_identity, "unknown-identity", "The complete module identity could not be established.");
    const auto expected = rp1GpclkExpectedDevelopmentIdentity(i.requested_route);
    if (!expected || i.identity.abi_min > kRp1GpclkDevelopmentUapiAbi ||
        i.identity.abi_max < kRp1GpclkDevelopmentUapiAbi ||
        i.identity.module_id != expected->module_id ||
        i.identity.build_id != expected->build_id)
        return deny(D::version_uapi_mismatch, "version-uapi-mismatch", "The module must report the exact reviewed 1.1.2 ABI v3 development identity.");
    if (i.identity.route != i.requested_route ||
        i.identity.compatibility_id != expected->compatibility_id)
        return deny(D::route_identity_mismatch, "route-identity-mismatch", "The exact r3 compatibility identity does not match the selected route.");
    if (i.identity.compatibility_state != kRp1GpclkDevelopmentCompatibilityExperimental)
        return deny(D::compatibility_not_experimental, "compatibility-not-experimental", "The reviewed development identity must be reported as Experimental.");
    if ((i.identity.capabilities & kRp1GpclkDevelopmentCapabilityLiveEligible) == 0 || !i.live_output_verified)
        return deny(D::live_output_unverified, "live-output-unverified", "The exact module instance must affirmatively report live eligibility with live_output=1.");
    if (!i.route_transaction_resolved || !i.route_manager_attributable)
        return deny(D::unresolved_route_transaction, "unresolved-route-transaction", "Route-manager state is unresolved or cannot be attributed.");
    if (!i.scheduler_idle)
        return deny(D::scheduler_conflict, "scheduler-conflict", "Scheduling must be idle before development output.");
    if (!i.application_owns_operation)
        return deny(D::transmission_owner_conflict, "transmission-owner-conflict", "WsprryPi does not exclusively own the intended operation.");
    if (!i.endpoint_available)
        return deny(D::endpoint_unavailable, "endpoint-unavailable", "The canonical endpoint is unavailable or was replaced.");
    if (!i.endpoint_closed || !i.endpoint_exclusively_acquirable)
        return deny(D::endpoint_already_owned, "endpoint-already-owned", "The endpoint is open, busy, or cannot be acquired exclusively.");
    if (i.cleanup_fault)
        return deny(D::cleanup_fault, "cleanup-fault", "A cleanup fault or unsafe terminal state is latched.");
    if (!i.confirmation_current || i.operation_id.empty() ||
        i.confirmation_operation_id != i.operation_id || i.confirmation_route != i.requested_route ||
        i.route_transaction_generation == 0 ||
        i.confirmation_route_transaction_generation != i.route_transaction_generation ||
        i.confirmation_identity != rp1GpclkDevelopmentIdentityBinding(i.identity))
        return deny(D::stale_operator_confirmation, "stale-operator-confirmation", "Operator confirmation is absent, stale, or bound to another route or operation.");
    if (!i.physical_connection_confirmed || !i.attenuation_and_load_confirmed ||
        !i.bounded_operation_confirmed || !i.non_radiating_topology_confirmed ||
        !i.experimental_status_acknowledged)
        return deny(D::physical_topology_unconfirmed, "physical-topology-unconfirmed", "Current physical routing, attenuation, load, and non-radiating topology confirmation is required.");
    return {true, D::allowed, "development-testing-allowed",
        "All guarded development gates passed for the exact selected route and operation.",
        "Experimental development use only; this is not qualification or normal product eligibility."};
}
} // namespace wsprrypi
