#include "rp1_gpclk_linux_provider.hpp"
#include "rp1_gpclk_uapi.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace wsprrypi
{
int Rp1GpclkPosixIo::openDevice(const char* path) noexcept
{ return ::open(path, O_RDWR | O_CLOEXEC); }
int Rp1GpclkPosixIo::control(int fd, unsigned long request, void* argument) noexcept
{ return ::ioctl(fd, request, argument); }
int Rp1GpclkPosixIo::closeDevice(int fd) noexcept { return ::close(fd); }
int Rp1GpclkPosixIo::lastError() const noexcept { return errno; }

Rp1GpclkLinuxProvider::Rp1GpclkLinuxProvider(
    Rp1GpclkIo& io, std::string device) noexcept
    : io_(io), device_(std::move(device)) {}

Rp1GpclkLinuxProvider::~Rp1GpclkLinuxProvider() { release(); }

bool Rp1GpclkLinuxProvider::failed(const char* operation, std::string& error) const
{
    error = std::string(operation) + ": " + std::strerror(io_.lastError());
    return false;
}

bool Rp1GpclkLinuxProvider::acquire(std::uint32_t drive_ma, std::string& error)
{
    if (fd_ >= 0) { error = "RP1 GPCLK provider is already acquired."; return false; }
    fd_ = io_.openDevice(device_.c_str());
    if (fd_ < 0) return failed("Could not open RP1 GPCLK provider", error);
    rp1_gpclk_acquire request{};
    request.version = RP1_GPCLK_UAPI_VERSION;
    request.size = sizeof(request);
    request.drive_ma = drive_ma;
    if (io_.control(fd_, RP1_GPCLK_IOC_ACQUIRE, &request) < 0) {
        failed("Could not acquire RP1 GPCLK provider", error);
        io_.closeDevice(fd_); fd_ = -1; return false;
    }
    return true;
}

bool Rp1GpclkLinuxProvider::submit(
    const Rp1GpclkProviderProgram& source, std::string& error)
{
    if (fd_ < 0) { error = "RP1 GPCLK provider is not acquired."; return false; }
    rp1_gpclk_program request{};
    request.version = RP1_GPCLK_UAPI_VERSION; request.size = sizeof(request);
    request.fractional_bits = source.fractional_bits;
    request.writes_per_symbol = source.writes_per_symbol;
    request.tick_divider = source.tick_divider;
    request.symbol_count = RP1_GPCLK_WSPR_SYMBOL_COUNT;
    request.tone_count = source.tones.size();
    request.generation = source.generation;
    for (std::size_t i = 0; i < source.tones.size(); ++i)
    {
        request.tones[i].lower_divider_word =
            source.tones[i].lower_divider_word;
        request.tones[i].upper_divider_word =
            source.tones[i].upper_divider_word;
        request.tones[i].lower_count = source.tones[i].lower_count;
        request.tones[i].upper_count = source.tones[i].upper_count;
    }
    for (std::size_t i = 0; i < source.symbols.size(); ++i)
        request.symbols[i] = source.symbols[i];
    if (io_.control(fd_, RP1_GPCLK_IOC_SUBMIT, &request) < 0)
        return failed("Could not submit RP1 GPCLK program", error);
    return true;
}

bool Rp1GpclkLinuxProvider::submitEvents(
    const Rp1GpclkProviderEventProgram& source, std::string& error)
{
    if (fd_ < 0) { error = "RP1 GPCLK provider is not acquired."; return false; }
    if (source.tones.size() > RP1_GPCLK_EVENT_MAX_TONES ||
        source.events.size() > RP1_GPCLK_EVENT_MAX_EVENTS)
    {
        error = "RP1 GPCLK event program exceeds the wire bounds.";
        return false;
    }
    rp1_gpclk_event_program request{};
    request.version = RP1_GPCLK_EVENT_UAPI_VERSION;
    request.size = sizeof(request);
    request.fractional_bits = source.fractional_bits;
    request.tick_divider = source.tick_divider;
    request.tone_count = source.tones.size();
    request.event_count = source.events.size();
    request.generation = source.generation;
    request.total_duration_ns = source.total_duration_ns;
    for (std::size_t i = 0; i < source.tones.size(); ++i)
    {
        request.tones[i].lower_divider_word = source.tones[i].lower_divider_word;
        request.tones[i].upper_divider_word = source.tones[i].upper_divider_word;
        request.tones[i].lower_count = source.tones[i].lower_count;
        request.tones[i].upper_count = source.tones[i].upper_count;
    }
    for (std::size_t i = 0; i < source.events.size(); ++i)
    {
        request.events[i].duration_ns = source.events[i].duration_ns;
        request.events[i].tone_index = source.events[i].tone_index;
        request.events[i].flags = source.events[i].rf_on ? RP1_GPCLK_EVENT_RF_ON : 0;
    }
    if (io_.control(fd_, RP1_GPCLK_IOC_SUBMIT_EVENTS, &request) < 0)
        return failed("Could not submit RP1 GPCLK event program", error);
    return true;
}

bool Rp1GpclkLinuxProvider::requestFiniteStop(
    std::uint64_t generation, std::string& error)
{
    rp1_gpclk_generation request{};
    request.version = RP1_GPCLK_UAPI_VERSION; request.size = sizeof(request);
    request.generation = generation;
    if (fd_ < 0 || io_.control(fd_, RP1_GPCLK_IOC_STOP, &request) < 0)
        return failed("Could not request RP1 GPCLK finite stop", error);
    return true;
}

Rp1GpclkCompletionState Rp1GpclkLinuxProvider::state(
    std::uint64_t generation) const noexcept
{
    rp1_gpclk_generation request{};
    request.version = RP1_GPCLK_UAPI_VERSION; request.size = sizeof(request);
    request.generation = generation;
    if (fd_ < 0 || io_.control(fd_, RP1_GPCLK_IOC_STATE, &request) < 0)
        return Rp1GpclkCompletionState::failed;
    switch (request.state) {
    case RP1_GPCLK_STATE_IDLE: return Rp1GpclkCompletionState::idle;
    case RP1_GPCLK_STATE_RUNNING: return Rp1GpclkCompletionState::running;
    case RP1_GPCLK_STATE_DRAINING: return Rp1GpclkCompletionState::draining;
    case RP1_GPCLK_STATE_COMPLETE: return Rp1GpclkCompletionState::complete;
    default: return Rp1GpclkCompletionState::failed;
    }
}

Rp1GpclkProviderEventState Rp1GpclkLinuxProvider::eventState(
    std::uint64_t generation) const noexcept
{
    rp1_gpclk_event_state request{};
    request.version = RP1_GPCLK_EVENT_UAPI_VERSION;
    request.size = sizeof(request);
    request.generation = generation;
    if (fd_ < 0 || io_.control(fd_, RP1_GPCLK_IOC_EVENT_STATE, &request) < 0)
        return {Rp1GpclkCompletionState::failed, 0, 0};
    Rp1GpclkCompletionState completion = Rp1GpclkCompletionState::failed;
    switch (request.state) {
    case RP1_GPCLK_STATE_IDLE: completion = Rp1GpclkCompletionState::idle; break;
    case RP1_GPCLK_STATE_RUNNING: completion = Rp1GpclkCompletionState::running; break;
    case RP1_GPCLK_STATE_DRAINING: completion = Rp1GpclkCompletionState::draining; break;
    case RP1_GPCLK_STATE_COMPLETE: completion = Rp1GpclkCompletionState::complete; break;
    default: break;
    }
    return {completion, request.current_event, request.terminal_reason};
}

void Rp1GpclkLinuxProvider::release() noexcept
{
    if (fd_ < 0) return;
    io_.control(fd_, RP1_GPCLK_IOC_RELEASE, nullptr);
    io_.closeDevice(fd_); fd_ = -1;
}
} // namespace wsprrypi
