#ifndef GOSSIPSYNC_SUBSCRIBER_HPP
#define GOSSIPSYNC_SUBSCRIBER_HPP

#include "common.hpp"

// ── Работа с файлом подписок ─────────────────────────────────

// Загрузить подписки из subscriptions.txt
inline std::vector<Subscription> load_subs(const Paths& p) {
    std::vector<Subscription> subs;
    for (auto& line : read_lines(p.subs_file)) {
        std::istringstream ss(line);
        Subscription s;
        if (ss >> s.name >> s.target)
            subs.push_back(s);
    }
    return subs;
}

// Добавить подписку
inline int cmd_subscribe(const Paths& p,
                         const std::string& name,
                         const std::string& target) {
    p.ensure();

    // Проверяем дубликат
    for (auto& s : load_subs(p)) {
        if (s.name == name) {
            std::cerr << "Подписка '" << name << "' уже есть.\n";
            return 1;
        }
    }

    append_line(p.subs_file, name + "\t" + target);
    fs::create_directories(p.feeds / name);

    std::cout << "Подписка: " << name << " -> " << target << "\n";
    return 0;
}

// Удалить подписку
inline int cmd_unsubscribe(const Paths& p, const std::string& name) {
    auto lines = read_lines(p.subs_file);
    std::ofstream out(p.subs_file, std::ios::trunc);
    bool found = false;

    for (auto& line : lines) {
        std::istringstream ss(line);
        std::string n;
        ss >> n;
        if (n == name) {
            found = true;   // пропускаем эту строку
        } else {
            out << line << "\n";
        }
    }

    if (!found) {
        std::cerr << "Подписка '" << name << "' не найдена.\n";
        return 1;
    }
    std::cout << "Отписка: " << name << "\n";
    return 0;
}

// Показать подписки
inline int cmd_list_subs(const Paths& p) {
    auto subs = load_subs(p);
    if (subs.empty()) {
        std::cout << "Нет подписок.\n"
                  << "  gossipsync subscribe <имя> <user@host:/path/>\n";
        return 0;
    }
    std::cout << "Подписки (" << subs.size() << "):\n\n";
    for (auto& s : subs) {
        fs::path local = p.feeds / s.name;
        std::cout << "  " << s.name << "\n"
                  << "    адрес:  " << s.target << "\n"
                  << "    локально: " << local
                  << " (" << count_files(local) << " файлов)\n\n";
    }
    return 0;
}

// ── Синхронизация ────────────────────────────────────────────

// Выбрать источник: оригинал или случайное зеркало
inline std::string pick_source(const Subscription& s, const fs::path& feeds) {
    fs::path mf = feeds / s.name / "mirrors.txt";
    auto mirrors = read_lines(mf);

    if (mirrors.empty() || rand_int(1) == 0)
        return s.target;  // оригинал

    // Случайное зеркало (первое поле строки)
    auto& line = mirrors[rand_int(mirrors.size() - 1)];
    std::istringstream ss(line);
    std::string addr;
    if (ss >> addr) return addr;
    return s.target;
}

// Синхронизировать одну подписку
inline bool sync_one(const Paths& p, const Subscription& s) {
    fs::path local = p.feeds / s.name;
    fs::create_directories(local);

    std::string source = pick_source(s, p.feeds);
    if (source != s.target)
        std::cout << "  (зеркало: " << source << ")\n";

    // Формируем SSH-опции
    std::string ssh = "ssh -o StrictHostKeyChecking=no -o ConnectTimeout=10";

    // Нестандартный порт можно задать через GOSSIPSYNC_SSH_PORT
    if (const char* port = std::getenv("GOSSIPSYNC_SSH_PORT")) {
        if (*port) {
            ssh += " -p ";
            ssh += port;
        }
    }

    if (fs::exists(p.key))
        ssh += " -i " + p.key.string();

    // rsync: -a (архивный), -v (подробно), -z (сжатие), --delete (удалять лишнее)
    std::string cmd = "rsync -avz --delete --timeout=30 "
                      "-e \"" + ssh + "\" "
                      + source + " " + local.string() + "/";

    int rc = run(cmd);

    // Если зеркало не сработало — пробуем оригинал
    if (rc != 0 && source != s.target) {
        std::cout << "  Зеркало недоступно, пробуем оригинал...\n";
        cmd = "rsync -avz --delete --timeout=30 "
              "-e \"" + ssh + "\" "
              + s.target + " " + local.string() + "/";
        rc = run(cmd);
    }

    return rc == 0;
}

// Синхронизировать все подписки
inline int cmd_sync(const Paths& p, bool jitter) {
    auto subs = load_subs(p);
    if (subs.empty()) {
        std::cout << "Нет подписок.\n";
        return 0;
    }

    // Случайная задержка — защита от «шквала запросов»
    // (все cron-задачи срабатывают в начале часа)
    if (jitter) {
        int delay = rand_int(300);  // 0–5 минут
        std::cout << "Jitter: " << delay << " сек.\n";
        // В реальном использовании: sleep(delay);
    }

    std::cout << "Синхронизация " << subs.size() << " подписок...\n\n";

    int ok = 0;
    for (auto& s : subs) {
        std::cout << "[" << s.name << "]\n";
        if (sync_one(p, s)) {
            std::cout << "  OK\n\n";
            ok++;
        } else {
            std::cout << "  ОШИБКА\n\n";
        }
    }

    std::cout << "Итого: " << ok << "/" << subs.size() << " успешно.\n";
    return (ok == (int)subs.size()) ? 0 : 1;
}

// Показать команду для crontab
inline int cmd_setup_cron(const Paths& p) {
    (void)p;
    std::cout << "Добавьте в crontab (crontab -e):\n\n"
              << "  0 * * * * /путь/к/gossipsync sync --jitter\n\n"
              << "Это будет синхронизировать подписки каждый час\n"
              << "со случайной задержкой для распределения нагрузки.\n";
    return 0;
}

#endif
