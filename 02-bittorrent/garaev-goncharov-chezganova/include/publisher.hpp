#ifndef GOSSIPSYNC_PUBLISHER_HPP
#define GOSSIPSYNC_PUBLISHER_HPP

#include "common.hpp"

// Инициализация узла: директории, SSH-ключ, начальные файлы
inline int cmd_init(const Paths& p) {
    std::cout << "Инициализация GossipSync...\n\n";
    p.ensure();

    // Начальный index.html
    fs::path index = p.feed / "index.html";
    if (!fs::exists(index)) {
        std::ofstream f(index);
        f << "<!DOCTYPE html>\n<html><body>\n"
          << "<h1>GossipSync Feed</h1>\n"
          << "<ul id='posts'></ul>\n"
          << "</body></html>\n";
        std::cout << "  Создан: " << index << "\n";
    }

    // Пустой mirrors.txt (список зеркал этой ленты)
    fs::path mirrors = p.feed / "mirrors.txt";
    if (!fs::exists(mirrors)) {
        std::ofstream(mirrors) << "# Зеркала этой ленты: user@host:/path/\n";
        std::cout << "  Создан: " << mirrors << "\n";
    }

    // Пустой файл подписок
    if (!fs::exists(p.subs_file)) {
        std::ofstream(p.subs_file)
            << "# Подписки: имя<TAB>user@host:/path/\n";
    }

    // SSH-ключ для гостевого доступа
    if (!fs::exists(p.key)) {
        std::cout << "\n  Генерация SSH-ключа...\n";
        int rc = run("ssh-keygen -t ed25519 -f " + p.key.string()
                      + " -N '' -C gossipsync -q");
        if (rc != 0) {
            std::cerr << "  Ошибка ssh-keygen\n";
            return 1;
        }
    }
    std::cout << "  Ключ: " << p.key << "\n";

    // Скрипт настройки гостевого SSH (rrsync read-only)
    fs::path script = p.base / "setup-guest-ssh.sh";
    {
        std::ofstream f(script);
        f << "#!/bin/bash\n"
          << "# Запустите от root для создания гостевого SSH-пользователя\n"
          << "set -euo pipefail\n\n"
          << "USER=guest_sync\n"
          << "FEED=\"" << p.feed.string() << "\"\n"
          << "PUBKEY=\"" << p.key.string() << ".pub\"\n\n"
          << "RRSYNC=$(find /usr -name rrsync -type f 2>/dev/null | head -1)\n"
          << "[ -z \"$RRSYNC\" ] && { echo 'rrsync не найден'; exit 1; }\n"
          << "chmod +x \"$RRSYNC\"\n\n"
          << "id \"$USER\" &>/dev/null || useradd -r -m -s /bin/bash \"$USER\"\n"
          << "H=$(eval echo ~$USER)\n"
          << "mkdir -p \"$H/.ssh\" && chmod 700 \"$H/.ssh\"\n\n"
          << "echo \"command=\\\"$RRSYNC -ro $FEED\\\","
          << "no-agent-forwarding,no-port-forwarding,no-pty "
          << "$(cat $PUBKEY)\" > \"$H/.ssh/authorized_keys\"\n"
          << "chmod 600 \"$H/.ssh/authorized_keys\"\n"
          << "chown -R \"$USER:$USER\" \"$H/.ssh\"\n\n"
          << "echo \"Готово! Подписчики могут синхронизироваться:\"\n"
          << "echo \"  rsync -avz -e \\\"ssh -i guest_key\\\" "
          << "guest_sync@$(hostname):/ ./feed/\"\n";
        fs::permissions(script,
            fs::perms::owner_all | fs::perms::group_read | fs::perms::others_read,
            fs::perm_options::replace);
    }

    std::cout << "\nГотово!\n"
              << "  Лента:   " << p.feed << "\n"
              << "  Скрипт:  " << script << "\n"
              << "  Далее: добавляйте файлы в ленту, настройте SSH\n";
    return 0;
}

// Создать новый пост в ленте
inline int cmd_post(const Paths& p, const std::string& title) {
    if (!fs::exists(p.feed)) {
        std::cerr << "Сначала: gossipsync init\n";
        return 1;
    }

    // Безопасное имя файла
    std::string safe;
    for (char c : title) {
        if (std::isalnum(c) || c == '-') safe += c;
        else if (c == ' ') safe += '-';
    }
    if (safe.empty()) safe = "post";

    std::string filename = today() + "-" + safe + ".md";
    fs::path filepath = p.feed / filename;

    // Тело поста — из stdin
    std::cout << "Введите текст (Ctrl+D для завершения):\n";
    std::ostringstream body;
    body << std::cin.rdbuf();

    std::ofstream f(filepath);
    f << "# " << title << "\n\n"
      << "*" << today() << "*\n\n"
      << body.str() << "\n";

    // Обновляем index.html
    fs::path index = p.feed / "index.html";
    if (fs::exists(index)) {
        std::string html;
        { std::ifstream in(index); std::ostringstream s; s << in.rdbuf(); html = s.str(); }
        auto pos = html.find("</ul>");
        if (pos != std::string::npos) {
            std::string li = "<li><a href=\"" + filename + "\">"
                           + title + "</a> (" + today() + ")</li>\n";
            html.insert(pos, li);
            std::ofstream out(index, std::ios::trunc);
            out << html;
        }
    }

    std::cout << "Опубликовано: " << filepath << "\n";
    return 0;
}

// Показать содержимое ленты
inline int cmd_list_feed(const Paths& p) {
    if (!fs::exists(p.feed)) {
        std::cerr << "Лента не создана. Выполните: gossipsync init\n";
        return 1;
    }
    std::cout << "Лента: " << p.feed << "\n\n";
    for (auto& e : fs::directory_iterator(p.feed)) {
        std::cout << "  " << e.path().filename().string();
        if (e.is_regular_file())
            std::cout << "  (" << e.file_size() << " б)";
        std::cout << "\n";
    }
    return 0;
}

#endif
