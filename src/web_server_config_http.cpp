/**
 * @file web_server_config_http.cpp
 * @brief Builds configuration and control responses without HTTP transport templates.
 */

#include "web_server_config_http.hpp"

#include "config_handler.hpp"
#include "json.hpp"
#include "logging.hpp"
#include "scheduling.hpp"

#include <string>

namespace web_server_routes
{
RouteResponse build_config_response()
{
    return {200, get_public_config_json().dump(4), "application/json", true};
}

RouteResponse apply_config_update(const std::string &request_body)
{
    try
    {
        nlohmann::json request = nlohmann::json::parse(request_body);
        patch_all_from_web(request);
        return {200, "Ok", "text/plain", false};
    }
    catch (const nlohmann::json::parse_error &error)
    {
        llog.logE(WARN, "Error parsing JSON: ", std::string(error.what()));
        const nlohmann::json body = {
            {"error", "invalid_json"}, {"message", error.what()}};
        return {400, body.dump(4), "application/json", false};
    }
    catch (const ConfigValidationError &error)
    {
        llog.logE(
            WARN, "Configuration update rejected: ",
            std::string(error.what()));
        nlohmann::json body = error.details();
        if (!body.is_object())
            body = nlohmann::json::object();
        body["error"] = "invalid_config";
        body["message"] = error.what();
        return {400, body.dump(4), "application/json", false};
    }
    catch (const std::exception &error)
    {
        llog.logE(
            WARN, "Configuration update rejected: ",
            std::string(error.what()));
        const nlohmann::json body = {
            {"error", "invalid_config"}, {"message", error.what()}};
        return {400, body.dump(4), "application/json", false};
    }
}

RouteResponse apply_config_repair(const std::string &request_body)
{
    try
    {
        const nlohmann::json request = nlohmann::json::parse(request_body);
        if (!request.contains("verb") || !request["verb"].is_string())
        {
            const nlohmann::json body = {
                {"error", "invalid_request"},
                {"message", "Missing or invalid 'verb'."}};
            return {400, body.dump(4), "application/json", true};
        }

        const std::string verb = request["verb"].get<std::string>();
        if (verb == "repair")
            repair_from_web(true);
        else if (verb == "restore")
            repair_from_web(false);
        else
        {
            const nlohmann::json body = {
                {"error", "invalid_verb"},
                {"message", "Verb must be 'repair' or 'restore'."}};
            return {400, body.dump(4), "application/json", true};
        }

        const nlohmann::json body = {
            {"status", "ok"},
            {"message", "Configuration operation completed."}};
        return {200, body.dump(4), "application/json", true};
    }
    catch (const nlohmann::json::parse_error &error)
    {
        llog.logE(WARN, "Error parsing JSON: ", std::string(error.what()));
        const nlohmann::json body = {
            {"error", "invalid_json"}, {"message", error.what()}};
        return {400, body.dump(4), "application/json", true};
    }
}

RouteResponse stop_transmission(const std::string &request_body)
{
    if (request_body.empty())
    {
        const nlohmann::json body = {
            {"error", "invalid_request"},
            {"message", "Request body must explicitly contain {\"command\":\"stop\"}."}};
        return {400, body.dump(4), "application/json", true};
    }

    try
    {
        const nlohmann::json request = nlohmann::json::parse(request_body);
        if (!request.contains("command") ||
            !request["command"].is_string() ||
            request["command"].get<std::string>() != "stop")
        {
            const nlohmann::json body = {
                {"error", "invalid_request"},
                {"message", "Request body must explicitly contain {\"command\":\"stop\"}."}};
            return {400, body.dump(4), "application/json", true};
        }

        const StopTransmissionResult stop_result =
            stop_transmission_by_user_request();
        const bool succeeded = stop_result.transmit_disabled;
        const nlohmann::json body = {
            {"command", "stop"},
            {"status", succeeded ? "ok" : "error"},
            {"transmission_active", stop_result.transmission_active},
            {"stop_performed", stop_result.stop_performed},
            {"transmit_disabled", stop_result.transmit_disabled},
            {"persisted", stop_result.persisted},
            {"message", stop_result.message}};
        return {succeeded ? 200 : 500, body.dump(4), "application/json", true};
    }
    catch (const nlohmann::json::parse_error &error)
    {
        const nlohmann::json body = {
            {"error", "invalid_json"}, {"message", error.what()}};
        return {400, body.dump(4), "application/json", true};
    }
}
} // namespace web_server_routes
