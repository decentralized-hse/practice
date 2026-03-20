#pragma once

#include "common.hpp"

// Добавить друга в friends.txt
inline int cmd_add_friend(const Paths& p,
                          const std::string& name,
                          const std::string& address) {
    p.ensure();
    for (auto& f : load_friends(p)) {
        if (f.name == name) {
            std::cerr << "«" << name << "» уже в друзьях.\n";
            return 1;
        }
    }
    append_line(p.friends_file, name + "\t" + address);
    std::cout << "Друг добавлен: " << name << " (" << address << ")\n";
    return 0;
}

// Удалить друга из friends.txt
inline int cmd_remove_friend(const Paths& p, const std::string& name) {
    auto lines = read_lines(p.friends_file);
    std::ofstream out(p.friends_file, std::ios::trunc);
    bool found = false;
    for (auto& line : lines) {
        std::istringstream ss(line);
        std::string n;
        ss >> n;
        if (n == name) {
            found = true;
        } else {
            out << line << "\n";
        }
    }
    if (!found) {
        std::cerr << "«" << name << "» не найден в друзьях.\n";
        return 1;
    }
    std::cout << "Друг удалён: " << name << "\n";
    return 0;
}

// Показать список друзей
inline int cmd_list_friends(const Paths& p) {
    auto friends = load_friends(p);
    if (friends.empty()) {
        std::cout << "Нет друзей.\n"
                  << "  gossipsync add-friend <имя> <адрес>\n";
        return 0;
    }
    std::cout << "Друзья (" << friends.size() << "):\n\n";
    for (auto& f : friends) {
        std::cout << "  " << f.name << "\n"
                  << "    адрес: " << f.address << "\n";
    }
    return 0;
}