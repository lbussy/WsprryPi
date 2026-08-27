#include "rp1_gpclk_linux_provider.hpp"
#include "rp1_gpclk_development_policy.hpp"
#include "rp1_gpclk_uapi.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace wsprrypi
{
namespace
{
constexpr std::uint64_t kKnownCapabilities =
    RP1_GPCLK_CAP_SUBMIT_WSPR | RP1_GPCLK_CAP_SUBMIT_EVENTS |
    RP1_GPCLK_CAP_STOP_DRAIN | RP1_GPCLK_CAP_STABLE_STATE |
    RP1_GPCLK_CAP_ROUTE_IDENTITY | RP1_GPCLK_CAP_COMPAT_IDENTITY |
    RP1_GPCLK_CAP_CLEANUP_FAULT_LATCH | RP1_GPCLK_CAP_LIVE_ELIGIBLE |
    RP1_GPCLK_CAP_TONE_CONTINUOUS | RP1_GPCLK_CAP_TONE_FINITE |
    RP1_GPCLK_CAP_PASSIVE_SNAPSHOT;

std::uint32_t driveMask(std::uint32_t drive_ma)
{
    switch (drive_ma)
    {
    case RP1_GPCLK_DRIVE_MA_2: return RP1_GPCLK_DRIVE_SUPPORT_2_MA;
    case RP1_GPCLK_DRIVE_MA_4: return RP1_GPCLK_DRIVE_SUPPORT_4_MA;
    case RP1_GPCLK_DRIVE_MA_8: return RP1_GPCLK_DRIVE_SUPPORT_8_MA;
    case RP1_GPCLK_DRIVE_MA_12: return RP1_GPCLK_DRIVE_SUPPORT_12_MA;
    default: return 0;
    }
}

template <typename Request> void initializeHeaderV1(Request& request)
{
    request.header.size = sizeof(Request);
    request.header.version = RP1_GPCLK_UAPI_ABI_V1;
}

template <typename Request> void initializeHeaderV2(Request& request)
{
    request.header.size = sizeof(Request);
    request.header.version = RP1_GPCLK_UAPI_ABI_V2;
}

bool boundedIdentity(const char* value, std::size_t size, std::string& output)
{
    const void* terminator = std::memchr(value, '\0', size);
    if (terminator == nullptr) return false;
    output.assign(value, static_cast<const char*>(terminator));
    return !output.empty();
}

bool knownCompatibility(std::uint32_t value)
{ return value >= RP1_GPCLK_COMPAT_QUALIFIED && value <= RP1_GPCLK_COMPAT_REJECTED; }
bool knownReason(std::uint32_t value)
{ return value <= RP1_GPCLK_COMPAT_REASON_ADMIN_ENROLLMENT_REQUIRED; }
bool knownRoute(std::uint32_t value)
{ return value == RP1_GPCLK_ROUTE_GPIO4 || value == RP1_GPCLK_ROUTE_GPIO20; }
bool knownState(std::uint32_t value) { return value <= RP1_GPCLK_STATE_DEAD; }
bool knownTerminalReason(std::uint32_t value)
{ return value <= RP1_GPCLK_REASON_INTERNAL_ERROR; }
bool knownObservation(std::uint32_t value)
{ return value <= RP1_GPCLK_OBSERVATION_TRUE; }
bool knownDrainState(std::uint32_t value)
{ return value <= RP1_GPCLK_DRAIN_COMPLETE; }
bool validStateReason(std::uint32_t state, std::uint32_t reason)
{
    if (state == RP1_GPCLK_STATE_IDLE || state == RP1_GPCLK_STATE_RUNNING ||
        state == RP1_GPCLK_STATE_DRAINING)
        return reason == RP1_GPCLK_REASON_NONE;
    if (state == RP1_GPCLK_STATE_COMPLETE)
        return reason == RP1_GPCLK_REASON_COMPLETE || reason == RP1_GPCLK_REASON_STOPPED;
    return reason != RP1_GPCLK_REASON_NONE && reason != RP1_GPCLK_REASON_COMPLETE;
}
}

int Rp1GpclkPosixIo::openDevice(const char* path, int flags) noexcept
{ return ::open(path, flags | O_CLOEXEC); }
int Rp1GpclkPosixIo::control(int fd, unsigned long request, void* argument) noexcept
{ return ::ioctl(fd, request, argument); }
int Rp1GpclkPosixIo::closeDevice(int fd) noexcept { return ::close(fd); }
int Rp1GpclkPosixIo::lastError() const noexcept { return errno; }

Rp1GpclkLinuxProvider::Rp1GpclkLinuxProvider(
    Rp1GpclkIo& io, std::string device) noexcept
    : io_(io), device_(std::move(device)) {}
Rp1GpclkLinuxProvider::~Rp1GpclkLinuxProvider()
{
    std::string ignored;
    (void)release(ignored);
}

bool Rp1GpclkLinuxProvider::failed(const char* operation, std::string& error) const
{
    error = std::string(operation) + ": " + std::strerror(io_.lastError());
    return false;
}

bool Rp1GpclkLinuxProvider::queryOpen(
    std::uint32_t expected_route, std::uint64_t required_capabilities,
    bool require_live_eligible, Rp1GpclkProviderIdentity& identity,
    std::string& error)
{
    rp1_gpclk_query_v2 request{};
    initializeHeaderV2(request);
    if (io_.control(fd_, RP1_GPCLK_IOC_QUERY_V2, &request) < 0)
    {
        if (io_.lastError() == EOPNOTSUPP)
            error = "RP1 GPCLK provider is an old module without ABI v2 support.";
        else
            failed("Could not query RP1 GPCLK ABI v2 provider", error);
        return false;
    }
    if (request.header.size != sizeof(request) ||
        request.header.version != RP1_GPCLK_UAPI_ABI_V2 ||
        request.header.flags != 0 || request.abi_min > RP1_GPCLK_UAPI_ABI_V2 ||
        request.abi_max < RP1_GPCLK_UAPI_ABI_V2)
    {
        error = "RP1 GPCLK provider ABI v2 identity mismatch."; return false;
    }
    if (!knownRoute(request.route) ||
        (expected_route != RP1_GPCLK_ROUTE_INVALID && request.route != expected_route))
    {
        error = "RP1 GPCLK provider reported an unexpected administrative route."; return false;
    }
    if (!knownCompatibility(request.compatibility_state) ||
        !knownReason(request.compatibility_reason))
    {
        error = "RP1 GPCLK provider reported an unknown compatibility state or reason."; return false;
    }
    if ((request.capabilities & ~kKnownCapabilities) != 0 ||
        (request.capabilities & required_capabilities) != required_capabilities)
    {
        error = "RP1 GPCLK provider is missing required capabilities or reported unknown capabilities."; return false;
    }
    if (require_live_eligible &&
        ((request.capabilities & RP1_GPCLK_CAP_LIVE_ELIGIBLE) == 0 ||
         request.compatibility_state != RP1_GPCLK_COMPAT_EXPERIMENTAL))
    {
        error = "RP1 GPCLK provider lacks exact Experimental LIVE_ELIGIBLE status."; return false;
    }
    if (request.compatibility_state == RP1_GPCLK_COMPAT_UNAVAILABLE ||
        request.compatibility_state == RP1_GPCLK_COMPAT_REJECTED)
    {
        error = "RP1 GPCLK provider compatibility is unavailable or rejected."; return false;
    }
    if (request.max_tones != RP1_GPCLK_MAX_TONES ||
        request.wspr_symbols != RP1_GPCLK_WSPR_SYMBOLS ||
        request.max_events != RP1_GPCLK_MAX_EVENTS ||
        request.max_dither_period != RP1_GPCLK_DITHER_PERIOD_MAX ||
        (request.supported_drive_ma_mask & ~RP1_GPCLK_DRIVE_SUPPORT_ALLOWED_MASK) != 0 ||
        request.max_event_duration_ns != RP1_GPCLK_EVENT_DURATION_NS_MAX ||
        request.max_request_duration_ns != RP1_GPCLK_REQUEST_DURATION_NS_MAX ||
        request.min_tone_duration_ns != RP1_GPCLK_TONE_DURATION_NS_MIN ||
        request.max_tone_duration_ns != RP1_GPCLK_TONE_DURATION_NS_MAX ||
        request.reserved0 != 0 || request.reserved1 != 0)
    {
        error = "RP1 GPCLK provider reported an incompatible ABI v2 limit or reserved value."; return false;
    }
    for (const auto value : request.reserved)
        if (value != 0) { error = "RP1 GPCLK provider reported nonzero reserved QUERY data."; return false; }

    identity = {};
    identity.abi_min = request.abi_min;
    identity.abi_max = request.abi_max;
    identity.route = request.route;
    identity.compatibility_state = request.compatibility_state;
    identity.compatibility_reason = request.compatibility_reason;
    identity.capabilities = request.capabilities;
    identity.supported_drive_ma_mask = request.supported_drive_ma_mask;
    identity.min_tone_duration_ns = request.min_tone_duration_ns;
    identity.max_tone_duration_ns = request.max_tone_duration_ns;
    if (!boundedIdentity(request.module_id, sizeof(request.module_id), identity.module_id) ||
        !boundedIdentity(request.build_id, sizeof(request.build_id), identity.build_id) ||
        !boundedIdentity(request.compatibility_id, sizeof(request.compatibility_id), identity.compatibility_id))
    {
        error = "RP1 GPCLK provider returned an empty or unterminated identity."; return false;
    }
    const auto expected = rp1GpclkExpectedDevelopmentIdentity(identity.route);
    if (!expected || identity.module_id != expected->module_id ||
        identity.build_id != expected->build_id ||
        identity.compatibility_id != expected->compatibility_id)
    {
        error = "RP1 GPCLK provider does not match the exact route-specific 1.1.2 r3 development identity."; return false;
    }
    return true;
}

bool Rp1GpclkLinuxProvider::query(
    std::uint32_t expected_route, std::uint64_t required_capabilities,
    bool require_live_eligible, Rp1GpclkProviderIdentity& identity,
    std::string& error)
{
    if (fd_ >= 0) { error = "RP1 GPCLK provider query requires no active lease."; return false; }
    fd_ = io_.openDevice(device_.c_str(), O_RDWR);
    if (fd_ < 0) return failed("Could not open canonical RP1 GPCLK provider", error);
    const auto v2_required = required_capabilities & ~RP1_GPCLK_CAP_PASSIVE_SNAPSHOT;
    const bool ok = queryOpen(expected_route, v2_required,
        require_live_eligible, identity, error);
    const int close_result = io_.closeDevice(fd_);
    fd_ = -1;
    if (close_result < 0) { error = "Could not close RP1 GPCLK provider after QUERY."; return false; }
    if (!ok) return false;
    Rp1GpclkPassiveSnapshot snapshot;
    if (!passiveSnapshot(snapshot, error)) return false;
    const bool safe_idle_eligibility_projection =
        !require_live_eligible &&
        identity.compatibility_state == RP1_GPCLK_COMPAT_COMPATIBLE_UNQUALIFIED &&
        (identity.capabilities & RP1_GPCLK_CAP_LIVE_ELIGIBLE) == 0 &&
        snapshot.compatibility_state == RP1_GPCLK_COMPAT_EXPERIMENTAL &&
        snapshot.live_output == RP1_GPCLK_OBSERVATION_FALSE &&
        snapshot.live_eligible == RP1_GPCLK_OBSERVATION_TRUE;
    if (snapshot.route != identity.route ||
        (snapshot.compatibility_state != identity.compatibility_state &&
            !safe_idle_eligibility_projection) ||
        snapshot.compatibility_reason != identity.compatibility_reason ||
        snapshot.module_id != identity.module_id || snapshot.build_id != identity.build_id ||
        snapshot.compatibility_id != identity.compatibility_id)
    {
        error = "RP1 GPCLK QUERY and passive snapshot identities disagree: query=" +
            std::to_string(identity.route) + "/" +
            std::to_string(identity.compatibility_state) + "/" +
            std::to_string(identity.compatibility_reason) + "/" + identity.module_id +
            "/" + identity.build_id + "/" + identity.compatibility_id +
            "; snapshot=" + std::to_string(snapshot.route) + "/" +
            std::to_string(snapshot.compatibility_state) + "/" +
            std::to_string(snapshot.compatibility_reason) + "/" + snapshot.module_id +
            "/" + snapshot.build_id + "/" + snapshot.compatibility_id;
        return false;
    }
    identity.abi_min = snapshot.abi_min;
    identity.abi_max = snapshot.abi_max;
    identity.compatibility_state = snapshot.compatibility_state;
    identity.capabilities |= snapshot.capabilities;
    if ((identity.capabilities & required_capabilities) != required_capabilities)
    {
        error = "RP1 GPCLK provider is missing a required passive capability.";
        return false;
    }
    return true;
}

bool Rp1GpclkLinuxProvider::passiveSnapshot(
    Rp1GpclkPassiveSnapshot& result, std::string& error) const
{
    if (fd_ >= 0)
    {
        error = "Passive RP1 GPCLK inspection requires no owned provider descriptor.";
        return false;
    }
    const int descriptor = io_.openDevice(device_.c_str(), O_RDONLY | O_NONBLOCK);
    if (descriptor < 0) return failed("Could not open canonical RP1 GPCLK provider read-only", error);
    rp1_gpclk_snapshot_v3 request{};
    request.header.size = sizeof(request);
    request.header.version = RP1_GPCLK_UAPI_ABI_V3;
    const int control_result = io_.control(
        descriptor, RP1_GPCLK_IOC_GET_SNAPSHOT_V3, &request);
    const int saved_error = io_.lastError();
    const int close_result = io_.closeDevice(descriptor);
    if (control_result < 0)
    {
        error = saved_error == EOPNOTSUPP || saved_error == ENOTTY
            ? "RP1 GPCLK provider does not support the passive ABI v3 snapshot."
            : "Could not read passive RP1 GPCLK ABI v3 snapshot: " +
                std::string(std::strerror(saved_error));
        return false;
    }
    if (close_result < 0)
    {
        error = "Could not close RP1 GPCLK passive inspector descriptor.";
        return false;
    }
    if (request.header.size != sizeof(request) ||
        request.header.version != RP1_GPCLK_UAPI_ABI_V3 ||
        request.header.flags != 0 || request.abi_min > RP1_GPCLK_UAPI_ABI_V3 ||
        request.abi_max < RP1_GPCLK_UAPI_ABI_V3 ||
        !knownRoute(request.route) ||
        !knownCompatibility(request.compatibility_state) ||
        !knownReason(request.compatibility_reason) ||
        !knownState(request.operation_state) ||
        !knownTerminalReason(request.terminal_reason) ||
        !validStateReason(request.operation_state, request.terminal_reason) ||
        !knownDrainState(request.drain_state) ||
        request.snapshot_flags & ~RP1_GPCLK_SNAPSHOT_F_ALLOWED_MASK ||
        request.capabilities & ~kKnownCapabilities ||
        (request.capabilities & RP1_GPCLK_CAP_PASSIVE_SNAPSHOT) == 0 ||
        request.reserved0 != 0)
    {
        error = "RP1 GPCLK provider returned malformed or unknown passive snapshot data.";
        return false;
    }
    for (const auto value : request.reserved)
        if (value != 0)
        {
            error = "RP1 GPCLK provider returned nonzero passive snapshot reserved data.";
            return false;
        }
    for (const auto value : {request.cleanup_fault, request.owner_present,
            request.lease_present, request.live_output, request.live_eligible,
            request.gpio_safe, request.clock_quiescent, request.dma_quiescent,
            request.stable})
        if (!knownObservation(value))
        {
            error = "RP1 GPCLK provider returned an unknown passive safety observation.";
            return false;
        }
    if ((request.snapshot_flags & RP1_GPCLK_SNAPSHOT_F_REMAINING_VALID) &&
        request.remaining_ns > RP1_GPCLK_REQUEST_DURATION_NS_MAX)
    {
        error = "RP1 GPCLK provider returned an out-of-bounds remaining duration.";
        return false;
    }
    result = {};
    result.snapshot_version = request.header.version;
    result.abi_min = request.abi_min; result.abi_max = request.abi_max;
    result.route = request.route;
    result.compatibility_state = request.compatibility_state;
    result.compatibility_reason = request.compatibility_reason;
    result.operation_state = request.operation_state;
    result.terminal_reason = request.terminal_reason;
    result.current_event = request.current_event;
    result.validity_flags = request.snapshot_flags;
    result.cleanup_fault = request.cleanup_fault;
    result.owner_present = request.owner_present;
    result.lease_present = request.lease_present;
    result.live_output = request.live_output;
    result.live_eligible = request.live_eligible;
    result.drain_state = request.drain_state;
    result.gpio_safe = request.gpio_safe;
    result.clock_quiescent = request.clock_quiescent;
    result.dma_quiescent = request.dma_quiescent;
    result.stable = request.stable;
    result.capabilities = request.capabilities;
    result.generation = request.generation;
    result.elapsed_ns = request.elapsed_ns;
    result.remaining_ns = request.remaining_ns;
    result.min_tone_duration_ns = request.min_tone_duration_ns;
    result.max_tone_duration_ns = request.max_tone_duration_ns;
    if (!boundedIdentity(request.module_id, sizeof(request.module_id), result.module_id) ||
        !boundedIdentity(request.build_id, sizeof(request.build_id), result.build_id) ||
        !boundedIdentity(request.compatibility_id, sizeof(request.compatibility_id),
            result.compatibility_id))
    {
        error = "RP1 GPCLK provider returned an empty or unterminated passive identity.";
        return false;
    }
    return true;
}

bool Rp1GpclkLinuxProvider::acquire(
    std::uint32_t expected_route, std::uint64_t required_capabilities,
    std::string& error)
{
    if (fd_ >= 0) { error = "RP1 GPCLK provider is already acquired."; return false; }
    fd_ = io_.openDevice(device_.c_str(), O_RDWR);
    if (fd_ < 0) return failed("Could not open canonical RP1 GPCLK provider", error);
    Rp1GpclkProviderIdentity identity;
    const bool require_live =
        (required_capabilities & RP1_GPCLK_CAP_LIVE_ELIGIBLE) != 0;
    if (!queryOpen(expected_route, required_capabilities, require_live, identity, error))
    {
        (void)io_.closeDevice(fd_); fd_ = -1; return false;
    }
    rp1_gpclk_acquire_v1 request{};
    initializeHeaderV1(request);
    request.expected_route = expected_route;
    request.required_capabilities = required_capabilities;
    if (io_.control(fd_, RP1_GPCLK_IOC_ACQUIRE, &request) < 0)
    {
        failed("Could not acquire RP1 GPCLK provider", error);
        (void)io_.closeDevice(fd_); fd_ = -1; return false;
    }
    if (request.header.size != sizeof(request) ||
        request.header.version != RP1_GPCLK_UAPI_ABI_V1 ||
        request.header.flags != 0 || request.lease_id == 0)
    {
        error = "RP1 GPCLK provider returned an invalid lease identity.";
        (void)io_.closeDevice(fd_); fd_ = -1; return false;
    }
    lease_id_ = request.lease_id;
    supported_drive_ma_mask_ = identity.supported_drive_ma_mask;
    return true;
}

bool Rp1GpclkLinuxProvider::submit(
    Rp1GpclkProviderProgram& source, std::string& error)
{
    if (fd_ < 0 || lease_id_ == 0) { error = "RP1 GPCLK provider is not acquired."; return false; }
    const auto required_drive = driveMask(source.drive_ma);
    if (required_drive == 0 || (supported_drive_ma_mask_ & required_drive) == 0)
    { error = "RP1 GPCLK provider does not support the requested drive strength."; return false; }
    rp1_gpclk_submit_wspr_v1 request{};
    initializeHeaderV1(request);
    request.lease_id = lease_id_; request.generation = 0;
    request.tones_ptr = reinterpret_cast<std::uintptr_t>(source.tones.data());
    request.symbols_ptr = reinterpret_cast<std::uintptr_t>(source.symbols.data());
    request.fractional_bits = source.fractional_bits;
    request.tick_divider = source.tick_divider;
    request.writes_per_symbol = source.writes_per_symbol;
    request.tone_count = source.tones.size(); request.symbol_count = source.symbols.size();
    request.drive_ma = source.drive_ma;
    request.expected_frame_duration_ns = 110591999892ULL;
    if (io_.control(fd_, RP1_GPCLK_IOC_SUBMIT_WSPR, &request) < 0)
        return failed("Could not submit RP1 GPCLK WSPR program", error);
    if (request.generation == 0)
    { error = "RP1 GPCLK provider returned an invalid WSPR generation identity."; return false; }
    source.generation = request.generation;
    active_generation_ = request.generation;
    active_generation_terminal_ = false;
    return true;
}

bool Rp1GpclkLinuxProvider::submitEvents(
    Rp1GpclkProviderEventProgram& source, std::string& error)
{
    if (fd_ < 0 || lease_id_ == 0) { error = "RP1 GPCLK provider is not acquired."; return false; }
    const auto required_drive = driveMask(source.drive_ma);
    if (required_drive == 0 || (supported_drive_ma_mask_ & required_drive) == 0)
    { error = "RP1 GPCLK provider does not support the requested drive strength."; return false; }
    if (source.tones.size() > RP1_GPCLK_MAX_TONES || source.events.size() > RP1_GPCLK_MAX_EVENTS)
    { error = "RP1 GPCLK event program exceeds the ABI v1 bounds."; return false; }
    std::vector<rp1_gpclk_tone_v1> tones(source.tones.size());
    for (std::size_t i = 0; i < source.tones.size(); ++i)
        tones[i] = {source.tones[i].lower_divider_word, source.tones[i].upper_divider_word,
            source.tones[i].lower_count, source.tones[i].upper_count};
    std::vector<rp1_gpclk_event_v1> events(source.events.size());
    for (std::size_t i = 0; i < source.events.size(); ++i)
    {
        events[i].duration_ns = source.events[i].duration_ns;
        events[i].tone_index = source.events[i].tone_index;
        events[i].flags = source.events[i].rf_on ? RP1_GPCLK_EVENT_F_OUTPUT_ENABLED : 0;
    }
    rp1_gpclk_submit_events_v1 request{};
    initializeHeaderV1(request);
    request.lease_id = lease_id_; request.generation = 0;
    request.tones_ptr = reinterpret_cast<std::uintptr_t>(tones.data());
    request.events_ptr = reinterpret_cast<std::uintptr_t>(events.data());
    request.mode = source.mode; request.fractional_bits = source.fractional_bits;
    request.tick_divider = source.tick_divider; request.tone_count = tones.size();
    request.event_count = events.size(); request.drive_ma = source.drive_ma;
    request.total_duration_ns = source.total_duration_ns;
    if (io_.control(fd_, RP1_GPCLK_IOC_SUBMIT_EVENTS, &request) < 0)
        return failed("Could not submit RP1 GPCLK event program", error);
    if (request.generation == 0)
    { error = "RP1 GPCLK provider returned an invalid event generation identity."; return false; }
    source.generation = request.generation;
    active_generation_ = request.generation;
    active_generation_terminal_ = false;
    return true;
}

bool Rp1GpclkLinuxProvider::submitTone(
    Rp1GpclkProviderToneProgram& source, std::string& error)
{
    if (fd_ < 0 || lease_id_ == 0)
    { error = "RP1 GPCLK provider is not acquired."; return false; }
    const auto required_drive = driveMask(source.drive_ma);
    if (required_drive == 0 || (supported_drive_ma_mask_ & required_drive) == 0)
    { error = "RP1 GPCLK provider does not support the requested drive strength."; return false; }
    const bool continuous = source.operation == RP1_GPCLK_TONE_OPERATION_CONTINUOUS;
    const bool finite = source.operation == RP1_GPCLK_TONE_OPERATION_FINITE;
    if ((!continuous && !finite) || (continuous && source.duration_ns != 0) ||
        (finite && (source.duration_ns < RP1_GPCLK_TONE_DURATION_NS_MIN ||
                    source.duration_ns > RP1_GPCLK_TONE_DURATION_NS_MAX)))
    { error = "RP1 GPCLK TONE operation or duration is invalid."; return false; }
    rp1_gpclk_submit_tone_v2 request{};
    initializeHeaderV2(request);
    request.lease_id = lease_id_;
    request.generation = 0;
    request.tone = {source.tone.lower_divider_word, source.tone.upper_divider_word,
        source.tone.lower_count, source.tone.upper_count};
    request.duration_ns = source.duration_ns;
    request.operation = source.operation;
    request.expected_route = source.expected_route;
    request.fractional_bits = source.fractional_bits;
    request.tick_divider = source.tick_divider;
    request.drive_ma = source.drive_ma;
    if (io_.control(fd_, RP1_GPCLK_IOC_SUBMIT_TONE_V2, &request) < 0)
        return failed("Could not submit RP1 GPCLK ABI v2 TONE", error);
    if (request.generation == 0)
    { error = "RP1 GPCLK provider returned an invalid TONE generation identity."; return false; }
    source.generation = request.generation;
    active_generation_ = request.generation;
    active_generation_terminal_ = false;
    return true;
}

bool Rp1GpclkLinuxProvider::requestFiniteStop(std::uint64_t generation, std::string& error)
{
    rp1_gpclk_stop_v1 request{}; initializeHeaderV1(request);
    request.lease_id = lease_id_; request.generation = generation;
    if (fd_ < 0 || lease_id_ == 0 || io_.control(fd_, RP1_GPCLK_IOC_STOP, &request) < 0)
        return failed("Could not request RP1 GPCLK finite stop", error);
    return true;
}

bool Rp1GpclkLinuxProvider::getState(
    std::uint64_t generation,
    Rp1GpclkProviderEventState& state_result,
    std::string& error) const
{
    rp1_gpclk_state_v1 request{}; initializeHeaderV1(request);
    request.lease_id = lease_id_; request.generation = generation;
    if (fd_ < 0 || lease_id_ == 0)
    {
        error = "RP1 GPCLK state query requires an owned lease.";
        return false;
    }
    if (io_.control(fd_, RP1_GPCLK_IOC_GET_STATE, &request) < 0)
        return failed("Could not get RP1 GPCLK generation state", error);
    if (request.lease_id != lease_id_ || request.generation != generation ||
        !knownState(request.state) || !knownTerminalReason(request.terminal_reason) ||
        !validStateReason(request.state, request.terminal_reason) ||
        request.cleanup_fault > 1)
    {
        error = "RP1 GPCLK provider returned stale or unknown state data.";
        return false;
    }
    Rp1GpclkCompletionState completion = Rp1GpclkCompletionState::failed;
    switch (request.state)
    {
    case RP1_GPCLK_STATE_IDLE: completion = Rp1GpclkCompletionState::idle; break;
    case RP1_GPCLK_STATE_RUNNING: completion = Rp1GpclkCompletionState::running; break;
    case RP1_GPCLK_STATE_DRAINING: completion = Rp1GpclkCompletionState::draining; break;
    case RP1_GPCLK_STATE_COMPLETE: completion = Rp1GpclkCompletionState::complete; break;
    case RP1_GPCLK_STATE_DEAD: completion = Rp1GpclkCompletionState::dead; break;
    default: break;
    }
    if (request.cleanup_fault != 0) completion = Rp1GpclkCompletionState::failed;
    state_result = {completion, request.current_event, request.terminal_reason};
    active_generation_terminal_ =
        completion == Rp1GpclkCompletionState::complete ||
        completion == Rp1GpclkCompletionState::failed ||
        completion == Rp1GpclkCompletionState::dead;
    return true;
}

Rp1GpclkProviderEventState Rp1GpclkLinuxProvider::eventState(std::uint64_t generation) const noexcept
{
    Rp1GpclkProviderEventState result{
        Rp1GpclkCompletionState::failed, 0, RP1_GPCLK_REASON_INTERNAL_ERROR};
    std::string ignored;
    (void)getState(generation, result, ignored);
    return result;
}

Rp1GpclkCompletionState Rp1GpclkLinuxProvider::state(std::uint64_t generation) const noexcept
{ return eventState(generation).completion; }

bool Rp1GpclkLinuxProvider::release(std::string& error) noexcept
{
    if (fd_ < 0) return true;
    bool ok = true;
    if (lease_id_ != 0)
    {
        int release_result = 0;
        if (active_generation_ != 0 && !active_generation_terminal_)
        {
            rp1_gpclk_release_v2 request{};
            initializeHeaderV2(request);
            request.lease_id = lease_id_;
            request.generation = active_generation_;
            release_result = io_.control(fd_, RP1_GPCLK_IOC_RELEASE_V2, &request);
        }
        else
        {
            rp1_gpclk_release_v1 request{};
            initializeHeaderV1(request);
            request.lease_id = lease_id_;
            release_result = io_.control(fd_, RP1_GPCLK_IOC_RELEASE, &request);
        }
        if (release_result < 0)
        {
            error = "Could not release the owned RP1 GPCLK lease.";
            ok = false;
        }
    }
    if (io_.closeDevice(fd_) < 0)
    {
        if (!error.empty()) error += " ";
        error += "Could not close the RP1 GPCLK endpoint.";
        ok = false;
    }
    lease_id_ = 0; active_generation_ = 0; active_generation_terminal_ = false;
    supported_drive_ma_mask_ = 0; fd_ = -1;
    return ok;
}
} // namespace wsprrypi
