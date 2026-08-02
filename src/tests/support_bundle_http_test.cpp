#include "support_bundle_http.hpp"

#include "json.hpp"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;
using namespace std::chrono_literals;

namespace {
std::string job_id(char value) {
    return std::string(32, value);
}

class FakeExecutor final : public SupportBundleJobExecutor {
public:
    enum class Mode { success, failure };

    SupportBundleExecutionResult run(const SupportBundleExecutionContext &context) override {
        std::unique_lock lock(mutex_);
        context_ = context;
        entered_ = true;
        entered_condition_.notify_all();
        release_condition_.wait(lock, [this] { return released_ || stopped_; });
        if (stopped_) {
            return {false, "stopped", "raw executor output /secret"};
        }
        if (mode_ == Mode::failure) {
            return {false, "raw:/secret", "raw executor output /secret"};
        }

        const bool probe = context.probe_i2c;
        const nlohmann::json result = {
            {"schema_version", 1},
            {"status", "success"},
            {"archive_filename", "WsprryPi-support-test.tar.gz"},
            {"sha256_filename", "WsprryPi-support-test.tar.gz.sha256"},
            {"sha256", std::string(64, 'a')},
            {"generated_at_utc", "20260101T000000Z"},
            {"configuration_files_included", false},
            {"full_logs_included", false},
            {"i2c_probe_requested", probe},
            {"i2c_probe_status", probe ? "succeeded" : "skipped_by_user"},
            {"privileged_diagnostics_may_be_incomplete", false},
        };
        std::ofstream(context.job_directory / "WsprryPi-support-test.tar.gz.result.json")
            << result.dump();
        return {true, {}, {}};
    }

    void request_stop() noexcept override {
        std::lock_guard lock(mutex_);
        stopped_ = true;
        release_condition_.notify_all();
    }

    void wait_entered() {
        std::unique_lock lock(mutex_);
        assert(entered_condition_.wait_for(lock, 2s, [this] { return entered_; }));
    }

    void release(Mode mode) {
        std::lock_guard lock(mutex_);
        mode_ = mode;
        entered_ = false;
        released_ = true;
        release_condition_.notify_all();
    }

    void reset() {
        std::lock_guard lock(mutex_);
        released_ = false;
        stopped_ = false;
    }

private:
    std::mutex mutex_;
    std::condition_variable entered_condition_;
    std::condition_variable release_condition_;
    SupportBundleExecutionContext context_;
    bool entered_ = false;
    bool released_ = false;
    bool stopped_ = false;
    Mode mode_ = Mode::success;
};

const httplib::Response &require_response(const httplib::Result &result) {
    assert(result);
    return result.value();
}

nlohmann::json response_json(const httplib::Response &response) {
    return nlohmann::json::parse(response.body);
}

void assert_restrictive(const httplib::Response &response) {
    assert(!response.has_header("Access-Control-Allow-Origin"));
    assert(!response.has_header("Access-Control-Allow-Methods"));
    assert(!response.has_header("Access-Control-Allow-Headers"));
}

httplib::Headers local_headers(int port) {
    return {{"Host", "127.0.0.1:" + std::to_string(port)},
            {"Origin", "http://127.0.0.1:" + std::to_string(port)}};
}

httplib::Response get_status(httplib::Client &client,
                             const httplib::Headers &headers,
                             const std::string &id) {
    return require_response(client.Get("/api/support-bundles/" + id, headers));
}

void wait_for_state(httplib::Client &client,
                    const httplib::Headers &headers,
                    const std::string &id,
                    const char *expected) {
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (std::chrono::steady_clock::now() < deadline) {
        const auto response = get_status(client, headers, id);
        if (response.status == 200 && response_json(response)["state"] == expected) {
            return;
        }
        std::this_thread::sleep_for(5ms);
    }
    assert(false);
}
}  // namespace

int main() {
    char template_path[] = "/tmp/wsprrypi-support-bundle-http-test.XXXXXX";
    assert(mkdtemp(template_path) != nullptr);
    const fs::path storage_root(template_path);
    assert(chmod(storage_root.c_str(), 0700) == 0);

    const auto executor = std::make_shared<FakeExecutor>();
    const auto provider_calls = std::make_shared<int>(0);
    int id_index = 0;
    SupportBundleJobManager manager(executor, [&] { return job_id(static_cast<char>('a' + id_index++)); }, storage_root);
    httplib::Server server;
    server.set_default_headers({{"Access-Control-Allow-Origin", "*"},
                                {"Access-Control-Allow-Methods", "GET, POST"},
                                {"Access-Control-Allow-Headers", "Content-Type"}});
    register_support_bundle_http_routes(server, manager, [provider_calls] {
        ++*provider_calls;
        return SupportRequestGuardSnapshot{true, "test-pi", {}, {}};
    });
    const int port = server.bind_to_any_port("127.0.0.1");
    assert(port > 0);
    std::thread server_thread([&] { server.listen_after_bind(); });
    server.wait_until_ready();
    httplib::Client client("127.0.0.1", port);
    const auto headers = local_headers(port);

    const auto created = require_response(client.Post(
        "/api/support-bundles", headers, "{}", "ApPlIcAtIoN/JsOn ; ChArSeT = UtF-8"));
    assert(created.status == 202);
    assert(*provider_calls == 1);
    assert_restrictive(created);
    const auto created_body = response_json(created);
    const std::string first_id = created_body["id"];
    assert(first_id == job_id('a'));
    assert(created_body["state"] == "queued");
    assert(created_body["probe_i2c_requested"] == false);
    assert(created_body["i2c_probe_status"] == "");
    assert(!created_body["download_available"]);

    executor->wait_entered();
    wait_for_state(client, headers, first_id, "running");
    const int calls_before_later_request = *provider_calls;
    const auto retained_provider = get_status(client, headers, first_id);
    assert(retained_provider.status == 200);
    assert(*provider_calls == calls_before_later_request + 1);
    const auto conflict = require_response(client.Post("/api/support-bundles", headers, "{\"probe_i2c\":true}", "application/json"));
    assert(conflict.status == 409);
    assert_restrictive(conflict);

    const std::vector<std::tuple<std::string, std::string, int>> invalid_requests = {
        {"{", "application/json", 400},
        {"{}", "text/plain", 415},
        {"{\"unknown\":true}", "application/json", 400},
        {"{\"probe_i2c\":\"true\"}", "application/json", 400},
        {std::string(1025, 'x'), "application/json", 413},
        {"{}", "application/json; charset=iso-8859-1", 415},
        {"{}", "application/json; boundary=test", 415},
        {"{}", "application/json; charset", 415},
    };
    for (const auto &[body, content_type, expected_status] : invalid_requests) {
        const auto response = require_response(client.Post("/api/support-bundles", headers, body, content_type));
        assert(response.status == expected_status);
        assert_restrictive(response);
    }

    executor->release(FakeExecutor::Mode::success);
    wait_for_state(client, headers, first_id, "succeeded");
    const auto succeeded = get_status(client, headers, first_id);
    assert(response_json(succeeded)["i2c_probe_status"] == "skipped_by_user");

    executor->reset();
    const auto probe_created = require_response(client.Post("/api/support-bundles", headers, "{\"probe_i2c\":true}", "application/json"));
    assert(probe_created.status == 202);
    const std::string probe_id = response_json(probe_created)["id"];
    executor->wait_entered();
    executor->release(FakeExecutor::Mode::success);
    wait_for_state(client, headers, probe_id, "succeeded");
    assert(response_json(get_status(client, headers, probe_id))["i2c_probe_status"] == "succeeded");

    executor->reset();
    const auto failed_created = require_response(client.Post("/api/support-bundles", headers, "{}", "application/json"));
    const std::string failed_id = response_json(failed_created)["id"];
    executor->wait_entered();
    executor->release(FakeExecutor::Mode::failure);
    wait_for_state(client, headers, failed_id, "failed");
    const auto failed = get_status(client, headers, failed_id);
    const auto failed_body = response_json(failed);
    assert(failed_body["failure_category"] == "collector_failed");
    assert(failed.body.find("/secret") == std::string::npos);
    assert(failed.body.find(storage_root.string()) == std::string::npos);
    assert(!failed_body["download_available"]);

    const auto malformed = require_response(client.Get("/api/support-bundles/not-an-id", headers));
    assert(malformed.status == 404);
    assert_restrictive(malformed);
    const auto unknown = require_response(client.Get("/api/support-bundles/" + job_id('z'), headers));
    assert(unknown.status == 404);
    assert_restrictive(unknown);
    const auto listing = require_response(client.Get("/api/support-bundles", headers));
    assert(listing.status == 404);
    assert_restrictive(listing);

    const httplib::Headers foreign_origin{{"Host", "127.0.0.1:" + std::to_string(port)},
                                          {"Origin", "http://example.invalid"}};
    const auto foreign = require_response(client.Post("/api/support-bundles", foreign_origin, "{}", "application/json"));
    assert(foreign.status == 403);
    assert_restrictive(foreign);
    const httplib::Headers null_origin{{"Host", "127.0.0.1:" + std::to_string(port)}, {"Origin", "null"}};
    assert(require_response(client.Post("/api/support-bundles", null_origin, "{}", "application/json")).status == 403);
    const httplib::Headers bad_host{{"Host", "example.invalid"}, {"X-Forwarded-For", "127.0.0.1"}};
    assert(require_response(client.Post("/api/support-bundles", bad_host, "{}", "application/json")).status == 403);
    const auto preflight = require_response(client.Options("/api/support-bundles", foreign_origin));
    assert(preflight.status == 403);
    assert_restrictive(preflight);

    server.stop();
    server_thread.join();
    manager.shutdown();
    fs::remove_all(storage_root);
    std::cout << "support_bundle_http_test: PASS\n";
}
