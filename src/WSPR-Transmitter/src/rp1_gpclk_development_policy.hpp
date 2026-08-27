/* SPDX-License-Identifier: MIT */
#pragma once

#include "rp1_gpclk_backend.hpp"

#include <cstdint>
#include <string>
#include <optional>
#include <string_view>

namespace wsprrypi
{
inline constexpr std::uint16_t kRp1GpclkDevelopmentUapiAbi = 3;
inline constexpr std::uint32_t kRp1GpclkDevelopmentRouteGpio4 = 1;
inline constexpr std::uint32_t kRp1GpclkDevelopmentRouteGpio20 = 2;
inline constexpr std::uint32_t kRp1GpclkDevelopmentCompatibilityExperimental = 2;
inline constexpr std::uint64_t kRp1GpclkDevelopmentCapabilityLiveEligible = 1ULL << 7;
inline constexpr std::string_view kRp1GpclkDevelopmentSourceRevision =
    "7421605e0a0e41c19c6d7142a9fa87ea3a42eb98";
inline constexpr std::string_view kRp1GpclkDevelopmentModuleId = "rp1-gpclk-dkms";
inline constexpr std::string_view kRp1GpclkDevelopmentModuleVersion = "1.1.2";
inline constexpr std::string_view kRp1GpclkDevelopmentUapiSha256 =
    "f0af5ffda91f4ba82285dc278452eae28b2eeffa635ebd6ee473bf7393a6a54e";
inline constexpr std::string_view kRp1GpclkDevelopmentGpio4Compatibility =
    "v1.1.2-pi5-gpio4-6.18.34-development-candidate-r3";
inline constexpr std::string_view kRp1GpclkDevelopmentGpio20Compatibility =
    "v1.1.2-pi5-gpio20-6.18.34-development-candidate-r3";

enum class Rp1GpclkDevelopmentDenial
{
    allowed,
    development_testing_disabled,
    unsupported_backend,
    invalid_route,
    zero_route_topology,
    ambiguous_topology,
    configured_active_mismatch,
    unknown_identity,
    version_uapi_mismatch,
    route_identity_mismatch,
    compatibility_not_experimental,
    live_output_unverified,
    unresolved_route_transaction,
    scheduler_conflict,
    transmission_owner_conflict,
    endpoint_unavailable,
    endpoint_already_owned,
    stale_operator_confirmation,
    physical_topology_unconfirmed,
    cleanup_fault,
    internal_state_invalid
};

struct Rp1GpclkDevelopmentPolicyInputs
{
    bool development_testing_enabled{false};
    bool rp1_backend_selected{false};
    std::uint32_t requested_route{0};
    std::uint32_t persisted_route{0};
    std::uint32_t configured_route{0};
    std::uint32_t active_route{0};
    std::uint32_t module_route{0};
    unsigned active_route_count{0};
    bool route_transaction_resolved{false};
    bool route_manager_attributable{false};
    bool scheduler_idle{false};
    bool application_owns_operation{false};
    bool endpoint_available{false};
    bool endpoint_closed{false};
    bool endpoint_exclusively_acquirable{false};
    bool cleanup_fault{false};
    bool live_output_verified{false};
    bool physical_connection_confirmed{false};
    bool attenuation_and_load_confirmed{false};
    bool bounded_operation_confirmed{false};
    bool non_radiating_topology_confirmed{false};
    bool experimental_status_acknowledged{false};
    bool confirmation_current{false};
    std::uint64_t route_transaction_generation{0};
    std::uint64_t confirmation_route_transaction_generation{0};
    std::string operation_id;
    std::string confirmation_operation_id;
    std::string confirmation_identity;
    std::uint32_t confirmation_route{0};
    Rp1GpclkProviderIdentity identity;
};

struct Rp1GpclkDevelopmentDecision
{
    bool allowed{false};
    Rp1GpclkDevelopmentDenial reason{
        Rp1GpclkDevelopmentDenial::development_testing_disabled};
    const char* code{"development-testing-disabled"};
    const char* explanation{"RP1 GPCLK development testing is disabled."};
    const char* warning{
        "Experimental development use only; this is not qualification or normal product eligibility."};
};

Rp1GpclkDevelopmentDecision decideRp1GpclkDevelopmentUse(
    const Rp1GpclkDevelopmentPolicyInputs& inputs) noexcept;
std::string rp1GpclkDevelopmentIdentityBinding(
    const Rp1GpclkProviderIdentity& identity);
std::optional<Rp1GpclkProviderIdentity>
rp1GpclkExpectedDevelopmentIdentity(std::uint32_t route);

void armRp1GpclkDevelopmentOperation(
    Rp1GpclkDevelopmentPolicyInputs inputs);
std::optional<Rp1GpclkDevelopmentPolicyInputs>
consumeRp1GpclkDevelopmentOperation(
    const std::string& operation_id,
    std::uint32_t requested_route,
    const Rp1GpclkProviderIdentity& identity);
void invalidateRp1GpclkDevelopmentOperation() noexcept;
bool rp1GpclkDevelopmentOperationArmedForRoute(
    std::uint32_t requested_route) noexcept;
} // namespace wsprrypi
