#pragma once

#include "rp1_gpclk_planner.hpp"

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include <string>

namespace wsprrypi
{

enum class Rp1GpclkCompletionState
{
    idle,
    running,
    draining,
    complete,
    failed
};

struct Rp1GpclkProviderSymbol
{
    std::uint64_t lower_divider_word{0};
    std::uint64_t upper_divider_word{0};
    std::uint32_t lower_count{0};
    std::uint32_t upper_count{0};
};

struct Rp1GpclkProviderProgram
{
    std::uint32_t fractional_bits{0};
    std::uint32_t writes_per_symbol{0};
    std::uint32_t tick_divider{0};
    std::uint64_t generation{0};
    std::array<Rp1GpclkProviderSymbol, 4> tones{};
    std::array<std::uint8_t, 162> symbols{};
};

struct Rp1GpclkProviderEvent
{
    std::uint64_t duration_ns{0};
    std::uint16_t tone_index{0};
    bool rf_on{false};
};

struct Rp1GpclkProviderEventProgram
{
    std::uint32_t fractional_bits{0};
    std::uint32_t tick_divider{0};
    std::uint64_t generation{0};
    std::uint64_t total_duration_ns{0};
    std::vector<Rp1GpclkProviderSymbol> tones;
    std::vector<Rp1GpclkProviderEvent> events;
};

struct Rp1GpclkProviderEventState
{
    Rp1GpclkCompletionState completion{Rp1GpclkCompletionState::idle};
    std::uint32_t current_event{0};
    std::uint32_t terminal_reason{0};
};

/** Provider-owned RP1 clock/DMA boundary.  Implementations must serialize
 * access with the RP1 clock provider and must not prepare or enable clk_gp0.
 */
class Rp1GpclkProvider
{
public:
    virtual ~Rp1GpclkProvider() = default;
    virtual bool acquire(std::uint32_t drive_ma, std::string& error) = 0;
    virtual bool submit(const Rp1GpclkProviderProgram&, std::string& error) = 0;
    virtual bool submitEvents(const Rp1GpclkProviderEventProgram&, std::string& error) = 0;
    virtual bool requestFiniteStop(std::uint64_t generation, std::string& error) = 0;
    virtual Rp1GpclkCompletionState state(std::uint64_t generation) const noexcept = 0;
    virtual Rp1GpclkProviderEventState eventState(std::uint64_t generation) const noexcept = 0;
    virtual void release() noexcept = 0;
};

class Rp1GpclkBackend
{
public:
    static constexpr std::uint32_t kDefaultDriveMa = 2;
    static constexpr std::uint32_t kWritesPerSymbol = 66792;
    static constexpr std::uint32_t kTickDivider = 511;

    explicit Rp1GpclkBackend(Rp1GpclkProvider& provider) noexcept;
    ~Rp1GpclkBackend();

    bool prepare(std::uint32_t drive_ma, std::string& error);
    bool emitFrame(
        const Rp1GpclkPlan&,
        const std::array<std::uint8_t, 162>& symbols,
        std::string& error);
    bool emitEvents(Rp1GpclkProviderEventProgram program, std::string& error);
    bool cancel(std::string& error);
    bool timedOut(std::string& error);
    bool cleanup(std::string& error);

    static bool validDrive(std::uint32_t drive_ma) noexcept;
    std::uint64_t generation() const noexcept;

private:
    bool requestStop(std::string& error);

    Rp1GpclkProvider& provider_;
    std::uint64_t generation_{0};
    bool acquired_{false};
    bool in_flight_{false};
    bool in_flight_events_{false};
};

} // namespace wsprrypi
