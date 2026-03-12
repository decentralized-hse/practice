#ifndef GOSSIPSYNC_MIRROR_HPP
#define GOSSIPSYNC_MIRROR_HPP

#include "common.hpp"

// Стать зеркалом для скачанной ленты
inline int cmd_mirror(const Paths& p,
                      const std::string& feed_name,
                      const std::string& my_address) {
    fs::path local = p.feeds / feed_name;
    if (!fs::exists(local)) {
        std::cerr << "Лента '" << feed_name << "' не найдена.\n"
                  << "Сначала: gossipsync sync\n";
        return 1;
    }

    fs::path mf = local / "mirrors.txt";

    // Проверяем, не добавлены ли уже
    for (auto& line : read_lines(mf)) {
        if (line.find(my_address) != std::string::npos) {
            std::cout << "Уже в списке зеркал.\n";
            return 0;
        }
    }

    // Добавляем свой адрес
    append_line(mf, my_address + "\t" + today());

    // Сохраняем конфиг зеркала
    fs::path conf = p.mirrors_dir / (feed_name + ".conf");
    {
        std::ofstream f(conf);
        f << "feed=" << feed_name << "\n"
          << "dir=" << local.string() << "\n"
          << "address=" << my_address << "\n"
          << "since=" << today() << "\n";
    }

    std::cout << "Зеркало включено для '" << feed_name << "'\n"
              << "  Адрес: " << my_address << "\n\n"
              << "Другие подписчики смогут скачивать ленту с вас.\n"
              << "Настройте SSH-доступ: см. README, раздел «Зеркалирование».\n";
    return 0;
}

// Показать активные зеркала этого узла
inline int cmd_list_mirrors(const Paths& p) {
    if (!fs::exists(p.mirrors_dir)) {
        std::cout << "Нет активных зеркал.\n";
        return 0;
    }

    bool found = false;
    for (auto& e : fs::directory_iterator(p.mirrors_dir)) {
        if (e.path().extension() != ".conf") continue;
        if (!found) {
            std::cout << "Активные зеркала:\n\n";
            found = true;
        }

        // Читаем key=value из .conf
        auto lines = read_lines(e.path());
        for (auto& l : lines)
            std::cout << "  " << l << "\n";
        std::cout << "\n";
    }

    if (!found) std::cout << "Нет активных зеркал.\n";
    return 0;
}

// Показать известные зеркала для конкретной ленты
inline int cmd_feed_mirrors(const Paths& p, const std::string& feed_name) {
    fs::path mf = p.feeds / feed_name / "mirrors.txt";
    auto lines = read_lines(mf);

    if (lines.empty()) {
        std::cout << "Нет зеркал для '" << feed_name << "'.\n";
        return 0;
    }

    std::cout << "Зеркала '" << feed_name << "':\n\n";
    for (auto& l : lines)
        std::cout << "  " << l << "\n";
    std::cout << "\nВсего: " << lines.size() << "\n";
    return 0;
}

#endif
