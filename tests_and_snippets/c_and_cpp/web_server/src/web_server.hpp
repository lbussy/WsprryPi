#ifndef WEB_SERVER_HPP
#define WEB_SERVER_HPP

#include "httplib.h" // From cpp-httplib (https://github.com/yhirose/cpp-httplib)
#include "json.hpp"  // From nlohmann/json (https://github.com/nlohmann/json)
#include <condition_variable>
#include <mutex>
#include <thread>
#include <stdexcept>
#include <iostream>
#include <string>

using json = nlohmann::json;

/**
 * @class WebServer
 * @brief A simple web server that runs in a separate thread with CORS support.
 *
 * @details
 * This class creates a web server that does not start until start(port) is called.
 * It listens on the specified port (e.g., between 1024 and 65535) and handles:
 *  - GET requests by returning a JSON object.
 *  - PUT/PATCH requests by parsing the request body as JSON, debug printing it,
 *    and returning a text response.
 *  - OPTIONS requests for CORS preflight, setting the appropriate headers.
 *
 * The server runs in its own thread and exits cleanly when stop() is called,
 * using condition variables to coordinate startup and shutdown.
 */
class WebServer
{
public:
    /**
     * @brief Default constructor.
     *
     * The server does not start automatically. Call start(port) to launch it.
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
     * Validates the port (must be between 1024 and 65535 for non-root processes),
     * launches the server in a separate thread, and waits until the server is running.
     *
     * @param port The port number to listen on.
     * @throws std::invalid_argument if the port is out of range.
     */
    void start(int port);

    /**
     * @brief Stops the web server.
     *
     * @details
     * Signals the server to stop and waits (using a condition variable) for the server thread
     * to exit cleanly.
     */
    void stop();

private:
    int port_;                         ///< Port on which the server listens.
    httplib::Server svr;               ///< The underlying HTTP server from cpp-httplib.
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
