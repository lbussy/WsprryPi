#include "support_bundle_runtime.hpp"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <set>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;

namespace {
class FakeExecutor final : public SupportBundleJobExecutor {
public:
    SupportBundleExecutionResult run(const SupportBundleExecutionContext &) override {
        ++runs;
        return {false, "test", "test"};
    }

    void request_stop() noexcept override {}

    int runs = 0;
};

bool is_lowercase_hex_id(const std::string &value) {
    if (value.size() != 32) {
        return false;
    }
    for (const unsigned char character : value) {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f'))) {
            return false;
        }
    }
    return true;
}

bool within_web_root(const fs::path &path) {
    const fs::path web_root("/var/www");
    const fs::path normalized = path.lexically_normal();
    auto web_part = web_root.begin();
    for (auto path_part = normalized.begin();
         web_part != web_root.end() && path_part != normalized.end();
         ++web_part, ++path_part) {
        if (*web_part != *path_part) {
            return false;
        }
    }
    return web_part == web_root.end();
}
}  // namespace

int main() {
    const fs::path collector_path{std::string(kSupportBundleProductionCollectorPath)};
    const fs::path storage_root{std::string(kSupportBundleProductionStorageRoot)};
    assert(collector_path.is_absolute());
    assert(storage_root.is_absolute());
    assert(!within_web_root(collector_path));
    assert(!within_web_root(storage_root));

    std::set<std::string> ids;
    for (int index = 0; index < 128; ++index) {
        const std::string id = SupportBundleRuntime::generate_secure_job_id();
        assert(is_lowercase_hex_id(id));
        assert(ids.insert(id).second);
    }

    char template_path[] = "/tmp/wsprrypi-runtime-test.XXXXXX";
    assert(mkdtemp(template_path) != nullptr);
    const fs::path temporary_root(template_path);
    assert(chmod(temporary_root.c_str(), 0700) == 0);

    const auto fake_executor = std::make_shared<FakeExecutor>();
    SupportBundleRuntimeTestDependencies dependencies;
    dependencies.collector_executable = "/test/collector";
    dependencies.storage_root = temporary_root;
    dependencies.id_generator = [] { return std::string(32, 'a'); };
    dependencies.executor = fake_executor;
    const auto manager = SupportBundleRuntime::create_for_testing(std::move(dependencies));
    assert(manager);
    assert(fake_executor->runs == 0);
    assert(!manager->lookup(std::string(32, 'a')));

    const bool production_storage_existed = fs::exists(storage_root);
    const auto production_manager = SupportBundleRuntime::create_production();
    assert(production_manager);
    assert(fs::exists(storage_root) == production_storage_existed);

    fs::remove_all(temporary_root);
    std::cout << "support_bundle_runtime_test: PASS\n";
}
