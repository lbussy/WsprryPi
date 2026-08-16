#pragma once

#include "execution_plan.hpp"
#include "rp1_gpclk_backend.hpp"

#include <string>

namespace wsprrypi
{

struct Rp1GpclkEventCompileResult
{
    bool ok{false};
    Rp1GpclkProviderEventProgram program{};
    std::string error;
};

Rp1GpclkEventCompileResult compileRp1GpclkEventProgram(
    const ExecutionPlan& plan);
bool validateRp1GpclkEventProgram(
    const Rp1GpclkProviderEventProgram& program,
    std::string& error) noexcept;

} // namespace wsprrypi
