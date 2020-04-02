#pragma once
#include <functional>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unordered_map>
#include "tcp_socket.hpp"
#include "thread_pool.hpp"

typedef std::function<void (const std::string&, std::string*)> Handler;

struct ThreadArg {
    TCPSocket conn_sock;
    std::string ip;
    uint16_t port;
};

// 线程池HTTP服务器接口封装类
class Server {
public:
    Server(const std::string& ip, uint16_t port)
        : _ip(ip)
        , _port(port) {}

    // server start
    bool Start() {
        // 0. create thread pool
        ThreadPool threadpool(1);
        threadpool.PoolInit();

        // 1. create socket
        if(!_listen_sock.Socket()) { return false; }

        // 2. bind port
        if(!_listen_sock.Bind(_ip, _port)) { return false; }

        // 3. listen
        if(!_listen_sock.Listen(10)) { return false; }

        while(1) {
            // 4. accept
            ThreadArg* arg = new ThreadArg();
            bool ret = _listen_sock.Accept(&arg->conn_sock, &arg->ip, &arg->port);
            if(!ret) {
                continue;
            }
            printf("[Client %s:%d] connected\n", arg->ip.c_str(), arg->port);

            // 5. wake up a thread
            ThreadTask task((void *)arg, Process);
            threadpool.PushTask(&task);
        }

        return true;
    }

    static void Process(void* _arg) {
        ThreadArg* arg = (ThreadArg *)_arg;
        printf("Thread %lld running!\n", (long long)pthread_self());
        while(1) {
            std::string msg;

            // receive request
            bool ret = arg->conn_sock.Recv(&msg);
            if(!ret) {
                printf("[Client %s:%d disconnected!]\n", arg->ip.c_str(), arg->port);
                break;
            }

            printf("----- Request -----\n");
            printf("%s", msg.c_str());
            printf("----- Done -----\n");

            // response
            // 1. parse header
            std::unordered_map<std::string, std::string> headers = ParseHeader(msg);
            for(auto i = headers.begin(); i != headers.end(); i++) {
                printf("[%s]:[%s]\n", i->first.c_str(), i->second.c_str());
            }

            // 2. open files
            int fd = OpenFile(headers["Path"]);
            if(fd == -1) {
                std::string buf(Re404());
                arg->conn_sock.Send(buf);
                continue;
            }
            printf("Open file[%d]\n", fd);

            // 3. response
            char buff[10240] = { 0 };

            ssize_t count = read(fd, buff, sizeof(buff));
            printf("Read [%ld] bytes\n", count);

            std::string buf;
            buf += ReHeader(count);
            buf.append(buff, count);
            printf("Response: %s", buf.c_str());

            arg->conn_sock.Send(buf);
        }
    }

private:
    static std::unordered_map<std::string, std::string> 
    ParseHeader(std::string& msg) {
        std::size_t found, nfound;

        // find header by \r\n\r\n
        found = msg.find("\r\n\r\n");
        if(found == std::string::npos) {
            printf("Cannot find \\r\\n\\r\\n!!!\n");
        }

        // get header
        std::string header = msg.substr(0, found + 4);
        printf("----- Header -----\n");
        printf("%s\n", header.c_str());
        printf("----- Done -----\n");

        // split header fields
        // handle first line
        std::unordered_map<std::string, std::string> headers;
        found = header.find(" ");  // find first space
        headers.insert( {{"Method", header.substr(0, found)}} );
        nfound = header.find(" ", found + 1);  // find second space
        headers.insert( {{"Path", header.substr(found + 1, nfound - found - 1)}} );
        found = nfound;
        nfound = header.find("\r\n", found + 1);  // find first /r/n
        headers.insert( {{"HTTPv", header.substr(found + 1, nfound - found - 1)}} );

        // handle other lines
        found = nfound + 2;
        while(1) {
            nfound = header.find(": ", found);
            std::size_t tmp = nfound + 2;
            std::size_t tmp2 = header.find("\r\n", tmp);
            headers.insert( {{header.substr(found, nfound - found), header.substr(tmp, tmp2 - tmp)}} );
            if(strcmp("\r\n", header.substr(tmp2 + 2, 2).c_str()) == 0) {
                break;
            }
            found = tmp2 + 2;
        }

        return headers;
    }

    // Open file
    // Return fd on success, -1 on file not exist
    static int OpenFile(std::string& path) {
        if(path.length() == 1) {
            path += "index.html";
        }

        int pos = path.length();
        if(path.find("?") != std::string::npos) {  // ignore options after ?
            pos = path.find("?");
        } 

        std::string base_path("/home/syndi/workspace/net/wwwroot");
        int fd = open((base_path + path.substr(0, pos)).c_str(), O_RDONLY);
        if(fd == -1 && errno == ENOENT) {
            printf("file not exists!\n");
        }
        printf("Opened file [%s]\n", (base_path + path.substr(0, pos)).c_str());

        return fd;
    }

    static std::string ReHeader(int body_len) {
        std::string buf;

        buf += "HTTP/1.1 200 OK\r\n";
        buf += "Server: Syndicate/0.1.0\r\n";
        buf += "Content-Type: text/html\r\n";
        buf += "Content-Length: ";
        buf += std::to_string(body_len);
        buf += "\r\n\r\n";

        return buf;
    }

    static std::string Re404() {
		std::string buf;
	
		buf += "HTTP/1.1 404 NOT FOUND\r\n";
        buf += "Server: Syndicate/0.1.0\r\n";
        buf += "Content-Type: text/html\r\n";
        buf += "Content-Length: 36\r\n\r\n";
        buf += "<html><h1>404 NOT FOUND!</h1></html>";

        return buf;
    }

private:
    TCPSocket _listen_sock;
    std::string _ip;
    uint16_t _port;
};
