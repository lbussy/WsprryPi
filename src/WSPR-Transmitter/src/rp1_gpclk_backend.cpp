#include "rp1_gpclk_backend.hpp"

#include <limits>

namespace wsprrypi
{

Rp1GpclkBackend::Rp1GpclkBackend(Rp1GpclkProvider& provider) noexcept
    : provider_(provider) {}

Rp1GpclkBackend::~Rp1GpclkBackend()
{
    if (acquired_ && !in_flight_)
    {
        std::string ignored;
        (void)provider_.release(ignored);
    }
}

bool Rp1GpclkBackend::validDrive(std::uint32_t drive_ma) noexcept
{
    return drive_ma == 2 || drive_ma == 4 || drive_ma == 8 || drive_ma == 12;
}

bool Rp1GpclkBackend::prepare(
    std::uint32_t drive_ma,
    std::uint32_t expected_route,
    std::uint64_t required_capabilities,
    std::string& error)
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
    if (!provider_.acquire(expected_route, required_capabilities, error))
        return false;
    drive_ma_ = drive_ma;
    acquired_ = true;
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
    program.generation = 0;
    program.drive_ma = drive_ma_;
    if (!provider_.submitEvents(program, error))
        return false;
    generation_ = program.generation;
    in_flight_ = true;
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
        const auto state = provider_.eventState(generation_).completion;
        if (state != Rp1GpclkCompletionState::complete &&
            state != Rp1GpclkCompletionState::failed)
        {
            error = "RP1 GPCLK descriptor is still draining.";
            return false;
        }
        in_flight_ = false;
    }
    if (!provider_.release(error))
        return false;
    drive_ma_ = 0;
    acquired_ = false;
    return true;
}

std::uint64_t Rp1GpclkBackend::generation() const noexcept { return generation_; }

} // namespace wsprrypi
