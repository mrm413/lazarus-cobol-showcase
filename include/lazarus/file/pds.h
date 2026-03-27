#ifndef LAZARUS_FILE_PDS_H
#define LAZARUS_FILE_PDS_H
// ============================================================================
// Lazarus C++17 Platform — Partitioned Data Set
// Torsova LLC — lazarus-systems.com
// ============================================================================

#include <string>
#include <vector>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <regex>
#include <chrono>
#include <optional>
#include <functional>

namespace lazarus { namespace file {

namespace fs = std::filesystem;

// ── Member statistics ──────────────────────────────────────────────────
struct MemberStats {
    std::string name;
    uintmax_t size_bytes = 0;
    std::string last_modified; // ISO-style string
};

// ── PDS: directory-as-dataset abstraction ──────────────────────────────
class Pds {
public:
    explicit Pds(const fs::path& directory)
        : dir_(directory) {
        fs::create_directories(dir_);
    }

    const fs::path& directory() const { return dir_; }

    // ── Member operations ──────────────────────────────────────────────

    // WRITE member (create or overwrite)
    void write_member(const std::string& name, const std::string& data) {
        validate_member_name(name);
        fs::path p = dir_ / name;
        std::ofstream out(p, std::ios::binary);
        if (!out) throw std::runtime_error("Cannot write member: " + name);
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
    }

    // READ member
    std::string read_member(const std::string& name) const {
        fs::path p = dir_ / name;
        if (!fs::exists(p))
            throw std::runtime_error("Member not found: " + name);
        std::ifstream in(p, std::ios::binary);
        std::ostringstream oss;
        oss << in.rdbuf();
        return oss.str();
    }

    // DELETE member
    bool delete_member(const std::string& name) {
        fs::path p = dir_ / name;
        if (!fs::exists(p)) return false;
        return fs::remove(p);
    }

    // RENAME member
    bool rename_member(const std::string& old_name,
                       const std::string& new_name) {
        validate_member_name(new_name);
        fs::path old_p = dir_ / old_name;
        fs::path new_p = dir_ / new_name;
        if (!fs::exists(old_p)) return false;
        if (fs::exists(new_p)) return false;
        fs::rename(old_p, new_p);
        return true;
    }

    // EXISTS check
    bool member_exists(const std::string& name) const {
        return fs::exists(dir_ / name);
    }

    // ── Directory listing ──────────────────────────────────────────────

    // List all member names (sorted)
    std::vector<std::string> list_members() const {
        std::vector<std::string> result;
        if (!fs::exists(dir_)) return result;
        for (auto& entry : fs::directory_iterator(dir_)) {
            if (entry.is_regular_file()) {
                result.push_back(entry.path().filename().string());
            }
        }
        std::sort(result.begin(), result.end());
        return result;
    }

    // List with statistics
    std::vector<MemberStats> list_members_with_stats() const {
        std::vector<MemberStats> result;
        if (!fs::exists(dir_)) return result;
        for (auto& entry : fs::directory_iterator(dir_)) {
            if (entry.is_regular_file()) {
                MemberStats ms;
                ms.name = entry.path().filename().string();
                ms.size_bytes = entry.file_size();
                auto ftime = entry.last_write_time();
                // C++17 portable: use file_time_type epoch directly
                auto dur = ftime.time_since_epoch();
                auto secs = std::chrono::duration_cast<std::chrono::seconds>(dur);
                ms.last_modified = std::to_string(secs.count());
                result.push_back(ms);
            }
        }
        std::sort(result.begin(), result.end(),
            [](const MemberStats& a, const MemberStats& b) {
                return a.name < b.name;
            });
        return result;
    }

    size_t member_count() const {
        size_t n = 0;
        if (!fs::exists(dir_)) return 0;
        for (auto& entry : fs::directory_iterator(dir_)) {
            if (entry.is_regular_file()) ++n;
        }
        return n;
    }

    // ── Pattern matching (glob-style) ──────────────────────────────────
    // Supports: * (any chars), ? (single char)
    std::vector<std::string> select_members(const std::string& pattern) const {
        std::string regex_pat = glob_to_regex(pattern);
        std::regex re(regex_pat, std::regex::icase);
        std::vector<std::string> result;

        for (auto& name : list_members()) {
            if (std::regex_match(name, re))
                result.push_back(name);
        }
        return result;
    }

    // ── IEBCOPY-style merge ────────────────────────────────────────────
    // Copy members from source PDS. replace_existing controls overwrite.
    struct MergeResult {
        size_t copied = 0;
        size_t skipped = 0;
        size_t replaced = 0;
        std::vector<std::string> copied_names;
    };

    MergeResult merge_from(const Pds& source, bool replace_existing = false) {
        MergeResult result;
        for (auto& name : source.list_members()) {
            bool exists = member_exists(name);
            if (exists && !replace_existing) {
                ++result.skipped;
                continue;
            }
            std::string data = source.read_member(name);
            write_member(name, data);
            if (exists)
                ++result.replaced;
            else
                ++result.copied;
            result.copied_names.push_back(name);
        }
        return result;
    }

    // Selective merge with pattern
    MergeResult merge_from(const Pds& source, const std::string& pattern,
                           bool replace_existing = false) {
        MergeResult result;
        auto selected = source.select_members(pattern);
        for (auto& name : selected) {
            bool exists = member_exists(name);
            if (exists && !replace_existing) {
                ++result.skipped;
                continue;
            }
            std::string data = source.read_member(name);
            write_member(name, data);
            if (exists)
                ++result.replaced;
            else
                ++result.copied;
            result.copied_names.push_back(name);
        }
        return result;
    }

private:
    fs::path dir_;

    static void validate_member_name(const std::string& name) {
        if (name.empty() || name.size() > 8)
            throw std::invalid_argument(
                "Member name must be 1-8 characters: " + name);
        // Allow alphanumeric, @, #, $ (mainframe member rules)
        for (char c : name) {
            char u = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            if (!std::isalnum(static_cast<unsigned char>(u)) &&
                u != '@' && u != '#' && u != '$') {
                throw std::invalid_argument(
                    "Invalid character in member name: " + name);
            }
        }
    }

    static std::string glob_to_regex(const std::string& glob) {
        std::string result = "^";
        for (char c : glob) {
            switch (c) {
                case '*': result += ".*"; break;
                case '?': result += "."; break;
                case '.': result += "\\."; break;
                case '(': result += "\\("; break;
                case ')': result += "\\)"; break;
                case '[': result += "\\["; break;
                case ']': result += "\\]"; break;
                case '{': result += "\\{"; break;
                case '}': result += "\\}"; break;
                case '\\': result += "\\\\"; break;
                case '+': result += "\\+"; break;
                case '^': result += "\\^"; break;
                case '$': result += "\\$"; break;
                case '|': result += "\\|"; break;
                default: result += c; break;
            }
        }
        result += "$";
        return result;
    }
};

}} // namespace lazarus::file

#endif // LAZARUS_FILE_PDS_H
