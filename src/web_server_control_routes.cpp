/**
 * @file web_server_control_routes.cpp
 * @brief Registers transmission-control HTTP routes.
 */

#include "web_server_routes.hpp"

#include "httplib.hpp"
#include "wtp_runtime_bridge.hpp"
#include "json.hpp"
#include <utility>
#include "web_server_config_http.hpp"

namespace web_server_routes
{
void register_control(
    httplib::Server &server,
    CorsHeaderSetter set_cors_headers)
{
    server.Get("/api/wtp", [](const httplib::Request &, httplib::Response &response) {
        response.set_header("Cache-Control", "no-store");
        response.set_content(wtp_runtime_json(), "application/json");
    });
    server.Post("/api/wtp/recover", [](const httplib::Request &request, httplib::Response &response) {
        response.set_header("Cache-Control", "no-store");
        try {
            const auto type = request.get_header_value("Content-Type");
            if ((type != "application/json" && !type.starts_with("application/json;")) || request.body.size() > 256)
                throw std::runtime_error("Expected a bounded JSON reconciliation request");
            bool key_seen = false;
            const auto body = nlohmann::json::parse(request.body, [&](int, nlohmann::json::parse_event_t event, nlohmann::json &) {
                if (event == nlohmann::json::parse_event_t::key && std::exchange(key_seen, true))
                    throw std::runtime_error("Expected one reconciliation operation");
                return true;
            });
            if (!body.is_object() || body.size() != 1 || body.value("operation", "") != "reconcile")
                throw std::runtime_error("Expected operation reconcile");
            auto result = wtp_runtime_recover();
            response.status = result.ok ? 200 : 409;
            response.set_content(nlohmann::json{{"ok", result.ok}, {"error", result.error},
                {"status", nlohmann::json::parse(wtp_runtime_json())}}.dump(), "application/json");
        } catch (const std::exception &error) {
            response.status = 400;
            response.set_content(nlohmann::json{{"ok",false},{"error",error.what()}}.dump(), "application/json");
        }
    });
    server.Post(
        "/control/stop",
        [set_cors_headers](
            const httplib::Request &request, httplib::Response &response)
        {
            write_route_response(
                response, stop_transmission(request.body), set_cors_headers);
        });
}
} // namespace web_server_routes
