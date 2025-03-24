#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include "httplib.hpp" // From cpp-httplib: https://github.com/yhirose/cpp-httplib
#include "json.hpp"    // From nlohmann/json: https://github.com/nlohmann/json
#include <condition_variable>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <iostream>
#include <string>

/**
 * @class WebServer
 * @brief A simple web server that runs in a separate thread with CORS support.
 *
 * @details
 * The WebServer class creates an HTTP server that listens on a user-specified port.
 * It will not start until the start() method is called with a valid port number (1024 to 65535).
 * The server handles:
 *  - GET requests by sending a JSON response.
 *  - PUT and PATCH requests by parsing the request body as JSON, printing it for debugging,
 *    and returning a text response.
 *  - OPTIONS requests to support CORS preflight, setting the proper headers.
 *
 * The server runs in its own thread and can be stopped cleanly by calling stop(), which uses
 * condition variables for synchronization.
 */
class WebServer
{
public:
    /**
     * @brief Constructs a WebServer instance.
     *
     * The server does not start automatically. Call start(port) to launch the server.
     */
    WebServer();

    /**
     * @brief Destructor.
     *
     * Ensures that the server is stopped and the thread is joined.
     */
    ~WebServer();

    /**
     * @brief Starts the web server on the specified port.
     *
     * @details
     * Validates the port, launches the HTTP server in a separate thread, and waits until
     * the server is running.
     *
     * @param port The port number to listen on (must be between 1024 and 65535).
     * @throws std::invalid_argument if the port is outside the allowed range.
     */
    void start(int port);

    /**
     * @brief Stops the web server.
     *
     * @details
     * Signals the server to stop and waits (using condition variables) for the server thread
     * to exit cleanly.
     */
    void stop();

private:
    int port_;                         ///< Port on which the server listens.
    httplib::Server svr;               ///< Underlying HTTP server from cpp-httplib.
    std::thread serverThread;          ///< Thread running the HTTP server.
    std::mutex mtx;                    ///< Mutex for synchronizing start/stop operations.
    std::condition_variable cvStarted; ///< Condition variable to signal when the server has started.
    bool running;                      ///< Flag indicating whether the server is running.

    /**
     * @brief Sets the CORS headers on the given HTTP response.
     *
     * @param res The HTTP response to modify.
     */
    void setCORSHeaders(httplib::Response &res);
};

#endif // WEB_SERVER_HPP
