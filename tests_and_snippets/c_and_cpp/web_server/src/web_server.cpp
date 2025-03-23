#include "web_server.hpp"

WebServer::WebServer() : port_(0), running(false) {}

WebServer::~WebServer()
{
    stop();
}

void WebServer::setCORSHeaders(httplib::Response &res)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, PUT, PATCH, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

void WebServer::start(int port)
{
    // For non-root processes, ports below 1024 are not allowed.
    if (port < 1024 || port > 49151)
    {
        throw std::invalid_argument("Port must be between 1024 and 49151");
    }
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (running)
        {
            // Server already running; nothing to do.
            return;
        }
        port_ = port;
    }

    // Launch the server in a separate thread.
    serverThread = std::thread([this]()
                               {
        {
            std::lock_guard<std::mutex> lock(mtx);
            running = true;
            cvStarted.notify_one();
        }

        // Set up OPTIONS handler for CORS preflight.
        svr.Options("/", [this](const httplib::Request &req, httplib::Response &res) {
            setCORSHeaders(res);
            res.set_content("", "text/plain");
        });

        // GET request handler.
        svr.Get("/", [this](const httplib::Request &req, httplib::Response &res) {
            json jResponse = {{"status", "ok"}, {"message", "Hello, world!"}};
            setCORSHeaders(res);
            res.set_content(jResponse.dump(4), "application/json");
        });

        // PUT request handler.
        svr.Put("/", [this](const httplib::Request &req, httplib::Response &res) {
            try {
                json j = json::parse(req.body);
                std::cout << "Received PUT JSON:" << std::endl << j.dump(4) << std::endl;
                setCORSHeaders(res);
                res.set_content("PUT request received", "text/plain");
            } catch (const std::exception &ex) {
                std::cerr << "Error parsing PUT JSON: " << ex.what() << std::endl;
                res.status = 400;
                setCORSHeaders(res);
                res.set_content("Invalid JSON in PUT request", "text/plain");
            }
        });

        // PATCH request handler.
        svr.Patch("/", [this](const httplib::Request &req, httplib::Response &res) {
            try {
                json j = json::parse(req.body);
                std::cout << "Received PATCH JSON:" << std::endl << j.dump(4) << std::endl;
                setCORSHeaders(res);
                res.set_content("PATCH request received", "text/plain");
            } catch (const std::exception &ex) {
                std::cerr << "Error parsing PATCH JSON: " << ex.what() << std::endl;
                res.status = 400;
                setCORSHeaders(res);
                res.set_content("Invalid JSON in PATCH request", "text/plain");
            }
        });

        std::cout << "Starting web server on port " << port_ << " ..." << std::endl;
        svr.listen("0.0.0.0", port_);
        {
            std::lock_guard<std::mutex> lock(mtx);
            running = false;
        } });

    // Wait until the server thread signals that it has started.
    {
        std::unique_lock<std::mutex> lock(mtx);
        cvStarted.wait(lock, [this]
                       { return running; });
    }
    std::cout << "Web server started on port " << port_ << std::endl;
}

void WebServer::stop()
{
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (!running)
            return; // Not running, nothing to do.
    }
    svr.stop(); // Signal the HTTP server to stop.
    if (serverThread.joinable())
    {
        serverThread.join(); // Wait for the server thread to finish.
    }
    std::cout << "Web server stopped." << std::endl;
}
