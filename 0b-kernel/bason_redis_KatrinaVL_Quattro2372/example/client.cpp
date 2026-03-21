#include "../src/redis/bason_redis_client.hpp"
#include <iostream>
#include <cstdlib>

using namespace bason;

static void section(const char* title) {
    std::cout << "\n── " << title << " ──" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    uint16_t    port = 6379;

    if (argc > 1) host = argv[1];
    if (argc > 2) port = static_cast<uint16_t>(std::atoi(argv[2]));

    try {
        BasonRedisClient client;
        client.connect(host, port);
        std::cout << "Connected to " << host << ":" << port << std::endl;

        section("PING");
        std::cout << "PING → " << client.ping() << std::endl;

        section("String");
        client.set("greeting", "Hello, BASON!");
        std::cout << "SET greeting 'Hello, BASON!'" << std::endl;
        std::cout << "GET greeting → " << client.get("greeting") << std::endl;

        section("Number");
        client.set("hits", "0");
        std::cout << "INCR hits    → " << client.incr("hits")          << std::endl;
        std::cout << "INCR hits    → " << client.incr("hits")          << std::endl;
        std::cout << "INCRBY hits 8→ " << client.incrby("hits", 8)     << std::endl;
        std::cout << "GET hits     → " << client.get("hits")           << std::endl;

        section("List");
        client.lpush("queue", "task-c");
        client.lpush("queue", "task-b");
        client.lpush("queue", "task-a");
        std::cout << "LPUSH queue task-a/b/c" << std::endl;
        std::cout << "LLEN  queue → " << client.llen("queue") << std::endl;

        section("Hash");
        client.hset("user:42", "name",  "Alice");
        client.hset("user:42", "email", "alice@example.com");
        client.hset("user:42", "score", "9001");
        std::cout << "HSET user:42 name/email/score" << std::endl;
        std::cout << "HGET user:42 name  → " << client.hget("user:42", "name")  << std::endl;
        std::cout << "HGET user:42 email → " << client.hget("user:42", "email") << std::endl;
        std::cout << "HGET user:42 score → " << client.hget("user:42", "score") << std::endl;

        section("TTL");
        client.set("session", "abc123");
        client.expire("session", 30000);
        std::cout << "SET session abc123 + EXPIRE 30000 ms" << std::endl;
        std::cout << "TTL session → " << client.ttl("session") << " ms" << std::endl;

        section("KEYS");
        auto all_keys = client.keys("*");
        std::cout << "KEYS * (" << all_keys.size() << " keys):" << std::endl;
        for (const auto& k : all_keys) {
            std::cout << "  " << k << std::endl;
        }

        auto user_keys = client.keys("user:*");
        std::cout << "KEYS user:* → " << user_keys.size() << " key(s)" << std::endl;

        section("DBSIZE");
        std::cout << "DBSIZE → " << client.dbsize() << std::endl;

        client.disconnect();
        std::cout << "\nDisconnected." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
