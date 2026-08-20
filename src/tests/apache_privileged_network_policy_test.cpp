#include "../apache_privileged_network_policy.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::size_t count(const std::string &text, const std::string &needle) {
    std::size_t result = 0;
    for (std::size_t position = text.find(needle); position != std::string::npos;
         position = text.find(needle, position + needle.size()))
        ++result;
    return result;
}

std::string read_file(const std::filesystem::path &path) {
    std::ifstream input(path);
    return {std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()};
}
} // namespace

int main(int argc, char **argv) {
    const auto rendered = render_apache_privileged_network_policy({
        {"192.168.50.42", "255.255.255.0"},
        {"192.168.50.10", "255.255.255.0"},
        {"fd00:1234::42", "ffff:ffff:ffff:ffff::"}});
    assert(rendered.valid() && rendered.error.empty());
    const std::string &policy = *rendered.configuration;
    if (argc == 2 && std::string(argv[1]) == "--render") {
        std::cout << policy;
        return 0;
    }
    assert(count(policy, "Require ip 192.168.50.0/24") == 6);
    assert(count(policy, "Require ip fd00:1234::/64") == 6);
    assert(count(policy, "Require local") == 6);
    assert(policy.find("<LocationMatch \"^/wsprrypi/config$\">") != std::string::npos);
    assert(policy.find("<LimitExcept GET>") != std::string::npos);
    assert(policy.find("<LocationMatch \"^/wsprrypi/config/\">") != std::string::npos);
    assert(policy.find("^/wsprrypi/control/stop(?:/|$)") != std::string::npos);
    assert(policy.find("^/wsprrypi/api/support-bundles(?:/|$)") != std::string::npos);
    assert(policy.find("^/wsprrypi/socket(?:/|$)") != std::string::npos);
    assert(policy.find("^/wsprrypi/api/network-safety$") != std::string::npos);
    assert(policy.find("support-intake") == std::string::npos);
    assert(policy.find("/wsprrypi/version") == std::string::npos);
    assert(policy.find("Forwarded") == std::string::npos);
    assert(policy.find("X-Forwarded") == std::string::npos);
    assert(policy.find("RemoteIP") == std::string::npos);
    const auto disabled = render_apache_privileged_network_policy(
        {}, PrivilegedNetworkMode::insecure_disabled);
    assert(disabled.valid());
    assert(*disabled.configuration ==
           "# NETWORK SAFETY OFF\n"
           "# Peer/subnet restrictions are explicitly disabled.\n");
    assert(disabled.configuration->find("Require") == std::string::npos);

    const auto source_root = std::filesystem::current_path();
    const std::string stock_vhost = read_file(source_root / "../config/wsprrypi.conf");
    const std::string installer = read_file(source_root / "../scripts/install.sh");
    assert(stock_vhost.find("ProxyPreserveHost On") != std::string::npos);
    assert(stock_vhost.find("ProxyPass        /wsprrypi/config") != std::string::npos);
    assert(stock_vhost.find("ProxyPass        /wsprrypi/socket") != std::string::npos);
    assert(installer.find("# BEGIN WsprryPi proxy configuration") != std::string::npos);
    assert(installer.find("ProxyPass        /wsprrypi/config") != std::string::npos);
    assert(installer.find("ProxyPass        /wsprrypi/socket") != std::string::npos);
    for (const std::string *source : {&stock_vhost, &installer}) {
        assert(source->find("RequestHeader set Host") == std::string::npos);
        assert(source->find("RequestHeader set Origin") == std::string::npos);
        assert(source->find("RemoteIPHeader") == std::string::npos);
    }

    for (const auto &networks : std::vector<std::vector<SupportLocalNetwork>>{
             {},
             {{"bad", "255.255.255.0"}},
             {{"192.168.50.10", "bad"}},
             {{"192.168.50.10", "0.0.0.0"}},
             {{"192.168.50.10", "255.0.255.0"}},
             {{"127.0.0.1", "255.0.0.0"}},
             {{"224.0.0.1", "255.255.255.0"}},
             {{"::1", "ffff:ffff:ffff:ffff::"}},
             {{"ff02::1", "ffff:ffff:ffff:ffff::"}},
             {{"::ffff:192.168.50.10", "ffff:ffff:ffff:ffff::"}},
             {{"fd00::10", "ffff:0:ffff::"}}}) {
        const auto invalid = render_apache_privileged_network_policy(networks);
        assert(!invalid.valid() && !invalid.error.empty());
    }

    std::cout << "apache_privileged_network_policy_test: PASS\n";
}
