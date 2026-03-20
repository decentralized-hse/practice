#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <random>
#include <set>
#include <map>

namespace fs = std::filesystem;

// Все пути проекта, вычисляются от $HOME
struct Paths {
    fs::path base;         // ~/.gossipsync/
    fs::path feed;         // ~/.gossipsync/public_feed (симлинк)
    fs::path feeds;        // ~/.gossipsync/feeds/
    fs::path subs_file;    // ~/.gossipsync/subscriptions.txt
    fs::path friends_file; // ~/.gossipsync/friends.txt
    fs::path node_conf;    // ~/.gossipsync/node.conf
    fs::path key;          // ~/.gossipsync/guest_key

    Paths() {
        const char* h = std::getenv("HOME");
        if (!h) { std::cerr << "HOME не задана\n"; std::exit(1); }
        base         = fs::path(h) / ".gossipsync";
        feed         = base / "public_feed";
        feeds        = base / "feeds";
        subs_file    = base / "subscriptions.txt";
        friends_file = base / "friends.txt";
        node_conf    = base / "node.conf";
        key          = base / "guest_key";
    }

    // Создать директории, если их нет
    void ensure() const {
        for (auto& d : {base, feed, feeds})
            fs::create_directories(d);
    }
};

// Конфигурация узла: имя и SSH-адрес
struct NodeConf {
    std::string name;
    std::string address;

    // Сохранить в node.conf
    void save(const Paths& p) const {
        std::ofstream f(p.node_conf);
        f << "name=" << name << "\n"
          << "address=" << address << "\n";
    }

    // Загрузить из node.conf
    static NodeConf load(const Paths& p) {
        NodeConf c;
        if (!fs::exists(p.node_conf)) return c;
        std::ifstream f(p.node_conf);
        std::string line;
        while (std::getline(f, line)) {
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            auto k = line.substr(0, eq), v = line.substr(eq + 1);
            if (k == "name")    c.name = v;
            if (k == "address") c.address = v;
        }
        return c;
    }
};

// Друг: имя + SSH-адрес до его feeds/
struct Friend {
    std::string name;
    std::string address;
};

// Подписка: имя ленты, которую хочу читать
struct Subscription {
    std::string feed_name;
};

// Текущая дата "YYYY-MM-DD"
inline std::string today() {
    std::time_t t = std::time(nullptr);
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&t));
    return buf;
}

// Текущие дата и время
inline std::string now_str() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return buf;
}

// Прочитать строки из файла, пропуская пустые и #-комментарии
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

// Дописать строку в конец файла
inline void append_line(const fs::path& path, const std::string& line) {
    std::ofstream f(path, std::ios::app);
    f << line << "\n";
}

// Запустить shell-команду, вернуть код возврата
inline int run(const std::string& cmd) {
    std::cout << "  > " << cmd << "\n";
    return std::system(cmd.c_str());
}

// Случайное целое от lo до hi включительно
inline int rand_int(int lo, int hi) {
    static std::mt19937 rng(std::random_device{}());
    return std::uniform_int_distribution<>(lo, hi)(rng);
}

// Количество файлов/папок в директории
inline int count_files(const fs::path& dir) {
    int n = 0;
    if (fs::exists(dir))
        for (auto& _ : fs::directory_iterator(dir)) { (void)_; n++; }
    return n;
}

// Загрузить список друзей из friends.txt
inline std::vector<Friend> load_friends(const Paths& p) {
    std::vector<Friend> result;
    for (auto& line : read_lines(p.friends_file)) {
        std::istringstream ss(line);
        Friend f;
        if (ss >> f.name >> f.address)
            result.push_back(f);
    }
    return result;
}

// Загрузить подписки из subscriptions.txt
inline std::vector<Subscription> load_subs(const Paths& p) {
    std::vector<Subscription> result;
    for (auto& line : read_lines(p.subs_file)) {
        std::istringstream ss(line);
        Subscription s;
        if (ss >> s.feed_name)
            result.push_back(s);
    }
    return result;
}

// Список всех лент (подпапок в feeds/)
inline std::vector<std::string> local_feeds(const Paths& p) {
    std::vector<std::string> result;
    if (!fs::exists(p.feeds)) return result;
    for (auto& e : fs::directory_iterator(p.feeds)) {
        if (e.is_directory())
            result.push_back(e.path().filename().string());
    }
    std::sort(result.begin(), result.end());
    return result;
}
