#pragma once

#include "rp1_gpclk_backend.hpp"

#include <string>

namespace wsprrypi
{
class Rp1GpclkIo
{
public:
    virtual ~Rp1GpclkIo() = default;
    virtual int openDevice(const char* path) noexcept = 0;
    virtual int control(int fd, unsigned long request, void* argument) noexcept = 0;
    virtual int closeDevice(int fd) noexcept = 0;
    virtual int lastError() const noexcept = 0;
};

class Rp1GpclkPosixIo final : public Rp1GpclkIo
{
public:
    int openDevice(const char* path) noexcept override;
    int control(int fd, unsigned long request, void* argument) noexcept override;
    int closeDevice(int fd) noexcept override;
    int lastError() const noexcept override;
};

class Rp1GpclkLinuxProvider final : public Rp1GpclkProvider
{
public:
    explicit Rp1GpclkLinuxProvider(
        Rp1GpclkIo& io,
        std::string device = "/dev/rp1-gpclk0") noexcept;
    ~Rp1GpclkLinuxProvider() override;

    bool acquire(std::uint32_t drive_ma, std::string& error) override;
    bool submit(const Rp1GpclkProviderProgram&, std::string& error) override;
    bool submitEvents(const Rp1GpclkProviderEventProgram&, std::string& error) override;
    bool requestFiniteStop(std::uint64_t generation, std::string& error) override;
    Rp1GpclkCompletionState state(std::uint64_t generation) const noexcept override;
    Rp1GpclkProviderEventState eventState(std::uint64_t generation) const noexcept override;
    void release() noexcept override;

private:
    bool failed(const char* operation, std::string& error) const;
    Rp1GpclkIo& io_;
    std::string device_;
    int fd_{-1};
};
} // namespace wsprrypi
