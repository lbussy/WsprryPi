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
constexpr const char *kRuntimeContract = "rp1-gpclk-route-manager-runtime-v1";
constexpr const char *kContract = "rp1-gpclk-route-manager-v1";
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
  const timeval timeout{30, 0};
  if (::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) ||
      ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout))) {
    ::close(fd);
    throw std::runtime_error("Could not bound the route-manager connection.");
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
          {"state", runtime_profile_ ? "runtime_recovery" : "unavailable"},
          {"profile", runtime_profile_ ? "runtime" : "legacy"},
          {"message", message},
          {"generation", generation_},
          {"requested", "Unavailable"},
          {"persisted", routeForGpio(operations_.persisted_gpio())},
          {"configured", "Unavailable"},
          {"active", "Unavailable"},
          {"reconciled", false},
          {"compatible", false},
          {"eligible", false},
          {"liveQualification", "Unavailable"},
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
    if (response.is_object() && response.value("schemaVersion", 0) == 3 &&
        response.value("contract", std::string{}) == kRuntimeContract &&
        response.contains("status") &&
        (response.contains("state") || response.contains("error"))) {
      runtime_profile_ = true;
      operations_.set_transmission_inhibited(true, "Runtime route administration is output-disabled");
      return response;
    }
    if (runtime_profile_ || !response.is_object() || response.value("schemaVersion", 0) != 1 ||
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
  if (runtime_profile_) {
    auto result = failure("runtime_inhibited", "Switching restarts a running Wsprry Pi in idle mode. Transmission does not resume.");
    const bool valid = state.is_object() && state.value("profile", std::string{}) == "runtime" &&
        state.contains("outputEnabled") && state["outputEnabled"] == false &&
        state.contains("qualification") && state["qualification"] == false &&
        state.contains("controller") && state["controller"].is_object();
    result["profile"] = "runtime";
    result["contractIdentity"] = kRuntimeContract;
    result["ok"] = valid && status != "error";
    result["compatible"] = valid;
    result["state"] = status == "error" ? "runtime_recovery" : "runtime_inhibited";
    result["active"] = routeForGpio(gpioForRoute(field(state, "activeRoute")));
    result["requested"] = requested.empty() ? routeForGpio(operations_.persisted_gpio()) : requested;
    result["liveOutput"] = "Disabled";
    result["bootOwnership"] = "runtime controller";
    result["journal"] = state.contains("pendingTransaction") && state["pendingTransaction"].is_object()
        ? field(state["pendingTransaction"], "phase") : "none";
    result["preflightValidated"] = valid && status == "ok" &&
        state.value("applicationRestorationVersion", 0) == 1 && field(state, "preflightToken").size() == 64;
    if (!field(state, "preflightToken").empty() && state.value("applicationRestorationVersion", 0) != 1) {
      result["message"] = "Upgrade the route manager before switching: this version cannot restore application availability.";
    }
    result["controller"] = state.value("controller", nlohmann::json::object());
    if (valid) {
      const auto phase = result["journal"].get<std::string>();
      if ((state["controller"].value("flags", 0) & 1) ||
          (phase != "none" && phase != "complete-inhibited" && phase != "recovered-inhibited"))
        result["state"] = "runtime_recovery";
    }
    if (state.contains("application") && state["application"].is_object()) {
      const auto &application = state["application"];
      const auto phase = field(application, "phase");
      result["application"] = application;
      result["services"] = {{"wsprrypi.service", phase}};
      if ((phase == "restored" || phase == "stopped" || phase == "administrator-masked") &&
          valid && application.value("controller", nlohmann::json::object()) == state["controller"] &&
          state["controller"].value("flags", 0) == 6 && state["controller"].value("error", -1) == 0 &&
          routeForGpio(operations_.persisted_gpio()) == field(result, "active") &&
          field(application, "boot") == field(state, "bootId") &&
          field(application, "binding") == field(state, "bindingSha256")) {
        result["state"] = "runtime_ready";
        result["configured"] = result["active"];
        result["reconciled"] = true;
        result["journal"] = "none";
        result["message"] = phase == "restored"
            ? "Last route switch restored Wsprry Pi in idle mode. Transmission was not resumed."
            : phase == "stopped" ? "Route switched; Wsprry Pi remains stopped as requested by its prior state."
            : "Route switched; the administrator's service mask is preserved.";
      } else if (phase == "restored" || phase == "stopped" || phase == "administrator-masked") {
        result["state"] = "runtime_recovery";
        result["ok"] = false;
        result["message"] = "The last restoration record does not match the current route and application configuration. Inspect the route before switching.";
      } else if (phase == "restoration-failed") {
        result["state"] = "runtime_restoration_failed";
        result["ok"] = false;
        result["message"] = "Route is installed, but application restoration failed. Run runtime_route_client.py restore --execute. " + field(application, "error");
      } else if (phase == "route-failed" || phase == "route-recovered") {
        result["state"] = "runtime_recovery";
      } else {
        result["state"] = "runtime_restoring";
        result["message"] = "Route administration is in progress. Refresh status after Wsprry Pi reconnects.";
      }
    }
    if (response.contains("error")) {
      result["message"] = field(response["error"], "message");
      result["error"] = response["error"];
      if (response["error"].contains("kernelError") && response["error"].contains("overlayId"))
        result["message"] = field(response["error"], "message") + " Kernel errno: " +
            response["error"]["kernelError"].dump() + "; retained overlay ID: " +
            response["error"]["overlayId"].dump() + ". Transmission remains inhibited.";
    }
    return result;
  }
  const std::string configured = routeForGpio(
                        gpioForRoute(field(state, "configuredRoute"))),
                    active =
                        routeForGpio(gpioForRoute(field(state, "activeRoute"))),
                    persisted = routeForGpio(operations_.persisted_gpio());
  const bool pending = state.contains("pendingTransaction") &&
                       !state["pendingTransaction"].is_null();
  const bool aligned =
      !persisted.empty() && persisted == configured && persisted == active;
  std::string ui = pending ? "pending"
                   : aligned ? "active" : "mismatch";
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
          {"compatible", aligned},
          {"preflightValidated", preflight_safe},
          {"eligible", false},
          {"liveQualification", "Unavailable"},
          {"contractIdentity", kContract},
          {"moduleRoute", active},
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
bool Rp1GpclkRouteService::acknowledgeRestoration(const std::string &token,
                                                 bool transmit) {
  std::lock_guard<std::mutex> guard(mutex_);
  if (transmit || !idle() || startup_failure_latched_)
    return false;
  const int gpio = operations_.persisted_gpio();
  if (gpio != 4 && gpio != 20)
    return false;
  const auto reply = request({{"schemaVersion", 3}, {"operation", "application-ready"},
      {"route", gpio == 4 ? "gpio4" : "gpio20"}, {"token", token},
      {"pid", static_cast<int>(::getpid())}, {"transmit", false}});
  return reply.value("status", std::string{}) == "ok";
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
  if (runtime_profile_) {
    if (!idle())
      return failure("not_idle", "Runtime administration requires the complete application lifecycle to be idle.");
    if (operation == "preflight") {
      runtime_token_.clear();
      preflight_route_.clear();
      if (executor_route.empty())
        return failure("invalid_route", "Route must be GPIO4 or GPIO20.");
      auto raw = request({{"schemaVersion", 3}, {"operation", "preflight"}, {"route", executor_route}});
      auto result = render(raw, route);
      if (result.value("ok", false) && result.value("preflightValidated", false)) {
        runtime_token_ = field(raw["state"], "preflightToken");
        preflight_route_ = executor_route;
        result["generation"] = ++generation_;
      } else {
        result["ok"] = false;
      }
      return result;
    }
    if (operation != "switch" && operation != "recover")
      return failure("invalid_operation", "Runtime profile requires explicit switch or recover; reboot operations are not translated.");
    if (operation == "switch" && (generation == 0 || generation != generation_ ||
        runtime_token_.empty() || executor_route != preflight_route_))
      return failure("generation_mismatch", "Repeat runtime preflight before switching.");
    operations_.set_transmission_inhibited(true, "Runtime route administration in progress");
    nlohmann::json value = {{"schemaVersion", 3}, {"operation", operation}, {"execute", true},
        {"actor", "wsprrypi.service"}, {"requestId", requestId(operation.c_str(), ++generation_)}};
    if (operation == "switch") {
      value["route"] = executor_route;
      value["preflightToken"] = runtime_token_;
      // The token binds the ID across application restarts as well as preflights.
      value["requestId"] = "wsprrypi-" + runtime_token_.substr(0, 48);
      // The privileged completion workflow persists the pin only after the
      // overlay succeeds, independently of this application's lifetime.
    }
    runtime_token_.clear();
    preflight_route_.clear();
    return render(request(value), route);
  }
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
        "External route-manager preflight passed. Product and RF "
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
      true, "RP1 GPCLK route reconciliation is pending");
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
        "RP1 GPCLK startup reconciliation is incomplete or mismatched.";
  }
  startup_failure_latched_ = !reconciled;
  operations_.set_transmission_inhibited(
      !reconciled, reconciled ? "" : "RP1 GPCLK reconciliation is incomplete");
  return rendered;
}
nlohmann::json Rp1GpclkRouteService::reconcileRuntime(
    const std::string &route, bool development) {
  const int gpio = gpioForRoute(route);
  if (!gpio || gpio != operations_.persisted_gpio())
    return failure("runtime_route_mismatch", "Requested and saved runtime route differ.");
  const auto raw = request({{"schemaVersion",3},
      {"operation",development ? "reconcile-output" : "idle"},
      {"route",gpio == 4 ? "gpio4" : "gpio20"}});
  const auto state = raw.value("state", nlohmann::json::object());
  const auto output = state.value("outputLifecycle", nlohmann::json::object());
  const auto controller = output.value("controller", nlohmann::json::object());
  const bool valid = raw.value("status", std::string{}) == "ok" &&
      output.value("ready", false) && !output.value("executionAuthorized", true) &&
      output.value("route", std::string{}) == (gpio == 4 ? "gpio4" : "gpio20") &&
      output.value("bootId", std::string{}) == state.value("bootId", std::string{}) &&
      !output.value("bootId", std::string{}).empty() &&
      output.value("bindingSha256", std::string{}) == state.value("bindingSha256", std::string{}) &&
      output.value("bindingSha256", std::string{}).size() == 64 &&
      controller == state.value("controller", nlohmann::json::object()) &&
      controller.value("route", 0) == (gpio == 4 ? 1 : 2) &&
      controller.value("flags", 0) == 6 && controller.value("error", -1) == 0 &&
      controller.value("session", 0ULL) != 0 && controller.value("generation", 0ULL) != 0;
  if (!valid) {
    const auto error = raw.value("error", nlohmann::json::object());
    return failure("runtime_reconciliation_failed", error.value("message",
        std::string("Runtime route reconciliation failed; output remains inhibited.")));
  }
  auto result = failure("runtime_reconciled", "Runtime route reconciled; existing operation authorization is still required.");
  result.update({{"ok",true},{"reconciled",true},{"compatible",true},
      {"requested",route},{"persisted",route},{"configured",route},{"active",route},
      {"journal","none"},{"bootOwnership","runtime"},{"endpointOwned",true},
      {"endpointOpen",false},{"liveOutput","disabled"},
      {"generation",++generation_},
      {"controllerGeneration",controller["generation"]},
      {"policyDomain",development ? "external-provider" : "startup-idle"},
      {"executionAuthorized",false},{"qualification",false}});
  operations_.set_transmission_inhibited(!development,
      development ? "" : "Runtime idle startup awaits an operation authorization");
  return result;
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

  auto discovered = request({{"schemaVersion", 1}, {"operation", "query"}});
  if (runtime_profile_) {
    auto result = reconcileRuntime(expected, false);
    startup_failure_latched_ = !result.value("ok", false);
    return result;
  }
  auto rendered = render(discovered, expected);
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
      "RP1 GPCLK route is reconciled for safe idle startup; a current "
      "operation-scoped authorization remains required for development output.";
  operations_.set_transmission_inhibited(
      true,
      "RP1 GPCLK development output awaits a bounded authorization");
  return rendered;
}
nlohmann::json Rp1GpclkRouteService::reconcileDevelopmentStartup(
    const std::string &route) {
  std::lock_guard<std::mutex> guard(mutex_);
  operations_.set_transmission_inhibited(
      true, "RP1 GPCLK route reconciliation is pending");
  if (startup_failure_latched_)
    return failure("startup_failure_latched",
                   "A prior RP1 GPCLK startup reconciliation failure keeps "
                   "transmission inhibited for this process lifetime.");
  const std::string expected = routeForGpio(gpioForRoute(route));
  if (expected.empty()) {
    startup_failure_latched_ = true;
    return failure("invalid_route",
                   "Development startup requires GPIO4 or GPIO20.");
  }

  ++generation_;
  auto discovered = request({{"schemaVersion", 1}, {"operation", "query"}});
  if (runtime_profile_) return reconcileRuntime(expected, true);
  auto rendered = render(discovered, expected);
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
  rendered["policyDomain"] = "external-provider";
  rendered["ok"] = reconciled;
  rendered["result"] = reconciled ? "development_route_reconciled"
                                    : "development_route_failed";
  rendered["message"] = reconciled
      ? "Externally provisioned route state reconciled; operation authorization and runtime provider checks remain required."
      : "Externally provisioned route state is incomplete, stale, ambiguous, or mismatched.";
  startup_failure_latched_ = !reconciled;
  operations_.set_transmission_inhibited(
      !reconciled,
      reconciled ? "" : "RP1 GPCLK route reconciliation is incomplete");
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
