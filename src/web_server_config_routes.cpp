/**
 * @file web_server_config_routes.cpp
 * @brief Registers configuration HTTP routes.
 */

#include "web_server_routes.hpp"

#include "httplib.hpp"
#include "web_server_config_http.hpp"

#include <utility>

namespace web_server_routes
{
void write_route_response(
    httplib::Response &response,
    RouteResponse result,
    const CorsHeaderSetter &set_cors_headers)
{
    if (result.allow_cors)
        set_cors_headers(response);
    else
        response.headers.erase("Access-Control-Allow-Origin");
    response.status = result.status;
    response.set_content(std::move(result.body), result.content_type);
}

void register_config(
    httplib::Server &server,
    CorsHeaderSetter set_cors_headers)
{
    server.Options(
        R"(/(.*))",
        [set_cors_headers](
            const httplib::Request &, httplib::Response &response)
        {
            set_cors_headers(response);
            response.set_content("", "text/plain");
        });

    server.Get(
        "/",
        [set_cors_headers](
            const httplib::Request &, httplib::Response &response)
        {
            set_cors_headers(response);
            response.set_content(
                "Wsprry Pi webserver is running.", "text/plain");
        });

    server.Get(
        "/config",
        [set_cors_headers](
            const httplib::Request &, httplib::Response &response)
        {
            write_route_response(
                response, build_config_response(), set_cors_headers);
        });

    const auto handle_put_patch =
        [set_cors_headers](
            const httplib::Request &request, httplib::Response &response)
        {
            write_route_response(
                response, apply_config_update(request.body), set_cors_headers);
        };
    server.Put("/config", handle_put_patch);
    server.Patch("/config", handle_put_patch);

    server.Post(
        "/config/repair",
        [set_cors_headers](
            const httplib::Request &request, httplib::Response &response)
        {
            write_route_response(
                response, apply_config_repair(request.body), set_cors_headers);
        });
}
} // namespace web_server_routes
