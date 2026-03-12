#ifndef GOSSIPSYNC_COMMON_HPP
#define GOSSIPSYNC_COMMON_HPP

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <random>

namespace fs = std::filesystem;

// ── Пути проекта ─────────────────────────────────────────────
// Всё хранится в ~/.gossipsync/

struct Paths {
    fs::path base;       // ~/.gossipsync/
    fs::path feed;       // ~/.gossipsync/public_feed/
    fs::path feeds;      // ~/.gossipsync/feeds/  (скачанные ленты)
    fs::path subs_file;  // ~/.gossipsync/subscriptions.txt
    fs::path mirrors_dir;// ~/.gossipsync/mirrors/
    fs::path key;        // ~/.gossipsync/guest_key

    Paths() {
        const char* h = std::getenv("HOME");
        if (!h) { std::cerr << "HOME не задана\n"; std::exit(1); }
        base       = fs::path(h) / ".gossipsync";
        feed       = base / "public_feed";
        feeds      = base / "feeds";
        subs_file  = base / "subscriptions.txt";
        mirrors_dir= base / "mirrors";
        key        = base / "guest_key";
    }

    // Создать все нужные директории
    void ensure() const {
        for (auto& d : {base, feed, feeds, mirrors_dir})
            fs::create_directories(d);
    }
};

// ── Подписка ─────────────────────────────────────────────────

struct Subscription {
    std::string name;    // "alice"
    std::string target;  // "guest_sync@host:/path/"
};

// ── Утилиты ──────────────────────────────────────────────────

// Текущая дата YYYY-MM-DD
inline std::string today() {
    std::time_t t = std::time(nullptr);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
    return buf;
}

// Прочитать строки из файла (без пустых и комментариев)
inline std::vector<std::string> read_lines(const fs::path& path) {
    std::vector<std::string> result;
    if (!fs::exists(path)) return result;
    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty() && line[0] != '#')
            result.push_back(line);
    }
    return result;
}

// Добавить строку в файл
inline void append_line(const fs::path& path, const std::string& line) {
    std::ofstream f(path, std::ios::app);
    f << line << "\n";
}

// Запустить команду, вернуть код возврата
inline int run(const std::string& cmd) {
    std::cout << "  > " << cmd << "\n";
    return std::system(cmd.c_str());
}

// Случайное число от 0 до max
inline int rand_int(int max) {
    static std::mt19937 rng(std::random_device{}());
    return std::uniform_int_distribution<>(0, max)(rng);
}

// Посчитать файлы в директории
inline int count_files(const fs::path& dir) {
    int n = 0;
    if (fs::exists(dir))
        for (auto& _ : fs::directory_iterator(dir)) { (void)_; n++; }
    return n;
}

#endif
