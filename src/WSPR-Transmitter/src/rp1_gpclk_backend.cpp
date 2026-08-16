#include "rp1_gpclk_backend.hpp"

#include <limits>

namespace wsprrypi
{

Rp1GpclkBackend::Rp1GpclkBackend(Rp1GpclkProvider& provider) noexcept
    : provider_(provider) {}

Rp1GpclkBackend::~Rp1GpclkBackend()
{
    if (acquired_ && !in_flight_)
        provider_.release();
}

bool Rp1GpclkBackend::validDrive(std::uint32_t drive_ma) noexcept
{
    return drive_ma == 2 || drive_ma == 4 || drive_ma == 8 || drive_ma == 12;
}

bool Rp1GpclkBackend::prepare(std::uint32_t drive_ma, std::string& error)
{
    if (acquired_)
    {
        error = "RP1 GPCLK backend is already prepared.";
        return false;
    }
    if (!validDrive(drive_ma))
    {
        error = "RP1 GPIO drive must be 2, 4, 8, or 12 mA.";
        return false;
    }
    if (!provider_.acquire(drive_ma, error))
        return false;
    acquired_ = true;
    return true;
}

bool Rp1GpclkBackend::emitFrame(
    const Rp1GpclkPlan& plan,
    const std::array<std::uint8_t, 162>& symbols,
    std::string& error)
{
    if (!acquired_ || in_flight_ ||
        plan.fractional_bits != 16 ||
        generation_ == std::numeric_limits<std::uint64_t>::max())
    {
        error = "RP1 GPCLK emit state or plan is invalid.";
        return false;
    }
    Rp1GpclkProviderProgram program;
    program.fractional_bits = plan.fractional_bits;
    program.writes_per_symbol = kWritesPerSymbol;
    program.tick_divider = kTickDivider;
    program.generation = ++generation_;
    for (std::size_t tone = 0; tone < plan.tones.size(); ++tone)
    {
        const auto& selected = plan.tones[tone];
        if (selected.lower_word_count + selected.upper_word_count !=
            kWritesPerSymbol)
        {
            error = "Every RP1 GPCLK symbol must contain exactly 66792 divider writes.";
            --generation_;
            return false;
        }
        program.tones[tone] = Rp1GpclkProviderSymbol{
            selected.lower_divider_word,
            selected.upper_divider_word,
            selected.lower_word_count,
            selected.upper_word_count};
    }
    for (std::size_t i = 0; i < symbols.size(); ++i)
    {
        if (symbols[i] >= plan.tones.size())
        {
            error = "RP1 GPCLK frame contains an invalid tone index.";
            --generation_;
            return false;
        }
        program.symbols[i] = symbols[i];
    }
    if (!provider_.submit(program, error))
        return false;
    in_flight_ = true;
    in_flight_events_ = false;
    return true;
}

bool Rp1GpclkBackend::emitEvents(
    Rp1GpclkProviderEventProgram program, std::string& error)
{
    if (!acquired_ || in_flight_)
    {
        error = acquired_ ? "RP1 GPCLK provider is already running."
                          : "RP1 GPCLK provider is not prepared.";
        return false;
    }
    if (generation_ == std::numeric_limits<std::uint64_t>::max())
    {
        error = "RP1 GPCLK generation is exhausted.";
        return false;
    }
    program.generation = ++generation_;
    if (!provider_.submitEvents(program, error))
        return false;
    in_flight_ = true;
    in_flight_events_ = true;
    return true;
}

bool Rp1GpclkBackend::requestStop(std::string& error)
{
    if (!in_flight_)
        return true;
    return provider_.requestFiniteStop(generation_, error);
}

bool Rp1GpclkBackend::cancel(std::string& error) { return requestStop(error); }
bool Rp1GpclkBackend::timedOut(std::string& error) { return requestStop(error); }

bool Rp1GpclkBackend::cleanup(std::string& error)
{
    if (!acquired_)
        return true;
    if (in_flight_)
    {
        const auto state = in_flight_events_
            ? provider_.eventState(generation_).completion
            : provider_.state(generation_);
        if (state != Rp1GpclkCompletionState::complete &&
            state != Rp1GpclkCompletionState::failed)
        {
            error = "RP1 GPCLK descriptor is still draining.";
            return false;
        }
        in_flight_ = false;
        in_flight_events_ = false;
    }
    provider_.release();
    acquired_ = false;
    return true;
}

std::uint64_t Rp1GpclkBackend::generation() const noexcept { return generation_; }

} // namespace wsprrypi
