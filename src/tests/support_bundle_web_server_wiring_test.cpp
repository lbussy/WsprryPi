#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
std::string read_file(const std::filesystem::path &path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::size_t count(const std::string &text, const std::string &needle) {
    std::size_t matches = 0;
    for (std::size_t position = text.find(needle); position != std::string::npos;
         position = text.find(needle, position + needle.size())) {
        ++matches;
    }
    return matches;
}
}  // namespace

int main() {
    const std::filesystem::path source_root = std::filesystem::current_path();
    const std::string source = read_file(source_root / "web_server.cpp");
    const std::string header = read_file(source_root / "web_server.hpp");

    const std::size_t construction = source.find("SupportBundleRuntime::create_production()");
    const std::size_t registration = source.find("register_support_bundle_http_routes(");
    assert(construction != std::string::npos && registration != std::string::npos);
    assert(construction < registration);
    assert(count(source, "SupportBundleRuntime::create_production()") == 1);
    assert(count(source, "register_support_bundle_http_routes(") == 1);
    assert(source.find("SupportRequestGuard::discover_local_networks()") != std::string::npos);
    assert(count(source, "resolve_support_bundle_intake_production") == 1);
    assert(source.find("resolve_support_bundle_intake_production);") != std::string::npos);

    const std::size_t stop = source.find("svr->stop();");
    const std::size_t join = source.find("serverThread.join();");
    const std::size_t shutdown = source.find("supportBundleJobManager_->shutdown();");
    assert(stop != std::string::npos && join != std::string::npos && shutdown != std::string::npos);
    assert(stop < join && join < shutdown);
    assert(header.find("std::unique_ptr<SupportBundleJobManager> supportBundleJobManager_") <
           header.find("std::unique_ptr<httplib::Server> svr"));

    const std::size_t manager_shutdown = source.find("supportBundleJobManager_->shutdown();");
    const std::size_t server_reset = source.find("svr.reset();");
    const std::size_t manager_reset = source.find("supportBundleJobManager_.reset();");
    assert(manager_shutdown < server_reset && server_reset < manager_reset);
    assert(source.find("svr = std::make_unique<httplib::Server>();") != std::string::npos);
    assert(source.find("supportBundleRoutesRegistered_ = false;") != std::string::npos);

    assert(source.find("svr->listen(\"0.0.0.0\", port_);") != std::string::npos);
    assert(count(source, "->listen(") == 1);
    assert(source.find("svr->Get(\"/config\"") != std::string::npos);
    assert(source.find("svr->Get(\"/version\"") != std::string::npos);
    assert(source.find("create_directory") == std::string::npos);
    assert(source.find("supportBundleJobManager_->create(") == std::string::npos);

    std::cout << "support_bundle_web_server_wiring_test: PASS\n";
}
