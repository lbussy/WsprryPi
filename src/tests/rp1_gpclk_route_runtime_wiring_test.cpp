#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    assert(input.is_open());
    return {std::istreambuf_iterator<char>(input), {}};
}
}

int main() {
    const auto root = std::filesystem::current_path();
    const auto service = read_file(root / "rp1_gpclk_route_service.cpp");
    const auto service_header = read_file(root / "rp1_gpclk_route_service.hpp");
    const auto http = read_file(root / "web_server.cpp");
    const auto scheduling = read_file(root / "scheduling.cpp");
    const auto arg_parser = read_file(root / "arg_parser.cpp");
    const auto transmit = read_file(
        root / "WSPR-Transmitter/src/rp1_gpclk_transmit_backend.cpp");
    const auto development_contract = read_file(
        root / "WSPR-Transmitter/src/rp1_gpclk_development_policy.hpp");
    const auto installer = read_file(root / "../scripts/install.sh");
    assert(service.find("/run/rp1-gpclk-dkms/route-manager.sock") != std::string::npos);
    assert(service.find("rp1-gpclk-route-manager-v1") != std::string::npos);
    assert(service.find("SOCK_STREAM") != std::string::npos && service.find("FD_CLOEXEC") != std::string::npos);
    assert(service.find("outputInhibitedValidated") != std::string::npos);
    assert(service.find("{\"eligible\", false}") != std::string::npos);
    assert(service.find("apply-and-reboot") != std::string::npos);
    assert(service.find("kRp1GpclkDevelopmentSourceRevision") != std::string::npos);
    assert(development_contract.find("0509909bd916ee738b14a8479d3be47863c6ac72") != std::string::npos);
    assert(service.find("247bd7da35e4ad812a13828668fe03673da127bad7ed2b3e970876f3f21c002d") != std::string::npos);
    assert(service.find("642793e04268ddb06e35f16249d09c98e4067acef93c62620307bbea50033f5a") == std::string::npos);
    assert(service.find("/boot/firmware/config.txt") == std::string::npos);
    assert(service.find("atomic_write_owned_fragment") == std::string::npos);
    assert(service.find("reboot_system()") == std::string::npos);
    assert(http.find("svr->Get(\"/api/rp1-gpclk-route\"") != std::string::npos);
    assert(http.find("svr->Post(\"/api/rp1-gpclk-route\"") != std::string::npos);
    const auto idle_startup = scheduling.find(".reconcileIdleStartup(");
    const auto bounded_request = scheduling.find("if (tone_request.rp1_development.enabled)");
    const auto development_reconciliation =
        scheduling.find(".reconcileDevelopmentStartup(", bounded_request);
    const auto request_commit =
        scheduling.find("commit_execution_request(request)", bounded_request);
    const auto listener_start =
        scheduling.find("const bool start_web =", idle_startup);
    assert(idle_startup != std::string::npos);
    assert(bounded_request != std::string::npos);
    assert(development_reconciliation != std::string::npos);
    assert(request_commit != std::string::npos);
    assert(listener_start != std::string::npos);
    assert(idle_startup < listener_start);
    assert(bounded_request < development_reconciliation);
    assert(development_reconciliation < request_commit);
    const auto direct_confirmation = scheduling.find(
        "apply_direct_rp1_development_confirmation(");
    const auto direct_start = scheduling.find(
        "static bool start_non_wspr_transmission_now(");
    const auto direct_reconciliation = scheduling.find(
        "apply_direct_rp1_development_confirmation(", direct_start);
    const auto direct_runtime_gate = scheduling.find(
        "runtime_transmit_enabled(cfg)", direct_reconciliation);
    const auto direct_commit = scheduling.find(
        "prepare_and_commit_non_wspr_request(", direct_runtime_gate);
    assert(arg_parser.find("rp1-development-confirmation-json") != std::string::npos);
    assert(direct_confirmation != std::string::npos);
    assert(direct_start != std::string::npos);
    assert(direct_reconciliation != std::string::npos);
    assert(direct_runtime_gate != std::string::npos);
    assert(direct_commit != std::string::npos);
    assert(direct_start < direct_reconciliation);
    assert(direct_reconciliation < direct_runtime_gate);
    assert(direct_runtime_gate < direct_commit);
    assert(scheduling.find("physical_connection_confirmed") != std::string::npos);
    assert(scheduling.find("confirmation_operation_id") != std::string::npos);
    assert(scheduling.find("rp1GpclkDevelopmentOperationArmedForRoute(route)") == std::string::npos);
    assert(scheduling.find("Legacy transmission output") == std::string::npos);
    assert(scheduling.find("rp1_gpclk_application_idle_state") != std::string::npos);
    assert(scheduling.find("rp1_live_qualification_inhibited") == std::string::npos);
    assert(transmit.find("decideRp1GpclkDevelopmentUse(current_policy)") != std::string::npos);
    assert(scheduling.find("productionRp1GpclkRouteService().query()") != std::string::npos);
    assert(installer.find("ProxyPass        /wsprrypi/api/rp1-gpclk-route") != std::string::npos);
    assert(service.find("std::system(") == std::string::npos && service.find("popen(") == std::string::npos);
    std::cout << "rp1_gpclk_route_runtime_wiring_test: PASS\n";
}
