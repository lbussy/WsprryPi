#include "../rp1_gpclk_route_service.hpp"
#include "../WSPR-Transmitter/src/rp1_gpclk_development_policy.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {

nlohmann::json state(const std::string &configured = "gpio4",
                     const std::string &active = "gpio4") {
  return {
      {"identity",
       {{"package", "rp1-gpclk-dkms"},
        {"debianVersion", "1.1.1-1"},
        {"module", "rp1_gpclk_dkms"},
        {"moduleVersion", "1.1.1"},
        {"uapiSha256",
         "998ab96d7dbcc0d935c05758c46acba56bbcf92aa1b674b899bdab6932dc8384"},
        {"overlaySha256",
         {{"gpio4",
           "c3e17a685694928468bb18c24f5bb4e25454745d6989e6c9d2c2acf447b908d6"},
          {"gpio20", "8eaa8afae7f88a665fc9bec6da1b013be049b2a32c909c729caeff918"
                     "1bcf3aa"}}}}},
      {"configuredRoute", configured},
      {"activeRoute", active},
      {"bootId", "0fc31228-4c38-4f05-8053-afb9f04fba52"},
      {"bootOwnership", "current"},
      {"pendingTransaction", nullptr},
      {"historicalJournals",
       {{{"schemaVersion", 1},
         {"status", "complete"},
         {"preservation", "in-place-byte-exact"}}}},
      {"safety",
       {{"endpointOwned", true},
        {"endpointOpen", false},
        {"liveOutput", false},
        {"services",
         {{"wsprrypi.service", "active"},
          {"soapyremote-server.service", "inactive"}}}}}};
}

nlohmann::json response(const std::string &operation, const std::string &status,
                        const nlohmann::json &value) {
  return {{"schemaVersion", 1},
          {"contract", "rp1-gpclk-route-manager-v1"},
          {"operation", operation},
          {"status", status},
          {"state", value}};
}

wsprrypi::Rp1GpclkApplicationIdleState idle() { return {}; }

wsprrypi::Rp1GpclkDevelopmentPolicyInputs armed_for(std::uint32_t route) {
  wsprrypi::Rp1GpclkDevelopmentPolicyInputs inputs;
  inputs.development_testing_enabled = true;
  inputs.rp1_backend_selected = true;
  inputs.requested_route = route;
  return inputs;
}

struct StartupOutcome {
  nlohmann::json result;
  std::vector<nlohmann::json> requests;
  bool inhibited{false};
};

StartupOutcome startup(const nlohmann::json &next, int persisted,
                       const std::string &development_route = {}) {
  StartupOutcome outcome;
  wsprrypi::Rp1GpclkRouteService service(
      {[&](const nlohmann::json &request) {
         outcome.requests.push_back(request);
         return next;
       },
       idle, [persisted] { return persisted; },
       [](int, std::string *) { return true; },
       [&](bool value, const std::string &) { outcome.inhibited = value; }});
  outcome.result = development_route.empty()
      ? service.reconcileStartup()
      : service.reconcileDevelopmentStartup(development_route);
  if (!outcome.result.value("ok", false)) {
    const auto request_count = outcome.requests.size();
    const auto second = development_route.empty()
        ? service.reconcileStartup()
        : service.reconcileDevelopmentStartup(development_route);
    assert(second.at("result") == "startup_failure_latched");
    assert(outcome.requests.size() == request_count);
    assert(outcome.inhibited);
  }
  return outcome;
}

} // namespace

int main() {
  std::vector<nlohmann::json> requests;
  nlohmann::json next = response("query", "ok", state());
  int persisted = 4;
  bool persistence_ok = true;
  int persisted_write = 0;
  bool inhibited = false;

  wsprrypi::Rp1GpclkRouteService service(
      {[&](const nlohmann::json &request) {
         requests.push_back(request);
         return next;
       },
       idle, [&] { return persisted; },
       [&](int gpio, std::string *) {
         persisted_write = gpio;
         if (persistence_ok)
           persisted = gpio;
         return persistence_ok;
       },
       [&](bool value, const std::string &) { inhibited = value; }});

  const auto query = service.query();
  assert(query.at("ok") == true);
  assert(query.at("outputInhibitedValidated") == true);
  assert(query.at("compatible") == true);
  assert(query.at("eligible") == false);
  assert(query.at("liveQualification") == "Unavailable");
  assert(query.at("historicalPredecessorOutputInhibitedEvidence")
             .at("gpio4Initial")
             .at("transaction") == "48ef743c-e127-45e1-9994-901006283a2d");
  assert(query.at("historicalPredecessorOutputInhibitedEvidence").at("gpio20").at("journalSha256") ==
         "212177a69d4f8d702fd5d0e6f9c25033adc1178b37814ac3996a7ea2310aa168");
  assert(query.at("historicalPredecessorOutputInhibitedEvidence")
             .at("gpio4Restored")
             .at("transaction") == "7197a0b1-3f69-4bbd-9220-47ac9abc5e2c");
  assert(query.at("requested") == "GPIO4");
  assert(query.at("configured") == "GPIO4");
  assert(query.at("active") == "GPIO4");
  assert(query.at("bootOwnership") == "current");
  assert(query.at("journal") == "none");
  assert(requests.size() == 1);
  assert(requests.back() ==
         nlohmann::json({{"schemaVersion", 1}, {"operation", "query"}}));

  next = response("preflight", "ok", state());
  auto preflight = service.operate("preflight", "GPIO20", 0);
  assert(preflight.at("ok") == true);
  assert(preflight.at("preflightValidated") == true);
  assert(preflight.at("services").at("wsprrypi.service") == "active");
  const auto generation = preflight.at("generation").get<std::uint64_t>();
  assert(generation != 0);
  assert(requests.back().at("route") == "gpio20");

  auto unsafe = state();
  unsafe["safety"]["endpointOpen"] = true;
  next = response("preflight", "ok", unsafe);
  assert(service.operate("preflight", "GPIO20", 0).at("ok") == false);
  unsafe = state();
  unsafe["safety"]["endpointOwned"] = false;
  next = response("preflight", "ok", unsafe);
  assert(service.operate("preflight", "GPIO20", 0).at("ok") == false);
  unsafe = state();
  unsafe["safety"]["liveOutput"] = true;
  next = response("preflight", "ok", unsafe);
  assert(service.operate("preflight", "GPIO20", 0).at("ok") == false);
  unsafe = state();
  unsafe["identity"]["moduleVersion"] = "1.1.2";
  next = response("preflight", "ok", unsafe);
  assert(service.operate("preflight", "GPIO20", 0).at("ok") == false);

  next = response("preflight", "ok", state("gpio4", "gpio4"));
  preflight = service.operate("preflight", "GPIO20", 0);
  const auto apply_generation = preflight.at("generation").get<std::uint64_t>();
  next = response("apply-and-reboot", "reboot-requested",
                  state("gpio20", "gpio4"));
  const auto applied =
      service.operate("apply-and-reboot", "GPIO20", apply_generation);
  assert(applied.at("ok") == true);
  assert(persisted_write == 20);
  assert(requests.back().at("operation") == "apply-and-reboot");
  assert(requests.back().at("route") == "gpio20");
  assert(requests.back().at("execute") == true);
  assert(requests.back().at("actor") == "wsprrypi.service");
  assert(!requests.back().contains("path"));
  assert(!requests.back().contains("service"));
  assert(!requests.back().contains("command"));

  persistence_ok = false;
  next = response("preflight", "ok", state("gpio20", "gpio20"));
  preflight = service.operate("preflight", "GPIO4", 0);
  const auto before_failed_apply = requests.size();
  assert(service
             .operate("apply-and-reboot", "GPIO4",
                      preflight.at("generation").get<std::uint64_t>())
             .at("result") == "persistence_failed");
  assert(requests.size() == before_failed_apply);
  persistence_ok = true;

  next = response("reconcile", "complete", state("gpio20", "gpio20"));
  const auto reconciled = service.reconcileStartup();
  assert(reconciled.at("reconciled") == true);
  assert(reconciled.at("compatible") == true);
  assert(inhibited == false);

  auto wrong_package = state("gpio20", "gpio20");
  wrong_package["identity"]["moduleVersion"] = "1.1.2";
  wsprrypi::armRp1GpclkDevelopmentOperation(
      armed_for(wsprrypi::kRp1GpclkDevelopmentRouteGpio20));
  const auto packaged_mismatch =
      startup(response("reconcile", "complete", wrong_package), 20);
  assert(packaged_mismatch.result.at("compatible") == false);
  assert(packaged_mismatch.inhibited);
  assert(!wsprrypi::rp1GpclkDevelopmentOperationArmedForRoute(
      wsprrypi::kRp1GpclkDevelopmentRouteGpio20));

  auto pending = state("gpio20", "gpio4");
  pending["pendingTransaction"] = {{"status", "awaiting-reboot"}};
  const auto awaiting =
      startup(response("reconcile", "awaiting-reboot", pending), 20);
  assert(awaiting.result.at("ok") == false);
  assert(awaiting.result.at("journal") == "pending");
  assert(awaiting.inhibited);

  const auto mismatch =
      startup(response("reconcile", "mismatch", state("gpio20", "gpio4")), 20);
  assert(mismatch.result.at("ok") == false);
  assert(mismatch.result.at("state") == "mismatch");
  assert(mismatch.inhibited);

  auto recovery_state = state("gpio20", "gpio4");
  recovery_state["pendingTransaction"] = {{"status", "interrupted"}};
  const auto recovery = startup(
      response("reconcile", "recovery-required", recovery_state), 20);
  assert(recovery.result.at("ok") == false);
  assert(recovery.result.at("state") == "rollback_required");
  assert(recovery.inhibited);

  auto predecessor = state();
  predecessor["identity"]["debianVersion"] = "1.0.0-1";
  predecessor["identity"]["moduleVersion"] = "1.0.0";
  for (const auto &route : {std::string("GPIO4"), std::string("GPIO20")}) {
    const int gpio = route == "GPIO4" ? 4 : 20;
    auto route_state = predecessor;
    route_state["configuredRoute"] = gpio == 4 ? "gpio4" : "gpio20";
    route_state["activeRoute"] = gpio == 4 ? "gpio4" : "gpio20";
    const auto development_route = gpio == 4
        ? wsprrypi::kRp1GpclkDevelopmentRouteGpio4
        : wsprrypi::kRp1GpclkDevelopmentRouteGpio20;
    wsprrypi::armRp1GpclkDevelopmentOperation(armed_for(development_route));
    const auto development =
        startup(response("query", "ok", route_state), gpio, route);
    assert(development.result.at("ok") == true);
    assert(development.result.at("policyDomain") == "source-development");
    assert(development.result.at("packageIdentityRequired") == false);
    assert(development.result.at("developmentIdentityRequired") == true);
    assert(development.result.at("developmentUapiAbi") == 3);
    assert(development.result.at("developmentCompatibilityState") ==
           "Experimental");
    assert(development.requests.size() == 1);
    assert(development.requests.front().at("operation") == "query");
    assert(!development.requests.front().contains("execute"));
    assert(!development.inhibited);
    assert(wsprrypi::rp1GpclkDevelopmentOperationArmedForRoute(
        development_route));
    wsprrypi::invalidateRp1GpclkDevelopmentOperation();
  }

  const auto wrong_development_route =
      startup(response("query", "ok", state("gpio20", "gpio20")), 20,
              "GPIO4");
  assert(wrong_development_route.result.at("ok") == false);
  assert(wrong_development_route.inhibited);

  auto ambiguous = state();
  ambiguous["pendingTransaction"] = {{"status", "awaiting-reboot"}};
  const auto ambiguous_development =
      startup(response("query", "ok", ambiguous), 4, "GPIO4");
  assert(ambiguous_development.result.at("ok") == false);
  assert(ambiguous_development.inhibited);

  next = response("rollback", "rolled-back", state("gpio4", "gpio4"));
  const auto rolled_back = service.operate("rollback", "GPIO4", 7);
  assert(rolled_back.at("ok") == true);
  assert(requests.back().at("operation") == "rollback");
  assert(requests.back().at("execute") == true);
  assert(!requests.back().contains("route"));

  next = {{"schemaVersion", 2},
          {"contract", "foreign"},
          {"status", "ok"},
          {"state", state()}};
  assert(service.query().at("result") == "contract_mismatch");

  std::cout << "rp1_gpclk_route_service_test: PASS\n";
}
