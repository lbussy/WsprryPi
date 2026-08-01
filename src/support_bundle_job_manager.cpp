#include "support_bundle_job_manager.hpp"
#include <algorithm>
#include <cctype>

SupportBundleJobManager::SupportBundleJobManager(std::shared_ptr<SupportBundleJobExecutor> executor, IdGenerator ids) : executor_(std::move(executor)), ids_(std::move(ids)) {}
SupportBundleJobManager::~SupportBundleJobManager() { shutdown(); }
bool SupportBundleJobManager::valid_id(const std::string &id) { return id.size() == 32 && std::all_of(id.begin(), id.end(), [](unsigned char c) { return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_'; }); }
std::optional<SupportBundleJobSnapshot> SupportBundleJobManager::create(SupportBundleJobRequest request, std::string &error) {
    std::lock_guard lock(mutex_);
    error.clear();
    if (shutting_down_) { error = "shutting_down"; return std::nullopt; }
    if (job_ && (job_->state == SupportBundleJobState::queued || job_->state == SupportBundleJobState::running)) { error = "job_active"; return std::nullopt; }
    std::string id;
    try { id = ids_(); } catch (...) { error = "id_generation_failed"; return std::nullopt; }
    if (!valid_id(id)) { error = "invalid_job_id"; return std::nullopt; }
    if (worker_.joinable()) worker_.join();
    job_ = SupportBundleJobSnapshot{id, SupportBundleJobState::queued, request.probe_i2c, "", "", false};
    try { worker_ = std::thread(&SupportBundleJobManager::run, this, id, request.probe_i2c); }
    catch (const std::system_error &) { job_.reset(); error = "worker_launch_failed"; return std::nullopt; }
    return job_;
}
std::optional<SupportBundleJobSnapshot> SupportBundleJobManager::lookup(const std::string &id) const { std::lock_guard lock(mutex_); if (!valid_id(id) || !job_ || job_->id != id) return std::nullopt; return job_; }
void SupportBundleJobManager::run(std::string id, bool probe_i2c) {
    { std::lock_guard lock(mutex_); if (!job_ || job_->id != id) return; job_->state = SupportBundleJobState::running; }
    SupportBundleExecutionResult result;
    try { result = executor_->run(probe_i2c); } catch (...) { result = {false, "executor_exception", "Support collection failed."}; }
    std::lock_guard lock(mutex_); if (!job_ || job_->id != id || job_->state == SupportBundleJobState::failed) return;
    if (shutting_down_) result = {false, "shutting_down", "Support collection stopped."};
    job_->state = result.succeeded ? SupportBundleJobState::succeeded : SupportBundleJobState::failed;
    job_->failure_category = result.succeeded ? "" : (result.failure_category == "shutting_down" ? "shutting_down" : (result.failure_category == "executor_exception" ? "executor_exception" : "collector_failed"));
    job_->failure_message = result.succeeded ? "" : (job_->failure_category == "shutting_down" ? "Support collection stopped." : "Support collection failed.");
}
void SupportBundleJobManager::shutdown() {
    { std::lock_guard lock(mutex_); shutting_down_ = true; }
    executor_->request_stop();
    if (worker_.joinable()) worker_.join();
}
