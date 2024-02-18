#pragma once

#include <cassert>
#include <fstream>
#include <list>
#include <optional>
#include <sstream>
#include <regex>
#include <iostream>

#include <fmt/core.h>

#include <picosha2.h>

enum class EntryType {
  Blob,
  Tree,
};

struct Entry {
  std::string name;
  std::string hash;
  EntryType type;
};

class Tree {
 private:
  static Entry ParseLine(const std::string& line) {
    static const std::regex blob_matcher("^([^\t :/]+):\t([a-fA-F0-9]{64})$");
    static const std::regex tree_matcher("^([^\t :/]+)/\t([a-fA-F0-9]{64})$");

    std::smatch pieces_match;
    Entry entry;

    if (std::regex_match(line, pieces_match, blob_matcher)) {
      entry.type = EntryType::Blob;
    } else if (std::regex_match(line, pieces_match, tree_matcher)) {
      entry.type = EntryType::Tree;
    } else {
      throw std::logic_error("incorrect format of tree file");
    }

    assert(pieces_match.size() == 3);

    entry.name = pieces_match[1].str();
    entry.hash = pieces_match[2].str();

    return entry;
  }

  explicit Tree(std::list<Entry> entries)
    : entries_{std::move(entries)} {}

  auto FindIt(const std::string& name) {
    auto it = std::find_if(entries_.begin(), entries_.end(), [&name](const Entry& entry) {
      return entry.name == name;
    });

    if (it == entries_.end()) {
      throw std::logic_error(fmt::format("entry: '{}' not found in tree", name));
    }

    return it;
  }

  std::string Serialize() {
    std::stringstream ss;

    for (const auto& entry : entries_) {
      if (entry.type == EntryType::Blob) {
        ss << entry.name << ":\t" << entry.hash << "\n";
      } else {
        ss << entry.name << "/\t" << entry.hash << "\n";
      }
    }

    return ss.str();
  }

 public:
  static Tree fromHash(const std::string& hash) {
    std::ifstream tree_file(hash, std::ios::in);

    if (!tree_file.good()) {
      throw std::logic_error(fmt::format("tree: '{}' not found", hash));
    }

    std::list<Entry> entries;

    for (std::string line; std::getline(tree_file, line);) {
      entries.push_back(ParseLine(line));
    }

    return Tree(std::move(entries));
  }

  const Entry& FindEntry(const std::string& name) {
    return *FindIt(name);
  }

  void RemoveEntry(const std::string& name) {
    entries_.erase(FindIt(name));
  }

  void ReplaceHash(const std::string& name, const std::string& hash) {
    FindIt(name)->hash = hash;
  }

  void WriteToFile() {
    std::string data = Serialize();
    std::string hash = picosha2::hash256_hex_string(data);

    std::ofstream tree_file(hash, std::ios::out | std::ios::trunc);

    if (!tree_file.good()) {
      throw std::runtime_error("can not open output file");
    }

    tree_file << data;
    tree_file.close();
  }

  std::string CalculateHash() {
    return picosha2::hash256_hex_string(Serialize());
  }

 private:
  std::list<Entry> entries_;
};
