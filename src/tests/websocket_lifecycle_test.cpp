#include "../web_socket.hpp"
#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

class WebSocketLifecycleTest {
    static void send_text(int fd, const std::string &text) {
        assert(text.size() < 126U);
        std::string frame;
        frame.push_back(static_cast<char>(0x81));
        frame.push_back(static_cast<char>(0x80U | text.size()));
        const unsigned char mask[4]={1,2,3,4};
        frame.append(reinterpret_cast<const char *>(mask),4);
        for(std::size_t i=0;i<text.size();++i) frame.push_back(static_cast<char>(text[i]^mask[i%4]));
        assert(send(fd,frame.data(),frame.size(),0)==static_cast<ssize_t>(frame.size()));
    }
    static int connect_client(unsigned short port) {
        int fd=socket(AF_INET,SOCK_STREAM,0); assert(fd>=0);
        sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(port); inet_pton(AF_INET,"127.0.0.1",&a.sin_addr);
        assert(connect(fd,reinterpret_cast<sockaddr*>(&a),sizeof(a))==0);
        const char *h="GET / HTTP/1.1\r\nHost: 127.0.0.1:\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
        assert(send(fd,h,strlen(h),0)>0); char b[512]{}; auto n=recv(fd,b,sizeof(b),0); assert(n>0); assert(std::string(b,n).find("HTTP/1.1 101") == 0); return fd;
    }
    static bool wait_zero(WebSocketServer &s) {
        std::unique_lock<std::mutex> l(s.clients_mutex_);
        return s.client_handlers_cv_.wait_for(l,std::chrono::seconds(2),[&]{return s.active_client_handlers_==0;});
    }
    static bool wait_handshake(WebSocketServer &s) {
        std::unique_lock<std::mutex> l(s.clients_mutex_);
        return s.client_handlers_cv_.wait_for(l,std::chrono::seconds(2),[&]{return !s.handshaking_sockets_.empty();});
    }
    static void verify_test_tone_transaction_lock(WebSocketServer &s) {
        std::mutex state_mutex;
        std::condition_variable state_cv;
        bool holder_acquired=false;
        bool release_holder=false;
        std::thread holder([&]{
            std::unique_lock<std::mutex> transaction(s.test_tone_command_mutex_);
            std::unique_lock<std::mutex> state(state_mutex);
            holder_acquired=true;
            state_cv.notify_all();
            state_cv.wait(state,[&]{return release_holder;});
        });
        {
            std::unique_lock<std::mutex> state(state_mutex);
            assert(state_cv.wait_for(state,std::chrono::seconds(2),[&]{return holder_acquired;}));
        }
        assert(!s.test_tone_command_mutex_.try_lock());
        {
            std::lock_guard<std::mutex> state(state_mutex);
            release_holder=true;
        }
        state_cv.notify_all();
        holder.join();
        assert(s.test_tone_command_mutex_.try_lock());
        s.test_tone_command_mutex_.unlock();
    }
    static void verify_loopback_binding(unsigned short port) {
        WebSocketServer local;
        assert(local.start(port, 0, true));
        sockaddr_in6 address{};
        socklen_t size = sizeof(address);
        assert(getsockname(
            local.listen_fd_, reinterpret_cast<sockaddr *>(&address), &size) == 0);
        assert(address.sin6_family == AF_INET6);
        assert(IN6_IS_ADDR_LOOPBACK(&address.sin6_addr));
        local.stop();
    }
public:
    static int run() {
        verify_loopback_binding(39518);
        WebSocketServer s; verify_test_tone_transaction_lock(s); const unsigned short p=39519; assert(s.start(p,0));
        int raw=socket(AF_INET,SOCK_STREAM,0); assert(raw>=0); sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(p); inet_pton(AF_INET,"127.0.0.1",&a.sin_addr); assert(connect(raw,reinterpret_cast<sockaddr*>(&a),sizeof(a))==0);
        assert(wait_handshake(s)); std::thread incomplete_stopper([&]{s.stop();}); incomplete_stopper.join(); close(raw);
        assert(s.active_client_handlers_==0 && s.client_sockets_.empty());
        assert(s.start(p,0));
        int invalid=connect_client(p); send_text(invalid,"{\"command\":42}"); char response[512]{}; auto response_size=recv(invalid,response,sizeof(response),0); assert(response_size>0); assert(std::string(response,response_size).find("command failure")!=std::string::npos); close(invalid); assert(wait_zero(s));
        int survivor=connect_client(p); close(survivor); assert(wait_zero(s));
        for(int i=0;i<8;++i) { int fd=connect_client(p); const unsigned char close_frame[]={0x88,0x80,0,0,0,0}; send(fd,close_frame,sizeof(close_frame),0); close(fd); assert(wait_zero(s)); }
        for(int i=0;i<8;++i) { int fd=connect_client(p); close(fd); assert(wait_zero(s)); }
        int blocked=connect_client(p); std::thread stopper([&]{s.stop();}); stopper.join(); close(blocked);
        assert(s.active_client_handlers_==0); assert(s.client_sockets_.empty()); s.stop();
        std::cout << "websocket lifecycle regression passed\n"; return 0;
    }
};
int main(){return WebSocketLifecycleTest::run();}
