#include "support_bundle_job_manager.hpp"
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>

class FakeExecutor final : public SupportBundleJobExecutor {
public:
    std::mutex mutex; std::condition_variable cv; bool entered=false, released=false, cancelled=false, fail=false, throw_exception=false; bool probe=false; int calls=0;
    SupportBundleExecutionResult run(bool requested) override {
        std::unique_lock lock(mutex); ++calls; probe=requested; entered=true; cv.notify_all();
        cv.wait(lock, [&] { return released || cancelled; });
        if (throw_exception) throw 1;
        if (cancelled) return {true, {}, {}}; // Manager must preserve cancellation over this late success.
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
static std::string id(char c) { return std::string(32, c); }

int main() {
    std::string error;
    auto success = std::make_shared<FakeExecutor>();
    SupportBundleJobManager manager(success, [] { return id('a'); });
    const auto first = manager.create({true}, error); assert(first && error.empty() && !first->download_available);
    success->wait_entered(); assert(success->probe && manager.lookup(first->id)->state == SupportBundleJobState::running);
    assert(!manager.create({}, error) && error == "job_active"); assert(manager.lookup(first->id)); assert(!manager.lookup("bad"));
    success->release(); wait_terminal(manager, first->id); assert(manager.lookup(first->id)->state == SupportBundleJobState::succeeded && !manager.lookup(first->id)->download_available);
    success->reset(); const auto next = manager.create({false}, error); assert(next); success->wait_entered(); assert(!success->probe); manager.shutdown();

    auto failing = std::make_shared<FakeExecutor>(); failing->fail=true;
    SupportBundleJobManager failed(failing, [] { return id('b'); }); auto f = failed.create({}, error); failing->wait_entered(); failing->release(); wait_terminal(failed, f->id); const auto fs=failed.lookup(f->id); assert(fs->state==SupportBundleJobState::failed && fs->failure_category=="collector_failed" && fs->failure_message=="Support collection failed." && !fs->download_available); failed.shutdown();
    auto throwing = std::make_shared<FakeExecutor>(); throwing->throw_exception=true;
    SupportBundleJobManager exception(throwing, [] { return id('c'); }); auto e=exception.create({},error); throwing->wait_entered(); throwing->release(); wait_terminal(exception,e->id); assert(exception.lookup(e->id)->failure_category=="executor_exception"); exception.shutdown();
    auto malformed = std::make_shared<FakeExecutor>(); SupportBundleJobManager bad_id(malformed, [] { return "bad"; }); assert(!bad_id.create({},error) && error=="invalid_job_id" && malformed->calls==0);
    auto throwing_id = std::make_shared<FakeExecutor>(); SupportBundleJobManager id_error(throwing_id, []()->std::string { throw 1; }); assert(!id_error.create({},error) && error=="id_generation_failed" && throwing_id->calls==0);
    std::string high=id('d'); high[0]=static_cast<char>(0x80); assert(!SupportBundleJobManager::valid_id(high));
    auto cancelling = std::make_shared<FakeExecutor>(); SupportBundleJobManager cancel(cancelling, [] { return id('e'); }); auto c=cancel.create({},error); cancelling->wait_entered(); cancel.shutdown(); assert(cancelling->was_cancelled()); const auto cs=cancel.lookup(c->id); assert(cs->state==SupportBundleJobState::failed && cs->failure_category=="shutting_down" && !cs->download_available); assert(!cancel.create({},error) && error=="shutting_down"); cancel.shutdown();
    { auto destructor_executor=std::make_shared<FakeExecutor>(); SupportBundleJobManager destruct(destructor_executor, [] { return id('f'); }); assert(destruct.create({},error)); destructor_executor->wait_entered(); }
    std::cout << "support_bundle_job_manager_test: PASS\n";
}
