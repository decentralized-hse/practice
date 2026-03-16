/*
 * GossipSync — децентрализованный веб-хостинг на rsync + SSH
 *
 * Точка входа: разбор аргументов и вызов нужной команды.
 *
 * Сборка:  g++ -std=c++17 -Wall -o gossipsync src/main.cpp
 */

#include "../include/publisher.hpp"
#include "../include/friends.hpp"
#include "../include/subscriber.hpp"

// Справка по командам
void usage(const char* prog) {
    std::cout
        << "GossipSync — децентрализованный хостинг (rsync + SSH + граф дружбы)\n\n"
        << "Использование: " << prog << " <команда> [аргументы]\n\n"
        << "Узел:\n"
        << "  init <имя> <ssh-адрес>        создать узел\n"
        << "  status                        обзор\n\n"
        << "Публикация:\n"
        << "  post <заголовок>              новый пост (тело из stdin)\n"
        << "  list-feed [лента]             содержимое ленты\n\n"
        << "Граф дружбы:\n"
        << "  add-friend <имя> <адрес>      добавить друга\n"
        << "  remove-friend <имя>           удалить друга\n"
        << "  friends                       список друзей\n\n"
        << "Подписки и синхронизация:\n"
        << "  subscribe <лента>             подписаться на ленту\n"
        << "  unsubscribe <лента>           отписаться\n"
        << "  subs                          мои подписки\n"
        << "  sync [--jitter]               синхронизация с друзьями\n"
        << "  feeds                         все ленты локально\n\n"
        << "Пример:\n"
        << "  gossipsync init charlie guest_sync@charlie.net:/~/.gossipsync/feeds/\n"
        << "  gossipsync add-friend alice guest_sync@alice.net:/~/.gossipsync/feeds/\n"
        << "  gossipsync subscribe alice\n"
        << "  gossipsync sync\n";
}

// Обзор состояния узла
int cmd_status(const Paths& p) {
    auto conf = NodeConf::load(p);
    auto friends = load_friends(p);
    auto subs = load_subs(p);
    auto feeds = local_feeds(p);

    std::cout << "=== GossipSync: " << conf.name << " ===\n\n"
              << "  Адрес:    " << conf.address << "\n"
              << "  Друзья:   " << friends.size();
    if (!friends.empty()) {
        std::cout << " (";
        for (size_t i = 0; i < friends.size(); i++) {
            if (i) std::cout << ", ";
            std::cout << friends[i].name;
        }
        std::cout << ")";
    }
    std::cout << "\n  Подписки: " << subs.size() << "\n"
              << "  Ленты:    " << feeds.size();
    if (!feeds.empty()) {
        std::cout << " (";
        for (size_t i = 0; i < feeds.size(); i++) {
            if (i) std::cout << ", ";
            std::cout << feeds[i];
        }
        std::cout << ")";
    }
    std::cout << "\n  SSH-ключ: "
              << (fs::exists(p.key) ? "есть" : "нет")
              << "\n";
    return 0;
}

// Разбор аргументов и вызов нужной команды
int main(int argc, char* argv[]) {
    if (argc < 2) { usage(argv[0]); return 1; }

    Paths p;
    std::string cmd = argv[1];

    if (cmd == "init") {
        if (argc < 4) {
            std::cerr << "gossipsync init <имя> <ssh-адрес>\n";
            return 1;
        }
        return cmd_init(p, argv[2], argv[3]);
    }
    if (cmd == "status")
        return cmd_status(p);

    if (cmd == "post") {
        if (argc < 3) { std::cerr << "gossipsync post <заголовок>\n"; return 1; }
        return cmd_post(p, argv[2]);
    }
    if (cmd == "list-feed") {
        std::string name = (argc >= 3) ? argv[2] : "";
        return cmd_list_feed(p, name);
    }

    if (cmd == "add-friend") {
        if (argc < 4) {
            std::cerr << "gossipsync add-friend <имя> <адрес>\n";
            return 1;
        }
        return cmd_add_friend(p, argv[2], argv[3]);
    }
    if (cmd == "remove-friend") {
        if (argc < 3) { std::cerr << "Укажите имя.\n"; return 1; }
        return cmd_remove_friend(p, argv[2]);
    }
    if (cmd == "friends")
        return cmd_list_friends(p);

    if (cmd == "subscribe" || cmd == "sub") {
        if (argc < 3) { std::cerr << "gossipsync subscribe <лента>\n"; return 1; }
        return cmd_subscribe(p, argv[2]);
    }
    if (cmd == "unsubscribe" || cmd == "unsub") {
        if (argc < 3) { std::cerr << "Укажите ленту.\n"; return 1; }
        return cmd_unsubscribe(p, argv[2]);
    }
    if (cmd == "subs")
        return cmd_list_subs(p);

    if (cmd == "sync") {
        bool jitter = (argc >= 3 && std::string(argv[2]) == "--jitter");
        return cmd_sync(p, jitter);
    }
    if (cmd == "feeds")
        return cmd_list_all_feeds(p);

    std::cerr << "Неизвестная команда: " << cmd << "\n\n";
    usage(argv[0]);
    return 1;
}