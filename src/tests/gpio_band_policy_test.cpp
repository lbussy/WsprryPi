#include "execution_plan_compiler.hpp"
#include "gpio_band_policy.hpp"
#include "transmission_controller.hpp"

#include <chrono>
#include <cstdlib>
#include <initializer_list>
#include <iostream>
#include <string>

namespace
{
void require(bool condition, const std::string& message)
{
    if (condition)
        return;

    std::cerr << "FAIL: " << message << std::endl;
    std::exit(EXIT_FAILURE);
}

class ProbeBackend final : public wsprrypi::ITransmissionBackend
{
public:
    explicit ProbeBackend(wsprrypi::BackendKind kind)
        : kind_(kind)
    {
    }

    wsprrypi::BackendInfo info() const override
    {
        return {kind_, "probe", "policy test backend"};
    }

    wsprrypi::BackendCapabilities capabilities() const override
    {
        return {};
    }

    wsprrypi::BackendCompileResult configure(
        const wsprrypi::ExecutionPlan&,
        const wsprrypi::BackendExecutionInputs&) override
    {
        ++configure_calls;
        return {true, {}, {}};
    }

    wsprrypi::ExecutionResult execute(
        const wsprrypi::ExecutionPlan&) override
    {
        return {true, false, false, {}};
    }

    wsprrypi::StartupQuiesceResult quiesceForStartup() override
    {
        return {true, {}};
    }

    void stop() noexcept override {}
    void cleanup() noexcept override {}

    int configure_calls{0};

private:
    wsprrypi::BackendKind kind_;
};

wsprrypi::MorseTiming short_timing()
{
    wsprrypi::MorseTiming timing;
    timing.dot = std::chrono::milliseconds(1);
    timing.dash = std::chrono::milliseconds(3);
    timing.intra_element_gap = std::chrono::milliseconds(1);
    timing.inter_character_gap = std::chrono::milliseconds(3);
    timing.inter_word_gap = std::chrono::milliseconds(7);
    return timing;
}

wsprrypi::TransmissionRequest request_for_mode(
    wsprrypi::BackendKind backend,
    wsprrypi::TransmissionMode mode,
    double frequency_hz)
{
    wsprrypi::TransmissionRequest request;
    request.mode = mode;
    request.output.backend = backend;
    request.output.output = backend == wsprrypi::BackendKind::SI5351
        ? wsprrypi::ClockSource::SI5351_CLK0
        : wsprrypi::ClockSource::GPIO_CLK;

    switch (mode)
    {
    case wsprrypi::TransmissionMode::WSPR:
    {
        PreparedWsprTransmission prepared;
        prepared.frames.resize(1);
        prepared.frames.front().symbols = {0, 1, 2, 3};
        prepared.current_frame = 1;
        wsprrypi::WsprPayload payload;
        payload.prepared = prepared;
        payload.base_frequency_hz = frequency_hz;
        request.payload = payload;
        break;
    }
    case wsprrypi::TransmissionMode::QRSS:
    {
        wsprrypi::QrssPayload payload;
        payload.message = "E";
        payload.frequency_hz = frequency_hz;
        payload.timing = short_timing();
        request.payload = payload;
        break;
    }
    case wsprrypi::TransmissionMode::FSKCW:
    {
        wsprrypi::FskcwPayload payload;
        payload.message = "E";
        payload.space_frequency_hz = frequency_hz;
        payload.mark_frequency_hz = frequency_hz + 5.0;
        payload.timing = short_timing();
        request.payload = payload;
        break;
    }
    case wsprrypi::TransmissionMode::DFCW:
    {
        wsprrypi::DfcwPayload payload;
        payload.message = "E";
        payload.dot_frequency_hz = frequency_hz;
        payload.dash_frequency_hz = frequency_hz + 5.0;
        payload.timing = short_timing();
        request.payload = payload;
        break;
    }
    case wsprrypi::TransmissionMode::TONE:
    {
        wsprrypi::TonePayload payload;
        payload.frequency_hz = frequency_hz;
        payload.duration = std::chrono::milliseconds(1);
        request.payload = payload;
        break;
    }
    case wsprrypi::TransmissionMode::CW:
        require(false, "generic CW compilation is not implemented");
        break;
    }

    return request;
}

void require_controller_policy(
    wsprrypi::BackendKind backend_kind,
    wsprrypi::TransmissionMode mode,
    double frequency_hz,
    bool expected_allowed,
    const std::string& context)
{
    wsprrypi::ExecutionPlanCompiler compiler;
    ProbeBackend backend(backend_kind);
    wsprrypi::TransmissionController controller(compiler, backend);
    const auto result = controller.prepare(
        request_for_mode(backend_kind, mode, frequency_hz));

    require(
        result.ok == expected_allowed,
        context + " policy result must match expectation");
    require(
        backend.configure_calls == (expected_allowed ? 1 : 0),
        context + " must reject before backend configuration");
    if (!expected_allowed)
    {
        require(
            result.error.find("Direct GPIO transmission is blocked") !=
                std::string::npos &&
            result.error.find("Si5351") != std::string::npos,
            context + " must provide an actionable GPIO-only error");
        require(
            controller.prepared_plan() == nullptr,
            context + " must not retain a rejected execution plan");
    }
}
} // namespace

int main()
{
    const std::initializer_list<wsprrypi::TransmissionMode> exercised_modes{
        wsprrypi::TransmissionMode::WSPR,
        wsprrypi::TransmissionMode::QRSS,
        wsprrypi::TransmissionMode::FSKCW,
        wsprrypi::TransmissionMode::DFCW,
        wsprrypi::TransmissionMode::TONE};

    for (const auto mode : exercised_modes)
    {
        for (const double blocked_band_frequency :
             {24950000.0, 51000000.0, 145000000.0})
        {
            require_controller_policy(
                wsprrypi::BackendKind::RPI_CLOCK_GPIO,
                mode,
                blocked_band_frequency,
                false,
                "disqualified GPIO mode");
        }
    }

    for (const double blocked_frequency :
         {24890000.0, 24924600.0, 24950000.0, 24990000.0,
          50000000.0, 50293000.0, 51000000.0, 52000000.0,
          144000000.0, 144489000.0, 145000000.0, 148000000.0})
    {
        require_controller_policy(
            wsprrypi::BackendKind::RPI_CLOCK_GPIO,
            wsprrypi::TransmissionMode::TONE,
            blocked_frequency,
            false,
            "disqualified GPIO frequency");
    }

    for (const auto mode : exercised_modes)
    {
        for (const double qualified_frequency :
             {3568600.0, 14095600.0, 21094600.0, 28124600.0})
        {
            require_controller_policy(
                wsprrypi::BackendKind::RPI_CLOCK_GPIO,
                mode,
                qualified_frequency,
                true,
                "qualified GPIO frequency");
        }
    }

    for (const double adjacent_frequency :
         {24889999.0, 24990001.0, 49999999.0, 52000001.0,
          143999999.0, 148000001.0})
    {
        require_controller_policy(
            wsprrypi::BackendKind::RPI_CLOCK_GPIO,
            wsprrypi::TransmissionMode::TONE,
            adjacent_frequency,
            true,
            "frequency adjacent to a disqualified band");
    }

    for (const auto mode : exercised_modes)
    {
        for (const double si5351_frequency :
             {24924600.0, 50293000.0, 144489000.0})
        {
            require_controller_policy(
                wsprrypi::BackendKind::SI5351,
                mode,
                si5351_frequency,
                true,
                "Si5351 frequency");
        }
    }

    std::cout << "gpio_band_policy_test passed" << std::endl;
    return EXIT_SUCCESS;
}
