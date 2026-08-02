#include "support_bundle_job_manager.hpp"
#include "support_bundle_result_validator.hpp"
#include <algorithm>
#include <cctype>
#include <sys/stat.h>
#include <unistd.h>

SupportBundleJobManager::SupportBundleJobManager(std::shared_ptr<SupportBundleJobExecutor> executor, IdGenerator ids, std::filesystem::path storage_root) : executor_(std::move(executor)), ids_(std::move(ids)) {
    struct stat info{}; if (!storage_root.is_absolute() || lstat(storage_root.c_str(), &info) != 0 || S_ISLNK(info.st_mode) || !S_ISDIR(info.st_mode) || info.st_uid != geteuid() || (info.st_mode & 0777) != 0700) return;
    std::error_code error; storage_root_ = std::filesystem::canonical(storage_root, error); storage_ready_ = !error;
}
SupportBundleJobManager::~SupportBundleJobManager() { shutdown(); }
bool SupportBundleJobManager::valid_id(const std::string &id) { return id.size() == 32 && std::all_of(id.begin(), id.end(), [](unsigned char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_'; }); }
std::optional<SupportBundleJobSnapshot> SupportBundleJobManager::create(SupportBundleJobRequest request, std::string &error) {
    std::lock_guard lock(mutex_);
    error.clear();
    if (shutting_down_) { error = "shutting_down"; return std::nullopt; }
    if (!storage_ready_) { error = "storage_unavailable"; return std::nullopt; }
    if (job_ && (job_->state == SupportBundleJobState::queued || job_->state == SupportBundleJobState::running)) { error = "job_active"; return std::nullopt; }
    std::string id;
    try { id = ids_(); } catch (...) { error = "id_generation_failed"; return std::nullopt; }
    if (!valid_id(id)) { error = "invalid_job_id"; return std::nullopt; }
    const auto directory = storage_root_ / id;
    struct stat existing{}; if (lstat(directory.c_str(), &existing) == 0 || mkdir(directory.c_str(), 0700) != 0) { error = "job_setup_failed"; return std::nullopt; }
    if (lstat(directory.c_str(), &existing) != 0 || !S_ISDIR(existing.st_mode) || S_ISLNK(existing.st_mode) || (existing.st_mode & 0777) != 0700) { error = "job_setup_failed"; return std::nullopt; }
    if (worker_.joinable()) worker_.join();
    job_ = SupportBundleJobSnapshot{id, SupportBundleJobState::queued, request.probe_i2c, "", "", "", false};
    try { worker_ = std::thread(&SupportBundleJobManager::run, this, id, request.probe_i2c, directory); }
    catch (const std::system_error &) { job_.reset(); error = "worker_launch_failed"; return std::nullopt; }
    return job_;
}
std::optional<SupportBundleJobSnapshot> SupportBundleJobManager::lookup(const std::string &id) const { std::lock_guard lock(mutex_); if (!valid_id(id) || !job_ || job_->id != id) return std::nullopt; return job_; }
void SupportBundleJobManager::run(std::string id, bool probe_i2c, std::filesystem::path job_directory) {
    { std::lock_guard lock(mutex_); if (!job_ || job_->id != id) return; job_->state = SupportBundleJobState::running; }
    SupportBundleExecutionResult result;
    try { result = executor_->run({probe_i2c, job_directory}); } catch (...) { result = {false, "executor_exception", "Support collection failed."}; }
    SupportBundleResultValidation validation;
    if (result.succeeded) validation = validate_support_bundle_result(job_directory, probe_i2c);
    std::lock_guard lock(mutex_); if (!job_ || job_->id != id || job_->state == SupportBundleJobState::failed) return;
    if (shutting_down_) result = {false, "shutting_down", "Support collection stopped."};
    if (result.succeeded && !validation.valid) {
        result.succeeded = false;
        switch (validation.failure) {
        case SupportBundleResultFailure::missing: result.failure_category = "result_missing"; break;
        case SupportBundleResultFailure::ambiguous: result.failure_category = "result_ambiguous"; break;
        case SupportBundleResultFailure::unsafe_file: result.failure_category = "result_unsafe"; break;
        case SupportBundleResultFailure::oversized: result.failure_category = "result_oversized"; break;
        case SupportBundleResultFailure::inconsistent: result.failure_category = "result_inconsistent"; break;
        default: result.failure_category = "result_invalid"; break;
        }
    }
    job_->state = result.succeeded ? SupportBundleJobState::succeeded : SupportBundleJobState::failed;
    job_->i2c_probe_status = result.succeeded ? validation.i2c_probe_status : "";
    job_->failure_category = result.succeeded ? "" : (result.failure_category.starts_with("result_") || result.failure_category == "shutting_down" || result.failure_category == "executor_exception" ? result.failure_category : "collector_failed");
    job_->failure_message = result.succeeded ? "" : (job_->failure_category == "shutting_down" ? "Support collection stopped." : "Support collection failed.");
}
void SupportBundleJobManager::shutdown() {
    { std::lock_guard lock(mutex_); shutting_down_ = true; }
    executor_->request_stop();
    if (worker_.joinable()) worker_.join();
}
