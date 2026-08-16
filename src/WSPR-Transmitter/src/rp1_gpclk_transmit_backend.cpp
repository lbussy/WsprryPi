#include "rp1_gpclk_transmit_backend.hpp"

#include "wspr_transmit.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace
{
constexpr double kRp1ParentFrequencyHz = 50000000.0;
constexpr double kWsprSymbolSeconds = 8192.0 / 12000.0;
constexpr double kWsprToneSpacingHz = 1.0 / kWsprSymbolSeconds;
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
    caps.supported_modes = 0xffffffffu;
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
    planner_input.parent_frequency_hz = kRp1ParentFrequencyHz;
    planner_input.source_rate_ppm = plan.calibration.ppm;
    planner_input.maximum_output_hz = 40000000.0;
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
        if (!backend_->prepare(configured_->drive_ma, error))
        {
            result.error = error;
            return result;
        }
        const bool submitted = configured_->finite_events
            ? backend_->emitEvents(configured_->event_program, error)
            : backend_->emitFrame(configured_->clock_plan, configured_->symbols, error);
        if (!submitted)
        {
            const std::string submit_error = error;
            std::string cleanup_error;
            if (!backend_->cleanup(cleanup_error) && !cleanup_error.empty())
                result.error = submit_error + " Cleanup failed: " + cleanup_error;
            else
                result.error = submit_error;
            return result;
        }
    }

    bool stop_sent = false;
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
        std::this_thread::sleep_for(std::chrono::milliseconds{10});
    }
}

wsprrypi::StartupQuiesceResult WsprRp1GpclkBackend::quiesceForStartup()
{
    std::string error;
    std::lock_guard<std::mutex> lock(backend_mutex_);
    if (!backend_->prepare(wsprrypi::Rp1GpclkBackend::kDefaultDriveMa, error))
        return {false, error};
    if (!backend_->cleanup(error))
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
    configured_.reset();
    return {ok, error};
}
