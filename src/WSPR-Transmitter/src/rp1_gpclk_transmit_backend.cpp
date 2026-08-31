#include "rp1_gpclk_transmit_backend.hpp"
#include "chipset_offsets.hpp"

#include "wspr_transmit.hpp"
#include "rp1_gpclk_uapi.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <mutex>
#include <openssl/sha.h>
#include <unistd.h>

namespace
{
constexpr double kWsprSymbolSeconds = 8192.0 / 12000.0;
constexpr double kWsprToneSpacingHz = 1.0 / kWsprSymbolSeconds;
std::mutex operation_record_mutex;
wsprrypi::Rp1GpclkOperationRecord operation_record;

void update_record(const wsprrypi::Rp1GpclkOperationRecord& value)
{
    std::lock_guard<std::mutex> lock(operation_record_mutex);
    operation_record = value;
}

std::array<std::uint8_t, 32> operation_authorization_digest(
    const wsprrypi::Rp1GpclkDevelopmentPolicyInputs& policy)
{
    const std::string material = policy.operation_id + "\n" +
        std::to_string(policy.requested_route) + "\n" +
        std::to_string(policy.route_transaction_generation);
    std::array<std::uint8_t, 32> digest{};
    SHA256(
        reinterpret_cast<const unsigned char*>(material.data()),
        material.size(), digest.data());
    return digest;
}
}

namespace wsprrypi
{
Rp1GpclkOperationRecord rp1GpclkOperationRecordSnapshot()
{
    std::lock_guard<std::mutex> lock(operation_record_mutex);
    return operation_record;
}
}

WsprRp1GpclkBackend::WsprRp1GpclkBackend(IControllerBridge& owner)
    : owner_(owner),
      io_(std::make_unique<wsprrypi::Rp1GpclkPosixIo>()),
      provider_(std::make_unique<wsprrypi::Rp1GpclkLinuxProvider>(*io_)),
      backend_(std::make_unique<wsprrypi::Rp1GpclkBackend>(*provider_))
{
}

WsprRp1GpclkBackend::WsprRp1GpclkBackend(
    IControllerBridge& owner,
    std::unique_ptr<wsprrypi::Rp1GpclkProvider> provider)
    : owner_(owner),
      provider_(std::move(provider)),
      backend_(std::make_unique<wsprrypi::Rp1GpclkBackend>(*provider_))
{
}

WsprRp1GpclkBackend::~WsprRp1GpclkBackend()
{
    cleanup();
}

wsprrypi::BackendInfo WsprRp1GpclkBackend::info() const
{
    return {wsprrypi::BackendKind::RP1_GPCLK, "rp1-gpclk",
        "RP1 kernel-owned GPCLK0 WSPR backend"};
}

wsprrypi::BackendCapabilities WsprRp1GpclkBackend::capabilities() const
{
    wsprrypi::BackendCapabilities caps;
    caps.output_class = wsprrypi::BackendOutputClass::PHYSICAL_GPIO_RF;
    caps.supported_modes =
        wsprrypi::transmission_mode_bit(wsprrypi::TransmissionMode::WSPR) |
        wsprrypi::transmission_mode_bit(wsprrypi::TransmissionMode::TONE) |
        wsprrypi::transmission_mode_bit(wsprrypi::TransmissionMode::QRSS) |
        wsprrypi::transmission_mode_bit(wsprrypi::TransmissionMode::FSKCW) |
        wsprrypi::transmission_mode_bit(wsprrypi::TransmissionMode::DFCW);
    caps.supports_precomputed_execution = true;
    caps.supports_frequency_switching = true;
    caps.supports_rf_gating = true;
    caps.min_event_duration = std::chrono::nanoseconds{682666667};
    caps.max_frequency_hz = 40000000.0;
    return caps;
}

wsprrypi::BackendCompileResult WsprRp1GpclkBackend::configure(
    const wsprrypi::ExecutionPlan& plan,
    const wsprrypi::BackendExecutionInputs& inputs)
{
    wsprrypi::BackendCompileResult result;
    configured_.reset();
    if (plan.backend != wsprrypi::BackendKind::RP1_GPCLK)
    {
        result.error = "Execution plan is not targeted for RP1 GPCLK.";
        return result;
    }
    if (!wsprrypi::Rp1GpclkBackend::validDrive(inputs.power_level))
    {
        result.error = "RP1 GPIO drive must be 2, 4, 8, or 12 mA.";
        return result;
    }
    const std::uint32_t route = inputs.tx_gpio == 4 ? RP1_GPCLK_ROUTE_GPIO4 :
        inputs.tx_gpio == 20 ? RP1_GPCLK_ROUTE_GPIO20 : RP1_GPCLK_ROUTE_INVALID;
    if (route == RP1_GPCLK_ROUTE_INVALID)
    {
        result.error = "RP1 GPCLK requires the independently selected GPIO4 or GPIO20 route.";
        return result;
    }
    const auto map_route = [](int gpio) {
        return gpio == 4 ? RP1_GPCLK_ROUTE_GPIO4 :
            gpio == 20 ? RP1_GPCLK_ROUTE_GPIO20 : RP1_GPCLK_ROUTE_INVALID;
    };
    auto make_policy = [&]() {
        wsprrypi::Rp1GpclkDevelopmentPolicyInputs policy;
        const auto& source = inputs.rp1_development;
        policy.development_testing_enabled = source.enabled;
        policy.rp1_backend_selected = true;
        policy.requested_route = route;
        policy.persisted_route = map_route(source.persisted_gpio);
        policy.configured_route = map_route(inputs.configured_tx_gpio);
        policy.active_route = map_route(source.active_gpio);
        policy.module_route = map_route(source.module_gpio);
        policy.active_route_count = source.active_route_count;
        policy.route_transaction_resolved = source.route_transaction_resolved;
        policy.scheduler_idle = source.scheduler_idle;
        policy.application_owns_operation = source.application_owns_operation;
        policy.endpoint_available = source.endpoint_available;
        policy.endpoint_closed = source.endpoint_closed;
        policy.endpoint_exclusively_acquirable = source.endpoint_exclusively_acquirable;
        policy.cleanup_fault = source.cleanup_fault;
        policy.live_output_verified = source.live_output_verified;
        policy.physical_connection_confirmed = source.physical_connection_confirmed;
        policy.attenuation_and_load_confirmed = source.attenuation_and_load_confirmed;
        policy.bounded_operation_confirmed = source.bounded_operation_confirmed;
        policy.non_radiating_topology_confirmed = source.non_radiating_topology_confirmed;
        policy.experimental_status_acknowledged = source.experimental_status_acknowledged;
        policy.confirmation_current = source.confirmation_current;
        policy.route_transaction_generation = source.route_transaction_generation;
        policy.confirmation_route_transaction_generation = source.confirmation_route_transaction_generation;
        policy.operation_id = source.operation_id;
        policy.confirmation_operation_id = source.confirmation_operation_id;
        policy.confirmation_route = map_route(source.confirmation_gpio);
        return policy;
    };
    if (plan.mode == wsprrypi::TransmissionMode::TONE)
    {
        if (plan.events.empty() || !plan.events.front().rf_on ||
            plan.events.front().frequency_hz != plan.reference_frequency_hz)
        { result.error = "RP1 GPCLK TONE requires one valid RF-on frequency."; return result; }
        wsprrypi::Rp1GpclkPlannerInput planner_input;
        planner_input.center_frequency_hz =
            plan.reference_frequency_hz + 1.5 * kWsprToneSpacingHz;
        planner_input.tone_spacing_hz = kWsprToneSpacingHz;
        planner_input.parent_frequency_hz =
            wsprrypi::kRp1GpclkNominalParentFrequencyHz;
        planner_input.source_rate_ppm = plan.calibration.ppm;
        planner_input.intrinsic_source_rate_ppm =
            wsprrypi::chipsetIntrinsicOffsetPpm(wsprrypi::ClockChipset::Rp1);
        planner_input.maximum_output_hz = wsprrypi::kRp1GpclkMaximumDirectOutputHz;
        planner_input.dither_sequence_length = wsprrypi::Rp1GpclkBackend::kWritesPerSymbol;
        const auto planned = wsprrypi::planRp1GpclkWspr(planner_input);
        if (!planned.ok)
        { result.error = planned.error; return result; }
        ConfiguredFrame frame;
        frame.plan_id = plan.id;
        frame.tone = true;
        frame.continuous_tone = !plan.duration_was_explicit;
        frame.drive_ma = static_cast<std::uint32_t>(inputs.power_level);
        frame.route = route;
        frame.required_capabilities =
            (frame.continuous_tone ? RP1_GPCLK_CAP_TONE_CONTINUOUS
                                   : RP1_GPCLK_CAP_TONE_FINITE) |
            RP1_GPCLK_CAP_STOP_DRAIN | RP1_GPCLK_CAP_STABLE_STATE |
            RP1_GPCLK_CAP_ROUTE_IDENTITY | RP1_GPCLK_CAP_COMPAT_IDENTITY |
            RP1_GPCLK_CAP_CLEANUP_FAULT_LATCH | RP1_GPCLK_CAP_LIVE_ELIGIBLE |
            RP1_GPCLK_CAP_OPERATION_LIVE_GATE;
        frame.development_policy = make_policy();
        if (frame.development_policy.development_testing_enabled)
            wsprrypi::armRp1GpclkDevelopmentOperation(frame.development_policy);
        const auto& tone = planned.plan.tones[0];
        frame.tone_program.tone = {tone.lower_divider_word, tone.upper_divider_word,
            tone.lower_word_count, tone.upper_word_count};
        frame.tone_program.duration_ns = frame.continuous_tone ? 0 :
            static_cast<std::uint64_t>(plan.events.front().duration.count());
        frame.tone_program.operation = frame.continuous_tone ?
            RP1_GPCLK_TONE_OPERATION_CONTINUOUS : RP1_GPCLK_TONE_OPERATION_FINITE;
        frame.tone_program.expected_route = route;
        frame.tone_program.fractional_bits = RP1_GPCLK_FRACTIONAL_BITS;
        frame.tone_program.tick_divider = RP1_GPCLK_TICK_DIVIDER;
        configured_ = std::move(frame);
        stop_requested_.store(false, std::memory_order_release);
        result.ok = true;
        return result;
    }
    if (plan.mode != wsprrypi::TransmissionMode::WSPR)
    {
        const auto compiled = wsprrypi::compileRp1GpclkEventProgram(plan);
        if (!compiled.ok)
        {
            result.error = compiled.error;
            return result;
        }
        ConfiguredFrame frame;
        frame.plan_id = plan.id;
        frame.event_program = compiled.program;
        frame.finite_events = true;
        frame.drive_ma = static_cast<std::uint32_t>(inputs.power_level);
        frame.route = route;
        frame.required_capabilities = RP1_GPCLK_CAP_SUBMIT_EVENTS |
            RP1_GPCLK_CAP_STOP_DRAIN | RP1_GPCLK_CAP_STABLE_STATE |
            RP1_GPCLK_CAP_ROUTE_IDENTITY | RP1_GPCLK_CAP_COMPAT_IDENTITY |
            RP1_GPCLK_CAP_CLEANUP_FAULT_LATCH | RP1_GPCLK_CAP_LIVE_ELIGIBLE |
            RP1_GPCLK_CAP_OPERATION_LIVE_GATE;
        frame.development_policy = make_policy();
        if (frame.development_policy.development_testing_enabled)
            wsprrypi::armRp1GpclkDevelopmentOperation(frame.development_policy);
        configured_ = std::move(frame);
        stop_requested_.store(false, std::memory_order_release);
        result.ok = true;
        return result;
    }
    if (plan.events.size() != 162)
    {
        result.error = "RP1 GPCLK requires exactly one 162-symbol WSPR frame.";
        return result;
    }

    wsprrypi::Rp1GpclkPlannerInput planner_input;
    planner_input.center_frequency_hz = plan.reference_frequency_hz;
    planner_input.tone_spacing_hz = kWsprToneSpacingHz;
    planner_input.parent_frequency_hz =
        wsprrypi::kRp1GpclkNominalParentFrequencyHz;
    planner_input.source_rate_ppm = plan.calibration.ppm;
    planner_input.intrinsic_source_rate_ppm =
        wsprrypi::chipsetIntrinsicOffsetPpm(wsprrypi::ClockChipset::Rp1);
    planner_input.maximum_output_hz = wsprrypi::kRp1GpclkMaximumDirectOutputHz;
    planner_input.dither_sequence_length =
        wsprrypi::Rp1GpclkBackend::kWritesPerSymbol;
    const auto planned = wsprrypi::planRp1GpclkWspr(planner_input);
    if (!planned.ok)
    {
        result.error = planned.error;
        return result;
    }

    ConfiguredFrame frame;
    frame.plan_id = plan.id;
    frame.clock_plan = planned.plan;
    frame.drive_ma = static_cast<std::uint32_t>(inputs.power_level);
    frame.route = route;
    frame.required_capabilities = RP1_GPCLK_CAP_SUBMIT_WSPR |
        RP1_GPCLK_CAP_STOP_DRAIN | RP1_GPCLK_CAP_STABLE_STATE |
        RP1_GPCLK_CAP_ROUTE_IDENTITY | RP1_GPCLK_CAP_COMPAT_IDENTITY |
        RP1_GPCLK_CAP_CLEANUP_FAULT_LATCH | RP1_GPCLK_CAP_LIVE_ELIGIBLE |
            RP1_GPCLK_CAP_OPERATION_LIVE_GATE;
    frame.development_policy = make_policy();
    if (frame.development_policy.development_testing_enabled)
        wsprrypi::armRp1GpclkDevelopmentOperation(frame.development_policy);
    for (std::size_t i = 0; i < plan.events.size(); ++i)
    {
        const auto& event = plan.events[i];
        if (!event.rf_on ||
            std::llabs(event.duration.count() - 682666667LL) > 1)
        {
            result.error = "RP1 GPCLK requires contiguous standard-duration WSPR symbols.";
            return result;
        }
        const long tone = std::lround(
            (event.frequency_hz - planned.plan.tones[0].requested_frequency_hz) /
            kWsprToneSpacingHz);
        if (tone < 0 || tone > 3 ||
            std::fabs(event.frequency_hz -
                planned.plan.tones[static_cast<std::size_t>(tone)].requested_frequency_hz) >
                0.001)
        {
            result.error = "RP1 GPCLK execution plan contains a non-WSPR tone.";
            return result;
        }
        frame.symbols[i] = static_cast<std::uint8_t>(tone);
        if (i > 0 && event.offset_from_start !=
                plan.events[i - 1].offset_from_start + plan.events[i - 1].duration)
        {
            result.error = "RP1 GPCLK WSPR symbols must be contiguous.";
            return result;
        }
    }
    configured_ = frame;
    stop_requested_.store(false, std::memory_order_release);
    result.ok = true;
    return result;
}

wsprrypi::ExecutionResult WsprRp1GpclkBackend::execute(
    const wsprrypi::ExecutionPlan& plan)
{
    wsprrypi::ExecutionResult result;
    if (!configured_ || configured_->plan_id.value != plan.id.value)
    {
        result.error = "RP1 GPCLK execution plan is not configured.";
        return result;
    }

    std::string error;
    {
        std::lock_guard<std::mutex> lock(backend_mutex_);
        wsprrypi::Rp1GpclkProviderIdentity identity;
        if (!provider_->query(configured_->route, configured_->required_capabilities,
                false, identity, error))
        {
            wsprrypi::invalidateRp1GpclkDevelopmentOperation();
            result.error = error;
            return result;
        }
        auto consumed = wsprrypi::consumeRp1GpclkDevelopmentOperation(
            configured_->development_policy.operation_id,
            configured_->route, identity);
        if (!consumed)
        {
            result.error = "stale-operator-confirmation: Development authorization was not armed for this exact operation and route.";
            return result;
        }
        auto current_policy = std::move(*consumed);
        current_policy.identity = std::move(identity);
        current_policy.module_route = current_policy.identity.route;
        const auto decision = wsprrypi::decideRp1GpclkDevelopmentUse(current_policy);
        if (!decision.allowed)
        {
            result.error = std::string(decision.code) + ": " + decision.explanation;
            return result;
        }
        wsprrypi::Rp1GpclkOperationRecord record;
        record.operation_id = current_policy.operation_id;
        record.module_id = current_policy.identity.module_id;
        record.module_version = current_policy.identity.build_id;
        record.compatibility_id = current_policy.identity.compatibility_id;
        record.uapi_abi = RP1_GPCLK_UAPI_ABI_V4;
        record.route = configured_->route;
        record.endpoint = provider_->endpoint();
        record.state = "authorized";
        record.execution_authorized = true;
        record.process_id = static_cast<std::uint64_t>(::getpid());
        record.started_monotonic_ns = static_cast<std::uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count());
        record.endpoint_closed = true;
        update_record(record);
        const auto record_failure = [&](const std::string& state,
                                        bool cleanup_attempted,
                                        bool cleanup_complete) {
            record.state = state;
            record.cleanup_attempted = cleanup_attempted;
            record.cleanup_complete = cleanup_complete;
            record.endpoint_closed = cleanup_complete;
            record.lease = cleanup_complete ? 0 : provider_->leaseId();
            record.finished_monotonic_ns = static_cast<std::uint64_t>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count());
            update_record(record);
        };
        if (!backend_->prepare(
                configured_->drive_ma,
                configured_->route,
                configured_->required_capabilities,
                operation_authorization_digest(current_policy),
                error))
        {
            record_failure("acquire-failed", true, true);
            result.error = error;
            return result;
        }
        record.lease = provider_->leaseId();
        record.endpoint_closed = false;
        record.state = "acquired";
        update_record(record);
        const bool submitted = configured_->tone
            ? backend_->emitTone(configured_->tone_program, error)
            : configured_->finite_events
                ? backend_->emitEvents(configured_->event_program, error)
                : backend_->emitFrame(configured_->clock_plan, configured_->symbols, error);
        if (!submitted)
        {
            const std::string submit_error = error;
            std::string cleanup_error;
            const bool cleanup_ok = backend_->cleanup(cleanup_error);
            record_failure(cleanup_ok ? "submit-failed" : "cleanup-fault",
                true, cleanup_ok);
            if (!cleanup_ok && !cleanup_error.empty())
                result.error = submit_error + " Cleanup failed: " + cleanup_error;
            else
                result.error = submit_error;
            return result;
        }
        record.generation = backend_->generation();
        record.state = "running";
        update_record(record);
    }

    bool stop_sent = false;
    const auto execution_deadline = configured_->continuous_tone
        ? std::optional<std::chrono::steady_clock::time_point>{}
        : std::optional<std::chrono::steady_clock::time_point>{
            std::chrono::steady_clock::now() + plan.summary.total_duration +
            std::chrono::seconds{5}};
    std::optional<std::chrono::steady_clock::time_point> drain_deadline;
    for (;;)
    {
        if ((stop_requested_.load(std::memory_order_acquire) ||
                owner_.backendShouldStop()) && !stop_sent)
        {
            std::lock_guard<std::mutex> lock(backend_mutex_);
            if (!backend_->cancel(error))
            {
                result.error = error;
                result.faulted = true;
                return result;
            }
            stop_sent = true;
            auto record = wsprrypi::rp1GpclkOperationRecordSnapshot();
            record.cancellation_requested = true;
            record.state = "draining";
            update_record(record);
            drain_deadline = std::chrono::steady_clock::now() +
                std::chrono::seconds{5};
        }

        if (!stop_sent && execution_deadline &&
            std::chrono::steady_clock::now() >= *execution_deadline)
        {
            std::lock_guard<std::mutex> lock(backend_mutex_);
            if (!backend_->timedOut(error))
            {
                result.error = error;
                result.faulted = true;
                return result;
            }
            stop_sent = true;
            auto record = wsprrypi::rp1GpclkOperationRecordSnapshot();
            record.cancellation_requested = true;
            record.state = "draining";
            update_record(record);
            drain_deadline = std::chrono::steady_clock::now() +
                std::chrono::seconds{5};
        }

        {
            std::lock_guard<std::mutex> lock(backend_mutex_);
            const auto event_state = configured_->finite_events
                ? provider_->eventState(backend_->generation())
                : wsprrypi::Rp1GpclkProviderEventState{
                      provider_->state(backend_->generation()), 0, 0};
            const auto state = event_state.completion;
            if (configured_->finite_events &&
                event_state.current_event < plan.events.size())
                owner_.backendReportExecutionProgress(event_state.current_event);
            if (state == wsprrypi::Rp1GpclkCompletionState::complete ||
                state == wsprrypi::Rp1GpclkCompletionState::failed)
            {
                if (!backend_->cleanup(error))
                {
                    result.error = error;
                    result.faulted = true;
                    return result;
                }
                auto record = wsprrypi::rp1GpclkOperationRecordSnapshot();
                record.terminal_reason = event_state.terminal_reason;
                record.state = state == wsprrypi::Rp1GpclkCompletionState::complete
                    ? "complete" : "failed";
                record.cleanup_attempted = true;
                record.cleanup_complete = true;
                record.endpoint_closed = true;
                record.lease = 0;
                record.finished_monotonic_ns = static_cast<std::uint64_t>(
                    std::chrono::duration_cast<std::chrono::nanoseconds>(
                        std::chrono::steady_clock::now().time_since_epoch()).count());
                update_record(record);
                if (state == wsprrypi::Rp1GpclkCompletionState::failed)
                {
                    result.error = "RP1 GPCLK provider reported frame failure.";
                    result.faulted = true;
                    return result;
                }
                result.ok = true;
                result.stopped = stop_sent;
                return result;
            }
        }
        if (drain_deadline && std::chrono::steady_clock::now() >= *drain_deadline)
        {
            result.error = "RP1 GPCLK provider did not reach a terminal state within the bounded drain interval.";
            result.faulted = true;
            return result;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
}

wsprrypi::StartupQuiesceResult WsprRp1GpclkBackend::quiesceForStartup()
{
    std::string error;
    std::lock_guard<std::mutex> lock(backend_mutex_);
    wsprrypi::Rp1GpclkProviderIdentity identity;
    constexpr std::uint64_t required = RP1_GPCLK_CAP_STABLE_STATE |
        RP1_GPCLK_CAP_ROUTE_IDENTITY | RP1_GPCLK_CAP_COMPAT_IDENTITY |
        RP1_GPCLK_CAP_CLEANUP_FAULT_LATCH;
    if (!provider_->query(
            RP1_GPCLK_ROUTE_INVALID, required, false, identity, error))
        return {false, error};
    return {true, {}};
}

void WsprRp1GpclkBackend::stop() noexcept
{
    stop_requested_.store(true, std::memory_order_release);
}

wsprrypi::CleanupResult WsprRp1GpclkBackend::cleanup() noexcept
{
    stop();
    std::string error;
    std::lock_guard<std::mutex> lock(backend_mutex_);
    const bool ok = backend_->cleanup(error);
    auto record = wsprrypi::rp1GpclkOperationRecordSnapshot();
    record.cleanup_attempted = true;
    record.cleanup_complete = ok;
    record.endpoint_closed = ok;
    record.lease = ok ? 0 : record.lease;
    if (!ok) record.state = "cleanup-fault";
    update_record(record);
    configured_.reset();
    wsprrypi::invalidateRp1GpclkDevelopmentOperation();
    return {ok, error};
}
