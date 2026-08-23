#pragma once

#include "rp1_gpclk_backend.hpp"
#include "rp1_gpclk_linux_provider.hpp"
#include "rp1_gpclk_event_program.hpp"
#include "rp1_gpclk_development_policy.hpp"
#include "transmission_backend.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

class IControllerBridge;

namespace wsprrypi
{
struct Rp1GpclkOperationRecord
{
    std::uint32_t schema_version{1};
    std::string operation_id;
    std::string module_id;
    std::string module_version;
    std::string compatibility_id;
    std::uint16_t uapi_abi{0};
    std::uint32_t route{0};
    std::string endpoint{"/dev/rp1-gpclk"};
    std::uint64_t lease{0};
    std::uint64_t generation{0};
    std::string state{"idle"};
    std::uint32_t terminal_reason{0};
    bool cancellation_requested{false};
    bool cleanup_attempted{false};
    bool cleanup_complete{false};
    bool endpoint_closed{true};
    bool execution_authorized{false};
    bool qualification_claim{false};
    std::uint64_t process_id{0};
    std::string executable{"wsprrypi"};
    std::uint64_t started_monotonic_ns{0};
    std::uint64_t finished_monotonic_ns{0};
};

Rp1GpclkOperationRecord rp1GpclkOperationRecordSnapshot();
}

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
        wsprrypi::Rp1GpclkProviderToneProgram tone_program{};
        bool finite_events{false};
        bool tone{false};
        bool continuous_tone{false};
        std::uint32_t drive_ma{2};
        std::uint32_t route{0};
        std::uint64_t required_capabilities{0};
        wsprrypi::Rp1GpclkDevelopmentPolicyInputs development_policy{};
    };

    IControllerBridge& owner_;
    std::unique_ptr<wsprrypi::Rp1GpclkIo> io_;
    std::unique_ptr<wsprrypi::Rp1GpclkProvider> provider_;
    std::unique_ptr<wsprrypi::Rp1GpclkBackend> backend_;
    std::optional<ConfiguredFrame> configured_;
    std::atomic<bool> stop_requested_{false};
    std::mutex backend_mutex_;
};
