#pragma once

#include "common.hpp"

// Инициализация узла: директории, SSH-ключ, скрипт настройки
inline int cmd_init(const Paths& p,
                    const std::string& node_name,
                    const std::string& node_address) {
    std::cout << "Инициализация узла «" << node_name << "»...\n\n";
    p.ensure();

    NodeConf conf{node_name, node_address};
    conf.save(p);

    // Моя лента — подпапка feeds/<моё_имя>/
    fs::path my_feed = p.feeds / node_name;
    fs::create_directories(my_feed);

    // Симлинк public_feed -> feeds/<имя> для удобства
    if (!fs::exists(p.feed)) {
        fs::create_directory_symlink(my_feed, p.feed);
        std::cout << "  " << p.feed << " -> " << my_feed << "\n";
    }

    // Начальный index.html
    fs::path index = my_feed / "index.html";
    if (!fs::exists(index)) {
        std::ofstream f(index);
        f << "<!DOCTYPE html>\n<html><body>\n"
          << "<h1>" << node_name << " — GossipSync</h1>\n"
          << "<ul id='posts'></ul>\n</body></html>\n";
        std::cout << "  Создан: " << index << "\n";
    }

    // Пустые конфиги
    if (!fs::exists(p.friends_file)) {
        std::ofstream(p.friends_file)
            << "# Друзья: имя<TAB>ssh-адрес-до-feeds/\n";
    }
    if (!fs::exists(p.subs_file)) {
        std::ofstream(p.subs_file)
            << "# Подписки на ленты (по одному имени на строку)\n";
    }

    // SSH-ключ для гостевого доступа
    if (!fs::exists(p.key)) {
        std::cout << "\n  Генерация SSH-ключа...\n";
        int rc = run("ssh-keygen -t ed25519 -f " + p.key.string()
                      + " -N '' -C gossipsync -q");
        if (rc != 0) { std::cerr << "  Ошибка ssh-keygen\n"; return 1; }
    }
    std::cout << "  Ключ: " << p.key << "\n";

    // Bash-скрипт настройки гостевого SSH (rrsync read-only к feeds/)
    fs::path script = p.base / "setup-guest-ssh.sh";
    {
        std::ofstream f(script);
        f << "#!/bin/bash\n"
          << "set -euo pipefail\n\n"
          << "USER=guest_sync\n"
          << "FEEDS=\"" << p.feeds.string() << "\"\n"
          << "PUBKEY=\"" << p.key.string() << ".pub\"\n\n"
          << "RRSYNC=$(find /usr -name rrsync -type f 2>/dev/null | head -1)\n"
          << "[ -z \"$RRSYNC\" ] && { echo 'rrsync не найден'; exit 1; }\n"
          << "chmod +x \"$RRSYNC\"\n\n"
          << "id \"$USER\" &>/dev/null || useradd -r -m -s /bin/bash \"$USER\"\n"
          << "H=$(eval echo ~$USER)\n"
          << "mkdir -p \"$H/.ssh\" && chmod 700 \"$H/.ssh\"\n\n"
          << "echo \"command=\\\"$RRSYNC -ro $FEEDS\\\","
          << "no-agent-forwarding,no-port-forwarding,no-pty "
          << "$(cat $PUBKEY)\" > \"$H/.ssh/authorized_keys\"\n"
          << "chmod 600 \"$H/.ssh/authorized_keys\"\n"
          << "chown -R \"$USER:$USER\" \"$H/.ssh\"\n\n"
          << "echo 'Готово!'\n";
        fs::permissions(script, fs::perms::owner_all, fs::perm_options::replace);
    }

    std::cout << "\nГотово!\n"
              << "  Узел:    " << node_name << "\n"
              << "  Адрес:   " << node_address << "\n"
              << "  Лента:   " << my_feed << "\n"
              << "  Скрипт:  " << script << "\n";
    return 0;
}

// Создать пост в моей ленте (заголовок из аргумента, тело из stdin)
inline int cmd_post(const Paths& p, const std::string& title) {
    auto conf = NodeConf::load(p);
    if (conf.name.empty()) {
        std::cerr << "Сначала: gossipsync init <имя> <адрес>\n";
        return 1;
    }

    fs::path my_feed = p.feeds / conf.name;

    // Безопасное имя файла из заголовка
    std::string safe;
    for (char c : title) {
        if (std::isalnum(c) || c == '-') safe += c;
        else if (c == ' ') safe += '-';
    }
    if (safe.empty()) safe = "post";

    std::string filename = today() + "-" + safe + ".md";
    fs::path filepath = my_feed / filename;

    std::cout << "Введите текст (Ctrl+D для завершения):\n";
    std::ostringstream body;
    body << std::cin.rdbuf();

    std::ofstream f(filepath);
    f << "# " << title << "\n\n"
      << "*" << today() << "*\n\n"
      << body.str() << "\n";

    // Вставляем ссылку в index.html
    fs::path index = my_feed / "index.html";
    if (fs::exists(index)) {
        std::string html;
        { std::ifstream in(index); std::ostringstream s; s << in.rdbuf(); html = s.str(); }
        auto pos = html.find("</ul>");
        if (pos != std::string::npos) {
            html.insert(pos, "<li><a href=\"" + filename + "\">"
                        + title + "</a> (" + today() + ")</li>\n");
            std::ofstream out(index, std::ios::trunc);
            out << html;
        }
    }

    std::cout << "Опубликовано: " << filepath << "\n";
    return 0;
}

// Показать содержимое ленты (своей или чужой)
inline int cmd_list_feed(const Paths& p, const std::string& feed_name) {
    std::string name = feed_name;
    if (name.empty()) {
        auto conf = NodeConf::load(p);
        name = conf.name;
    }

    fs::path dir = p.feeds / name;
    if (!fs::exists(dir)) {
        std::cerr << "Лента «" << name << "» не найдена.\n";
        return 1;
    }

    std::cout << "Лента «" << name << "» (" << dir << "):\n\n";
    for (auto& e : fs::directory_iterator(dir)) {
        std::cout << "  " << e.path().filename().string();
        if (e.is_regular_file())
            std::cout << "  (" << e.file_size() << " б)";
        std::cout << "\n";
    }
    return 0;
}