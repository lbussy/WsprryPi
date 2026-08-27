#include "rp1_gpclk_route_service.hpp"
#include "WSPR-Transmitter/src/rp1_gpclk_development_policy.hpp"
#ifndef WSPRRYPI_ROUTE_SERVICE_TEST
#include "config_handler.hpp"
#include "scheduling.hpp"
#endif
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
namespace wsprrypi {
namespace {
#ifndef WSPRRYPI_ROUTE_SERVICE_TEST
constexpr const char *kSocket = "/run/rp1-gpclk-dkms/route-manager.sock";
#endif
constexpr const char *kContract = "rp1-gpclk-route-manager-v1";
constexpr const char *kPackageSha256 =
    "247bd7da35e4ad812a13828668fe03673da127bad7ed2b3e970876f3f21c002d";
const std::string kDevelopmentSourceIdentity =
    "RP1-GPCLK-DKMS@" + std::string(kRp1GpclkDevelopmentSourceRevision) +
    "; package-unreleased";
constexpr const char *kEvidenceArchiveSha256 =
    "af4bb75d7d747a6e9bab067c563fba4031db08c1ed1800c3cb4c8c4d2587561e";
constexpr const char *kEvidenceManifestSha256 =
    "0078e69f6886282ce4822bacf03b32056cd47dedf5f0cd3fc6357484c0379a29";
const nlohmann::json &outputInhibitedEvidenceBindings() {
  static const nlohmann::json bindings = {
      {"gpio4Initial",
       {{"transaction", "48ef743c-e127-45e1-9994-901006283a2d"},
        {"journalSha256",
         "b5dc50842151f6719980ec5d7d06a0d12f514074215684929d5eb55dc71b361e"}}},
      {"gpio20",
       {{"transaction", "14470a51-dede-4ebc-badb-8e63d8789a65"},
        {"journalSha256",
         "212177a69d4f8d702fd5d0e6f9c25033adc1178b37814ac3996a7ea2310aa168"}}},
      {"gpio4Restored",
       {{"transaction", "7197a0b1-3f69-4bbd-9220-47ac9abc5e2c"},
        {"journalSha256",
         "244b8604293b30912ec79a4b9fd4a4ad8b9caa899657c912542ef01b2dd49d9d"}}}};
  return bindings;
}
#ifndef WSPRRYPI_ROUTE_SERVICE_TEST
nlohmann::json socketRequest(const nlohmann::json &request) {
  const int fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    throw std::runtime_error(
        "Could not create the RP1 GPCLK route-manager socket.");
  if (::fcntl(fd, F_SETFD, FD_CLOEXEC) != 0) {
    ::close(fd);
    throw std::runtime_error(
        "Could not secure the RP1 GPCLK route-manager socket.");
  }
  sockaddr_un address{};
  address.sun_family = AF_UNIX;
  if (std::strlen(kSocket) >= sizeof(address.sun_path)) {
    ::close(fd);
    throw std::runtime_error("The fixed route-manager socket path is invalid.");
  }
  std::strcpy(address.sun_path, kSocket);
  if (::connect(fd, reinterpret_cast<sockaddr *>(&address), sizeof(address)) !=
      0) {
    const std::string detail = std::strerror(errno);
    ::close(fd);
    throw std::runtime_error(
        "Could not connect to the RP1 GPCLK route manager: " + detail);
  }
  const std::string payload = request.dump() + "\n";
  std::size_t offset = 0;
  while (offset < payload.size()) {
    const auto count = ::send(fd, payload.data() + offset,
                              payload.size() - offset, MSG_NOSIGNAL);
    if (count < 0 && errno == EINTR)
      continue;
    if (count <= 0) {
      ::close(fd);
      throw std::runtime_error("Could not send the RP1 GPCLK route request.");
    }
    offset += static_cast<std::size_t>(count);
  }
  if (::shutdown(fd, SHUT_WR) != 0) {
    ::close(fd);
    throw std::runtime_error("Could not finish the RP1 GPCLK route request.");
  }
  std::string response;
  char buffer[4096];
  for (;;) {
    const auto count = ::read(fd, buffer, sizeof(buffer));
    if (count < 0 && errno == EINTR)
      continue;
    if (count < 0) {
      ::close(fd);
      throw std::runtime_error("Could not read the RP1 GPCLK route response.");
    }
    if (count == 0)
      break;
    response.append(buffer, static_cast<std::size_t>(count));
    if (response.size() > 1024U * 1024U) {
      ::close(fd);
      throw std::runtime_error(
          "The RP1 GPCLK route response exceeded its bound.");
    }
  }
  ::close(fd);
  if (response.empty())
    throw std::runtime_error(
        "The RP1 GPCLK route manager returned no response.");
  return nlohmann::json::parse(response);
}
#endif
std::string field(const nlohmann::json &value, const char *name) {
  return value.contains(name) && value[name].is_string()
             ? value[name].get<std::string>()
             : std::string{};
}
bool exactPackagedIdentity(const nlohmann::json &state) {
  if (!state.contains("identity") || !state["identity"].is_object())
    return false;
  const auto &identity = state["identity"];
  if (field(identity, "package") != "rp1-gpclk-dkms" ||
      field(identity, "debianVersion") != "1.1.1-1" ||
      field(identity, "module") != "rp1_gpclk_dkms" ||
      field(identity, "moduleVersion") != "1.1.1" ||
      field(identity, "uapiSha256") !=
          "998ab96d7dbcc0d935c05758c46acba56bbcf92aa1b674b899bdab6932dc8384" ||
      !identity.contains("overlaySha256") ||
      !identity["overlaySha256"].is_object())
    return false;
  const auto &overlays = identity["overlaySha256"];
  return field(overlays, "gpio4") == "c3e17a685694928468bb18c24f5bb4e25454745d6"
                                     "989e6c9d2c2acf447b908d6" &&
         field(overlays, "gpio20") ==
             "8eaa8afae7f88a665fc9bec6da1b013be049b2a32c909c729caeff9181bcf3aa";
}
} // namespace
Rp1GpclkRouteService::Rp1GpclkRouteService(
    Rp1GpclkRouteExecutorOperations operations)
    : operations_(std::move(operations)) {}
std::string Rp1GpclkRouteService::routeForGpio(int gpio) {
  return gpio == 4 ? "GPIO4" : gpio == 20 ? "GPIO20" : std::string{};
}
int Rp1GpclkRouteService::gpioForRoute(const std::string &route) {
  return route == "gpio4" || route == "GPIO4"     ? 4
         : route == "gpio20" || route == "GPIO20" ? 20
                                                  : 0;
}
bool Rp1GpclkRouteService::idle() const {
  return operations_.idle_state && operations_.idle_state().complete();
}
std::string Rp1GpclkRouteService::requestId(const char *operation,
                                            std::uint64_t generation) {
  std::ostringstream value;
  value << "wsprrypi-" << operation << '-' << std::hex << std::setw(8)
        << std::setfill('0') << generation;
  return value.str();
}
nlohmann::json Rp1GpclkRouteService::failure(const std::string &result,
                                             const std::string &message) const {
  return {{"ok", false},
          {"result", result},
          {"state", "unavailable"},
          {"message", message},
          {"generation", generation_},
          {"requested", "Unavailable"},
          {"persisted", routeForGpio(operations_.persisted_gpio())},
          {"configured", "Unavailable"},
          {"active", "Unavailable"},
          {"reconciled", false},
          {"compatible", false},
          {"outputInhibitedValidated", false},
          {"eligible", false},
          {"liveQualification", "Unavailable"},
          {"packageIdentity", kPackageSha256},
          {"contractIdentity", kContract},
          {"bootOwnership", "unknown"},
          {"journal", "unknown"},
          {"services", nlohmann::json::object()},
          {"endpointOwned", false},
          {"endpointOpen", true},
          {"liveOutput", "unknown"}};
}
nlohmann::json Rp1GpclkRouteService::request(const nlohmann::json &value) {
  try {
    const auto response = operations_.request(value);
    if (!response.is_object() || response.value("schemaVersion", 0) != 1 ||
        response.value("contract", std::string{}) != kContract ||
        !response.contains("status") ||
        (response.contains("state") == response.contains("error")))
      return failure("contract_mismatch",
                     "The RP1 GPCLK route-manager response contract differs.");
    return response;
  } catch (const std::exception &error) {
    return failure("provider_unavailable", error.what());
  }
}
nlohmann::json Rp1GpclkRouteService::render(const nlohmann::json &response,
                                            const std::string &requested) {
  if (response.contains("ok"))
    return response;
  const std::string status = field(response, "status");
  if (!response.contains("state")) {
    const auto message = response.contains("error")
                             ? field(response["error"], "message")
                             : "Route-manager request failed.";
    return failure("route_manager_error", message);
  }
  const auto &state = response["state"];
  const std::string configured = routeForGpio(
                        gpioForRoute(field(state, "configuredRoute"))),
                    active =
                        routeForGpio(gpioForRoute(field(state, "activeRoute"))),
                    persisted = routeForGpio(operations_.persisted_gpio());
  const bool pending = state.contains("pendingTransaction") &&
                       !state["pendingTransaction"].is_null();
  const bool identity_matches = exactPackagedIdentity(state);
  const bool aligned =
      !persisted.empty() && persisted == configured && persisted == active;
  const bool output_inhibited_validated =
      identity_matches && (configured == "GPIO4" || configured == "GPIO20");
  std::string ui = pending ? "pending"
                   : aligned && output_inhibited_validated
                       ? "output-inhibited-validated"
                   : aligned ? "compatible-unqualified"
                             : "mismatch";
  if (status == "awaiting-reboot" || status == "reboot-requested")
    ui = "staged";
  else if (status == "recovery-required")
    ui = "rollback_required";
  const nlohmann::json safety =
      state.contains("safety") && state["safety"].is_object()
          ? state["safety"]
          : nlohmann::json::object();
  const bool live_output_disabled = safety.contains("liveOutput") &&
                                    safety["liveOutput"].is_boolean() &&
                                    !safety["liveOutput"].get<bool>();
  const bool preflight_safe = safety.value("endpointOwned", false) &&
                              !safety.value("endpointOpen", true) &&
                              live_output_disabled;
  const bool accepted = status == "ok" || status == "complete" ||
                        status == "rolled-back" || status == "reboot-requested";
  return {{"ok", accepted},
          {"result", status},
          {"state", ui},
          {"message", ""},
          {"generation", generation_},
          {"requested", requested.empty()
                            ? persisted
                            : routeForGpio(gpioForRoute(requested))},
          {"persisted", persisted},
          {"configured", configured},
          {"active", active},
          {"reconciled", aligned && !pending},
          {"compatible", identity_matches},
          {"outputInhibitedValidated", output_inhibited_validated},
          {"preflightValidated", preflight_safe},
          {"eligible", false},
          {"liveQualification", "Unavailable"},
          {"packageIdentity", kPackageSha256},
          {"contractIdentity", kContract},
          {"moduleVersion", "1.1.1"},
          {"uapiAbi", 1},
          {"compatibilityState", "Unavailable"},
          {"moduleRoute", active},
          {"historicalPredecessorEvidenceArchiveSha256", kEvidenceArchiveSha256},
          {"historicalPredecessorEvidenceManifestSha256", kEvidenceManifestSha256},
          {"historicalPredecessorOutputInhibitedEvidence",
           outputInhibitedEvidenceBindings()},
          {"bootOwnership", field(state, "bootOwnership")},
          {"bootId", field(state, "bootId")},
          {"journal", pending ? "pending" : "none"},
          {"services", safety.value("services", nlohmann::json::object())},
          {"endpointOwned", safety.value("endpointOwned", false)},
          {"endpointOpen", safety.value("endpointOpen", true)},
          {"liveOutput",
           safety.contains("liveOutput") && safety["liveOutput"].is_boolean()
               ? nlohmann::json(safety["liveOutput"].get<bool>() ? "enabled"
                                                                 : "disabled")
               : nlohmann::json("unknown")}};
}
nlohmann::json Rp1GpclkRouteService::query() {
  std::lock_guard<std::mutex> guard(mutex_);
  return render(request({{"schemaVersion", 1}, {"operation", "query"}}));
}
nlohmann::json Rp1GpclkRouteService::operate(const std::string &operation,
                                             const std::string &route,
                                             std::uint64_t generation) {
  std::lock_guard<std::mutex> guard(mutex_);
  invalidateRp1GpclkDevelopmentOperation();
  const int gpio = gpioForRoute(route);
  const std::string executor_route = gpio == 4    ? "gpio4"
                                     : gpio == 20 ? "gpio20"
                                                  : "";
  if (operation == "preflight") {
    if (!idle())
      return failure("not_idle",
                     "Route changes require the controller, scheduler, "
                     "provider, drain, and cleanup lifecycle to be idle.");
    if (executor_route.empty())
      return failure("invalid_route", "Route must be exactly GPIO4 or GPIO20.");
    auto raw = request({{"schemaVersion", 1},
                        {"operation", "preflight"},
                        {"route", executor_route}});
    if (!raw.contains("state"))
      return render(raw, route);
    auto rendered = render(raw, route);
    if (!rendered.value("ok", false))
      return rendered;
    if (!rendered.value("compatible", false) ||
        !rendered.value("preflightValidated", false)) {
      rendered["ok"] = false;
      rendered["result"] = "preflight_failed";
      rendered["message"] =
          "Preflight did not confirm the exact packaged identity, endpoint "
          "closure, ownership, and live_output=0.";
      return rendered;
    }
    ++generation_;
    preflight_route_ = executor_route;
    rendered["generation"] = generation_;
    rendered["message"] =
        "Exact packaged route-manager preflight passed. Product and RF "
        "qualification remain unavailable.";
    return rendered;
  }
  if (operation == "apply-and-reboot") {
    if (!idle())
      return failure(
          "not_idle",
          "Route changes require a completely idle application lifecycle.");
    if (generation == 0 || generation != generation_ ||
        executor_route != preflight_route_)
      return failure("generation_mismatch",
                     "The RP1 GPCLK preflight generation is stale.");
    std::string persistence_error;
    if (!operations_.persist_gpio ||
        !operations_.persist_gpio(gpio, &persistence_error))
      return failure("persistence_failed",
                     persistence_error.empty()
                         ? "Could not persist the requested route."
                         : persistence_error);
    const auto raw = request({{"schemaVersion", 1},
                              {"operation", "apply-and-reboot"},
                              {"route", executor_route},
                              {"execute", true},
                              {"requestId", requestId("apply", generation)},
                              {"actor", "wsprrypi.service"}});
    return render(raw, route);
  }
  if (operation == "rollback") {
    const auto raw = request({{"schemaVersion", 1},
                              {"operation", "rollback"},
                              {"execute", true},
                              {"requestId", requestId("rollback", generation)},
                              {"actor", "wsprrypi.service"}});
    return render(raw);
  }
  return failure("invalid_operation",
                 "Operation must be preflight, apply-and-reboot, or rollback.");
}
nlohmann::json Rp1GpclkRouteService::reconcileStartup() {
  std::lock_guard<std::mutex> guard(mutex_);
  invalidateRp1GpclkDevelopmentOperation();
  operations_.set_transmission_inhibited(
      true, "RP1 GPCLK exact-package reconciliation is pending");
  if (startup_failure_latched_)
    return failure("startup_failure_latched",
                   "A prior RP1 GPCLK startup reconciliation failure keeps "
                   "transmission inhibited for this process lifetime.");
  const auto raw =
      request({{"schemaVersion", 1},
               {"operation", "reconcile"},
               {"execute", true},
               {"requestId", requestId("reconcile", ++generation_)},
               {"actor", "wsprrypi.service"}});
  auto rendered = render(raw);
  const bool reconciled = rendered.value("ok", false) &&
                          rendered.value("reconciled", false) &&
                          rendered.value("compatible", false) &&
                          rendered.value("journal", std::string{}) == "none";
  rendered["ok"] = reconciled;
  if (!reconciled) {
    rendered["result"] = "startup_reconciliation_failed";
    rendered["message"] =
        "Exact packaged startup reconciliation is incomplete or mismatched.";
  }
  startup_failure_latched_ = !reconciled;
  operations_.set_transmission_inhibited(
      !reconciled, reconciled ? "" : "RP1 GPCLK reconciliation is incomplete");
  return rendered;
}
nlohmann::json Rp1GpclkRouteService::reconcileIdleStartup(
    const std::string &route) {
  std::lock_guard<std::mutex> guard(mutex_);
  operations_.set_transmission_inhibited(
      true, "RP1 GPCLK idle startup reconciliation is pending");
  if (startup_failure_latched_)
    return failure("startup_failure_latched",
                   "A prior RP1 GPCLK startup reconciliation failure keeps "
                   "transmission inhibited for this process lifetime.");
  const std::string expected = routeForGpio(gpioForRoute(route));
  if (expected.empty()) {
    startup_failure_latched_ = true;
    return failure("invalid_route",
                   "RP1 GPCLK idle startup requires GPIO4 or GPIO20.");
  }

  auto rendered = render(request({{"schemaVersion", 1},
                                  {"operation", "query"}}), expected);
  const auto exact_route_state = [&expected](const nlohmann::json &value) {
    return value.value("ok", false) &&
           value.value("reconciled", false) &&
           value.value("requested", std::string{}) == expected &&
           value.value("persisted", std::string{}) == expected &&
           value.value("configured", std::string{}) == expected &&
           value.value("active", std::string{}) == expected &&
           value.value("journal", std::string{}) == "none" &&
           value.value("bootOwnership", std::string{}) == "current";
  };
  if (!exact_route_state(rendered)) {
    startup_failure_latched_ = true;
    rendered["ok"] = false;
    rendered["result"] = "idle_startup_reconciliation_failed";
    rendered["message"] =
        "RP1 GPCLK idle startup route state is incomplete, stale, ambiguous, or mismatched.";
    return rendered;
  }

  if (rendered.value("compatible", false)) {
    const auto raw = request({{"schemaVersion", 1},
                              {"operation", "reconcile"},
                              {"execute", true},
                              {"requestId", requestId("reconcile", ++generation_)},
                              {"actor", "wsprrypi.service"}});
    rendered = render(raw, expected);
    const bool reconciled = exact_route_state(rendered) &&
                            rendered.value("compatible", false);
    rendered["ok"] = reconciled;
    rendered["policyDomain"] = "packaged";
    rendered["executionAuthorized"] = false;
    if (!reconciled) {
      startup_failure_latched_ = true;
      rendered["result"] = "startup_reconciliation_failed";
      rendered["message"] =
          "Exact packaged startup reconciliation is incomplete or mismatched.";
    }
    operations_.set_transmission_inhibited(
        !reconciled,
        reconciled ? "" : "RP1 GPCLK reconciliation is incomplete");
    return rendered;
  }

  if (!rendered.value("endpointOwned", false) ||
      rendered.value("endpointOpen", true)) {
    startup_failure_latched_ = true;
    rendered["ok"] = false;
    rendered["result"] = "idle_startup_endpoint_unsafe";
    rendered["message"] =
        "RP1 GPCLK idle startup requires the endpoint to be correctly owned and closed.";
    return rendered;
  }

  rendered["ok"] = true;
  rendered["result"] = "idle_route_reconciled";
  rendered["policyDomain"] = "startup-idle";
  rendered["executionAuthorized"] = false;
  rendered["developmentAuthorizationRequired"] = true;
  rendered["message"] =
      "RP1 GPCLK route is reconciled for safe idle startup; an exact "
      "operation-scoped authorization remains required for source-development output.";
  operations_.set_transmission_inhibited(
      true,
      "RP1 GPCLK source-development output awaits an exact bounded authorization");
  return rendered;
}
nlohmann::json Rp1GpclkRouteService::reconcileDevelopmentStartup(
    const std::string &route) {
  std::lock_guard<std::mutex> guard(mutex_);
  operations_.set_transmission_inhibited(
      true, "RP1 GPCLK source-development reconciliation is pending");
  if (startup_failure_latched_)
    return failure("startup_failure_latched",
                   "A prior RP1 GPCLK startup reconciliation failure keeps "
                   "transmission inhibited for this process lifetime.");
  const std::string expected = routeForGpio(gpioForRoute(route));
  if (expected.empty()) {
    startup_failure_latched_ = true;
    return failure("invalid_route",
                   "Source-development startup requires GPIO4 or GPIO20.");
  }

  ++generation_;
  auto rendered = render(request({{"schemaVersion", 1},
                                  {"operation", "query"}}), expected);
  const bool reconciled = rendered.value("ok", false) &&
                          rendered.value("reconciled", false) &&
                          rendered.value("requested", std::string{}) == expected &&
                          rendered.value("persisted", std::string{}) == expected &&
                          rendered.value("configured", std::string{}) == expected &&
                          rendered.value("active", std::string{}) == expected &&
                          rendered.value("journal", std::string{}) == "none" &&
                          rendered.value("bootOwnership", std::string{}) == "current" &&
                          rendered.value("endpointOwned", false) &&
                          !rendered.value("endpointOpen", true);
  rendered["policyDomain"] = "source-development";
  rendered["developmentSourceIdentity"] = kDevelopmentSourceIdentity;
  rendered["developmentModuleVersion"] =
      kRp1GpclkDevelopmentModuleVersion.data();
  rendered["developmentUapiAbi"] = 3;
  rendered["developmentCompatibilityState"] = "Experimental";
  rendered["packageIdentityRequired"] = false;
  rendered["developmentIdentityRequired"] = true;
  rendered["ok"] = reconciled;
  rendered["result"] = reconciled ? "development_route_reconciled"
                                    : "development_route_failed";
  rendered["message"] = reconciled
      ? "Exact-source development route state reconciled; provider identity and operation authorization remain required."
      : "Exact-source development route state is incomplete, stale, ambiguous, or mismatched.";
  startup_failure_latched_ = !reconciled;
  operations_.set_transmission_inhibited(
      !reconciled,
      reconciled ? "" : "RP1 GPCLK source-development reconciliation is incomplete");
  return rendered;
}
#ifndef WSPRRYPI_ROUTE_SERVICE_TEST
Rp1GpclkRouteService &productionRp1GpclkRouteService() {
  static Rp1GpclkRouteService service(
      {socketRequest, rp1_gpclk_application_idle_state,
       [] { return config.gpio_tx_pin; },
       [](int gpio, std::string *error) {
         return persist_rp1_gpclk_route_config(gpio, error);
       },
       [](bool inhibited, const std::string &) {
         set_rp1_route_transaction_inhibited(inhibited);
       }});
  return service;
}
#endif
} // namespace wsprrypi
