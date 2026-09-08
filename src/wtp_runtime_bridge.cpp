// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Lee Bussy
#include "wtp_runtime_bridge.hpp"
#include "json.hpp"
#include "wtp_integration/usb_cdc.hpp"
#include <cstdlib>
#include <memory>
#include <mutex>
#include <stdexcept>
namespace {
struct NativeRuntime {
  WtpSettings settings;
  wsprrypi::WtpSystemScheduleClock clock{wsprrypi::wtp_host_utc_valid};
  wsprrypi::PosixCdcSystem system;
  wsprrypi::UsbCdcStream stream{system};
  std::unique_ptr<wsprrypi::WtpApplication> app;
  explicit NativeRuntime(WtpSettings s)
      : settings(std::move(s)),
        app(std::make_unique<wsprrypi::WtpApplication>(
            clock, stream, settings,
            wsprrypi::wtp::SessionOptions{wsprrypi::wtp_random_identity(),
                                          wsprrypi::wtp_random_identity(),
                                          settings.device_id},
            [this] {
              if (const char *disabled =
                      std::getenv("WSPRRYPI_DISABLE_HARDWARE_ACCESS");
                  disabled && std::string(disabled) == "1")
                return false;
              stream.close();
              if (!stream.begin_open(
                      {settings.path, settings.usb_serial,
                       static_cast<std::uint16_t>(settings.vendor_id),
                       static_cast<std::uint16_t>(settings.product_id)},
                      clock.now_ms()))
                return false;
              while (stream.state() == wsprrypi::CdcState::Resetting) {
                stream.poll_open(clock.now_ms());
                if (stream.state() == wsprrypi::CdcState::Resetting)
                  clock.wait_ms(10);
              }
              return stream.state() == wsprrypi::CdcState::Ready;
            })) {}
  NativeRuntime(WtpSettings s, wsprrypi::WtpScheduleClock &c,
                wsprrypi::wtp::ByteStream &b,
                wsprrypi::wtp::SessionOptions options,
                std::function<bool()> reopen)
      : settings(std::move(s)),
        app(std::make_unique<wsprrypi::WtpApplication>(
            c, b, settings, std::move(options), std::move(reopen))) {}
};
std::mutex operation_mutex, runtime_mutex;
std::shared_ptr<NativeRuntime> runtime;
std::shared_ptr<NativeRuntime> get() {
  std::lock_guard lock(runtime_mutex);
  return runtime;
}
std::shared_ptr<NativeRuntime> require() {
  auto r = get();
  if (!r)
    throw std::runtime_error("Pico backend is not selected");
  return r;
}
} // namespace
void set_wtp_runtime_for_test(WtpSettings settings,
                              wsprrypi::WtpScheduleClock &clock,
                              wsprrypi::wtp::ByteStream &stream,
                              wsprrypi::wtp::SessionOptions options,
                              std::function<bool()> reopen) {
  std::lock_guard operation(operation_mutex);
  std::lock_guard lock(runtime_mutex);
  if (runtime)
    throw std::logic_error("Test runtime must start unselected");
  runtime =
      std::make_shared<NativeRuntime>(std::move(settings), clock, stream,
                                      std::move(options), std::move(reopen));
}
std::string
wtp_runtime_selection_error(const std::optional<WtpSettings> &settings) {
  std::lock_guard operation(operation_mutex);
  auto r = get();
  if (r && (!settings || r->settings != *settings) && !r->app->replaceable())
    return "Resolve Pico ownership and output before changing the backend or "
           "endpoint";
  return {};
}
void wtp_runtime_prepare_skip() {
  std::lock_guard operation(operation_mutex);
  require()->app->prepare_skip();
}
void select_wtp_runtime(const std::optional<WtpSettings> &settings) {
  std::lock_guard operation(operation_mutex);
  std::lock_guard lock(runtime_mutex);
  if (runtime && settings && runtime->settings == *settings)
    return;
  if (runtime && !runtime->app->replaceable())
    throw std::runtime_error("Resolve Pico ownership and output before "
                             "changing the backend or endpoint");
  runtime = settings ? std::make_shared<NativeRuntime>(*settings) : nullptr;
}
bool wtp_runtime_selected() noexcept { return static_cast<bool>(get()); }
bool wtp_runtime_ready() noexcept {
  auto r = get();
  return r && r->app->ready();
}
bool wtp_runtime_invalidate_for_reload() {
  std::lock_guard operation(operation_mutex);
  auto r = get();
  if (!r)
    return false;
  r->app
      ->invalidate(); // CAS decides whether waiting work or committed work won.
  const auto phase = r->app->phase();
  if (phase == wsprrypi::WtpSchedulePhase::Preparing ||
      phase == wsprrypi::WtpSchedulePhase::Executing)
    return true;
  (void)r->app->stop();
  return false;
}
bool wtp_runtime_defers_reload() noexcept {
  auto r = get();
  if (!r)
    return false;
  auto phase = r->app->status().phase;
  return phase == wsprrypi::WtpSchedulePhase::Preparing ||
         phase == wsprrypi::WtpSchedulePhase::Executing;
}
WsprTransmitState wtp_runtime_state() noexcept {
  auto r = get();
  if (!r)
    return WsprTransmitState::DISABLED;
  auto s = r->app->status();
  if (s.recovery_required || s.safety_fault || !r->app->ready())
    return WsprTransmitState::FAILED;
  if (s.job && s.job->state == wsprrypi::wtp::State::Running)
    return WsprTransmitState::TRANSMITTING;
  if (r->app->active() || r->app->skipping() ||
      s.phase != wsprrypi::WtpSchedulePhase::Idle)
    return WsprTransmitState::ENABLED;
  if (s.last_report) {
    if (s.last_report->outcome == wsprrypi::WtpScheduleOutcome::Complete)
      return WsprTransmitState::COMPLETE;
    if (s.last_report->outcome == wsprrypi::WtpScheduleOutcome::Cancelled)
      return WsprTransmitState::CANCELLED;
    if (s.last_report->outcome == wsprrypi::WtpScheduleOutcome::Failed)
      return WsprTransmitState::FAILED;
  }
  return WsprTransmitState::DISABLED;
}
wsprrypi::TransmissionMode wtp_runtime_mode() noexcept {
  auto r = get();
  return r ? r->app->mode() : wsprrypi::TransmissionMode::WSPR;
}
void wtp_runtime_prepare(wsprrypi::TransmissionRequest r) {
  std::lock_guard operation(operation_mutex);
  require()->app->prepare(std::move(r));
}
std::chrono::nanoseconds wtp_runtime_preparation_lead() {
  return std::chrono::nanoseconds(require()->app->preparation_lead_ns());
}
void wtp_runtime_start() {
  std::lock_guard operation(operation_mutex);
  require()->app->start();
}
wsprrypi::CleanupResult wtp_runtime_stop() {
  std::lock_guard operation(operation_mutex);
  auto r = get();
  return r ? r->app->stop() : wsprrypi::CleanupResult{true, {}};
}
wsprrypi::StartupQuiesceResult wtp_runtime_inspect() {
  std::lock_guard operation(operation_mutex);
  return require()->app->inspect();
}
wsprrypi::CleanupResult wtp_runtime_recover() {
  std::lock_guard operation(operation_mutex);
  auto r = get();
  return r ? r->app->recover()
           : wsprrypi::CleanupResult{false, "Pico is not selected"};
}
std::string wtp_runtime_json() {
  auto r = get();
  if (!r)
    return R"({"selected":false})";
  auto j = nlohmann::json::parse(
      wsprrypi::wtp_runtime_status_json(r->app->status()));
  j["now_ms"] = std::to_string(r->clock.now_ms());
  j["selected"] = true;
  j["worker_active"] = r->app->active();
  j["host_skip_waiting"] = r->app->skipping();
  j["ready"] = r->app->ready();
  j["host_utc_valid"] = wsprrypi::wtp_host_utc_valid();
  return j.dump();
}
std::optional<wsprrypi::WtpScheduleReport> wtp_runtime_completion() {
  auto r = get();
  return r ? r->app->take_completion() : std::nullopt;
}
