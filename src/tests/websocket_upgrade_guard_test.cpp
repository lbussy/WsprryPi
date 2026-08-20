#include "../websocket_upgrade_guard.hpp"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

namespace {
std::string request(std::string host = "wsprrypi:31416",
                    std::string origin = {}) {
    std::string value =
        "GET / HTTP/1.1\r\nHost: " + host +
        "\r\nUpgrade: websocket\r\nConnection: keep-alive, Upgrade\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n";
    if (!origin.empty()) value += "Origin: " + origin + "\r\n";
    return value + "\r\n";
}
} // namespace

int main() {
    const SupportRequestGuardSnapshot snapshot{
        true, "wsprrypi", {}, {{"192.168.50.10", "255.255.255.0"}}};
    const auto evaluate = [&](const std::string &upgrade, const std::string &peer) {
        return evaluate_websocket_upgrade(upgrade, peer, snapshot).decision;
    };

    assert(evaluate(request(), "127.0.0.1") == WebSocketUpgradeGuardDecision::allowed);
    assert(evaluate(request("wsprrypi:31416", "http://wsprrypi:31416"),
                    "192.168.50.42") == WebSocketUpgradeGuardDecision::allowed);
    assert(evaluate(request(), "192.168.51.42") == WebSocketUpgradeGuardDecision::rejected);
    assert(evaluate(request("evil"), "192.168.50.42") ==
           WebSocketUpgradeGuardDecision::rejected);
    assert(evaluate(request("wsprrypi:31416", "null"), "192.168.50.42") ==
           WebSocketUpgradeGuardDecision::rejected);

    std::string forwarded = request();
    forwarded.insert(forwarded.size() - 2,
                     "Forwarded: for=192.168.50.42\r\nX-Forwarded-For: 192.168.50.42\r\n");
    assert(evaluate(forwarded, "192.168.51.42") == WebSocketUpgradeGuardDecision::rejected);

    for (const std::string &malformed : std::vector<std::string>{
             std::string{},
             "POST / HTTP/1.1\r\nHost: wsprrypi\r\n\r\n",
             "GET / HTTP/1.1\r\nUpgrade: websocket\r\n\r\n",
             "GET / HTTP/1.1\r\nHost : wsprrypi\r\nUpgrade: websocket\r\n"
             "Connection: Upgrade\r\nSec-WebSocket-Version: 13\r\n"
             "Sec-WebSocket-Key: key\r\n\r\n",
             request() + "extra",
             "GET / HTTP/1.1\r\nHost: wsprrypi\r\nHost: wsprrypi\r\n"
             "Upgrade: websocket\r\nConnection: Upgrade\r\n"
             "Sec-WebSocket-Version: 13\r\nSec-WebSocket-Key: key\r\n\r\n"}) {
        assert(evaluate(malformed, "127.0.0.1") ==
               WebSocketUpgradeGuardDecision::malformed);
    }

    std::cout << "websocket_upgrade_guard_test: PASS\n";
}
