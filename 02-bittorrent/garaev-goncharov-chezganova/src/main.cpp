/*
 * GossipSync — децентрализованный веб-хостинг на rsync + SSH
 *
 * Точка входа: разбор аргументов и вызов нужной команды.
 *
 * Сборка:  g++ -std=c++17 -Wall -o gossipsync src/main.cpp
 */

#include "publisher.hpp"
#include "subscriber.hpp"
#include "mirror.hpp"

void usage(const char* prog) {
    std::cout
        << "GossipSync — децентрализованный хостинг на rsync + SSH\n\n"
        << "Использование: " << prog << " <команда> [аргументы]\n\n"
        << "Издатель:\n"
        << "  init                          инициализация узла\n"
        << "  post <заголовок>              новый пост (тело из stdin)\n"
        << "  list-feed                     содержимое ленты\n\n"
        << "Подписчик:\n"
        << "  subscribe <имя> <адрес>       подписаться на ленту\n"
        << "  unsubscribe <имя>             отписаться\n"
        << "  list-subs                     список подписок\n"
        << "  sync [--jitter]               синхронизировать всё\n"
        << "  setup-cron                    команда для crontab\n\n"
        << "Зеркала:\n"
        << "  mirror <лента> <мой_адрес>    стать зеркалом\n"
        << "  list-mirrors                  мои зеркала\n"
        << "  feed-mirrors <лента>          зеркала ленты\n\n"
        << "Общее:\n"
        << "  status                        обзор узла\n";
}

int cmd_status(const Paths& p) {
    std::cout << "=== GossipSync ===\n\n"
              << "Директория:  " << p.base << "\n"
              << "Лента:       " << p.feed
              << " (" << count_files(p.feed) << " файлов)\n"
              << "Подписки:    " << load_subs(p).size() << "\n"
              << "SSH-ключ:    "
              << (fs::exists(p.key) ? "есть" : "нет") << "\n";

    auto subs = load_subs(p);
    if (!subs.empty()) {
        std::cout << "\nЛенты:\n";
        for (auto& s : subs)
            std::cout << "  " << s.name << " <- " << s.target << "\n";
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) { usage(argv[0]); return 1; }

    Paths p;
    std::string cmd = argv[1];

    // ── Издатель ──
    if (cmd == "init")
        return cmd_init(p);

    if (cmd == "post") {
        if (argc < 3) { std::cerr << "Нужен заголовок.\n"; return 1; }
        return cmd_post(p, argv[2]);
    }

    if (cmd == "list-feed")
        return cmd_list_feed(p);

    // ── Подписчик ──
    if (cmd == "subscribe" || cmd == "sub") {
        if (argc < 4) {
            std::cerr << "gossipsync subscribe <имя> <user@host:/path/>\n";
            return 1;
        }
        return cmd_subscribe(p, argv[2], argv[3]);
    }

    if (cmd == "unsubscribe" || cmd == "unsub") {
        if (argc < 3) { std::cerr << "Укажите имя.\n"; return 1; }
        return cmd_unsubscribe(p, argv[2]);
    }

    if (cmd == "list-subs" || cmd == "subs")
        return cmd_list_subs(p);

    if (cmd == "sync") {
        bool jitter = (argc >= 3 && std::string(argv[2]) == "--jitter");
        return cmd_sync(p, jitter);
    }

    if (cmd == "setup-cron")
        return cmd_setup_cron(p);

    // ── Зеркала ──
    if (cmd == "mirror") {
        if (argc < 4) {
            std::cerr << "gossipsync mirror <лента> <user@host:/path/>\n";
            return 1;
        }
        return cmd_mirror(p, argv[2], argv[3]);
    }

    if (cmd == "list-mirrors")
        return cmd_list_mirrors(p);

    if (cmd == "feed-mirrors") {
        if (argc < 3) { std::cerr << "Укажите имя ленты.\n"; return 1; }
        return cmd_feed_mirrors(p, argv[2]);
    }

    // ── Общее ──
    if (cmd == "status")
        return cmd_status(p);

    std::cerr << "Неизвестная команда: " << cmd << "\n";
    usage(argv[0]);
    return 1;
}
