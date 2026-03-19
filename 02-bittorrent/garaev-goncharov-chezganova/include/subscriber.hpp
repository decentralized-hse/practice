#pragma once

#include "common.hpp"

// Подписаться на ленту (добавить имя в subscriptions.txt)
inline int cmd_subscribe(const Paths& p, const std::string& feed_name) {
    p.ensure();
    for (auto& s : load_subs(p)) {
        if (s.feed_name == feed_name) {
            std::cerr << "Уже подписаны на «" << feed_name << "».\n";
            return 1;
        }
    }
    append_line(p.subs_file, feed_name);
    std::cout << "Подписка: " << feed_name << "\n";
    return 0;
}

// Отписаться от ленты
inline int cmd_unsubscribe(const Paths& p, const std::string& feed_name) {
    auto lines = read_lines(p.subs_file);
    std::ofstream out(p.subs_file, std::ios::trunc);
    bool found = false;
    for (auto& line : lines) {
        std::istringstream ss(line);
        std::string n;
        ss >> n;
        if (n == feed_name) found = true;
        else out << line << "\n";
    }
    if (!found) { std::cerr << "Подписка не найдена.\n"; return 1; }
    std::cout << "Отписка: " << feed_name << "\n";
    return 0;
}

// Показать подписки: [+] есть локально, [ ] ещё нет
inline int cmd_list_subs(const Paths& p) {
    auto subs = load_subs(p);
    if (subs.empty()) {
        std::cout << "Нет подписок.\n";
        return 0;
    }
    std::cout << "Подписки:\n";
    for (auto& s : subs) {
        fs::path dir = p.feeds / s.feed_name;
        bool have = fs::exists(dir) && count_files(dir) > 0;
        std::cout << "  " << (have ? "[+] " : "[ ] ") << s.feed_name;
        if (have) std::cout << " (" << count_files(dir) << " файлов)";
        std::cout << "\n";
    }
    return 0;
}

// Узнать, какие ленты есть у друга (rsync --list-only, без скачивания)
inline std::vector<std::string> list_remote_feeds(const Paths& p,
                                                  const Friend& fr) {
    std::vector<std::string> result;

    std::string ssh = "ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10";
    if (fs::exists(p.key))
        ssh += " -i " + p.key.string();

    std::string cmd = "rsync --list-only -e \"" + ssh + "\" "
                    + fr.address + " 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return result;

    char buf[1024];
    while (fgets(buf, sizeof(buf), pipe)) {
        std::string line(buf);
        // Строки с 'd' — директории (ленты)
        if (line.size() > 1 && line[0] == 'd') {
            auto pos = line.rfind(' ');
            if (pos != std::string::npos) {
                std::string name = line.substr(pos + 1);
                while (!name.empty() && (name.back() == '/' ||
                       name.back() == '\n' || name.back() == '\r'))
                    name.pop_back();
                if (!name.empty() && name != ".")
                    result.push_back(name);
            }
        }
    }
    pclose(pipe);
    return result;
}

// Скачать одну ленту от одного друга
inline bool sync_feed_from(const Paths& p, const Friend& fr,
                           const std::string& feed_name,
                           const NodeConf& my_conf) {
    // Свою ленту не качаем обратно
    if (feed_name == my_conf.name) return true;

    fs::path local = p.feeds / feed_name;
    fs::create_directories(local);

    std::string ssh = "ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10";
    if (fs::exists(p.key))
        ssh += " -i " + p.key.string();

    // rsync конкретную подпапку, не всю feeds/ целиком
    std::string cmd = "rsync -avz --update --timeout=30"
                      " -e \"" + ssh + "\""
                      " " + fr.address + feed_name + "/"
                      " " + local.string() + "/";

    std::cout << "  > " << cmd << "\n";
    return std::system(cmd.c_str()) == 0;
}

// Главная команда: обойти всех друзей, забрать все ленты
inline int cmd_sync(const Paths& p, bool jitter) {
    auto conf = NodeConf::load(p);
    auto friends = load_friends(p);

    if (friends.empty()) {
        std::cout << "Нет друзей — не с кем синхронизироваться.\n"
                  << "  gossipsync add-friend <имя> <адрес>\n";
        return 0;
    }

    // Случайная задержка 0–5 мин, чтобы не перегружать друзей
    if (jitter) {
        int delay = rand_int(0, 300);
        std::cout << "Jitter: " << delay << " сек.\n";
    }

    std::cout << "Синхронизация с " << friends.size()
              << " друзьями...\n\n";

    int total_feeds = 0, total_ok = 0;

    for (auto& fr : friends) {
        std::cout << "── Друг: " << fr.name
                  << " (" << fr.address << ") ──\n";

        // Шаг 1: какие ленты есть у друга?
        auto remote = list_remote_feeds(p, fr);

        if (remote.empty()) {
            // Fallback: пробуем хотя бы ленту самого друга
            std::cout << "  Пробуем ленту «" << fr.name << "» напрямую...\n";
            total_feeds++;
            if (sync_feed_from(p, fr, fr.name, conf)) {
                std::cout << "  OK\n";
                total_ok++;
            } else {
                std::cout << "  ОШИБКА\n";
            }
            std::cout << "\n";
            continue;
        }

        std::cout << "  Доступные ленты: ";
        for (size_t i = 0; i < remote.size(); i++) {
            if (i) std::cout << ", ";
            std::cout << remote[i];
        }
        std::cout << "\n\n";

        // Шаг 2: скачиваем каждую ленту отдельно
        for (auto& feed_name : remote) {
            total_feeds++;
            std::cout << "  [" << feed_name << "] ";
            if (sync_feed_from(p, fr, feed_name, conf)) {
                std::cout << "OK\n";
                total_ok++;
            } else {
                std::cout << "ОШИБКА\n";
            }
        }
        std::cout << "\n";
    }

    // Итог
    auto all_local = local_feeds(p);
    std::cout << "Итого: " << total_ok << "/" << total_feeds
              << " лент синхронизировано.\n"
              << "Всего лент локально: " << all_local.size() << " (";
    for (size_t i = 0; i < all_local.size(); i++) {
        if (i) std::cout << ", ";
        std::cout << all_local[i];
    }
    std::cout << ")\n";
    return (total_ok == total_feeds) ? 0 : 1;
}

// Показать все локальные ленты: [я] моя, [*] подписка, [ ] транзит
inline int cmd_list_all_feeds(const Paths& p) {
    auto conf = NodeConf::load(p);
    auto subs = load_subs(p);
    auto feeds = local_feeds(p);

    std::set<std::string> sub_set;
    for (auto& s : subs) sub_set.insert(s.feed_name);

    if (feeds.empty()) { std::cout << "Нет лент.\n"; return 0; }

    std::cout << "Ленты локально (" << feeds.size() << "):\n\n";
    for (auto& name : feeds) {
        fs::path dir = p.feeds / name;
        bool is_mine = (name == conf.name);
        bool is_sub  = sub_set.count(name);

        std::cout << "  ";
        if (is_mine)     std::cout << "[я]  ";
        else if (is_sub) std::cout << "[*]  ";
        else             std::cout << "[ ]  ";

        std::cout << name << " (" << count_files(dir) << " файлов)\n";
    }
    std::cout << "\n  [я] = моя лента, [*] = подписка, [ ] = транзит\n";
    return 0;
}
