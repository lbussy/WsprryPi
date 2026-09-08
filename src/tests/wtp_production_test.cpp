// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Lee Bussy
#include "arg_parser.hpp"
#include "config_handler.hpp"
#include "runtime_config_bridge.hpp"
#include "transmitter_runtime_bridge.hpp"
#include "wtp_backend_peer.hpp"
#include "wtp_runtime_bridge.hpp"
#include "wtp_settings_json.hpp"
#include <fstream>
#include <thread>
#include <unistd.h>
using namespace std::chrono_literals;
using backend_test::checks;
namespace {
struct Clock : wsprrypi::WtpScheduleClock {
  backend_test::Peer peer;
  std::uint64_t now_ms() const override { return peer.now; }
  std::optional<std::uint64_t> utc_now_ns() const override {
    return peer.utc_base + peer.mono();
  }
  void wait_ms(std::uint64_t n) override { peer.advance(peer.now + n); }
};
void run() {
  char filename[] = "/tmp/wtp-production-config-XXXXXX";
  const int fd = mkstemp(filename);
  CHECK(fd >= 0);
  close(fd);
  struct Remove {
    const char *path;
    ~Remove() { unlink(path); }
  } remove{filename};
  init_config_json();
  json_to_config();
  set_patch_all_from_web_runtime_apply_suppressed_for_test(true);
  iniFile.set_filename(filename);
  config.use_ini = true;
  config.ini_filename = filename;
  config.transmit_backend = TransmitBackendKind::SIMULATED;
  config.transmit = false;
  config.use_led = config.use_amp = config.use_shutdown = false;
  config.callsign = "AA0NT";
  config.grid_square = "EM18";
  config.power_dbm = 20;
  config.frequencies = "20m";
  resolve_backend_specific_config(config);
  config_to_json();
  const auto old_gpio = jConfig.at("GPIO");
  WtpSettings settings{"/dev/ttyACM1", "000012345678", backend_test::device,
                       0xcafe,         0x4012,         1000};
  patch_all_from_web({{"Operation", {{"Transmit Backend", "wtp"}}},
                      {"WTP", wtp_settings_json(settings)}});
  CHECK(config.transmit_backend == TransmitBackendKind::WTP &&
        config.wtp == settings);
  CHECK(config.tx_pin == 0 && config.ppm == 0 && config.power_level == 0);
  CHECK(jConfig.at("GPIO") == old_gpio);
  CHECK(get_public_config_json().at("WTP") == wtp_settings_json(settings));
  RuntimeConfigCandidate candidate;
  prepare_runtime_config_candidate(filename, candidate);
  if (!candidate.valid)
    throw std::runtime_error(candidate.error_reason);
  CHECK(candidate.normalized_config.wtp == settings);
  for (auto patch : {nlohmann::json{{"WTP", {{"USB Serial", ""}}}},
                     nlohmann::json{{"WTP", {{"USB Vendor ID", 1.5}}}},
                     nlohmann::json{{"WTP", {{"Unknown", 1}}}},
                     nlohmann::json{{"Operation", {{"Use Amp", true}}}}}) {
    const auto before = jConfig;
    bool rejected = false;
    try {
      patch_all_from_web(patch);
    } catch (...) {
      rejected = true;
    }
    CHECK(rejected && jConfig == before && config.wtp == settings);
  }
  ArgParserConfig copy;
  copy_runtime_config(config, copy);
  CHECK(copy.wtp == settings);
  Clock clock;
  set_wtp_runtime_for_test(
      settings, clock, clock.peer,
      {backend_test::sid, backend_test::owner_id, backend_test::device}, [&] {
        clock.peer.open();
        return true;
      });
  CHECK(transmitter_quiesce_for_startup().ok);
  CHECK(wtp_runtime_ready() &&
        wtp_runtime_selection_error(std::nullopt).empty());
  wsprrypi::TransmissionRequest request;
  request.mode = wsprrypi::TransmissionMode::TONE;
  request.output.backend = wsprrypi::BackendKind::WTP;
  request.payload = wsprrypi::TonePayload{14097100, 200ms, {}};
  request.policy.allow_unqualified_frequency = true;
  transmitter_configure_execution(request, {});
  CHECK(!wtp_runtime_invalidate_for_reload());
  CHECK(clock.peer.executions == 0 &&
        std::count(clock.peer.operations.begin(), clock.peer.operations.end(),
                   wsprrypi::wtp::Operation::Arm) == 0);
  CHECK(wtp_runtime_selection_error(std::nullopt).empty());
  bool complete = false;
  const auto owner_thread = std::this_thread::get_id();
  transmitter_set_callbacks([&](auto event, auto, const auto &, auto) {
    CHECK(std::this_thread::get_id() == owner_thread);
    CHECK(event == WsprTransmissionCallbackEvent::COMPLETE);
    complete = true;
  });
  transmitter_configure_execution(request, {});
  CHECK(!wtp_runtime_selection_error(std::nullopt).empty());
  bool refused = false;
  try {
    select_wtp_runtime(std::nullopt);
  } catch (...) {
    refused = true;
  }
  CHECK(refused && wtp_runtime_selected());
  transmitter_start_async();
  const auto deadline = std::chrono::steady_clock::now() + 10s;
  while (!complete && std::chrono::steady_clock::now() < deadline) {
    transmitter_poll_events();
    std::this_thread::sleep_for(1ms);
  }
  CHECK(complete && clock.peer.executions == 1);
  auto snapshot = nlohmann::json::parse(wtp_runtime_json());
  CHECK(snapshot["last_report"]["outcome"] == "complete");
  CHECK(snapshot["remote"]["output_active"] == false);
  // Unknown ARM crosses the actual parent bridge and cannot be generically cleared.
  clock.peer.lose_reply = wsprrypi::wtp::Operation::Arm;
  bool blocked = false;
  transmitter_set_callbacks([&](auto event, auto, const auto &, auto) {
    CHECK(std::this_thread::get_id() == owner_thread);
    CHECK(event == WsprTransmissionCallbackEvent::FAILED);
    blocked = true;
  });
  transmitter_configure_execution(request, {});
  transmitter_start_async();
  const auto blocked_deadline = std::chrono::steady_clock::now() + 10s;
  while (!blocked && std::chrono::steady_clock::now() < blocked_deadline) {
    transmitter_poll_events();
    std::this_thread::sleep_for(1ms);
  }
  CHECK(blocked && !wtp_runtime_ready());
  transmitter_clear_soft_off();
  transmitter_clear_execution_state_after_stop();
  transmitter_stop_and_join();
  CHECK(!wtp_runtime_selection_error(std::nullopt).empty());
  CHECK(wtp_runtime_recover().ok && wtp_runtime_ready());
  snapshot = nlohmann::json::parse(wtp_runtime_json());
  CHECK(snapshot["last_report"]["outcome"] == "blocked");
  CHECK(snapshot["remote"]["output_active"] == false);
  CHECK(std::count(clock.peer.operations.begin(), clock.peer.operations.end(),
                   wsprrypi::wtp::Operation::Arm) == 2);
  transmitter_stop_and_join();
  select_wtp_runtime(std::nullopt);
  CHECK(!wtp_runtime_selected());
  std::cout << backend_test::checks
            << " production WTP configuration/runtime checks passed\n";
}
} // namespace
int main() {
  try {
    run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
