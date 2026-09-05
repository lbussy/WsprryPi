/**
 * @file web_server_control_routes.cpp
 * @brief Registers transmission-control HTTP routes.
 */

#include "web_server_routes.hpp"

#include "httplib.hpp"
#include "web_server_config_http.hpp"

namespace web_server_routes
{
void register_control(
    httplib::Server &server,
    CorsHeaderSetter set_cors_headers)
{
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
