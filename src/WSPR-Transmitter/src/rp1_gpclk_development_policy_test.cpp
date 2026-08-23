/* SPDX-License-Identifier: MIT */
#include "rp1_gpclk_development_policy.hpp"
#include <cassert>
#include <iostream>

using namespace wsprrypi;

static Rp1GpclkDevelopmentPolicyInputs allowed(std::uint32_t route)
{
    Rp1GpclkDevelopmentPolicyInputs i;
    i.development_testing_enabled = i.rp1_backend_selected = true;
    i.requested_route = i.persisted_route = i.configured_route = i.active_route = i.module_route = route;
    i.active_route_count = 1; i.route_transaction_resolved = i.route_manager_attributable = true;
    i.scheduler_idle = i.application_owns_operation = true;
    i.endpoint_available = i.endpoint_closed = i.endpoint_exclusively_acquirable = true;
    i.live_output_verified = i.physical_connection_confirmed =
        i.attenuation_and_load_confirmed = i.bounded_operation_confirmed =
        i.non_radiating_topology_confirmed = i.experimental_status_acknowledged =
        i.confirmation_current = true;
    i.route_transaction_generation = i.confirmation_route_transaction_generation = 17;
    i.operation_id = i.confirmation_operation_id = "bounded-operation-7"; i.confirmation_route = route;
    i.identity.abi_min = 2; i.identity.abi_max = 2; i.identity.route = route;
    i.identity.compatibility_state = kRp1GpclkDevelopmentCompatibilityExperimental;
    i.identity.capabilities = kRp1GpclkDevelopmentCapabilityLiveEligible;
    i.identity.module_id = "rp1-gpclk-dkms"; i.identity.build_id = "1.1.2";
    i.identity.compatibility_id = route == kRp1GpclkDevelopmentRouteGpio4
        ? "v1.1.2-pi5-gpio4-6.18.34-development-candidate-r2"
        : "v1.1.2-pi5-gpio20-6.18.34-development-candidate-r2";
    i.confirmation_identity = rp1GpclkDevelopmentIdentityBinding(i.identity);
    return i;
}

int main()
{
    Rp1GpclkDevelopmentPolicyInputs defaults;
    assert(!decideRp1GpclkDevelopmentUse(defaults).allowed);
    for (auto route : {kRp1GpclkDevelopmentRouteGpio4, kRp1GpclkDevelopmentRouteGpio20})
        assert(decideRp1GpclkDevelopmentUse(allowed(route)).allowed);
    auto i = allowed(kRp1GpclkDevelopmentRouteGpio4);
    i.identity.compatibility_id = "v1.1.2-pi5-gpio20-6.18.34-development-candidate-r2";
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::route_identity_mismatch);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.confirmation_route = kRp1GpclkDevelopmentRouteGpio20;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::stale_operator_confirmation);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.identity.build_id = "1.1.1";
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::version_uapi_mismatch);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.identity.compatibility_state = 1;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::compatibility_not_experimental);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.live_output_verified = false;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::live_output_unverified);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.active_route_count = 2;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::ambiguous_topology);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.configured_route = kRp1GpclkDevelopmentRouteGpio20;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::configured_active_mismatch);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.route_transaction_resolved = false;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::unresolved_route_transaction);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.scheduler_idle = false;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::scheduler_conflict);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.endpoint_closed = false;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::endpoint_already_owned);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.cleanup_fault = true;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::cleanup_fault);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.physical_connection_confirmed = false;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::physical_topology_unconfirmed);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.identity.build_id = "1.1.2-changed";
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::version_uapi_mismatch);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.confirmation_route_transaction_generation++;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::stale_operator_confirmation);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.rp1_backend_selected = false;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::unsupported_backend);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.active_route_count = 0;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::zero_route_topology);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.identity.module_id.clear();
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::unknown_identity);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.route_manager_attributable = false;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::unresolved_route_transaction);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.application_owns_operation = false;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::transmission_owner_conflict);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.endpoint_available = false;
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::endpoint_unavailable);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4); i.confirmation_identity += "-stale";
    assert(decideRp1GpclkDevelopmentUse(i).reason == Rp1GpclkDevelopmentDenial::stale_operator_confirmation);
    i = allowed(kRp1GpclkDevelopmentRouteGpio4);
    armRp1GpclkDevelopmentOperation(i);
    invalidateRp1GpclkDevelopmentOperation();
    assert(!consumeRp1GpclkDevelopmentOperation(
        i.operation_id, i.requested_route, i.identity).has_value());
    armRp1GpclkDevelopmentOperation(i);
    assert(!consumeRp1GpclkDevelopmentOperation(
        i.operation_id, kRp1GpclkDevelopmentRouteGpio20, i.identity).has_value());
    armRp1GpclkDevelopmentOperation(i);
    assert(consumeRp1GpclkDevelopmentOperation(
        i.operation_id, i.requested_route, i.identity).has_value());
    assert(!consumeRp1GpclkDevelopmentOperation(
        i.operation_id, i.requested_route, i.identity).has_value());
    std::cout << "RP1 GPCLK guarded development policy tests passed\n";
}
