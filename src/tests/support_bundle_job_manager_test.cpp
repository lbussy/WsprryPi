#include "support_bundle_job_manager.hpp"
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <filesystem>
#include <fstream>
#include "json.hpp"
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

class FakeExecutor final : public SupportBundleJobExecutor {
public:
    enum class ResultMode { valid, none, multiple, symlink, oversized, malformed, schema_invalid, inconsistent };
    std::mutex mutex; std::condition_variable cv; bool entered=false, released=false, cancelled=false, fail=false, throw_exception=false; bool probe=false; int calls=0;
    ResultMode mode = ResultMode::valid;
    std::filesystem::path directory;
    std::filesystem::path symlink_target;
    SupportBundleExecutionResult run(const SupportBundleExecutionContext &context) override {
        const bool requested = context.probe_i2c; directory = context.job_directory;
        std::unique_lock lock(mutex); ++calls; probe=requested; entered=true; cv.notify_all();
        cv.wait(lock, [&] { return released || cancelled; });
        if (throw_exception) throw 1;
        if (cancelled) return {true, {}, {}}; // Manager must preserve cancellation over this late success.
        if (!fail && !throw_exception && mode != ResultMode::none) {
            nlohmann::json result={{"schema_version",1},{"status","success"},{"archive_filename","WsprryPi-support-test.tar.gz"},{"sha256_filename","WsprryPi-support-test.tar.gz.sha256"},{"sha256",std::string(64,'A')},{"generated_at_utc","20260101T000000Z"},{"configuration_files_included",true},{"full_logs_included",false},{"i2c_probe_requested",probe},{"i2c_probe_status",probe?"succeeded":"skipped_by_user"},{"privileged_diagnostics_may_be_incomplete",false}};
            if (mode == ResultMode::schema_invalid) result["schema_version"] = 2;
            if (mode == ResultMode::inconsistent) result["i2c_probe_status"] = probe ? "skipped_by_user" : "succeeded";
            const auto path = context.job_directory / "WsprryPi-support-test.tar.gz.result.json";
            if (mode == ResultMode::symlink) {
                const auto target = symlink_target.empty() ? std::filesystem::path("/tmp") : symlink_target;
                symlink(target.c_str(), path.c_str());
            }
            else if (mode == ResultMode::oversized) std::ofstream(path) << std::string(65537, 'x');
            else if (mode == ResultMode::malformed) std::ofstream(path) << "{";
            else { std::ofstream(path) << result.dump(); if (mode == ResultMode::multiple) std::ofstream(context.job_directory / "WsprryPi-support-two.tar.gz.result.json") << result.dump(); }
        }
        return fail ? SupportBundleExecutionResult{false, "raw:/secret", "/secret/token"} : SupportBundleExecutionResult{true, {}, {}};
    }
    void request_stop() noexcept override { std::lock_guard lock(mutex); cancelled=true; cv.notify_all(); }
    void wait_entered() { std::unique_lock lock(mutex); assert(cv.wait_for(lock, std::chrono::seconds(2), [&] { return entered; })); }
    void release() { std::lock_guard lock(mutex); released=true; cv.notify_all(); }
    void reset() { std::lock_guard lock(mutex); entered=false; released=false; cancelled=false; }
    bool was_cancelled() { std::lock_guard lock(mutex); return cancelled; }
};

static void wait_terminal(SupportBundleJobManager &manager, const std::string &id) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        const auto state = manager.lookup(id)->state;
        if (state == SupportBundleJobState::succeeded || state == SupportBundleJobState::failed) return;
        std::this_thread::yield();
    }
    assert(false);
}
static void wait_removed(const std::filesystem::path &path) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (std::chrono::steady_clock::now() < deadline) {
        if (!std::filesystem::exists(path)) return;
        std::this_thread::yield();
    }
    assert(false);
}
static std::string id(char c) { return std::string(32, c); }
static void assert_no_download_metadata(const SupportBundleDownloadReference &reference) {
    assert(reference.archive_path.empty());
    assert(reference.archive_basename.empty());
    assert(reference.checksum_path.empty());
    assert(reference.checksum_basename.empty());
    assert(reference.expected_sha256.empty());
}
static void expect_storage_failure(const std::filesystem::path &root, const std::string &expected) {
    auto executor = std::make_shared<FakeExecutor>();
    SupportBundleJobManager manager(executor, [] { return id('z'); }, root);
    std::string error;
    assert(!manager.create({}, error) && error == expected && executor->calls == 0);
}
static void expect_result_failure(const std::filesystem::path &root, FakeExecutor::ResultMode mode, const std::string &category, char token) {
    auto executor = std::make_shared<FakeExecutor>(); executor->mode = mode;
    std::atomic<int> cleanup_result{-1};
    SupportBundleJobManager manager(
        executor, [token] { return id(token); }, root,
        [&cleanup_result](const std::filesystem::path &storage_root, const std::string &job_id) {
            const auto result = remove_support_bundle_job_directory(storage_root, job_id);
            cleanup_result.store(static_cast<int>(result.failure));
            return result;
        });
    std::string error;
    const auto job = manager.create({}, error); executor->wait_entered(); executor->release(); wait_terminal(manager, job->id);
    const auto snapshot = manager.lookup(job->id); assert(snapshot->state == SupportBundleJobState::failed && snapshot->failure_category == category && snapshot->failure_message == "Support collection failed." && snapshot->i2c_probe_status.empty() && !snapshot->download_available);
    const auto reference = manager.download_reference(job->id);
    assert(reference.status == SupportBundleDownloadReferenceStatus::no_download);
    assert_no_download_metadata(reference);
    wait_removed(root / job->id);
    const auto cleanup_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (cleanup_result.load() == -1 && std::chrono::steady_clock::now() < cleanup_deadline) {
        std::this_thread::yield();
    }
    assert(cleanup_result.load() ==
           static_cast<int>(SupportBundleJobDirectoryRemovalFailure::none));
    manager.shutdown();
}

int main() {
    char template_path[] = "/tmp/wsprrypi-job-manager-test.XXXXXX";
    assert(mkdtemp(template_path) != nullptr);
    const std::filesystem::path root = template_path;
    assert(chmod(root.c_str(), 0700) == 0);
    const auto sibling = root / id('p');
    assert(std::filesystem::create_directory(sibling));
    assert(chmod(sibling.c_str(), 0700) == 0);
    { std::ofstream sentinel(sibling / "keep"); sentinel << "sentinel"; }
    expect_storage_failure("relative-root", "storage_unavailable");
    expect_storage_failure(root / "missing", "storage_unavailable");
    const auto link_root = root / "root-link"; assert(symlink(root.c_str(), link_root.c_str()) == 0); expect_storage_failure(link_root, "storage_unavailable");
    for (const mode_t mode : {0750, 0070, 0770, 0777}) { const auto invalid = root / ("mode-" + std::to_string(mode)); assert(std::filesystem::create_directory(invalid)); assert(chmod(invalid.c_str(), mode) == 0); expect_storage_failure(invalid, "storage_unavailable"); assert(chmod(invalid.c_str(), 0700) == 0); }
    std::string error;
    auto success = std::make_shared<FakeExecutor>();
    int generated = 0; SupportBundleJobManager manager(success, [&] { return id(generated++ == 0 ? 'a' : 'g'); }, root);
    const auto first = manager.create({true}, error); assert(first && error.empty() && !first->download_available);
    const auto queued_reference = manager.download_reference(first->id);
    assert(queued_reference.status == SupportBundleDownloadReferenceStatus::not_ready);
    assert_no_download_metadata(queued_reference);
    assert(manager.delete_download("bad").status ==
           SupportBundleDownloadDeletionStatus::malformed_or_unknown_id);
    assert(manager.delete_download(first->id).status ==
           SupportBundleDownloadDeletionStatus::not_terminal);
    success->wait_entered(); struct stat job_info{}; assert(success->probe && success->directory == root / first->id && manager.lookup(first->id)->i2c_probe_status.empty() && lstat(success->directory.c_str(), &job_info) == 0 && S_ISDIR(job_info.st_mode) && (job_info.st_mode & 0777) == 0700 && manager.lookup(first->id)->state == SupportBundleJobState::running);
    const auto running_reference = manager.download_reference(first->id);
    assert(running_reference.status == SupportBundleDownloadReferenceStatus::not_ready);
    assert_no_download_metadata(running_reference);
    assert(!manager.create({}, error) && error == "job_active"); assert(manager.lookup(first->id)); assert(!manager.lookup("bad"));
    success->release(); wait_terminal(manager, first->id); assert(manager.lookup(first->id)->state == SupportBundleJobState::succeeded && manager.lookup(first->id)->i2c_probe_status == "succeeded" && manager.lookup(first->id)->download_available && std::filesystem::exists(success->directory));
    const auto ready = manager.download_reference(first->id);
    assert(ready.status == SupportBundleDownloadReferenceStatus::available);
    assert(ready.archive_path == root / first->id / "WsprryPi-support-test.tar.gz");
    assert(ready.archive_basename == "WsprryPi-support-test.tar.gz");
    assert(ready.checksum_path == root / first->id / "WsprryPi-support-test.tar.gz.sha256");
    assert(ready.checksum_basename == "WsprryPi-support-test.tar.gz.sha256");
    assert(ready.expected_sha256 == std::string(64, 'a'));
    const auto public_snapshot = manager.lookup(first->id); assert(public_snapshot->failure_message.find("WsprryPi-support-test.tar.gz") == std::string::npos && public_snapshot->failure_message.find(root.string()) == std::string::npos && public_snapshot->i2c_probe_status == "succeeded");
    assert(manager.delete_download(id('q')).status ==
           SupportBundleDownloadDeletionStatus::malformed_or_unknown_id);
    assert(manager.delete_download(first->id).status ==
           SupportBundleDownloadDeletionStatus::removed);
    const auto deleted_snapshot = manager.lookup(first->id);
    assert(deleted_snapshot->state == SupportBundleJobState::succeeded &&
           !deleted_snapshot->download_available &&
           deleted_snapshot->failure_category.empty() && deleted_snapshot->failure_message.empty() &&
           !std::filesystem::exists(root / first->id));
    const auto deleted_reference = manager.download_reference(first->id);
    assert(deleted_reference.status == SupportBundleDownloadReferenceStatus::no_download);
    assert_no_download_metadata(deleted_reference);
    assert(manager.delete_download(first->id).status ==
           SupportBundleDownloadDeletionStatus::already_removed);
    const auto malformed_reference = manager.download_reference("bad");
    assert(malformed_reference.status == SupportBundleDownloadReferenceStatus::malformed_or_unknown_id);
    assert_no_download_metadata(malformed_reference);
    const auto unknown_reference = manager.download_reference(id('q'));
    assert(unknown_reference.status == SupportBundleDownloadReferenceStatus::malformed_or_unknown_id);
    assert_no_download_metadata(unknown_reference);
    success->reset(); const auto next = manager.create({false}, error); assert(next); success->wait_entered(); assert(!success->probe); success->release(); wait_terminal(manager,next->id); assert(manager.lookup(next->id)->i2c_probe_status == "skipped_by_user" && manager.lookup(next->id)->download_available); manager.shutdown();

    const auto existing_dir = root / id('z'); assert(std::filesystem::create_directory(existing_dir)); expect_storage_failure(root, "job_setup_failed"); std::filesystem::remove(existing_dir);
    const auto existing_file = root / id('z'); { std::ofstream output(existing_file); output << "x"; } expect_storage_failure(root, "job_setup_failed"); std::filesystem::remove(existing_file);
    const auto existing_link = root / id('z'); assert(symlink(root.c_str(), existing_link.c_str()) == 0); expect_storage_failure(root, "job_setup_failed"); std::filesystem::remove(existing_link);

    expect_result_failure(root, FakeExecutor::ResultMode::none, "result_missing", 'h');
    expect_result_failure(root, FakeExecutor::ResultMode::multiple, "result_ambiguous", 'i');
    expect_result_failure(root, FakeExecutor::ResultMode::symlink, "result_unsafe", 'j');
    expect_result_failure(root, FakeExecutor::ResultMode::oversized, "result_oversized", 'k');
    expect_result_failure(root, FakeExecutor::ResultMode::malformed, "result_invalid", 'l');
    expect_result_failure(root, FakeExecutor::ResultMode::schema_invalid, "result_invalid", 'm');
    expect_result_failure(root, FakeExecutor::ResultMode::inconsistent, "result_inconsistent", 'n');
    assert(std::filesystem::exists(sibling / "keep"));

    char external_template[] = "/tmp/wsprrypi-job-manager-cleanup-external.XXXXXX";
    assert(mkdtemp(external_template) != nullptr);
    const std::filesystem::path external = external_template;
    assert(chmod(external.c_str(), 0700) == 0);
    { std::ofstream external_sentinel(external / "keep"); external_sentinel << "sentinel"; }
    auto symlink_failure = std::make_shared<FakeExecutor>();
    symlink_failure->mode = FakeExecutor::ResultMode::symlink;
    symlink_failure->symlink_target = external;
    SupportBundleJobManager external_safe(symlink_failure, [] { return id('s'); }, root);
    const auto external_job = external_safe.create({}, error);
    symlink_failure->wait_entered();
    symlink_failure->release();
    wait_terminal(external_safe, external_job->id);
    wait_removed(root / external_job->id);
    assert(std::filesystem::exists(external / "keep"));
    external_safe.shutdown();
    std::filesystem::remove_all(external);

    auto failing = std::make_shared<FakeExecutor>(); failing->fail=true;
    SupportBundleJobManager failed(failing, [] { return id('b'); }, root); auto f = failed.create({}, error); failing->wait_entered(); failing->release(); wait_terminal(failed, f->id); wait_removed(root / f->id); const auto fs=failed.lookup(f->id); assert(fs->state==SupportBundleJobState::failed && fs->failure_category=="collector_failed" && fs->failure_message=="Support collection failed." && !fs->download_available); const auto failed_reference = failed.download_reference(f->id); assert(failed_reference.status == SupportBundleDownloadReferenceStatus::no_download); assert_no_download_metadata(failed_reference); assert(failed.delete_download(f->id).status == SupportBundleDownloadDeletionStatus::no_retained_download); failed.shutdown();
    auto throwing = std::make_shared<FakeExecutor>(); throwing->throw_exception=true;
    SupportBundleJobManager exception(throwing, [] { return id('c'); }, root); auto e=exception.create({},error); throwing->wait_entered(); throwing->release(); wait_terminal(exception,e->id); wait_removed(root / e->id); assert(exception.lookup(e->id)->failure_category=="executor_exception"); exception.shutdown();
    auto malformed = std::make_shared<FakeExecutor>(); SupportBundleJobManager bad_id(malformed, [] { return "bad"; }, root); assert(!bad_id.create({},error) && error=="invalid_job_id" && malformed->calls==0);
    auto throwing_id = std::make_shared<FakeExecutor>(); SupportBundleJobManager id_error(throwing_id, []()->std::string { throw 1; }, root); assert(!id_error.create({},error) && error=="id_generation_failed" && throwing_id->calls==0);
    std::string high=id('d'); high[0]=static_cast<char>(0x80); assert(!SupportBundleJobManager::valid_id(high));
    auto cancelling = std::make_shared<FakeExecutor>(); SupportBundleJobManager cancel(cancelling, [] { return id('e'); }, root); auto c=cancel.create({},error); cancelling->wait_entered(); cancel.shutdown(); assert(cancelling->was_cancelled()); const auto cs=cancel.lookup(c->id); assert(cs->state==SupportBundleJobState::failed && cs->failure_category=="shutting_down" && !cs->download_available && !std::filesystem::exists(root / c->id)); const auto cancelled_reference = cancel.download_reference(c->id); assert(cancelled_reference.status == SupportBundleDownloadReferenceStatus::no_download); assert_no_download_metadata(cancelled_reference); assert(cancel.delete_download(c->id).status == SupportBundleDownloadDeletionStatus::no_retained_download); assert(!cancel.create({},error) && error=="shutting_down"); cancel.shutdown();
    std::string destroyed_id;
    { auto destructor_executor=std::make_shared<FakeExecutor>(); SupportBundleJobManager destruct(destructor_executor, [] { return id('f'); }, root); const auto d = destruct.create({},error); destroyed_id = d->id; destructor_executor->wait_entered(); }
    assert(!std::filesystem::exists(root / destroyed_id));
    std::atomic<int> cleanup_calls = 0;
    auto cleanup_failure_executor = std::make_shared<FakeExecutor>();
    cleanup_failure_executor->mode = FakeExecutor::ResultMode::none;
    SupportBundleJobManager cleanup_failure(
        cleanup_failure_executor, [] { return id('r'); }, root,
        [&cleanup_calls](const std::filesystem::path &, const std::string &) {
            ++cleanup_calls;
            return SupportBundleJobDirectoryRemovalResult{
                SupportBundleJobDirectoryRemovalFailure::removal_failed};
        });
    const auto cleanup_job = cleanup_failure.create({}, error);
    cleanup_failure_executor->wait_entered();
    cleanup_failure_executor->release();
    wait_terminal(cleanup_failure, cleanup_job->id);
    const auto cleanup_snapshot = cleanup_failure.lookup(cleanup_job->id);
    const auto cleanup_deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (cleanup_calls.load() == 0 && std::chrono::steady_clock::now() < cleanup_deadline) {
        std::this_thread::yield();
    }
    assert(cleanup_calls.load() == 1 && cleanup_snapshot->state == SupportBundleJobState::failed &&
           cleanup_snapshot->failure_category == "result_missing" &&
           cleanup_snapshot->failure_message == "Support collection failed." &&
           !cleanup_snapshot->download_available &&
           std::filesystem::exists(root / cleanup_job->id));
    cleanup_failure.shutdown();
    std::filesystem::remove_all(root / cleanup_job->id);

    std::atomic<int> delete_cleanup_calls = 0;
    auto delete_retry_executor = std::make_shared<FakeExecutor>();
    SupportBundleJobManager delete_retry(
        delete_retry_executor, [] { return id('t'); }, root,
        [&delete_cleanup_calls](const std::filesystem::path &storage_root,
                                const std::string &job_id) {
            if (++delete_cleanup_calls == 1) {
                return SupportBundleJobDirectoryRemovalResult{
                    SupportBundleJobDirectoryRemovalFailure::removal_failed};
            }
            return remove_support_bundle_job_directory(storage_root, job_id);
        });
    const auto retry_job = delete_retry.create({}, error);
    delete_retry_executor->wait_entered();
    delete_retry_executor->release();
    wait_terminal(delete_retry, retry_job->id);
    assert(delete_retry.download_reference(retry_job->id).status ==
           SupportBundleDownloadReferenceStatus::available);
    assert(delete_retry.delete_download(retry_job->id).status ==
           SupportBundleDownloadDeletionStatus::cleanup_failed);
    assert(delete_retry.download_reference(retry_job->id).status ==
           SupportBundleDownloadReferenceStatus::available &&
           delete_retry.lookup(retry_job->id)->download_available &&
           std::filesystem::exists(root / retry_job->id));
    assert(delete_retry.delete_download(retry_job->id).status ==
           SupportBundleDownloadDeletionStatus::removed);
    assert(delete_cleanup_calls.load() == 2 &&
           delete_retry.download_reference(retry_job->id).status ==
               SupportBundleDownloadReferenceStatus::no_download);
    delete_retry.shutdown();

    auto concurrent_executor = std::make_shared<FakeExecutor>();
    SupportBundleJobManager concurrent(concurrent_executor, [] { return id('u'); }, root);
    const auto concurrent_job = concurrent.create({}, error);
    concurrent_executor->wait_entered();
    concurrent_executor->release();
    wait_terminal(concurrent, concurrent_job->id);
    std::atomic<bool> reader_started = false;
    std::thread reader([&] {
        reader_started = true;
        for (int iteration = 0; iteration < 64; ++iteration) {
            assert(concurrent.lookup(concurrent_job->id));
            (void)concurrent.download_reference(concurrent_job->id);
        }
    });
    while (!reader_started.load()) std::this_thread::yield();
    assert(concurrent.delete_download(concurrent_job->id).status ==
           SupportBundleDownloadDeletionStatus::removed);
    reader.join();
    concurrent.shutdown();
    assert(std::filesystem::exists(sibling / "keep"));
    std::filesystem::remove_all(root);
    std::cout << "support_bundle_job_manager_test: PASS\n";
}
