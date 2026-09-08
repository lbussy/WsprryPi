#include "execution_plan.hpp"
#include "transmission_controller.hpp"
#include "wspr_transmit.hpp"
#include "wspr_transmit_backend_si5351.hpp"

#include <array>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
    constexpr std::uint8_t kOutputEnableRegister = 3;
    constexpr std::uint8_t kPllAParameterBaseRegister = 26;
    constexpr std::uint8_t kPllResetRegister = 177;
    constexpr std::uint8_t kOutputDisableAll = 0xff;
    constexpr std::uint8_t kClk0Enabled = 0xfe;
    int failures = 0;

    void expect(bool condition, const std::string& message)
    {
        if (condition)
            return;
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }

    class FakeI2CAdapter final : public Si5351Device::I2CAdapter
    {
    public:
        int openDevice(const std::string&, int) override
        {
            ++open_calls;
            return 42;
        }

        int selectSlave(int, std::uint8_t) override
        {
            ++select_calls;
            return 0;
        }

        ssize_t writeData(int, const void* data, std::size_t size) override
        {
            const auto* bytes = static_cast<const std::uint8_t*>(data);
            if (size == 1)
            {
                selected_register = bytes[0];
                return 1;
            }
            if (size < 2 || size > 9) { errno = EIO; return -1; }
            transactions.push_back(std::vector<std::uint8_t>(bytes, bytes + size));
            for (std::size_t i = 1; i < size; ++i)
            {
                const std::uint8_t address = bytes[0] + i - 1;
                ++address_attempts[address];
                if (address == fail_address && address_attempts[address] == fail_address_occurrence)
                { errno = EIO; return i == 1 ? -1 : static_cast<ssize_t>(i); }
                registers[address] = bytes[i];
                writes.push_back({address, bytes[i]});
                if (after_write) after_write(address, bytes[i], address_attempts[address]);
            }
            return static_cast<ssize_t>(size);
        }

        ssize_t readData(int, void* data, std::size_t size) override
        {
            if (size != 1)
            {
                errno = EIO;
                return -1;
            }
            *static_cast<std::uint8_t*>(data) = registers[selected_register];
            return 1;
        }

        int closeDevice(int) override
        {
            ++close_calls;
            return 0;
        }

        std::array<std::uint8_t, 256> registers{};
        std::array<std::size_t, 256> address_attempts{};
        std::vector<std::pair<std::uint8_t, std::uint8_t>> writes;
        std::vector<std::vector<std::uint8_t>> transactions;
        std::uint8_t selected_register = 0;
        std::uint8_t fail_address = 0xff;
        std::size_t fail_address_occurrence = 0;
        std::function<void(std::uint8_t, std::uint8_t, std::size_t)>
            after_write;
        int open_calls = 0;
        int select_calls = 0;
        int close_calls = 0;
    };

    class TestBridge final : public IControllerBridge
    {
    public:
        WsprTransmitState backendStateValue() const noexcept override
        {
            return state_.load();
        }

        void backendSetStateValue(WsprTransmitState state) noexcept override
        {
            state_.store(state);
        }

        bool backendShouldStop() const noexcept override
        {
            return stop_requested_.load();
        }
        void backendSignalStopRequest() noexcept override
        {
            stop_requested_.store(true);
        }
        void backendRequestStopTxNoJoin() noexcept override
        {
            backendSignalStopRequest();
        }
        bool backendWaitInterruptableFor(std::chrono::nanoseconds) override
        {
            ++wait_calls;
            if (interrupt_on_wait_call != 0 &&
                wait_calls == interrupt_on_wait_call)
            {
                backendSignalStopRequest();
                return false;
            }
            return true;
        }
        void backendThrowIfStopRequested(const char*) override {}
        void backendReportExecutionProgress(std::size_t) noexcept override
        {
            ++progress_calls;
        }
        void backendFireTransmitCallback(
            WsprTransmissionCallbackEvent,
            WsprTransmitLogLevel,
            const std::string& message,
            double) override
        {
            logs.push_back(message);
        }
        bool backendRestartCurrentConfiguration() override { return false; }

        std::size_t progress_calls = 0;
        std::size_t wait_calls = 0;
        std::size_t interrupt_on_wait_call = 0;
        std::vector<std::string> logs;

    private:
        std::atomic<WsprTransmitState> state_{WsprTransmitState::ENABLED};
        std::atomic<bool> stop_requested_{false};
    };

    class FixedPlanCompiler final : public wsprrypi::IExecutionPlanCompiler
    {
    public:
        explicit FixedPlanCompiler(wsprrypi::ExecutionPlan plan)
            : plan_(std::move(plan))
        {
        }

        wsprrypi::ExecutionPlan compile(
            const wsprrypi::TransmissionRequest&) const override
        {
            return plan_;
        }

    private:
        wsprrypi::ExecutionPlan plan_;
    };

    WsprSi5351Backend::Config config(
        const std::shared_ptr<FakeI2CAdapter>& adapter,
        bool dry_run = false)
    {
        WsprSi5351Backend::Config result;
        result.device.reference_hz = 27000000;
        result.device_adapter = adapter;
        result.planner.reference_hz = 27000000;
        result.planner.tx_output = Si5351Device::Output::CLK0;
        result.power_level = 1;
        result.dry_run = dry_run;
        return result;
    }

    wsprrypi::ExecutionPlan four_tone_plan(
        std::size_t symbol_count,
        std::chrono::nanoseconds symbol_interval =
            std::chrono::nanoseconds::zero(),
        std::chrono::nanoseconds final_duration =
            std::chrono::nanoseconds::zero())
    {
        constexpr double tones_hz[] = {
            144490497.802734375,
            144490499.267578125,
            144490500.732421875,
            144490502.197265625};
        wsprrypi::ExecutionPlan plan;
        plan.id.value = 379;
        plan.request_id.value = 379;
        plan.mode = wsprrypi::TransmissionMode::WSPR;
        plan.backend = wsprrypi::BackendKind::SI5351;
        plan.reference_frequency_hz = 27000000.0;

        for (std::size_t i = 0; i < symbol_count; ++i)
        {
            wsprrypi::RfEvent event;
            event.offset_from_start = symbol_interval *
                static_cast<std::int64_t>(i);
            event.duration = i + 1 == symbol_count
                ? final_duration
                : std::chrono::nanoseconds::zero();
            event.type = wsprrypi::RfEventType::SET_FREQUENCY;
            event.frequency_hz = tones_hz[i % 4];
            event.rf_on = true;
            plan.events.push_back(event);
        }
        plan.summary.event_count = plan.events.size();
        return plan;
    }

    wsprrypi::ExecutionPlan single_tone_plan(
        double ppm = 2.409358,
        std::chrono::nanoseconds duration =
            std::chrono::nanoseconds::zero())
    {
        wsprrypi::ExecutionPlan plan;
        plan.id.value = 383;
        plan.request_id.value = 383;
        plan.mode = wsprrypi::TransmissionMode::TONE;
        plan.backend = wsprrypi::BackendKind::SI5351;
        plan.reference_frequency_hz = 27000000.0;
        plan.calibration.ppm = ppm;

        wsprrypi::RfEvent event;
        event.duration = duration;
        event.type = wsprrypi::RfEventType::SET_FREQUENCY;
        event.frequency_hz = 144490497.802734375;
        event.rf_on = true;
        plan.events.push_back(event);
        plan.summary.event_count = 1;
        return plan;
    }

    bool configure(
        WsprSi5351Backend& backend,
        const wsprrypi::ExecutionPlan& plan)
    {
        return backend.configure(
            plan,
            wsprrypi::BackendExecutionInputs{1, 0}).ok;
    }

    std::size_t count_write(
        const FakeI2CAdapter& adapter,
        std::uint8_t address,
        std::uint8_t value)
    {
        std::size_t count = 0;
        for (const auto& write : adapter.writes)
        {
            if (write.first == address && write.second == value)
                ++count;
        }
        return count;
    }

    bool has_log(const TestBridge& bridge, const std::string& text)
    {
        for (const std::string& message : bridge.logs)
        {
            if (message.find(text) != std::string::npos)
                return true;
        }
        return false;
    }

    void expect_interrupted_and_inhibited(
        const wsprrypi::ExecutionResult& result,
        const TestBridge& bridge,
        const FakeI2CAdapter& adapter,
        std::size_t expected_progress,
        const std::string& label);

    void test_pll_only_and_readiness()
    {
        for (bool locked : {true, false}) {
            TestBridge bridge;
            auto adapter = std::make_shared<FakeI2CAdapter>();
            adapter->registers[0] = locked ? 0 : 0x20;
            auto cfg = config(adapter); cfg.pll_only_updates = true;
            WsprSi5351Backend backend(bridge, cfg);
            auto plan = four_tone_plan(4);
            expect(configure(backend, plan), "PLL-only config");
            auto result = backend.execute(plan);
            expect(result.ok == locked, "PLLA readiness gates output");
            expect(adapter->registers[3] == 255, "PLL-only cleanup disabled");
            if (locked) {
                expect(adapter->address_attempts[177] == 2, "Only startup/first tone reset");
                expect(count_write(*adapter,3,0xfe) == 1, "No output re-enable between compatible PLL tones");
                expect(adapter->address_attempts[42] == 1, "MS configured only for first tone");
            } else {
                expect(count_write(*adapter,3,0xfe) == 0, "Unlocked PLL never enables output");
                expect(bridge.wait_calls == 50, "Readiness retry count bounded");
            }
        }
        for (bool bus_failure : {true, false}) {
            TestBridge bridge;
            auto adapter=std::make_shared<FakeI2CAdapter>();
            auto cfg=config(adapter);cfg.pll_only_updates=true;
            if (bus_failure) { adapter->fail_address=26;adapter->fail_address_occurrence=3; }
            else adapter->after_write=[adapter](std::uint8_t reg,std::uint8_t,std::size_t count) {
                if (reg==26 && count==3) adapter->registers[0]=0x20;
            };
            WsprSi5351Backend backend(bridge,cfg);auto plan=four_tone_plan(4);
            expect(configure(backend,plan), "Fast retune failure config");
            auto result=backend.execute(plan);
            expect(!result.ok && adapter->registers[3]==255, "Fast retune error inhibits output");
            expect(count_write(*adapter,3,0xfe)==1, "Fast retune error never re-enables RF");
            adapter->after_write={};
        }
        TestBridge bridge; bridge.interrupt_on_wait_call = 1;
        auto adapter = std::make_shared<FakeI2CAdapter>(); adapter->registers[0] = 0x80;
        WsprSi5351Backend backend(bridge,config(adapter));auto plan=single_tone_plan();
        expect(configure(backend,plan), "Readiness cancellation config");
        auto result=backend.execute(plan);
        expect(result.stopped && adapter->registers[3]==255, "Readiness cancellation stays inhibited");
    }

    void test_committed_calibration_snapshot_and_reporting()
    {
        TestBridge bridge;
        auto adapter = std::make_shared<FakeI2CAdapter>();
        WsprSi5351Backend backend(bridge, config(adapter));
        wsprrypi::ExecutionPlan plan = four_tone_plan(4);
        plan.calibration.ppm = 2.409358;

        expect(configure(backend, plan),
            "committed positive calibration should configure");
        expect(has_log(bridge, "Si5351 calibration: ppm=2.409358"),
            "backend should report the committed calibration PPM");
        expect(has_log(bridge, "nominal reference=27000000"),
            "backend should report the nominal Si5351 reference");
        expect(has_log(bridge, "effective reference=26999934"),
            "backend should report the calibrated effective reference");
        expect(has_log(bridge, "requested RF=144490497"),
            "backend should report requested RF separately");
        expect(has_log(bridge, "calculated output=144490497"),
            "backend should report calculated output separately");
        expect(adapter->open_calls == 0 && adapter->select_calls == 0 &&
                adapter->writes.empty(),
            "configuration and reporting must not access I2C");
    }

    void test_invalid_calibration_fails_before_output_enable()
    {
        const double invalid_ppm[] = {
            std::numeric_limits<double>::quiet_NaN(),
            std::numeric_limits<double>::infinity(),
            -std::numeric_limits<double>::infinity(),
            201.0};

        for (const double ppm : invalid_ppm)
        {
            TestBridge bridge;
            auto adapter = std::make_shared<FakeI2CAdapter>();
            WsprSi5351Backend backend(bridge, config(adapter));
            wsprrypi::ExecutionPlan plan = four_tone_plan(4);
            plan.calibration.ppm = ppm;

            const wsprrypi::BackendCompileResult result = backend.configure(
                plan,
                wsprrypi::BackendExecutionInputs{1, 0});
            expect(!result.ok &&
                    result.error.find("calibration PPM") !=
                        std::string::npos,
                "invalid calibration should fail with a calibration error");
            expect(adapter->open_calls == 0 && adapter->select_calls == 0 &&
                    adapter->close_calls == 0 && adapter->writes.empty() &&
                    count_write(
                        *adapter,
                        kOutputEnableRegister,
                        kClk0Enabled) == 0,
                "invalid calibration must fail before I2C output enable");
        }
    }

    void test_single_tone_calibration_reporting_and_cleanup()
    {
        TestBridge bridge;
        auto adapter = std::make_shared<FakeI2CAdapter>();
        WsprSi5351Backend backend(bridge, config(adapter));
        const wsprrypi::ExecutionPlan plan = single_tone_plan();

        expect(configure(backend, plan),
            "calibrated single 2 m tone should configure");
        expect(has_log(bridge, "Si5351 calibration: ppm=2.409358"),
            "single tone should report committed calibration");
        expect(has_log(bridge, "effective reference=26999934"),
            "single tone should report effective reference");
        expect(has_log(bridge, "requested RF=144490497"),
            "single tone should report requested RF");
        expect(has_log(bridge, "calculated output=144490497"),
            "single tone should report calculated output");
        expect(adapter->writes.empty(),
            "single-tone configuration must not access I2C");

        const wsprrypi::ExecutionResult result = backend.execute(plan);
        expect(result.ok && !result.stopped && !result.faulted,
            "calibrated single 2 m tone should execute");
        expect(bridge.progress_calls == 1,
            "single-tone execution should report one event");
        expect(count_write(
                *adapter,
                kOutputEnableRegister,
                kClk0Enabled) == 1,
            "single-tone execution should enable CLK0 exactly once");
        expect(adapter->address_attempts[kPllResetRegister] == 2,
            "startup plus the single 2 m tone should reset PLLA twice");

        bool output_inhibited = true;
        for (const auto& write : adapter->writes)
        {
            if (write.first == kOutputEnableRegister)
                output_inhibited = write.second == kOutputDisableAll;
            if (write.first == kPllAParameterBaseRegister ||
                write.first == kPllResetRegister)
            {
                expect(output_inhibited,
                    "single-tone PLL programming must remain output "
                    "inhibited");
            }
        }
        expect(adapter->registers[kOutputEnableRegister] == kOutputDisableAll,
            "single-tone cleanup should leave register 3 at 0xFF");
        expect(adapter->close_calls == 1,
            "single-tone cleanup should close the device session");
    }

    void test_single_tone_interruption_cleans_up()
    {
        TestBridge bridge;
        bridge.interrupt_on_wait_call = 1;
        auto adapter = std::make_shared<FakeI2CAdapter>();
        WsprSi5351Backend backend(bridge, config(adapter));
        const wsprrypi::ExecutionPlan plan = single_tone_plan(
            2.409358,
            std::chrono::seconds(1));

        expect(configure(backend, plan),
            "interruptible single 2 m tone should configure");
        const wsprrypi::ExecutionResult result = backend.execute(plan);
        expect_interrupted_and_inhibited(
            result,
            bridge,
            *adapter,
            1,
            "single-tone wait interruption");
    }

    void test_162_symbol_transition_order()
    {
        TestBridge bridge;
        auto adapter = std::make_shared<FakeI2CAdapter>();
        WsprSi5351Backend backend(bridge, config(adapter));
        const wsprrypi::ExecutionPlan plan = four_tone_plan(162);

        expect(configure(backend, plan),
            "four-tone 2 m plan should configure");
        const wsprrypi::ExecutionResult result = backend.execute(plan);
        expect(result.ok && !result.faulted,
            "162-symbol fake-I2C execution should succeed");
        expect(bridge.progress_calls == 162,
            "162-symbol execution should report every symbol");
        expect(adapter->open_calls == 1 && adapter->select_calls == 1 &&
                adapter->close_calls == 1,
            "fake-I2C execution should use one bounded device session");
        expect(count_write(*adapter, kOutputEnableRegister, kClk0Enabled) ==
                162,
            "every 2 m symbol should end with CLK0 enabled");
        expect(adapter->address_attempts[kPllResetRegister] == 163,
            "startup plus every 2 m symbol should reset PLLA");

        bool output_inhibited = true;
        for (const auto& write : adapter->writes)
        {
            if (write.first == kOutputEnableRegister)
                output_inhibited = write.second == kOutputDisableAll;
            if (write.first == kPllAParameterBaseRegister)
            {
                expect(output_inhibited,
                    "every PLL retune must begin while all outputs are "
                    "inhibited");
            }
            if (write.first == kPllResetRegister)
            {
                expect(output_inhibited,
                    "every PLL reset must occur while all outputs are "
                    "inhibited");
            }
        }
        expect(adapter->registers[kOutputEnableRegister] == kOutputDisableAll,
            "successful execution cleanup should leave all outputs disabled");
    }

    void test_controller_consumes_completed_cleanup_without_reopening()
    {
        const wsprrypi::TransmissionMode modes[] = {
            wsprrypi::TransmissionMode::WSPR,
            wsprrypi::TransmissionMode::QRSS,
            wsprrypi::TransmissionMode::FSKCW,
            wsprrypi::TransmissionMode::DFCW};
        for (const wsprrypi::TransmissionMode mode : modes)
        {
            TestBridge bridge;
            auto adapter = std::make_shared<FakeI2CAdapter>();
            WsprSi5351Backend backend(bridge, config(adapter));
            wsprrypi::ExecutionPlan plan = mode ==
                    wsprrypi::TransmissionMode::WSPR
                ? four_tone_plan(4)
                : mode == wsprrypi::TransmissionMode::QRSS
                    ? single_tone_plan()
                    : four_tone_plan(2);
            plan.mode = mode;
            for (std::size_t i = 0; i < plan.events.size(); ++i)
                plan.events[i].frequency_hz = 14097100.0 +
                    static_cast<double>(i);
            FixedPlanCompiler compiler(plan);
            wsprrypi::TransmissionController controller(compiler, backend);
            wsprrypi::TransmissionRequest request;
            request.id.value = plan.request_id.value;
            request.output.backend = wsprrypi::BackendKind::SI5351;

            const wsprrypi::BackendCompileResult prepared = controller.prepare(
                request, wsprrypi::TransmissionPrepareOptions{1});
            expect(prepared.ok,
                "controller-level Si5351 plan should prepare");
            const wsprrypi::ExecutionResult result =
                controller.execute_prepared();
            expect(result.ok && result.cleanup_attempted && result.cleanup.ok,
                "controller must consume successful in-execute cleanup");
            expect(adapter->registers[kOutputEnableRegister] ==
                    kOutputDisableAll,
                "controller cleanup must leave register 3 at 0xFF");
            expect(adapter->open_calls == 1 && adapter->close_calls == 1,
                "controller cleanup must not reopen or re-close I2C");
            expect(backend.cleanup().ok,
                "cleanup must remain idempotent after controller consumption");
            expect(adapter->open_calls == 1 && adapter->close_calls == 1,
                "repeated cleanup must not touch the closed I2C session");
        }
    }

    void test_controller_preserves_genuine_idle_cleanup_failure()
    {
        TestBridge bridge;
        auto adapter = std::make_shared<FakeI2CAdapter>();
        FakeI2CAdapter* const adapter_state = adapter.get();
        bool output_was_enabled = false;
        adapter->after_write =
            [adapter_state, &output_was_enabled](
                std::uint8_t address,
                std::uint8_t value,
                std::size_t)
            {
                if (address != kOutputEnableRegister)
                    return;
                if (value == kClk0Enabled)
                {
                    output_was_enabled = true;
                    return;
                }
                if (output_was_enabled && value == kOutputDisableAll)
                {
                    adapter_state->fail_address = 16;
                    adapter_state->fail_address_occurrence =
                        adapter_state->address_attempts[16] + 1;
                }
            };
        WsprSi5351Backend backend(bridge, config(adapter));
        wsprrypi::ExecutionPlan plan = single_tone_plan();
        FixedPlanCompiler compiler(plan);
        wsprrypi::TransmissionController controller(compiler, backend);
        wsprrypi::TransmissionRequest request;
        request.id.value = plan.request_id.value;
        request.output.backend = wsprrypi::BackendKind::SI5351;

        expect(controller.prepare(
                request, wsprrypi::TransmissionPrepareOptions{1}).ok,
            "cleanup-failure controller plan should prepare");
        const wsprrypi::ExecutionResult result = controller.execute_prepared();
        expect(!result.ok && result.faulted && result.cleanup_attempted &&
                !result.cleanup.ok,
            "genuine idle-programming failure must fail controller cleanup");
        expect(result.error.find("idle programming") != std::string::npos,
            "genuine cleanup failure detail must remain observable");
        expect(adapter->registers[kOutputEnableRegister] == kOutputDisableAll,
            "idle-programming failure must still leave output inhibited");
        expect(adapter->open_calls == 1 && adapter->close_calls == 1,
            "failed cleanup result must not trigger post-close I2C access");
    }

    void test_controller_consumes_interrupted_cleanup()
    {
        TestBridge bridge;
        bridge.interrupt_on_wait_call = 1;
        auto adapter = std::make_shared<FakeI2CAdapter>();
        WsprSi5351Backend backend(bridge, config(adapter));
        wsprrypi::ExecutionPlan plan = single_tone_plan(
            2.409358, std::chrono::seconds(1));
        plan.events[0].frequency_hz = 14097100.0;
        FixedPlanCompiler compiler(plan);
        wsprrypi::TransmissionController controller(compiler, backend);
        wsprrypi::TransmissionRequest request;
        request.id.value = plan.request_id.value;
        request.output.backend = wsprrypi::BackendKind::SI5351;

        expect(controller.prepare(
                request, wsprrypi::TransmissionPrepareOptions{1}).ok,
            "interrupted controller plan should prepare");
        const wsprrypi::ExecutionResult result = controller.execute_prepared();
        expect(result.ok && result.stopped && result.cleanup_attempted &&
                result.cleanup.ok,
            "controller must consume successful interrupted cleanup");
        expect(adapter->registers[kOutputEnableRegister] == kOutputDisableAll,
            "interrupted controller cleanup must inhibit the output");
        expect(adapter->open_calls == 1 && adapter->close_calls == 1,
            "interrupted controller cleanup must use one I2C session");
    }

    void test_transition_failure_stays_inhibited()
    {
        TestBridge bridge;
        auto adapter = std::make_shared<FakeI2CAdapter>();
        adapter->fail_address = 28;
        adapter->fail_address_occurrence = 3;
        WsprSi5351Backend backend(bridge, config(adapter));
        const wsprrypi::ExecutionPlan plan = four_tone_plan(4);

        expect(configure(backend, plan),
            "failure-path 2 m plan should configure before I2C execution");
        const wsprrypi::ExecutionResult result = backend.execute(plan);
        expect(!result.ok && !result.error.empty(),
            "PLL write failure should fail execution with an error");
        expect(bridge.progress_calls == 2,
            "injected second-symbol failure should stop immediately");
        expect(adapter->registers[kOutputEnableRegister] == kOutputDisableAll,
            "PLL write failure and cleanup must leave all outputs disabled");
        expect(count_write(*adapter, kOutputEnableRegister, kClk0Enabled) == 1,
            "failed second transition must not re-enable CLK0");
        expect(adapter->close_calls == 1,
            "PLL write failure should close the device session");
    }

    void expect_interrupted_and_inhibited(
        const wsprrypi::ExecutionResult& result,
        const TestBridge& bridge,
        const FakeI2CAdapter& adapter,
        std::size_t expected_progress,
        const std::string& label)
    {
        expect(result.ok && result.stopped && !result.faulted &&
                result.error.empty(),
            label + " should report a clean interruption");
        expect(bridge.progress_calls == expected_progress,
            label + " should stop at the expected event");
        expect(adapter.registers[kOutputEnableRegister] ==
                kOutputDisableAll,
            label + " should leave all outputs disabled");
        expect(adapter.close_calls == 1,
            label + " should close the device session");
    }

    void test_transition_interruptions_stay_inhibited()
    {
        struct InterruptPoint
        {
            std::uint8_t address;
            std::size_t occurrence;
            const char* label;
        };
        const InterruptPoint points[] = {
            {kOutputEnableRegister, 3, "output-inhibit interruption"},
            {28, 2, "PLL-parameter interruption"},
            {kPllResetRegister, 2, "PLL-reset interruption"}};

        for (const InterruptPoint& point : points)
        {
            TestBridge bridge;
            auto adapter = std::make_shared<FakeI2CAdapter>();
            adapter->after_write =
                [&bridge, point](
                    std::uint8_t address,
                    std::uint8_t,
                    std::size_t occurrence)
                {
                    if (address == point.address &&
                        occurrence == point.occurrence)
                    {
                        bridge.backendSignalStopRequest();
                    }
                };
            WsprSi5351Backend backend(bridge, config(adapter));
            const wsprrypi::ExecutionPlan plan = four_tone_plan(4);

            expect(configure(backend, plan),
                std::string(point.label) + " plan should configure");
            const wsprrypi::ExecutionResult result = backend.execute(plan);
            expect_interrupted_and_inhibited(
                result, bridge, *adapter, 1, point.label);
            expect(count_write(
                    *adapter,
                    kOutputEnableRegister,
                    kClk0Enabled) == 0,
                std::string(point.label) + " must not re-enable CLK0");
        }
    }

    void test_symbol_wait_interruption_cleans_up()
    {
        TestBridge bridge;
        bridge.interrupt_on_wait_call = 1;
        auto adapter = std::make_shared<FakeI2CAdapter>();
        WsprSi5351Backend backend(bridge, config(adapter));
        const wsprrypi::ExecutionPlan plan = four_tone_plan(
            4,
            std::chrono::seconds(1));

        expect(configure(backend, plan),
            "symbol-wait interruption plan should configure");
        const wsprrypi::ExecutionResult result = backend.execute(plan);
        expect_interrupted_and_inhibited(
            result, bridge, *adapter, 1, "symbol-wait interruption");
    }

    void test_final_wait_interruption_cleans_up()
    {
        TestBridge bridge;
        bridge.interrupt_on_wait_call = 1;
        auto adapter = std::make_shared<FakeI2CAdapter>();
        WsprSi5351Backend backend(bridge, config(adapter));
        const wsprrypi::ExecutionPlan plan = four_tone_plan(
            4,
            std::chrono::nanoseconds::zero(),
            std::chrono::seconds(1));

        expect(configure(backend, plan),
            "final-wait interruption plan should configure");
        const wsprrypi::ExecutionResult result = backend.execute(plan);
        expect_interrupted_and_inhibited(
            result, bridge, *adapter, 4, "final-wait interruption");
    }

    void test_162_symbol_dry_run_avoids_i2c()
    {
        TestBridge bridge;
        auto adapter = std::make_shared<FakeI2CAdapter>();
        WsprSi5351Backend backend(bridge, config(adapter, true));
        const wsprrypi::ExecutionPlan plan = four_tone_plan(162);

        expect(configure(backend, plan),
            "dry-run four-tone 2 m plan should configure");
        const wsprrypi::ExecutionResult result = backend.execute(plan);
        expect(result.ok && bridge.progress_calls == 162,
            "dry-run should traverse all 162 symbols");
        expect(adapter->open_calls == 0 && adapter->select_calls == 0 &&
                adapter->close_calls == 0 && adapter->writes.empty(),
            "dry-run 2 m execution must not access I2C");
    }
}

void test_burst_and_cache_failure_contract()
{
    auto adapter = std::make_shared<FakeI2CAdapter>();
    Si5351Device::Config cfg; cfg.optimize_register_writes = true;
    Si5351Device device(cfg, adapter); expect(device.open(), "open burst device");
    std::vector<Si5351Device::RegisterWrite> writes;
    for (unsigned r = 26; r < 42; ++r) writes.push_back({static_cast<std::uint8_t>(r), 1});
    expect(device.writeRegisters(writes), "adjacent PLL blocks write");
    expect(adapter->transactions.size() == 2 && adapter->transactions[0].size() == 9 &&
        adapter->transactions[1][0] == 34, "burst stops at parameter block boundary");
    expect(device.writeRegister(16, 0x4c) && device.writeRegister(16, 0x4c), "cached clock control");
    expect(adapter->address_attempts[16] == 1, "identical stable control elided");
    device.writeRegister(177, 0x20); device.writeRegister(177, 0x20);
    device.writeRegister(3, 255); device.writeRegister(3, 255);
    expect(adapter->address_attempts[177] == 2 && adapter->address_attempts[3] == 2,
        "reset and output disable never elided");
    adapter->fail_address = 28; adapter->fail_address_occurrence = 2;
    expect(!device.writeRegisters(writes), "partial burst fails without retry");
    expect(adapter->address_attempts[28] == 2, "failed burst not retried");
    expect(device.writeRegister(16, 0x4c), "control reapplied after partial failure");
    expect(adapter->address_attempts[16] == 2, "failure invalidates cache");
    device.close(); expect(device.open(), "reopen burst device");
    device.writeRegister(16, 0x4c);
    expect(adapter->address_attempts[16] == 3, "reopen invalidates cache");
}

void test_optimized_backend_and_drive()
{
    for (bool optimized : {false, true}) for (int power = 1; power <= 4; ++power)
    {
        TestBridge bridge; auto adapter = std::make_shared<FakeI2CAdapter>();
        auto cfg = config(adapter); cfg.device.optimize_register_writes = optimized;
        cfg.pll_only_updates = true;
        WsprSi5351Backend backend(bridge, cfg); auto plan = four_tone_plan(4);
        expect(backend.configure(plan, wsprrypi::BackendExecutionInputs{power, 0}).ok,
            "drive comparison configure");
        bool active_drive_ok = true;
        adapter->after_write = [adapter, power, &active_drive_ok](std::uint8_t r, std::uint8_t v, std::size_t) {
            if (r == 3 && v == 0xfe && (adapter->registers[16] & 3) != power - 1)
                active_drive_ok = false;
        };
        expect(backend.execute(plan).ok && active_drive_ok, "tone programming preserves chosen drive");
        expect(adapter->registers[3] == 255 && adapter->address_attempts[177] == 2,
            "optimized backend retains cleanup and first-only reset");
        if (optimized) expect(adapter->transactions.size() < 30, "parameter batches reduce transaction count");
        adapter->after_write = {};
    }
    for (bool cancel : {false, true})
    {
        TestBridge bridge; auto adapter = std::make_shared<FakeI2CAdapter>();
        auto cfg = config(adapter); cfg.device.optimize_register_writes = true; cfg.pll_only_updates = true;
        if (cancel) adapter->after_write = [&bridge](std::uint8_t r,std::uint8_t,std::size_t n) {
            if (r == 28 && n == 3) bridge.backendSignalStopRequest();
        };
        else {adapter->fail_address = 28; adapter->fail_address_occurrence = 3;}
        WsprSi5351Backend backend(bridge,cfg); auto plan=four_tone_plan(4);
        expect(configure(backend,plan), "optimized failure configure"); auto result=backend.execute(plan);
        expect(cancel ? result.stopped : !result.ok, "mid-burst cancellation/failure reported");
        expect(adapter->registers[3] == 255 && count_write(*adapter,3,0xfe) == 1,
            "mid-burst cancellation/failure disables without re-enable");
    }
}

int main()
{
    test_burst_and_cache_failure_contract();
    test_optimized_backend_and_drive();
    test_pll_only_and_readiness();
    test_committed_calibration_snapshot_and_reporting();
    test_invalid_calibration_fails_before_output_enable();
    test_single_tone_calibration_reporting_and_cleanup();
    test_single_tone_interruption_cleans_up();
    test_162_symbol_transition_order();
    test_controller_consumes_completed_cleanup_without_reopening();
    test_controller_preserves_genuine_idle_cleanup_failure();
    test_controller_consumes_interrupted_cleanup();
    test_transition_failure_stays_inhibited();
    test_transition_interruptions_stay_inhibited();
    test_symbol_wait_interruption_cleans_up();
    test_final_wait_interruption_cleans_up();
    test_162_symbol_dry_run_avoids_i2c();

    if (failures != 0)
    {
        std::cerr << failures << " Si5351 transition test(s) failed.\n";
        return 1;
    }
    std::cout << "Si5351 transition tests passed.\n";
    return 0;
}
