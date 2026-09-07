// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Lee Bussy
#pragma once
#include "WSPR-Transmitter/src/transmission_controller.hpp"
#include "backend.hpp"
#include <functional>

namespace wsprrypi {
// Checked epoch arithmetic, including signed offsets. No floating-point time.
std::optional<std::uint64_t> wtp_slot_utc_ns(const ScheduledSlot &) noexcept;
class WtpScheduleClock : public WtpHostClock {
public:
  // nullopt means host UTC is unavailable/untrusted, independent of device UTC.
  virtual std::optional<std::uint64_t> utc_now_ns() const = 0;
};
class WtpSystemScheduleClock final : public WtpScheduleClock {
public:
  // Required provider attests current host UTC validity; no implicit trust.
  explicit WtpSystemScheduleClock(std::function<bool()> utc_valid);
  std::uint64_t now_ms() const override;
  void wait_ms(std::uint64_t) override;
  std::optional<std::uint64_t> utc_now_ns() const override;

private:
  std::function<bool()> utc_valid_;
  WtpSteadyClock steady_;
};
struct WtpSchedulePolicy {
  std::uint64_t preparation_ms{5000}, arm_submission_ms{1000};
  std::uint64_t clock_step_tolerance_ms{100}, maximum_wait_ms{86400000};
};
enum class WtpSchedulePhase {
  Idle,
  Waiting,
  Invalidated,
  Preparing,
  Executing,
  Blocked
};
enum class WtpScheduleOutcome {
  Complete,
  Cancelled,
  Invalidated,
  Failed,
  Blocked
};
struct WtpScheduleReport {
  WtpScheduleOutcome outcome{WtpScheduleOutcome::Failed};
  RequestId request_id{};
  std::string job_id, error;
  std::uint64_t start_utc_ns{}, dispatch_utc_ns{};
  bool arm_handed_off{}, reload_deferred{};
  ExecutionResult execution{};
};
// Dedicated early scheduler for explicitly constructed WTP development runtime.
// One owner thread calls connect/submit/run/recover/disconnect and reads
// reports. Only request_stop(), invalidate_pending() and phase() may be
// concurrent with run(). Clock and stream outlive this object. No threads,
// devices or fallback are created.
class WtpScheduler {
public:
  WtpScheduler(WtpScheduleClock &, wtp::SessionOptions);
  bool connect(wtp::ByteStream &);
  bool disconnect(); // local closure only, while Idle/Blocked
  bool submit(TransmissionRequest, std::string job_id,
              std::uint64_t uncertainty_ns, WtpSchedulePolicy = {});
  WtpScheduleReport run(); // one frozen job, including bounded cleanup
  void request_stop() noexcept;
  // Reload cancels waiting work and defers after commitment.
  void invalidate_pending() noexcept;
  // Explicit reconciliation after same-session reconnect.
  CleanupResult recover();
  WtpSchedulePhase phase() const noexcept { return phase_.load(); }
  const std::string &diagnostic() const noexcept { return error_; }
  const WtpTransmitBackend &backend() const noexcept { return backend_; }

private:
  struct Pending {
    TransmissionRequest request;
    WtpPlanOptions options;
    WtpSchedulePolicy policy;
    std::uint64_t dispatch{}, arm_notice{}, anchor_utc{}, anchor_mono{};
  };
  bool fail(std::string);
  std::optional<std::uint64_t> observe();
  bool admit_arm();
  WtpScheduleReport finish(WtpScheduleOutcome, std::string,
                           ExecutionResult = {});
  WtpScheduleClock &clock_;
  WtpTransmitBackend backend_;
  ExecutionPlanCompiler compiler_;
  TransmissionController controller_;
  std::optional<Pending> pending_;
  std::set<std::string> used_jobs_;
  std::atomic<WtpSchedulePhase> phase_{WtpSchedulePhase::Idle};
  std::atomic_bool stopped_{false}, invalidated_{false};
  bool handed_off_{};
  std::string error_;
};
} // namespace wsprrypi
