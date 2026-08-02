#pragma once

#include <functional>
#include <filesystem>
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
    std::string i2c_probe_status;
    std::string failure_category;
    std::string failure_message;
    bool download_available = false;
};
enum class SupportBundleDownloadReferenceStatus {
    available,
    malformed_or_unknown_id,
    not_ready,
    no_download,
};
struct SupportBundleDownloadReference {
    SupportBundleDownloadReferenceStatus status =
        SupportBundleDownloadReferenceStatus::malformed_or_unknown_id;
    std::filesystem::path archive_path;
};
struct SupportBundleExecutionResult { bool succeeded = false; std::string failure_category; std::string failure_message; };
struct SupportBundleExecutionContext { bool probe_i2c = false; std::filesystem::path job_directory; };
class SupportBundleJobExecutor {
public:
    virtual ~SupportBundleJobExecutor() = default;
    virtual SupportBundleExecutionResult run(const SupportBundleExecutionContext &context) = 0;
    virtual void request_stop() noexcept = 0;
};
class SupportBundleJobManager {
public:
    using IdGenerator = std::function<std::string()>;
    SupportBundleJobManager(std::shared_ptr<SupportBundleJobExecutor> executor, IdGenerator ids, std::filesystem::path storage_root);
    ~SupportBundleJobManager();
    SupportBundleJobManager(const SupportBundleJobManager &) = delete;
    std::optional<SupportBundleJobSnapshot> create(SupportBundleJobRequest request, std::string &error);
    std::optional<SupportBundleJobSnapshot> lookup(const std::string &id) const;
    SupportBundleDownloadReference download_reference(const std::string &id) const;
    void shutdown();
    static bool valid_id(const std::string &id);
private:
    void run(std::string id, bool probe_i2c, std::filesystem::path job_directory);
    std::shared_ptr<SupportBundleJobExecutor> executor_;
    IdGenerator ids_;
    std::filesystem::path storage_root_;
    bool storage_ready_ = false;
    mutable std::mutex mutex_;
    std::optional<SupportBundleJobSnapshot> job_;
    std::filesystem::path job_directory_;
    std::string validated_archive_filename_;
    std::thread worker_;
    bool shutting_down_ = false;
};
