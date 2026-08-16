#pragma once

#include "rp1_gpclk_backend.hpp"
#include "rp1_gpclk_linux_provider.hpp"
#include "rp1_gpclk_event_program.hpp"
#include "transmission_backend.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>

class IControllerBridge;

class WsprRp1GpclkBackend final : public wsprrypi::ITransmissionBackend
{
public:
    explicit WsprRp1GpclkBackend(IControllerBridge& owner);
    WsprRp1GpclkBackend(
        IControllerBridge& owner,
        std::unique_ptr<wsprrypi::Rp1GpclkProvider> provider);
    ~WsprRp1GpclkBackend() override;

    wsprrypi::BackendInfo info() const override;
    wsprrypi::BackendCapabilities capabilities() const override;
    wsprrypi::BackendCompileResult configure(
        const wsprrypi::ExecutionPlan& plan,
        const wsprrypi::BackendExecutionInputs& inputs) override;
    wsprrypi::ExecutionResult execute(
        const wsprrypi::ExecutionPlan& plan) override;
    wsprrypi::StartupQuiesceResult quiesceForStartup() override;
    void stop() noexcept override;
    wsprrypi::CleanupResult cleanup() noexcept override;

private:
    struct ConfiguredFrame
    {
        wsprrypi::PlanId plan_id{};
        wsprrypi::Rp1GpclkPlan clock_plan{};
        std::array<std::uint8_t, 162> symbols{};
        wsprrypi::Rp1GpclkProviderEventProgram event_program{};
        bool finite_events{false};
        std::uint32_t drive_ma{2};
    };

    IControllerBridge& owner_;
    std::unique_ptr<wsprrypi::Rp1GpclkIo> io_;
    std::unique_ptr<wsprrypi::Rp1GpclkProvider> provider_;
    std::unique_ptr<wsprrypi::Rp1GpclkBackend> backend_;
    std::optional<ConfiguredFrame> configured_;
    std::atomic<bool> stop_requested_{false};
    std::mutex backend_mutex_;
};
