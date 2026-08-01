#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

enum class SupportBundleJobState { queued, running, succeeded, failed };
struct SupportBundleJobRequest { bool probe_i2c = false; };
struct SupportBundleJobSnapshot {
    std::string id;
    SupportBundleJobState state = SupportBundleJobState::queued;
    bool probe_i2c_requested = false;
    std::string failure_category;
    std::string failure_message;
    bool download_available = false;
};
struct SupportBundleExecutionResult { bool succeeded = false; std::string failure_category; std::string failure_message; };
class SupportBundleJobExecutor {
public:
    virtual ~SupportBundleJobExecutor() = default;
    virtual SupportBundleExecutionResult run(bool probe_i2c) = 0;
    virtual void request_stop() noexcept = 0;
};
class SupportBundleJobManager {
public:
    using IdGenerator = std::function<std::string()>;
    SupportBundleJobManager(std::shared_ptr<SupportBundleJobExecutor> executor, IdGenerator ids);
    ~SupportBundleJobManager();
    SupportBundleJobManager(const SupportBundleJobManager &) = delete;
    std::optional<SupportBundleJobSnapshot> create(SupportBundleJobRequest request, std::string &error);
    std::optional<SupportBundleJobSnapshot> lookup(const std::string &id) const;
    void shutdown();
    static bool valid_id(const std::string &id);
private:
    void run(std::string id, bool probe_i2c);
    std::shared_ptr<SupportBundleJobExecutor> executor_;
    IdGenerator ids_;
    mutable std::mutex mutex_;
    std::optional<SupportBundleJobSnapshot> job_;
    std::thread worker_;
    bool shutting_down_ = false;
};
