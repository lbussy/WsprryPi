#include "rp1_gpclk_linux_provider.hpp"
#include "rp1_gpclk_uapi.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <vector>

namespace
{
int failures;
void expect(bool value, const char* message)
{ if (!value) { std::cerr << "FAIL: " << message << '\n'; ++failures; } }

std::array<std::uint8_t, 32> authorizationDigest()
{
    std::array<std::uint8_t, 32> value{};
    value.front() = 1;
    return value;
}

constexpr std::uint64_t kAdministrativeCapabilities =
    RP1_GPCLK_CAP_STABLE_STATE | RP1_GPCLK_CAP_ROUTE_IDENTITY |
    RP1_GPCLK_CAP_COMPAT_IDENTITY | RP1_GPCLK_CAP_CLEANUP_FAULT_LATCH;

class Io final : public wsprrypi::Rp1GpclkIo
{
public:
    int openDevice(const char* value, int value_flags) noexcept override
    { path = value; flags.push_back(value_flags); ++opens; return open_result; }
    int control(int, unsigned long request, void* argument) noexcept override
    {
        requests.push_back(request);
        if (fail_request == request) { error = fail_error; return -1; }
        if (request == RP1_GPCLK_IOC_QUERY_V2)
        {
            auto* value = static_cast<rp1_gpclk_query_v2*>(argument);
            const auto input = value->header;
            *value = query;
            value->header = input;
            if (malformed_query_size) --value->header.size;
            if (wrong_query_version) value->header.version = RP1_GPCLK_UAPI_ABI_V1;
            if (unknown_query_flags) value->header.flags = 1;
        }
        else if (request == RP1_GPCLK_IOC_GET_SNAPSHOT_V3)
        {
            auto* value = static_cast<rp1_gpclk_snapshot_v3*>(argument);
            const auto input = value->header;
            *value = snapshot;
            value->header = input;
            if (unknown_snapshot_flags) value->header.flags = 1;
            value->route = query.route;
            value->compatibility_state = independent_safe_idle_snapshot
                ? RP1_GPCLK_COMPAT_EXPERIMENTAL : query.compatibility_state;
            value->compatibility_reason = query.compatibility_reason;
            value->capabilities = query.capabilities | RP1_GPCLK_CAP_PASSIVE_SNAPSHOT |
                RP1_GPCLK_CAP_OPERATION_LIVE_GATE |
                (independent_safe_idle_snapshot
                    ? RP1_GPCLK_CAP_LIVE_ELIGIBLE |
                        RP1_GPCLK_CAP_OPERATION_LIVE_GATE
                    : 0);
            value->live_eligible = independent_safe_idle_snapshot ||
                query.capabilities & RP1_GPCLK_CAP_LIVE_ELIGIBLE
                ? RP1_GPCLK_OBSERVATION_TRUE : RP1_GPCLK_OBSERVATION_FALSE;
            std::strcpy(value->module_id, query.module_id);
            std::strcpy(value->build_id, query.build_id);
            std::strcpy(value->compatibility_id, query.compatibility_id);
        }
        else if (request == RP1_GPCLK_IOC_ACQUIRE_V4)
        {
            acquire_v4 = *static_cast<rp1_gpclk_acquire_v4*>(argument);
            static_cast<rp1_gpclk_acquire_v4*>(argument)->lease_id = lease_id;
        }
        else if (request == RP1_GPCLK_IOC_SUBMIT_WSPR) {
            auto* value=static_cast<rp1_gpclk_submit_wspr_v1*>(argument); wspr=*value; value->generation=returned_generation; }
        else if (request == RP1_GPCLK_IOC_SUBMIT_EVENTS) {
            auto* value=static_cast<rp1_gpclk_submit_events_v1*>(argument); events=*value; value->generation=returned_generation; }
        else if (request == RP1_GPCLK_IOC_SUBMIT_TONE_V2) {
            auto* value=static_cast<rp1_gpclk_submit_tone_v2*>(argument); tone=*value; value->generation=returned_generation; }
        else if (request == RP1_GPCLK_IOC_STOP)
            stop = *static_cast<rp1_gpclk_stop_v1*>(argument);
        else if (request == RP1_GPCLK_IOC_GET_STATE)
        {
            auto* value = static_cast<rp1_gpclk_state_v1*>(argument);
            value->state = state;
            value->terminal_reason = terminal_reason;
            value->current_event = current_event;
            value->cleanup_fault = cleanup_fault;
            if (stale_generation) ++value->generation;
            if (foreign_lease) ++value->lease_id;
        }
        else if (request == RP1_GPCLK_IOC_RELEASE)
            release = *static_cast<rp1_gpclk_release_v1*>(argument);
        else if (request == RP1_GPCLK_IOC_RELEASE_V2)
            release_v2 = *static_cast<rp1_gpclk_release_v2*>(argument);
        return 0;
    }
    int closeDevice(int) noexcept override { ++closes; return close_result; }
    int lastError() const noexcept override { return error; }

    Io()
    {
        query.header.size = sizeof(query);
        query.header.version = RP1_GPCLK_UAPI_ABI_V2;
        query.abi_min = RP1_GPCLK_UAPI_ABI_V1;
        query.abi_max = RP1_GPCLK_UAPI_ABI_V2;
        query.route = RP1_GPCLK_ROUTE_GPIO4;
        query.compatibility_state = RP1_GPCLK_COMPAT_COMPATIBLE_UNQUALIFIED;
        query.compatibility_reason = RP1_GPCLK_COMPAT_REASON_ADMIN_ENROLLMENT_REQUIRED;
        query.capabilities = kAdministrativeCapabilities |
            RP1_GPCLK_CAP_SUBMIT_WSPR | RP1_GPCLK_CAP_SUBMIT_EVENTS |
            RP1_GPCLK_CAP_STOP_DRAIN | RP1_GPCLK_CAP_TONE_CONTINUOUS |
            RP1_GPCLK_CAP_TONE_FINITE;
        query.max_tones = RP1_GPCLK_MAX_TONES;
        query.wspr_symbols = RP1_GPCLK_WSPR_SYMBOLS;
        query.max_events = RP1_GPCLK_MAX_EVENTS;
        query.max_dither_period = RP1_GPCLK_DITHER_PERIOD_MAX;
        query.supported_drive_ma_mask = RP1_GPCLK_DRIVE_SUPPORT_ALLOWED_MASK;
        query.max_event_duration_ns = RP1_GPCLK_EVENT_DURATION_NS_MAX;
        query.max_request_duration_ns = RP1_GPCLK_REQUEST_DURATION_NS_MAX;
        query.min_tone_duration_ns = RP1_GPCLK_TONE_DURATION_NS_MIN;
        query.max_tone_duration_ns = RP1_GPCLK_TONE_DURATION_NS_MAX;
        std::strcpy(query.module_id, "rp1-gpclk-dkms");
        std::strcpy(query.build_id, "1.1.2");
        std::strcpy(query.compatibility_id,
            "v1.1.2-pi5-gpio4-6.18.34-development-candidate-r4");
        snapshot.header.size = sizeof(snapshot);
        snapshot.header.version = RP1_GPCLK_UAPI_ABI_V3;
        snapshot.abi_min = RP1_GPCLK_UAPI_ABI_V1;
        snapshot.abi_max = RP1_GPCLK_UAPI_ABI_V4;
        snapshot.operation_state = RP1_GPCLK_STATE_IDLE;
        snapshot.terminal_reason = RP1_GPCLK_REASON_NONE;
        snapshot.cleanup_fault = RP1_GPCLK_OBSERVATION_FALSE;
        snapshot.owner_present = RP1_GPCLK_OBSERVATION_FALSE;
        snapshot.lease_present = RP1_GPCLK_OBSERVATION_FALSE;
        snapshot.live_output = RP1_GPCLK_OBSERVATION_FALSE;
        snapshot.live_eligible = RP1_GPCLK_OBSERVATION_FALSE;
        snapshot.drain_state = RP1_GPCLK_DRAIN_NONE;
        snapshot.gpio_safe = RP1_GPCLK_OBSERVATION_TRUE;
        snapshot.clock_quiescent = RP1_GPCLK_OBSERVATION_TRUE;
        snapshot.dma_quiescent = RP1_GPCLK_OBSERVATION_TRUE;
        snapshot.stable = RP1_GPCLK_OBSERVATION_TRUE;
        snapshot.capabilities = query.capabilities | RP1_GPCLK_CAP_PASSIVE_SNAPSHOT;
        snapshot.min_tone_duration_ns = RP1_GPCLK_TONE_DURATION_NS_MIN;
        snapshot.max_tone_duration_ns = RP1_GPCLK_TONE_DURATION_NS_MAX;
    }

    rp1_gpclk_query_v2 query{};
    rp1_gpclk_snapshot_v3 snapshot{};
    rp1_gpclk_acquire_v1 acquire{};
    rp1_gpclk_acquire_v4 acquire_v4{};
    rp1_gpclk_submit_wspr_v1 wspr{};
    rp1_gpclk_submit_events_v1 events{};
    rp1_gpclk_submit_tone_v2 tone{};
    rp1_gpclk_stop_v1 stop{};
    rp1_gpclk_release_v1 release{};
    rp1_gpclk_release_v2 release_v2{};
    std::string path;
    std::vector<int> flags;
    std::vector<unsigned long> requests;
    int open_result{7}, close_result{0}, error{ENOENT}, opens{}, closes{};
    unsigned long fail_request{};
    int fail_error{EINVAL};
    std::uint64_t lease_id{41};
    std::uint64_t returned_generation{9};
    std::uint32_t state{RP1_GPCLK_STATE_IDLE};
    std::uint32_t terminal_reason{RP1_GPCLK_REASON_NONE};
    std::uint32_t current_event{};
    std::uint32_t cleanup_fault{};
    bool stale_generation{};
    bool foreign_lease{};
    bool malformed_query_size{};
    bool wrong_query_version{};
    bool unknown_query_flags{};
    bool unknown_snapshot_flags{};
    bool independent_safe_idle_snapshot{};
};

void test_query_and_fail_closed_validation()
{
    Io io;
    wsprrypi::Rp1GpclkLinuxProvider provider(io);
    wsprrypi::Rp1GpclkProviderIdentity identity;
    std::string error;
    expect(provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "valid ABI v2 QUERY must parse");
    expect(io.path == "/dev/rp1-gpclk", "only the canonical endpoint may be opened");
    expect(identity.route == RP1_GPCLK_ROUTE_GPIO4 &&
        identity.compatibility_state == RP1_GPCLK_COMPAT_COMPATIBLE_UNQUALIFIED,
        "QUERY must preserve route and compatibility independently");
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO20, kAdministrativeCapabilities,
        false, identity, error), "GPIO4 evidence must not satisfy GPIO20");

    io.query.capabilities |= (1ULL << 63);
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "unknown ABI v2 capability must fail closed");
    io.query.capabilities &= ~(1ULL << 63);

    io.malformed_query_size = true;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "malformed ABI v2 QUERY size must fail closed");
    io.malformed_query_size = false;
    io.wrong_query_version = true;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "wrong ABI v2 QUERY version must fail closed");
    io.wrong_query_version = false;
    io.unknown_query_flags = true;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "unknown ABI v2 QUERY flag must fail closed");
    io.unknown_query_flags = false;
    io.query.reserved[0] = 1;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "nonzero ABI v2 reserved data must fail closed");
    io.query.reserved[0] = 0;

    io.query.capabilities &= ~RP1_GPCLK_CAP_TONE_CONTINUOUS;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4,
        kAdministrativeCapabilities | RP1_GPCLK_CAP_TONE_CONTINUOUS,
        false, identity, error), "missing continuous TONE capability must fail closed");
    io.query.capabilities |= RP1_GPCLK_CAP_TONE_CONTINUOUS;
    io.query.capabilities &= ~RP1_GPCLK_CAP_TONE_FINITE;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4,
        kAdministrativeCapabilities | RP1_GPCLK_CAP_TONE_FINITE,
        false, identity, error), "missing finite TONE capability must fail closed");
    io.query.capabilities |= RP1_GPCLK_CAP_TONE_FINITE;

    io.query.abi_max = RP1_GPCLK_UAPI_ABI_V1;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "ABI mismatch must fail closed");
    io.query.abi_max = RP1_GPCLK_UAPI_ABI_V2;
    io.query.capabilities &= ~RP1_GPCLK_CAP_STABLE_STATE;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "missing capability must fail closed");
    io.query.capabilities |= RP1_GPCLK_CAP_STABLE_STATE;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        true, identity, error), "compatible-unqualified is not LIVE_ELIGIBLE");
    io.query.compatibility_state = 99;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "unknown compatibility state must fail closed");
    io.query.compatibility_state = RP1_GPCLK_COMPAT_UNAVAILABLE;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "unavailable compatibility must fail closed");
    io.query.compatibility_state = RP1_GPCLK_COMPAT_REJECTED;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "rejected compatibility must fail closed");
    io.query.compatibility_state = RP1_GPCLK_COMPAT_COMPATIBLE_UNQUALIFIED;
    std::strcpy(io.query.build_id, "1.0.1");
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "non-frozen module/build identity must fail closed");
}

void test_safe_idle_query_projection_uses_authenticated_passive_identity()
{
    Io io;
    io.query.compatibility_reason = RP1_GPCLK_COMPAT_REASON_NONE;
    io.independent_safe_idle_snapshot = true;
    wsprrypi::Rp1GpclkLinuxProvider provider(io);
    wsprrypi::Rp1GpclkProviderIdentity identity;
    std::string error;
    expect(provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error),
        "safe-idle ABI v2 projection must reconcile with passive ABI v3 eligibility");
    expect(identity.compatibility_state == RP1_GPCLK_COMPAT_EXPERIMENTAL &&
        (identity.capabilities & RP1_GPCLK_CAP_LIVE_ELIGIBLE) != 0,
        "passive ABI v3 identity must become authoritative after reconciliation");
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        true, identity, error),
        "safe-idle projection must not satisfy an already-live query");
}

void test_old_module_and_tone_v2()
{
    Io old;
    old.fail_request = RP1_GPCLK_IOC_QUERY_V2;
    old.fail_error = EOPNOTSUPP;
    wsprrypi::Rp1GpclkLinuxProvider old_provider(old);
    wsprrypi::Rp1GpclkProviderIdentity identity;
    std::string error;
    expect(!old_provider.query(RP1_GPCLK_ROUTE_GPIO4,
        kAdministrativeCapabilities, false, identity, error) &&
        error.find("old module") != std::string::npos,
        "old modules must receive deterministic ABI v2 rejection");

    Io io;
    io.query.capabilities |= RP1_GPCLK_CAP_LIVE_ELIGIBLE;
    io.query.compatibility_state = RP1_GPCLK_COMPAT_EXPERIMENTAL;
    wsprrypi::Rp1GpclkLinuxProvider provider(io);
    const auto required = kAdministrativeCapabilities |
        RP1_GPCLK_CAP_TONE_CONTINUOUS | RP1_GPCLK_CAP_TONE_FINITE |
        RP1_GPCLK_CAP_STOP_DRAIN | RP1_GPCLK_CAP_LIVE_ELIGIBLE |
        RP1_GPCLK_CAP_OPERATION_LIVE_GATE;
    expect(provider.acquire(RP1_GPCLK_ROUTE_GPIO4, required, authorizationDigest(), error),
        "ABI v2 TONE fixture must acquire exact capabilities");
    wsprrypi::Rp1GpclkProviderToneProgram tone;
    tone.tone = {1, 2, 1, 1};
    tone.generation = 12;
    tone.operation = RP1_GPCLK_TONE_OPERATION_CONTINUOUS;
    tone.expected_route = RP1_GPCLK_ROUTE_GPIO4;
    tone.fractional_bits = RP1_GPCLK_FRACTIONAL_BITS;
    tone.tick_divider = RP1_GPCLK_TICK_DIVIDER;
    tone.drive_ma = RP1_GPCLK_DRIVE_MA_2;
    expect(provider.submitTone(tone, error) &&
        io.tone.operation == RP1_GPCLK_TONE_OPERATION_CONTINUOUS &&
        io.tone.duration_ns == 0,
        "continuous TONE must submit with explicit zero duration");
    tone.duration_ns = 1;
    expect(!provider.submitTone(tone, error),
        "continuous TONE must reject every hidden duration");
    expect(provider.requestFiniteStop(io.returned_generation, error) &&
        io.stop.generation == io.returned_generation,
        "operator STOP must bind the active continuous generation");
    expect(provider.release(error) &&
        io.release_v2.generation == io.returned_generation,
        "continuous TONE release must bind its generation");

    Io finite_io;
    finite_io.query.capabilities |= RP1_GPCLK_CAP_LIVE_ELIGIBLE;
    finite_io.query.compatibility_state = RP1_GPCLK_COMPAT_EXPERIMENTAL;
    wsprrypi::Rp1GpclkLinuxProvider finite(finite_io);
    expect(finite.acquire(RP1_GPCLK_ROUTE_GPIO4, required, authorizationDigest(), error),
        "finite TONE fixture must acquire");
    tone.operation = RP1_GPCLK_TONE_OPERATION_FINITE;
    tone.duration_ns = 1000000000ULL;
    expect(finite.submitTone(tone, error) && finite_io.tone.duration_ns == 1000000000ULL,
        "finite TONE must preserve its exact kernel duration");
    expect(finite.release(error), "one-second finite TONE fixture must release");

    Io boundary_io;
    boundary_io.query.capabilities |= RP1_GPCLK_CAP_LIVE_ELIGIBLE;
    boundary_io.query.compatibility_state = RP1_GPCLK_COMPAT_EXPERIMENTAL;
    wsprrypi::Rp1GpclkLinuxProvider boundary(boundary_io);
    expect(boundary.acquire(RP1_GPCLK_ROUTE_GPIO4, required, authorizationDigest(), error),
        "finite boundary fixture must acquire");
    tone.duration_ns = RP1_GPCLK_TONE_DURATION_NS_MIN;
    expect(boundary.submitTone(tone, error), "finite minimum duration must be accepted");
    expect(boundary.release(error), "finite minimum fixture must release");

    Io maximum_io;
    maximum_io.query.capabilities |= RP1_GPCLK_CAP_LIVE_ELIGIBLE;
    maximum_io.query.compatibility_state = RP1_GPCLK_COMPAT_EXPERIMENTAL;
    wsprrypi::Rp1GpclkLinuxProvider maximum(maximum_io);
    expect(maximum.acquire(RP1_GPCLK_ROUTE_GPIO4, required, authorizationDigest(), error),
        "finite maximum fixture must acquire");
    tone.duration_ns = RP1_GPCLK_TONE_DURATION_NS_MAX;
    expect(maximum.submitTone(tone, error), "finite maximum duration must be accepted");
    expect(maximum.release(error), "finite maximum fixture must release");

    Io invalid_io;
    invalid_io.query.capabilities |= RP1_GPCLK_CAP_LIVE_ELIGIBLE;
    invalid_io.query.compatibility_state = RP1_GPCLK_COMPAT_EXPERIMENTAL;
    wsprrypi::Rp1GpclkLinuxProvider invalid(invalid_io);
    expect(invalid.acquire(RP1_GPCLK_ROUTE_GPIO4, required, authorizationDigest(), error),
        "finite invalid-duration fixture must acquire");
    tone.duration_ns = 0;
    expect(!invalid.submitTone(tone, error), "finite zero duration must fail before ioctl");
    tone.duration_ns = RP1_GPCLK_TONE_DURATION_NS_MAX + 1;
    expect(!invalid.submitTone(tone, error), "finite above-maximum duration must fail");
    expect(invalid.release(error), "finite invalid-duration fixture must release");
}

void test_passive_snapshot_is_read_only_and_fail_closed()
{
    Io io;
    wsprrypi::Rp1GpclkLinuxProvider provider(io);
    wsprrypi::Rp1GpclkPassiveSnapshot snapshot;
    std::string error;
    expect(provider.passiveSnapshot(snapshot, error),
        "valid ABI v3 passive snapshot must parse");
    expect(io.opens == 1 && io.closes == 1 && !io.flags.empty() &&
        (io.flags.back() & O_ACCMODE) == O_RDONLY,
        "passive inspection must use one read-only descriptor and close it");
    expect(snapshot.generation == 0 &&
        snapshot.owner_present == RP1_GPCLK_OBSERVATION_FALSE &&
        snapshot.lease_present == RP1_GPCLK_OBSERVATION_FALSE,
        "initial passive snapshot must preserve generation zero and no ownership");

    io.snapshot.owner_present = RP1_GPCLK_OBSERVATION_TRUE;
    io.snapshot.lease_present = RP1_GPCLK_OBSERVATION_TRUE;
    io.snapshot.operation_state = RP1_GPCLK_STATE_RUNNING;
    expect(provider.passiveSnapshot(snapshot, error) &&
        snapshot.owner_present == RP1_GPCLK_OBSERVATION_TRUE &&
        snapshot.lease_present == RP1_GPCLK_OBSERVATION_TRUE,
        "passive inspection must report presence without exposing a token");

    io.snapshot.owner_present = RP1_GPCLK_OBSERVATION_FALSE;
    io.snapshot.lease_present = RP1_GPCLK_OBSERVATION_FALSE;
    io.snapshot.operation_state = RP1_GPCLK_STATE_IDLE;
    io.snapshot.stable = 99;
    expect(!provider.passiveSnapshot(snapshot, error),
        "unknown passive safety observations must fail closed");
    io.snapshot.stable = RP1_GPCLK_OBSERVATION_TRUE;
    io.unknown_snapshot_flags = true;
    expect(!provider.passiveSnapshot(snapshot, error),
        "unknown passive header flags must fail closed");
    io.unknown_snapshot_flags = false;
    io.snapshot.reserved[0] = 1;
    expect(!provider.passiveSnapshot(snapshot, error),
        "nonzero passive reserved data must fail closed");
    io.snapshot.reserved[0] = 0;
    io.fail_request = RP1_GPCLK_IOC_GET_SNAPSHOT_V3;
    io.fail_error = ENOTTY;
    const auto closes_before = io.closes;
    expect(!provider.passiveSnapshot(snapshot, error) &&
        error.find("does not support") != std::string::npos &&
        io.closes == closes_before + 1,
        "unsupported passive ioctl must close its descriptor");
}

void test_acquire_state_release_and_generation()
{
    Io io;
    io.query.capabilities |= RP1_GPCLK_CAP_LIVE_ELIGIBLE;
    io.query.compatibility_state = RP1_GPCLK_COMPAT_EXPERIMENTAL;
    wsprrypi::Rp1GpclkLinuxProvider provider(io);
    std::string error;
    const auto required = kAdministrativeCapabilities |
        RP1_GPCLK_CAP_SUBMIT_WSPR | RP1_GPCLK_CAP_STOP_DRAIN |
        RP1_GPCLK_CAP_LIVE_ELIGIBLE |
        RP1_GPCLK_CAP_OPERATION_LIVE_GATE;
    expect(provider.acquire(RP1_GPCLK_ROUTE_GPIO4, required, authorizationDigest(), error),
        "live-eligible exact-route provider must acquire");
    expect(io.acquire_v4.expected_route == RP1_GPCLK_ROUTE_GPIO4 &&
        io.acquire_v4.required_capabilities == required &&
        io.acquire_v4.authorization_digest[0] == 1,
        "ACQUIRE must bind route and capabilities");

    wsprrypi::Rp1GpclkProviderProgram program{};
    program.fractional_bits = RP1_GPCLK_FRACTIONAL_BITS;
    program.tick_divider = RP1_GPCLK_TICK_DIVIDER;
    program.writes_per_symbol = RP1_GPCLK_WSPR_WRITES_PER_SYMBOL_MAX;
    program.drive_ma = RP1_GPCLK_DRIVE_MA_2;
    program.generation = 9;
    expect(provider.submit(program, error), "WSPR submission must carry the lease");
    expect(io.wspr.lease_id == io.lease_id && io.wspr.generation == 0 &&
        program.generation == io.returned_generation &&
        io.wspr.drive_ma == RP1_GPCLK_DRIVE_MA_2,
        "submission must preserve lease, generation, and drive");
    io.state = RP1_GPCLK_STATE_COMPLETE;
    io.terminal_reason = RP1_GPCLK_REASON_COMPLETE;
    expect(provider.state(9) == wsprrypi::Rp1GpclkCompletionState::complete,
        "known terminal state must map exactly");
    io.stale_generation = true;
    expect(provider.state(9) == wsprrypi::Rp1GpclkCompletionState::failed,
        "stale generation state must fail closed");
    io.stale_generation = false;
    io.foreign_lease = true;
    expect(provider.state(9) == wsprrypi::Rp1GpclkCompletionState::failed,
        "foreign lease state must fail closed");
    io.foreign_lease = false;
    io.state = RP1_GPCLK_STATE_RUNNING;
    io.terminal_reason = RP1_GPCLK_REASON_COMPLETE;
    expect(provider.state(9) == wsprrypi::Rp1GpclkCompletionState::failed,
        "impossible nonterminal state/reason pair must fail closed");
    expect(provider.requestFiniteStop(9, error) && io.stop.lease_id == io.lease_id,
        "STOP must bind lease and generation");
    io.fail_request = RP1_GPCLK_IOC_STOP;
    io.fail_error = EALREADY;
    io.state = RP1_GPCLK_STATE_COMPLETE;
    io.terminal_reason = RP1_GPCLK_REASON_COMPLETE;
    error.clear();
    expect(provider.requestFiniteStop(9, error),
        "STOP EALREADY must be idempotent only for authenticated terminal state");
    io.state = RP1_GPCLK_STATE_RUNNING;
    io.terminal_reason = RP1_GPCLK_REASON_NONE;
    error.clear();
    expect(!provider.requestFiniteStop(9, error),
        "STOP EALREADY must fail closed for nonterminal state");
    io.fail_request = 0;
    expect(provider.release(error), "owned lease must release and close cleanly");
    expect(io.release.lease_id == io.lease_id && io.closes == 2,
        "terminal generation must use terminal-only RELEASE before close");
}

void test_failure_cleanup_and_historical_endpoint_rejection()
{
    Io missing;
    missing.open_result = -1;
    wsprrypi::Rp1GpclkLinuxProvider provider(missing);
    wsprrypi::Rp1GpclkProviderIdentity identity;
    std::string error;
    expect(!provider.query(RP1_GPCLK_ROUTE_GPIO4, kAdministrativeCapabilities,
        false, identity, error), "missing canonical endpoint must fail closed");
    expect(missing.path == "/dev/rp1-gpclk",
        "missing canonical endpoint must never fall back to rp1-gpclk0");

    Io acquire_failure;
    acquire_failure.query.capabilities |= RP1_GPCLK_CAP_LIVE_ELIGIBLE;
    acquire_failure.query.compatibility_state = RP1_GPCLK_COMPAT_EXPERIMENTAL;
    acquire_failure.fail_request = RP1_GPCLK_IOC_ACQUIRE_V4;
    wsprrypi::Rp1GpclkLinuxProvider failed(acquire_failure);
    expect(!failed.acquire(RP1_GPCLK_ROUTE_GPIO4,
        kAdministrativeCapabilities | RP1_GPCLK_CAP_LIVE_ELIGIBLE |
            RP1_GPCLK_CAP_OPERATION_LIVE_GATE,
        authorizationDigest(), error) &&
        acquire_failure.closes == 2,
        "ACQUIRE failure must close without leaking ownership");

    Io release_failure;
    release_failure.query.capabilities |= RP1_GPCLK_CAP_LIVE_ELIGIBLE;
    release_failure.query.compatibility_state = RP1_GPCLK_COMPAT_EXPERIMENTAL;
    wsprrypi::Rp1GpclkLinuxProvider cleanup(release_failure);
    expect(cleanup.acquire(RP1_GPCLK_ROUTE_GPIO4,
        kAdministrativeCapabilities | RP1_GPCLK_CAP_LIVE_ELIGIBLE |
            RP1_GPCLK_CAP_OPERATION_LIVE_GATE,
        authorizationDigest(), error),
        "release-failure fixture must acquire");
    release_failure.fail_request = RP1_GPCLK_IOC_RELEASE;
    expect(!cleanup.release(error) && release_failure.closes == 2,
        "RELEASE failure must still close and report cleanup failure");
}
}

int main()
{
    test_query_and_fail_closed_validation();
    test_safe_idle_query_projection_uses_authenticated_passive_identity();
    test_acquire_state_release_and_generation();
    test_old_module_and_tone_v2();
    test_passive_snapshot_is_read_only_and_fail_closed();
    test_failure_cleanup_and_historical_endpoint_rejection();
    if (failures) return 1;
    std::cout << "RP1 GPCLK ABI v2 execution and ABI v3 passive provider tests passed\n";
}
