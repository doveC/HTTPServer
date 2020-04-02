#pragma once
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

// TCP socket 接口封装类
class TCPSocket {
public:
    TCPSocket()
        : _fd(-1) {}
    TCPSocket(int fd)
        : _fd(fd) {}

    bool Socket() {
        _fd = socket(AF_INET, SOCK_STREAM, 0);
        if(_fd < 0) {
            perror("socket() failed");
            return false;
        }
        printf("open fd = %d\n", _fd);
        return true;
    }

    bool Close() {
        close(_fd);
        printf("close fd = %d\n", _fd);
        return true;
    }

    bool Bind(const std::string& ip, uint16_t port) const {
        sockaddr_in addr;
        
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        int ret = bind(_fd, (sockaddr *)&addr, sizeof(addr));
        if(ret < 0) {
            perror("bind() failed");
            return false;
        }
        return true;
    }    

    bool Listen(int num) const {
        int ret = listen(_fd, num);
        if(ret < 0) {
            perror("listen() failed");
            return false;
        }
        return true;
    }

    bool Accept(TCPSocket* peer, std::string* ip = nullptr, uint16_t* port = nullptr) const {
        sockaddr_in peer_addr;
        socklen_t len = sizeof(peer_addr);

        int connfd = accept(_fd, (sockaddr *)&peer_addr, &len);
        if(connfd < 0) {
            perror("accept() failed");
            return false;
        }
        printf("accept fd = %d\n", connfd);

        peer->_fd = connfd;
        if(ip != nullptr) {
            *ip = inet_ntoa(peer_addr.sin_addr);
        }
        if(port != nullptr) {
            *port = ntohs(peer_addr.sin_port);
        }

        return true;
    }

    bool Recv(std::string* buf) const {
        buf->clear();
        char tmp[10240] = { 0 };

        ssize_t read_size = recv(_fd, tmp, sizeof(tmp), 0);
        if(read_size <= 0) {
            perror("recv() failed");
            return false;
        }

        buf->assign(tmp, read_size);
        return true;
    }

    bool Send(const std::string& buf) const {
        ssize_t write_size = send(_fd, buf.data(), buf.size(), 0);
        if(write_size < 0) {
            perror("send() failed");
            return false;
        }
        return true;
    }

    bool Connect(const std::string& ip, uint16_t port) const {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        int ret = connect(_fd, (sockaddr *)&addr, sizeof(addr));
        if(ret < 0) {
            perror("connect() failed");
            return false;
        }
        return true;
    }

    int Getfd() const {
        return _fd;
    }
private:
    int _fd;
};
