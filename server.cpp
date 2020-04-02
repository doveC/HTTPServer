#include "tcp_server_thpool.hpp"

int main() {
    Server server("192.168.126.111", 23231);

    server.Start();

    return 0;
}
