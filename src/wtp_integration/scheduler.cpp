// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Lee Bussy
#include "scheduler.hpp"
#include "WSPR-Transmitter/src/gpio_band_policy.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace wsprrypi {
namespace {
constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
std::optional<std::uint64_t>
epoch_ns(std::chrono::system_clock::time_point time) noexcept {
  using Scale = std::ratio_divide<std::chrono::system_clock::period, std::nano>;
  static_assert(Scale::den == 1,
                "System clock needs integral nanosecond ticks");
  const auto ticks = time.time_since_epoch().count();
  if (ticks < 0 || static_cast<std::uint64_t>(ticks) > maximum / Scale::num)
    return {};
  return static_cast<std::uint64_t>(ticks) * Scale::num;
}
} // namespace
std::optional<std::uint64_t>
wtp_slot_utc_ns(const ScheduledSlot &slot) noexcept {
  auto base = epoch_ns(slot.start_time);
  if (!base)
    return {};
  auto offset = slot.start_offset.count();
  std::uint64_t value;
  if (offset < 0) {
    const auto magnitude = static_cast<std::uint64_t>(-(offset + 1)) + 1;
    if (magnitude >= *base)
      return {};
    value = *base - magnitude;
  } else {
    if (static_cast<std::uint64_t>(offset) > maximum - *base)
      return {};
    value = *base + static_cast<std::uint64_t>(offset);
  }
  return value ? std::optional{value} : std::nullopt;
}
WtpSystemScheduleClock::WtpSystemScheduleClock(std::function<bool()> utc_valid)
    : utc_valid_(std::move(utc_valid)) {
  if (!utc_valid_)
    throw std::invalid_argument("WTP requires a host UTC validity provider");
}
std::uint64_t WtpSystemScheduleClock::now_ms() const {
  return steady_.now_ms();
}
void WtpSystemScheduleClock::wait_ms(std::uint64_t ms) { steady_.wait_ms(ms); }
std::optional<std::uint64_t> WtpSystemScheduleClock::utc_now_ns() const {
  return utc_valid_() ? epoch_ns(std::chrono::system_clock::now())
                      : std::nullopt;
}
WtpScheduler::WtpScheduler(WtpScheduleClock &clock, wtp::SessionOptions options)
    : clock_(clock),
      backend_(clock, std::move(options), [this] { return admit_arm(); }),
      controller_(compiler_, backend_) {}
bool WtpScheduler::fail(std::string error) {
  error_ = std::move(error);
  return false;
}
bool WtpScheduler::connect(wtp::ByteStream &stream) {
  if (phase() != WtpSchedulePhase::Idle && phase() != WtpSchedulePhase::Blocked)
    return fail("Cannot replace a stream while a WTP request is pending");
  if (!backend_.connect(stream))
    return fail(backend_.diagnostic());
  return true; // Blocked remains blocked until explicit recover().
}
bool WtpScheduler::disconnect() {
  if (phase() != WtpSchedulePhase::Idle && phase() != WtpSchedulePhase::Blocked)
    return fail("Cannot disconnect a pending WTP request; stop and finish "
                "cleanup first");
  backend_.disconnect();
  return true;
}
bool WtpScheduler::submit(TransmissionRequest request, std::string job_id,
                          std::uint64_t uncertainty, WtpSchedulePolicy policy) {
  if (phase() != WtpSchedulePhase::Idle || pending_)
    return fail("WTP scheduler has pending or unresolved work");
  error_.clear();
  const auto &session = backend_.session();
  if (session.phase() != wtp::SessionPhase::Ready || !session.capabilities() ||
      session.safety_fault() || session.uncertain() || session.owns())
    return fail("WTP submission requires a healthy negotiated idle session");
  if (!request.id.value || request.output.backend != BackendKind::WTP ||
      request.output.gpio ||
      request.output.output != ClockSource::UNSPECIFIED ||
      request.policy.hardware_profile != HardwareProfile::UNSPECIFIED)
    return fail("WTP request must have an explicit identity and no host output "
                "controls");
  // The shared compiler selects one frame from a multi-frame payload. Require
  // an explicitly extracted slot frame here rather than silently losing work.
  if (request.mode == TransmissionMode::WSPR) {
    const auto *wspr = std::get_if<WsprPayload>(&request.payload);
    if (!wspr || wspr->prepared.frames.size() != 1)
      return fail(
          "WTP scheduling requires exactly one explicit WSPR frame per slot");
  }
  if (used_jobs_.contains(job_id) || used_jobs_.size() >= 4096)
    return fail(
        "WTP job identity reused or scheduler identity capacity exhausted");
  if (!policy.preparation_ms || !policy.arm_submission_ms ||
      !policy.maximum_wait_ms || policy.maximum_wait_ms > 86400000 ||
      policy.clock_step_tolerance_ms > policy.arm_submission_ms ||
      policy.preparation_ms > maximum / 1000000 ||
      policy.arm_submission_ms > maximum / 1000000 - policy.preparation_ms)
    return fail("Invalid WTP scheduling allowances");
  const auto &caps = *session.capabilities();
  const auto preparation =
      (policy.preparation_ms + policy.arm_submission_ms) * 1000000;
  if (preparation > maximum - caps.minimum_arm_lead_ns)
    return fail("WTP preparation lead overflow");
  const auto lead = preparation + caps.minimum_arm_lead_ns;
  if (lead > caps.maximum_arm_ahead_ns)
    return fail("WTP preparation allowances do not fit device ARM horizon");
  auto start = wtp_slot_utc_ns(request.slot);
  const auto now = clock_.utc_now_ns();
  const auto mono = clock_.now_ms();
  if (!start || !now || !*now || *start < *now || *start - *now < lead ||
      *start - *now > policy.maximum_wait_ms * 1000000)
    return fail("WTP slot is invalid, too late, too distant, or host UTC is "
                "unavailable");
  WtpPlanOptions options{std::move(job_id), *start, uncertainty};
  // Pure preflight before waiting or any ownership mutation. Controller repeats
  // compile/admission from this same frozen request at early dispatch.
  try {
    auto plan = compiler_.compile(request);
    plan.id.value = 1;
    auto conversion = prepare_wtp_plan(plan, caps, options);
    if (!conversion)
      return fail(conversion.explanation);
    const auto policy_result = evaluate_gpio_band_policy(plan);
    if (!policy_result.allowed)
      return fail(policy_result.error);
  } catch (const std::exception &e) {
    return fail(e.what());
  }
  pending_ =
      Pending{std::move(request),
              std::move(options),
              policy,
              *start - lead,
              caps.minimum_arm_lead_ns + policy.arm_submission_ms * 1000000,
              *now,
              mono};
  used_jobs_.insert(pending_->options.job_id);
  stopped_ = false;
  invalidated_ = false;
  handed_off_ = false;
  phase_ = WtpSchedulePhase::Waiting;
  return true;
}
std::optional<std::uint64_t> WtpScheduler::observe() {
  const auto utc = clock_.utc_now_ns();
  const auto mono = clock_.now_ms();
  const auto &p = *pending_;
  if (!utc || !*utc || mono < p.anchor_mono ||
      mono - p.anchor_mono > maximum / 1000000) {
    fail("WTP host clock unavailable or monotonic time regressed");
    return {};
  }
  const auto elapsed = (mono - p.anchor_mono) * 1000000;
  if (elapsed > maximum - p.anchor_utc) {
    fail("WTP host time overflow");
    return {};
  }
  const auto expected = p.anchor_utc + elapsed;
  const auto difference = *utc > expected ? *utc - expected : expected - *utc;
  if (difference > p.policy.clock_step_tolerance_ms * 1000000) {
    fail("WTP host UTC changed relative to the committed schedule");
    return {};
  }
  return utc;
}
bool WtpScheduler::admit_arm() {
  if (!pending_ || stopped_)
    return fail("WTP stop requested before ARM handoff");
  const auto now = observe();
  if (!now)
    return false;
  if (*now > pending_->options.start_utc_ns ||
      pending_->options.start_utc_ns - *now < pending_->arm_notice)
    return fail("WTP preparation consumed the ARM submission allowance; slot "
                "will not run late");
  handed_off_ = true;
  return true; // Host UTC cannot move the job after this submission boundary.
}
WtpScheduleReport WtpScheduler::finish(WtpScheduleOutcome outcome,
                                       std::string error,
                                       ExecutionResult execution) {
  WtpScheduleReport report;
  report.outcome = outcome;
  report.error = std::move(error);
  report.execution = std::move(execution);
  if (pending_) {
    report.request_id = pending_->request.id;
    report.job_id = pending_->options.job_id;
    report.start_utc_ns = pending_->options.start_utc_ns;
    report.dispatch_utc_ns = pending_->dispatch;
  }
  report.arm_handed_off = handed_off_;
  report.reload_deferred =
      invalidated_ && (phase() == WtpSchedulePhase::Preparing ||
                       phase() == WtpSchedulePhase::Executing);
  pending_.reset();
  controller_.reset();
  phase_ = outcome == WtpScheduleOutcome::Blocked ? WtpSchedulePhase::Blocked
                                                  : WtpSchedulePhase::Idle;
  error_ = report.error;
  return report;
}
WtpScheduleReport WtpScheduler::run() {
  if ((phase() != WtpSchedulePhase::Waiting &&
       phase() != WtpSchedulePhase::Invalidated) ||
      !pending_)
    return {phase() == WtpSchedulePhase::Blocked ? WtpScheduleOutcome::Blocked
                                                 : WtpScheduleOutcome::Failed,
            {},
            {},
            "No runnable WTP request"};
  error_.clear();
  try {
    for (;;) {
      if (stopped_)
        return finish(WtpScheduleOutcome::Cancelled,
                      "Pending WTP request stopped before preparation");
      if (invalidated_)
        return finish(WtpScheduleOutcome::Invalidated,
                      "Pending WTP request invalidated by reload");
      auto now = observe();
      if (!now)
        return finish(WtpScheduleOutcome::Failed, error_);
      if (*now >= pending_->dispatch) {
        if (*now > pending_->options.start_utc_ns ||
            pending_->options.start_utc_ns - *now <= pending_->arm_notice)
          return finish(WtpScheduleOutcome::Failed,
                        "WTP dispatch missed the preparation window");
        break;
      }
      auto before = clock_.now_ms();
      clock_.wait_ms(std::min<std::uint64_t>(
          10, (pending_->dispatch - *now - 1) / 1000000 + 1));
      if (clock_.now_ms() <= before)
        return finish(WtpScheduleOutcome::Failed,
                      "WTP waiting clock stalled or regressed");
    }
    // Linearization boundary for reload: later invalidation is deferred, while
    // stop still cancels. No mutable global configuration is read from here on.
    auto expected_phase = WtpSchedulePhase::Waiting;
    if (!phase_.compare_exchange_strong(expected_phase,
                                        WtpSchedulePhase::Preparing))
      return finish(WtpScheduleOutcome::Invalidated,
                    "Pending WTP request invalidated before commit");
    if (!backend_.schedule(pending_->options))
      return finish(WtpScheduleOutcome::Failed, backend_.diagnostic());
    if (stopped_) {
      backend_.stop();
      return finish(WtpScheduleOutcome::Cancelled,
                    "Pending WTP request stopped before LOAD");
    }
    auto prepared = controller_.prepare(pending_->request);
    if (!prepared.ok) {
      // Controller attempts cleanup; repeat its bounded authoritative check to
      // retain an explicit result instead of parsing its combined error text.
      auto cleanup = backend_.cleanup();
      const bool cancelled = stopped_ && cleanup.ok;
      ExecutionResult result{false,          cancelled, !cancelled,
                             prepared.error, true,      cleanup};
      return finish(!cleanup.ok ? WtpScheduleOutcome::Blocked
                    : cancelled ? WtpScheduleOutcome::Cancelled
                                : WtpScheduleOutcome::Failed,
                    prepared.error, result);
    }
    phase_ = WtpSchedulePhase::Executing;
    auto result = controller_.execute_prepared();
    auto outcome = !result.cleanup.ok ? WtpScheduleOutcome::Blocked
                   : result.ok        ? WtpScheduleOutcome::Complete
                   : result.stopped   ? WtpScheduleOutcome::Cancelled
                                      : WtpScheduleOutcome::Failed;
    return finish(outcome, error_.empty() ? result.error : error_, result);
  } catch (const std::exception &e) {
    if (phase() == WtpSchedulePhase::Waiting ||
        phase() == WtpSchedulePhase::Invalidated)
      return finish(WtpScheduleOutcome::Failed, e.what());
    auto cleanup = backend_.cleanup();
    return finish(cleanup.ok ? WtpScheduleOutcome::Failed
                             : WtpScheduleOutcome::Blocked,
                  e.what(), {false, false, true, e.what(), true, cleanup});
  } catch (...) {
    if (phase() == WtpSchedulePhase::Waiting ||
        phase() == WtpSchedulePhase::Invalidated)
      return finish(WtpScheduleOutcome::Failed,
                    "Unexpected WTP waiting exception");
    auto cleanup = backend_.cleanup();
    return finish(WtpScheduleOutcome::Blocked,
                  "Unexpected WTP scheduling exception",
                  {false, false, true, "Unexpected exception", true, cleanup});
  }
}
void WtpScheduler::request_stop() noexcept {
  stopped_ = true;
  backend_.stop();
}
void WtpScheduler::invalidate_pending() noexcept {
  invalidated_ = true;
  auto expected = WtpSchedulePhase::Waiting;
  phase_.compare_exchange_strong(expected, WtpSchedulePhase::Invalidated);
}
CleanupResult WtpScheduler::recover() {
  if (phase() != WtpSchedulePhase::Blocked)
    return {false, "WTP scheduler is not blocked"};
  auto cleanup = backend_.cleanup();
  if (cleanup.ok) {
    controller_.reset();
    phase_ = WtpSchedulePhase::Idle;
    error_.clear();
  }
  return cleanup;
}
} // namespace wsprrypi
