#ifndef WEBSOCKET_HPP
#define WEBSOCKET_HPP

#include <string>
#include <cstdint>
#include <atomic>
#include <thread>
#include <mutex>

class WebSocketServer
{
public:
    WebSocketServer();
    ~WebSocketServer();

    // Start listening on the specified port (1024-49151) and set the keep-alive interval (in seconds)
    bool start(uint16_t port, uint32_t keep_alive_secs = 30);

    // Stop the server and close any active connection.
    void stop();

    // Send a text message (as a WebSocket frame) to the connected client.
    void send_to_client(const std::string &message);

private:
    int listen_fd_;
    std::atomic<bool> running_;
    int client_sock_;
    uint32_t keep_alive_secs_;

    std::thread server_thread_;
    std::thread keep_alive_thread_;
    std::mutex client_mutex_;

    // Internal loops and handshake function.
    void server_loop();
    void keep_alive_loop(uint32_t interval);
    bool perform_handshake(int client);
};

#endif // WEBSOCKET_HPP
