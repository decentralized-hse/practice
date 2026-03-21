#include "../src/redis/bason_redis_server.hpp"
#include <iostream>
#include <cstdlib>
#include <signal.h>

using namespace bason;

static BasonRedisServer* g_server = nullptr;

static void on_signal(int) {
    std::cout << "\nShutting down..." << std::endl;
    if (g_server) g_server->shutdown();
    exit(0);
}

int main(int argc, char* argv[]) {
    uint16_t port     = 6379;
    std::string dir   = "./data";

    if (argc > 1) port = static_cast<uint16_t>(std::atoi(argv[1]));
    if (argc > 2) dir  = argv[2];

    signal(SIGINT,  on_signal);
    signal(SIGTERM, on_signal);

    try {
        BasonRedisServer server(dir);
        g_server = &server;

        std::cout << "BASONRedis listening on port " << port
                  << "  (data: " << dir << ")" << std::endl;
        std::cout << "Press Ctrl+C to stop." << std::endl;

        server.listen(port);

    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
