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
public:
    static int run() {
        WebSocketServer s; const unsigned short p=39519; assert(s.start(p,0));
        int raw=socket(AF_INET,SOCK_STREAM,0); assert(raw>=0); sockaddr_in a{}; a.sin_family=AF_INET; a.sin_port=htons(p); inet_pton(AF_INET,"127.0.0.1",&a.sin_addr); assert(connect(raw,reinterpret_cast<sockaddr*>(&a),sizeof(a))==0);
        assert(wait_handshake(s)); std::thread incomplete_stopper([&]{s.stop();}); incomplete_stopper.join(); close(raw);
        assert(s.active_client_handlers_==0 && s.client_sockets_.empty());
        assert(s.start(p,0));
        for(int i=0;i<8;++i) { int fd=connect_client(p); const unsigned char close_frame[]={0x88,0x80,0,0,0,0}; send(fd,close_frame,sizeof(close_frame),0); close(fd); assert(wait_zero(s)); }
        for(int i=0;i<8;++i) { int fd=connect_client(p); close(fd); assert(wait_zero(s)); }
        int blocked=connect_client(p); std::thread stopper([&]{s.stop();}); stopper.join(); close(blocked);
        assert(s.active_client_handlers_==0); assert(s.client_sockets_.empty()); s.stop();
        std::cout << "websocket lifecycle regression passed\n"; return 0;
    }
};
int main(){return WebSocketLifecycleTest::run();}
