#pragma once

#include "support_bundle_job_directory_remover.hpp"

#include <functional>
#include <filesystem>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

enum class SupportBundleJobState { queued, running, succeeded, failed };
enum class SupportBundlePrivateLifecycle { none, candidate_ready };
struct SupportBundleJobRequest { bool probe_i2c = false; };
struct SupportBundleJobSnapshot {
    std::string id;
    SupportBundleJobState state = SupportBundleJobState::queued;
    bool probe_i2c_requested = false;
    std::string i2c_probe_status;
    std::string failure_category;
    std::string failure_message;
    bool download_available = false;
    std::string case_id;
    SupportBundlePrivateLifecycle private_lifecycle = SupportBundlePrivateLifecycle::none;
};
enum class SupportBundleDownloadReferenceStatus {
    available,
    malformed_or_unknown_id,
    not_ready,
    no_download,
};
enum class SupportBundleDownloadDeletionStatus {
    removed,
    already_removed,
    malformed_or_unknown_id,
    not_terminal,
    no_retained_download,
    cleanup_failed,
};
struct SupportBundleDownloadDeletionResult {
    SupportBundleDownloadDeletionStatus status =
        SupportBundleDownloadDeletionStatus::malformed_or_unknown_id;

    [[nodiscard]] bool removed() const noexcept {
        return status == SupportBundleDownloadDeletionStatus::removed;
    }
};
struct SupportBundleDownloadReference {
    SupportBundleDownloadReferenceStatus status =
        SupportBundleDownloadReferenceStatus::malformed_or_unknown_id;
    std::filesystem::path archive_path;
    std::string archive_basename;
    std::filesystem::path checksum_path;
    std::string checksum_basename;
    std::string expected_sha256;
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
    inline static constexpr std::chrono::hours kProductionRetention = std::chrono::hours(24);
    inline static constexpr std::chrono::minutes kProductionRetryDelay = std::chrono::minutes(5);
    using IdGenerator = std::function<std::string()>;
    using JobDirectoryRemover = std::function<SupportBundleJobDirectoryRemovalResult(
        const std::filesystem::path &, const std::string &)>;
    SupportBundleJobManager(std::shared_ptr<SupportBundleJobExecutor> executor,
                            IdGenerator ids,
                            std::filesystem::path storage_root,
                            JobDirectoryRemover remover = remove_support_bundle_job_directory,
                            std::chrono::milliseconds retention = kProductionRetention,
                            std::chrono::milliseconds retry_delay = kProductionRetryDelay);
    ~SupportBundleJobManager();
    SupportBundleJobManager(const SupportBundleJobManager &) = delete;
    std::optional<SupportBundleJobSnapshot> create(SupportBundleJobRequest request, std::string &error);
    std::optional<SupportBundleJobSnapshot> lookup(const std::string &id) const;
    SupportBundleDownloadReference download_reference(const std::string &id) const;
    SupportBundleDownloadDeletionResult delete_download(const std::string &id);
    void shutdown();
    static bool valid_id(const std::string &id);
private:
    void run(std::string id, bool probe_i2c, std::filesystem::path job_directory);
    void expiration_loop();
    void clear_current_download_locked(const std::string &id);
    void cancel_expiration_locked(const std::string &id);
    void remove_unsuccessful_job_directory(const std::string &id,
                                           const std::filesystem::path &job_directory);
    std::shared_ptr<SupportBundleJobExecutor> executor_;
    IdGenerator ids_;
    std::filesystem::path storage_root_;
    JobDirectoryRemover remover_;
    std::chrono::milliseconds retention_;
    std::chrono::milliseconds retry_delay_;
    bool storage_ready_ = false;
    mutable std::mutex mutex_;
    std::optional<SupportBundleJobSnapshot> job_;
    std::filesystem::path job_directory_;
    std::string validated_archive_filename_;
    std::string validated_checksum_filename_;
    std::string validated_sha256_;
    bool download_removed_ = false;
    std::thread worker_;
    struct ExpirationEntry {
        std::string id;
        std::filesystem::path job_directory;
        std::chrono::steady_clock::time_point deadline;
    };
    std::vector<ExpirationEntry> expirations_;
    std::condition_variable expiration_cv_;
    std::thread expiration_worker_;
    bool shutting_down_ = false;
};
