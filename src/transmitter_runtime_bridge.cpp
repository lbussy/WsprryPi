/**
 * @file transmitter_runtime_bridge.cpp
 * @brief Adapts scheduler operations to the concrete transmitter runtime.
 */

#include "transmitter_runtime_bridge.hpp"

#include "wspr_transmit.hpp"

#include <utility>

WsprTransmitState transmitter_state() noexcept
{
    return wsprTransmitter.getState();
}

std::string transmitter_state_string_lower(WsprTransmitState state)
{
    return wsprTransmitter.stateToStringLower(state);
}

bool transmitter_active_execution_is_tone() noexcept
{
    return wsprTransmitter.activeExecutionIsTone();
}

TransmitterRuntimeStatus transmitter_runtime_status()
{
    const auto status = wsprTransmitter.runtimeExecutionStatusSnapshot();
    return {status.mode, status.cw_message, status.cw_active_char_index};
}

std::string transmitter_reload_defer_debug_state()
{
    return wsprTransmitter.reloadDeferDebugState();
}

void transmitter_clear_execution_state_after_stop() noexcept
{
    wsprTransmitter.clearExecutionStateAfterStop();
}

void transmitter_configure_execution(const TransmissionRequest &request)
{
    wsprTransmitter.configureExecution(request);
}

void transmitter_configure_execution(
    const wsprrypi::TransmissionRequest &request,
    const TransmissionRequest &legacy_request)
{
    wsprTransmitter.configureExecution(request, legacy_request);
}

wsprrypi::StartupQuiesceResult transmitter_quiesce_for_startup()
{
    return wsprTransmitter.quiesceForStartup();
}

void transmitter_set_thread_scheduling(int policy, int priority)
{
    wsprTransmitter.setThreadScheduling(policy, priority);
}

void transmitter_set_callbacks(TransmitterRuntimeCallback callback)
{
    wsprTransmitter.setTransmissionCallbacks(std::move(callback));
}

std::string transmitter_format_frequency_mhz(double frequency_hz)
{
    return WsprTransmitter::formatFrequencyMHz(frequency_hz);
}

void transmitter_start_async()
{
    wsprTransmitter.startAsync();
}

void transmitter_stop_and_join()
{
    wsprTransmitter.stopAndJoin();
}

void transmitter_shutdown_for_process_exit()
{
    wsprTransmitter.shutdownForProcessExit();
}

void transmitter_clear_soft_off() noexcept
{
    wsprTransmitter.clearSoftOff();
}
