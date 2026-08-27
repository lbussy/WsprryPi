#pragma once
#include "WSPR-Transmitter/src/rp1_gpclk_application_idle.hpp"
#include "json.hpp"
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
namespace wsprrypi {
struct Rp1GpclkRouteExecutorOperations {
  std::function<nlohmann::json(const nlohmann::json &)> request;
  std::function<Rp1GpclkApplicationIdleState()> idle_state;
  std::function<int()> persisted_gpio;
  std::function<bool(int, std::string *)> persist_gpio;
  std::function<void(bool, const std::string &)> set_transmission_inhibited;
};
class Rp1GpclkRouteService {
public:
  explicit Rp1GpclkRouteService(Rp1GpclkRouteExecutorOperations);
  nlohmann::json query();
  nlohmann::json operate(const std::string &, const std::string &,
                         std::uint64_t);
  nlohmann::json reconcileStartup();
  nlohmann::json reconcileDevelopmentStartup(const std::string &route);

private:
  nlohmann::json request(const nlohmann::json &);
  nlohmann::json render(const nlohmann::json &,
                        const std::string &requested = {});
  nlohmann::json failure(const std::string &, const std::string &) const;
  bool idle() const;
  static std::string routeForGpio(int);
  static int gpioForRoute(const std::string &);
  static std::string requestId(const char *, std::uint64_t);
  Rp1GpclkRouteExecutorOperations operations_;
  std::mutex mutex_;
  std::uint64_t generation_{0};
  std::string preflight_route_;
  bool startup_failure_latched_{false};
};
Rp1GpclkRouteService &productionRp1GpclkRouteService();
} // namespace wsprrypi
