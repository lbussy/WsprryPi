/**
 * @file web_server_control_routes.cpp
 * @brief Registers transmission-control HTTP routes.
 */

#include "web_server_routes.hpp"

#include "httplib.hpp"
#include "wtp_runtime_bridge.hpp"
#include "json.hpp"
#include "config_handler.hpp"
#include "wtp_integration/browser_api.hpp"
#include <utility>
#include "web_server_config_http.hpp"

namespace web_server_routes
{
void register_control(
    httplib::Server &server,
    CorsHeaderSetter set_cors_headers)
{
    static wsprrypi::WtpBrowserApi api(
        [] { return nlohmann::json::parse(wtp_runtime_json()); },
        wtp_runtime_management,
        [](const std::string &job) {
            const auto result = wtp_runtime_cancel_job(job);
            return wsprrypi::PicoHttpResponse{result.ok ? 200U : 409U,
                result.ok ? R"({"cleanup_ok":true})" :
                    nlohmann::json{{"error", {{"code", "not_owner_or_cleanup_unresolved"}}}}.dump(), {}};
        });
    const auto shared = [](const httplib::Request &request, httplib::Response &response) {
        if (request.target != request.path || request.body.size() > 32768 ||
            request.get_header_value_count("Host") != 1 ||
            request.get_header_value_count("Origin") > 1 ||
            request.get_header_value_count("Content-Type") > 1 ||
            request.get_header_value_count("If-Match") > 1 ||
            request.get_header_value_count("X-WsprryPico-Request") > 1) {
            response.status = 400;
            response.set_content(R"({"error":{"code":"invalid_request"}})", "application/json");
            return;
        }
        const auto fetch = request.get_header_value("Sec-Fetch-Site");
        const bool context = request.has_header("Origin") &&
            request.get_header_value("Content-Type") == "application/json" &&
            request.get_header_value("X-WsprryPico-Request") == "1" &&
            (fetch.empty() || fetch == "same-origin" || fetch == "none");
        wsprrypi::PicoHttpResponse result;
        if (request.path == "/api/v1/host/config") {
            try {
                if (request.method == "PUT") {
                    if (!context) result = {403, R"({"error":{"code":"origin_or_content_type"}})", {}};
                    else if (request.get_header_value("If-Match").empty()) result = {428, R"({"error":{"code":"revision_required"}})", {}};
                    else {
                        const auto revision = patch_all_from_web_revision(wsprrypi::strict_browser_json(request.body), request.get_header_value("If-Match"));
                        result = {200, R"({"ok":true})", revision};
                    }
                } else if (request.method == "GET") {
                    auto [config, revision] = get_public_config_snapshot();
                    result = {200, nlohmann::json{{"config", config}, {"scope", "wsprrypi-host-config/1"}}.dump(), revision};
                } else result = {404, R"({"error":{"code":"not_found"}})", {}};
            } catch (const std::exception &error) {
                const bool conflict = std::string(error.what()) == "revision_conflict";
                result = {conflict ? 412U : 400U,
                    nlohmann::json{{"error", {{"code", conflict ? "revision_conflict" : "invalid_config"}}}}.dump(), {}};
            }
        } else result = api.handle({request.method, request.path, request.body, request.get_header_value("If-Match"), context});
        response.status = result.status;
        response.set_header("Cache-Control", "no-store");
        response.set_header("X-Content-Type-Options", "nosniff");
        if (!result.etag.empty()) response.set_header("ETag", result.etag);
        response.headers.erase("Access-Control-Allow-Origin");
        response.set_content(result.body, "application/json");
    };
    server.Get(R"(/api/v1/.*)", shared);
    server.Put(R"(/api/v1/.*)", shared);
    server.Post(R"(/api/v1/.*)", shared);
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
