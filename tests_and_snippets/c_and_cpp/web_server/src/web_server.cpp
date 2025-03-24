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
    // Validate port range for non-root processes.
    if (port < 1024 || port > 49151)
    {
        throw std::invalid_argument("Port must be between 1024 and 49151.");
    }
    {
        std::lock_guard<std::mutex> lock(mtx);
        if (running)
        {
            // Server is already running.
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
        // Set up the OPTIONS handler for CORS preflight.
        svr.Options("/", [this](const httplib::Request &req, httplib::Response &res) {
            setCORSHeaders(res);
            res.set_content("", "text/plain");
        });
        // GET handler: Return a JSON object.
        svr.Get("/", [this](const httplib::Request &req, httplib::Response &res) {
            nlohmann::json jResponse = {{"status", "ok"}, {"message", "Hello, world!"}};
            setCORSHeaders(res);
            res.set_content(jResponse.dump(4), "application/json");
        });
        // Unified handler for PUT and PATCH requests.
        auto handlePutPatch = [this](const httplib::Request &req, httplib::Response &res) {
            try {
                nlohmann::json j = nlohmann::json::parse(req.body);
                std::cout << "Received JSON:" << std::endl << j.dump(4) << std::endl;
                setCORSHeaders(res);
                res.set_content("JSON received", "text/plain");
            } catch (const std::exception &ex) {
                std::cerr << "Error parsing JSON: " << ex.what() << std::endl;
                res.status = 400;
                setCORSHeaders(res);
                res.set_content("Invalid JSON", "text/plain");
            }
        };
        svr.Put("/", handlePutPatch);
        svr.Patch("/", handlePutPatch);

        std::cout << "Starting web server on port " << port_ << "." << std::endl;
#ifdef LOCAL_ONLY
        // Accept from local host only
        svr.listen("127.0.0.1", port_);
#else
        // Accept from any host
        svr.listen("0.0.0.0", port_);
#endif
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
        {
            return;
        }
    }
    svr.stop(); // Signal the HTTP server to stop.
    if (serverThread.joinable())
    {
        serverThread.join(); // Wait for the thread to exit.
    }
    std::cout << "Web server stopped." << std::endl;
}
