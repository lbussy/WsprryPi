#include "web_socket.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <thread>
#include <vector>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

// Connect with wscat -c ws://localhost:31416

// ---------------------
// Minimal Base64 Encoder
// ---------------------
std::string base64_encode(const unsigned char *data, size_t len)
{
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string encoded;
    encoded.reserve(((len + 2) / 3) * 4);
    for (size_t i = 0; i < len; i += 3)
    {
        int val = data[i] << 16;
        if (i + 1 < len)
        {
            val |= data[i + 1] << 8;
        }
        if (i + 2 < len)
        {
            val |= data[i + 2];
        }
        encoded.push_back(table[(val >> 18) & 0x3F]);
        encoded.push_back(table[(val >> 12) & 0x3F]);
        encoded.push_back(i + 1 < len ? table[(val >> 6) & 0x3F] : '=');
        encoded.push_back(i + 2 < len ? table[val & 0x3F] : '=');
    }
    return encoded;
}

// ---------------------
// Minimal SHA1 Implementation
// ---------------------
class SHA1
{
public:
    SHA1() : h0(0x67452301), h1(0xEFCDAB89), h2(0x98BADCFE),
             h3(0x10325476), h4(0xC3D2E1F0), buffer_len(0), total_len(0) {}

    void update(const std::string &s)
    {
        update(reinterpret_cast<const unsigned char *>(s.c_str()), s.size());
    }

    void update(const unsigned char *data, size_t len)
    {
        total_len += len;
        size_t i = 0;
        if (buffer_len > 0)
        {
            size_t to_copy = std::min(len, size_t(64 - buffer_len));
            std::memcpy(buffer + buffer_len, data, to_copy);
            buffer_len += to_copy;
            i += to_copy;
            if (buffer_len == 64)
            {
                process_chunk(buffer);
                buffer_len = 0;
            }
        }
        for (; i + 64 <= len; i += 64)
        {
            process_chunk(data + i);
        }
        if (i < len)
        {
            buffer_len = len - i;
            std::memcpy(buffer, data + i, buffer_len);
        }
    }

    // Finalize the hash and return the 20-byte result.
    std::string final()
    {
        uint64_t total_bits = total_len * 8;
        // Append the '1' bit
        buffer[buffer_len++] = 0x80;
        // Pad with zeros until length is 56 bytes mod 64
        if (buffer_len > 56)
        {
            while (buffer_len < 64)
                buffer[buffer_len++] = 0;
            process_chunk(buffer);
            buffer_len = 0;
        }
        while (buffer_len < 56)
            buffer[buffer_len++] = 0;
        // Append the 64-bit length in big-endian
        for (int i = 7; i >= 0; i--)
        {
            buffer[buffer_len++] = (total_bits >> (i * 8)) & 0xFF;
        }
        process_chunk(buffer);

        unsigned char hash[20];
        auto write_uint32 = [&](uint32_t val, int offset)
        {
            hash[offset] = (val >> 24) & 0xFF;
            hash[offset + 1] = (val >> 16) & 0xFF;
            hash[offset + 2] = (val >> 8) & 0xFF;
            hash[offset + 3] = val & 0xFF;
        };
        write_uint32(h0, 0);
        write_uint32(h1, 4);
        write_uint32(h2, 8);
        write_uint32(h3, 12);
        write_uint32(h4, 16);
        return std::string(reinterpret_cast<char *>(hash), 20);
    }

private:
    uint32_t h0, h1, h2, h3, h4;
    unsigned char buffer[64];
    size_t buffer_len;
    uint64_t total_len;

    static uint32_t leftrotate(uint32_t value, unsigned int bits)
    {
        return (value << bits) | (value >> (32 - bits));
    }

    void process_chunk(const unsigned char chunk[64])
    {
        uint32_t w[80];
        for (int i = 0; i < 16; i++)
        {
            w[i] = (chunk[i * 4] << 24) | (chunk[i * 4 + 1] << 16) | (chunk[i * 4 + 2] << 8) | (chunk[i * 4 + 3]);
        }
        for (int i = 16; i < 80; i++)
        {
            uint32_t temp = w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16];
            w[i] = leftrotate(temp, 1);
        }
        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
        for (int i = 0; i < 80; i++)
        {
            uint32_t f, k;
            if (i < 20)
            {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            }
            else if (i < 40)
            {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            }
            else if (i < 60)
            {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            }
            else
            {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = leftrotate(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = leftrotate(b, 30);
            b = a;
            a = temp;
        }
        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }
};

// Compute the "Sec-WebSocket-Accept" value from the client's key.
std::string compute_websocket_accept(const std::string &client_key)
{
    const std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string concatenated = client_key + magic;
    SHA1 sha;
    sha.update(concatenated);
    std::string sha1_result = sha.final(); // 20-byte binary hash
    return base64_encode(reinterpret_cast<const unsigned char *>(sha1_result.data()), sha1_result.size());
}

// ---------------------
// Helper: Decode a WebSocket Frame
// ---------------------
namespace
{
    std::string decode_websocket_frame(const char *data, size_t data_len, size_t &frame_size, uint8_t &opcode)
    {
        if (data_len < 2)
        {
            frame_size = 0;
            return "";
        }
        uint8_t byte1 = static_cast<uint8_t>(data[0]);
        opcode = byte1 & 0x0F;
        // bool fin = byte1 & 0x80;
        uint8_t byte2 = static_cast<uint8_t>(data[1]);
        bool mask = byte2 & 0x80;
        uint64_t payload_length = byte2 & 0x7F;
        size_t pos = 2;
        if (payload_length == 126)
        {
            if (data_len < pos + 2)
            {
                frame_size = 0;
                return "";
            }
            payload_length = (static_cast<uint8_t>(data[pos]) << 8) | static_cast<uint8_t>(data[pos + 1]);
            pos += 2;
        }
        else if (payload_length == 127)
        {
            if (data_len < pos + 8)
            {
                frame_size = 0;
                return "";
            }
            payload_length = 0;
            for (int i = 0; i < 8; i++)
            {
                payload_length = (payload_length << 8) | static_cast<uint8_t>(data[pos + i]);
            }
            pos += 8;
        }
        if (mask)
        {
            if (data_len < pos + 4)
            {
                frame_size = 0;
                return "";
            }
            uint8_t masking_key[4];
            masking_key[0] = data[pos];
            masking_key[1] = data[pos + 1];
            masking_key[2] = data[pos + 2];
            masking_key[3] = data[pos + 3];
            pos += 4;
            if (data_len < pos + payload_length)
            {
                frame_size = 0;
                return "";
            }
            std::string message;
            message.resize(payload_length);
            for (uint64_t i = 0; i < payload_length; i++)
            {
                message[i] = data[pos + i] ^ masking_key[i % 4];
            }
            frame_size = pos + payload_length;
            return message;
        }
        else
        {
            // Unmasked frame (unlikely from a client)
            if (data_len < pos + payload_length)
            {
                frame_size = 0;
                return "";
            }
            std::string message(data + pos, payload_length);
            frame_size = pos + payload_length;
            return message;
        }
    }
} // anonymous namespace

// ---------------------
// WebSocketServer Implementation
// ---------------------
WebSocketServer::WebSocketServer()
    : listen_fd_(-1), running_(false), client_sock_(-1), keep_alive_secs_(30) {}

WebSocketServer::~WebSocketServer()
{
    stop();
}

bool WebSocketServer::start(uint16_t port, uint32_t keep_alive_secs) {
    if (port < 1024 || port > 49151) {
        std::cerr << "Port must be between 1024 and 49151." << std::endl;
        return false;
    }
    keep_alive_secs_ = keep_alive_secs;

    // Create an IPv6 socket.
    listen_fd_ = socket(AF_INET6, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        std::perror("socket");
        return false;
    }

    // Allow reuse of the address.
    int opt = 1;
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::perror("setsockopt");
        return false;
    }

    // Disable IPV6_V6ONLY to accept both IPv6 and IPv4 connections.
    int off = 0;
    if (setsockopt(listen_fd_, IPPROTO_IPV6, IPV6_V6ONLY, &off, sizeof(off)) < 0) {
        std::perror("setsockopt IPV6_V6ONLY");
        // Not a fatal error on some systems, so you may continue.
    }

    // Bind to the unspecified IPv6 address (this accepts all addresses)
    struct sockaddr_in6 addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin6_family = AF_INET6;
    addr.sin6_addr = in6addr_any; // Accept connections from any IPv6 (and IPv4 via mapped addresses)
    addr.sin6_port = htons(port);
    if (bind(listen_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::perror("bind");
        return false;
    }

    if (listen(listen_fd_, 10) < 0) {
        std::perror("listen");
        return false;
    }

    running_ = true;
    server_thread_ = std::thread(&WebSocketServer::server_loop, this);
    if (keep_alive_secs_ > 0) {
        keep_alive_thread_ = std::thread(&WebSocketServer::keep_alive_loop, this, keep_alive_secs_);
    }

    std::cout << "WebSocketServer started on port " << port << std::endl;
    return true;
}

void WebSocketServer::stop()
{
    running_ = false;
    if (listen_fd_ != -1)
    {
        close(listen_fd_);
        listen_fd_ = -1;
    }
    {
        std::lock_guard<std::mutex> lock(client_mutex_);
        if (client_sock_ != -1)
        {
            close(client_sock_);
            client_sock_ = -1;
        }
    }
    if (server_thread_.joinable())
        server_thread_.join();
    if (keep_alive_thread_.joinable())
        keep_alive_thread_.join();
    std::cout << "WebSocketServer stopped." << std::endl;
}

void WebSocketServer::send_to_client(const std::string &message)
{
    std::lock_guard<std::mutex> lock(client_mutex_);
    if (client_sock_ == -1)
    {
        std::cerr << "No client connected." << std::endl;
        return;
    }
    std::string frame;
    // FIN bit set and opcode 0x1 for text frame.
    frame.push_back(static_cast<char>(0x81));
    size_t len = message.size();
    if (len < 126)
    {
        frame.push_back(static_cast<char>(len));
    }
    else if (len < 65536)
    {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    }
    else
    {
        frame.push_back(127);
        for (int i = 7; i >= 0; i--)
        {
            frame.push_back((len >> (8 * i)) & 0xFF);
        }
    }
    frame.append(message);
    ssize_t sent = send(client_sock_, frame.data(), frame.size(), 0);
    if (sent < 0)
        std::perror("send");
}

void WebSocketServer::server_loop()
{
    while (running_)
    {
        sockaddr_in client_addr;
        socklen_t addrlen = sizeof(client_addr);
        int client = accept(listen_fd_, reinterpret_cast<sockaddr *>(&client_addr), &addrlen);
        if (client < 0)
        {
            if (running_)
                std::perror("accept");
            break;
        }
#ifdef LOCAL_ONLY
        // Verify that the client is connecting from localhost.
        if (std::string(inet_ntoa(client_addr.sin_addr)) != "127.0.0.1")
        {
            std::cerr << "Rejected connection from non-localhost: "
                      << inet_ntoa(client_addr.sin_addr) << std::endl;
            close(client);
            continue;
        }
#endif // LOCAL_ONLY
        std::cout << "Client connected." << std::endl;
        if (!perform_handshake(client))
        {
            std::cerr << "Handshake failed." << std::endl;
            close(client);
            continue;
        }
        {
            std::lock_guard<std::mutex> lock(client_mutex_);
            client_sock_ = client;
        }
        // Read and decode client messages.
        char buf[1024];
        while (running_)
        {
            ssize_t bytes = recv(client, buf, sizeof(buf), 0);
            if (bytes <= 0)
                break;
            size_t pos = 0;
            while (pos < static_cast<size_t>(bytes))
            {
                size_t frame_size = 0;
                uint8_t opcode = 0;
                std::string message = decode_websocket_frame(buf + pos, bytes - pos, frame_size, opcode);
                if (frame_size == 0)
                    break; // incomplete frame
                pos += frame_size;
                if (opcode == 0x1)
                {
                    std::cout << "Received message: " << message << std::endl;
                }
                else
                {
                    std::cout << "Received non-text frame (opcode: " << static_cast<int>(opcode) << ")" << std::endl;
                }
            }
        }
        std::cout << "Client disconnected." << std::endl;
        {
            std::lock_guard<std::mutex> lock(client_mutex_);
            close(client);
            client_sock_ = -1;
        }
    }
}

void WebSocketServer::keep_alive_loop(uint32_t interval)
{
    while (running_)
    {
        std::this_thread::sleep_for(std::chrono::seconds(interval));
        std::lock_guard<std::mutex> lock(client_mutex_);
        if (client_sock_ != -1)
        {
            // Send a ping frame: FIN set and opcode 0x9.
            unsigned char ping[2] = {0x89, 0x00};
            if (send(client_sock_, ping, 2, 0) < 0)
                std::perror("send ping");
        }
    }
}

bool WebSocketServer::perform_handshake(int client)
{
    std::cerr << "Handshake started." << std::endl; // ! DEBUG
    const int buf_size = 4096;
    char buf[buf_size];
    std::string request;
    // Read until the end of the HTTP headers.
    while (request.find("\r\n\r\n") == std::string::npos)
    {
        ssize_t bytes = recv(client, buf, buf_size - 1, 0);
        if (bytes <= 0)
            return false;
        buf[bytes] = '\0';
        request.append(buf);
        if (request.size() > 4096)
            break;
    }
    std::cerr << "Handshake request: " << request << std::endl; // ! DEBUG
    std::istringstream stream(request);
    std::string line;
    std::string key;
    while (std::getline(stream, line))
    {
        if (line.find("Sec-WebSocket-Key:") != std::string::npos)
        {
            size_t pos = line.find(":");
            if (pos != std::string::npos)
            {
                key = line.substr(pos + 1);
                key.erase(std::remove(key.begin(), key.end(), '\r'), key.end());
                key.erase(0, key.find_first_not_of(" \t"));
            }
            break;
        }
    }
    if (key.empty())
    {
        std::cerr << "WebSocket key not found in request." << std::endl;
        return false;
    }
    std::string accept_key = compute_websocket_accept(key);
    std::ostringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n"
             << "Upgrade: websocket\r\n"
             << "Connection: Upgrade\r\n"
             << "Sec-WebSocket-Accept: " << accept_key << "\r\n\r\n";
    std::string response_str = response.str();
    if (send(client, response_str.c_str(), response_str.size(), 0) < 0)
    {
        std::perror("send handshake");
        return false;
    }
    return true;
}
