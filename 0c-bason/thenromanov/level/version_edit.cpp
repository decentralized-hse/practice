#include "level/version_edit.hpp"

#include "util/util.hpp"

#include <string>

namespace bason_db {

    BasonRecord to_bason(const VersionEdit::FileAdd& add) {
        return BasonRecord{
            .type = BasonType::Object,
            .key = "",
            .value = "",
            .children =
                {
                    make_number_record("level", static_cast<uint64_t>(add.level)),
                    make_string_record("file", add.file.string()),
                    make_string_record("first_key", add.first_key),
                    make_string_record("last_key", add.last_key),
                    make_number_record("size", add.file_size),
                    make_number_record("min_offset", add.min_offset),
                    make_number_record("max_offset", add.max_offset),
                },
        };
    }

    void from_bason(const BasonRecord& record, VersionEdit::FileAdd& out) {
        for (const auto& child : record.children) {
            if (child.key == "level") {
                out.level = std::stoi(child.value);
            } else if (child.key == "file") {
                out.file = child.value;
            } else if (child.key == "first_key") {
                out.first_key = child.value;
            } else if (child.key == "last_key") {
                out.last_key = child.value;
            } else if (child.key == "size") {
                out.file_size = std::stoull(child.value);
            } else if (child.key == "min_offset") {
                out.min_offset = std::stoull(child.value);
            } else if (child.key == "max_offset") {
                out.max_offset = std::stoull(child.value);
            }
        }
    }

    BasonRecord to_bason(const VersionEdit::FileRemove& remove) {
        return BasonRecord{
            .type = BasonType::Object,
            .key = "",
            .value = "",
            .children =
                {
                    make_number_record("level", static_cast<uint64_t>(remove.level)),
                    make_string_record("file", remove.file.string()),
                },
        };
    }

    void from_bason(const BasonRecord& record, VersionEdit::FileRemove& out) {
        for (const auto& child : record.children) {
            if (child.key == "level") {
                out.level = std::stoi(child.value);
            } else if (child.key == "file") {
                out.file = child.value;
            }
        }
    }

    BasonRecord to_bason(const VersionEdit& edit) {
        auto record = BasonRecord{
            .type = BasonType::Object,
            .key = "",
            .value = "",
            .children = {},
        };

        if (!edit.additions.empty()) {
            auto add_array = BasonRecord{};
            add_array.type = BasonType::Array;
            add_array.key = "add";
            for (const auto& add : edit.additions) {
                add_array.children.emplace_back(to_bason(add));
            }
            record.children.emplace_back(std::move(add_array));
        }

        if (!edit.removals.empty()) {
            auto remove_array = BasonRecord{};
            remove_array.type = BasonType::Array;
            remove_array.key = "remove";
            for (const auto& remove : edit.removals) {
                remove_array.children.emplace_back(to_bason(remove));
            }
            record.children.emplace_back(std::move(remove_array));
        }

        record.children.emplace_back(make_number_record("flush_offset", edit.flush_offset));

        return record;
    }

    void from_bason(const BasonRecord& record, VersionEdit& out) {
        for (const auto& child : record.children) {
            if (child.key == "add" && child.type == BasonType::Array) {
                for (const auto& item : child.children) {
                    from_bason(item, out.additions.emplace_back());
                }
            } else if (child.key == "remove" && child.type == BasonType::Array) {
                for (const auto& item : child.children) {
                    from_bason(item, out.removals.emplace_back());
                }
            } else if (child.key == "flush_offset" && child.type == BasonType::Number) {
                out.flush_offset = std::stoull(child.value);
            }
        }
    }

} // namespace bason_db
