#include "support_bundle_runtime.hpp"

#include "support_bundle_collector_executor.hpp"

#include <array>
#include <cerrno>
#include <stdexcept>
#include <sys/random.h>
#include <utility>

namespace {
constexpr char kHexDigits[] = "0123456789abcdef";
}

std::string SupportBundleRuntime::generate_secure_job_id() {
    std::array<unsigned char, 16> random_bytes{};
    std::size_t offset = 0;

    while (offset < random_bytes.size()) {
        const ssize_t bytes_read = getrandom(random_bytes.data() + offset,
                                             random_bytes.size() - offset,
                                             0);
        if (bytes_read > 0) {
            offset += static_cast<std::size_t>(bytes_read);
            continue;
        }
        if (bytes_read == 0) {
            throw std::runtime_error("secure randomness unavailable");
        }
        if (bytes_read < 0 && errno == EINTR) {
            continue;
        }
        throw std::runtime_error("secure randomness unavailable");
    }

    std::string id;
    id.reserve(random_bytes.size() * 2);
    for (const unsigned char byte : random_bytes) {
        id.push_back(kHexDigits[byte >> 4]);
        id.push_back(kHexDigits[byte & 0x0f]);
    }
    return id;
}

std::unique_ptr<SupportBundleJobManager> SupportBundleRuntime::create_production() {
    SupportBundleRuntimeTestDependencies dependencies;
    dependencies.collector_executable =
        std::string(kSupportBundleProductionCollectorPath);
    dependencies.storage_root = std::string(kSupportBundleProductionStorageRoot);
    dependencies.id_generator = generate_secure_job_id;
    dependencies.timeout = std::chrono::minutes(10);
    dependencies.term_grace = std::chrono::seconds(2);
    dependencies.executor = std::make_shared<SupportBundleCollectorExecutor>(
        dependencies.collector_executable.string(),
        dependencies.timeout,
        dependencies.term_grace);
    return create_for_testing(std::move(dependencies));
}

std::unique_ptr<SupportBundleJobManager> SupportBundleRuntime::create_for_testing(
    SupportBundleRuntimeTestDependencies dependencies) {
    if (!dependencies.executor) {
        dependencies.executor = std::make_shared<SupportBundleCollectorExecutor>(
            dependencies.collector_executable.string(),
            dependencies.timeout,
            dependencies.term_grace);
    }

    return std::make_unique<SupportBundleJobManager>(
        std::move(dependencies.executor),
        std::move(dependencies.id_generator),
        std::move(dependencies.storage_root));
}
