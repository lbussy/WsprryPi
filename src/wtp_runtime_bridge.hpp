// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Lee Bussy
#pragma once
#include "transmission_request.hpp"
#include "wspr_transmit_types.hpp"
#include "wtp_integration/application.hpp"
#include <optional>
std::string wtp_runtime_selection_error(const std::optional<WtpSettings> &);
void wtp_runtime_prepare_skip();
void select_wtp_runtime(const std::optional<WtpSettings> &);
bool wtp_runtime_selected() noexcept;
bool wtp_runtime_ready() noexcept;
bool wtp_runtime_invalidate_for_reload();
bool wtp_runtime_defers_reload() noexcept;
WsprTransmitState wtp_runtime_state() noexcept;
wsprrypi::TransmissionMode wtp_runtime_mode() noexcept;
void wtp_runtime_prepare(wsprrypi::TransmissionRequest);
std::chrono::nanoseconds wtp_runtime_preparation_lead();
void wtp_runtime_start();
wsprrypi::CleanupResult wtp_runtime_stop();
wsprrypi::StartupQuiesceResult wtp_runtime_inspect();
wsprrypi::CleanupResult wtp_runtime_recover();
std::string wtp_runtime_json();
std::optional<wsprrypi::WtpScheduleReport> wtp_runtime_completion();

// Typed injection only: no configuration, environment or HTTP selector exposes
// it. The caller keeps clock and stream alive until the idle runtime is
// deselected.
void set_wtp_runtime_for_test(WtpSettings, wsprrypi::WtpScheduleClock &,
                              wsprrypi::wtp::ByteStream &,
                              wsprrypi::wtp::SessionOptions,
                              std::function<bool()>);
