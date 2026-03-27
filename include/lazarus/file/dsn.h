#ifndef LAZARUS_FILE_DSN_H
#define LAZARUS_FILE_DSN_H
// ============================================================================
// Lazarus C++17 Platform — Data Set Name Resolution
// Torsova LLC — lazarus-systems.com
// ============================================================================

#include <string>
#include <string_view>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sstream>
#include <functional>
#include <optional>

namespace lazarus { namespace file {

namespace fs = std::filesystem;

// ── Parsed DSN components ──────────────────────────────────────────────
struct DsnComponents {
    std::string hlq;                     // high-level qualifier
    std::vector<std::string> qualifiers; // remaining qualifiers
    std::string member;                  // PDS member (may be empty)
    int gdg_relative = 0;               // 0 = not GDG, +N/-N = relative
    bool is_gdg = false;
    bool is_temporary = false;           // &&name style
};

// ── DSN validation ─────────────────────────────────────────────────────
inline bool is_valid_qualifier(const std::string& q) {
    if (q.empty() || q.size() > 8) return false;
    // First char must be alpha or national (@, #, $)
    char c0 = static_cast<char>(std::toupper(static_cast<unsigned char>(q[0])));
    if (!std::isalpha(static_cast<unsigned char>(c0)) &&
        c0 != '@' && c0 != '#' && c0 != '$') {
        return false;
    }
    for (size_t i = 1; i < q.size(); ++i) {
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(q[i])));
        if (!std::isalnum(static_cast<unsigned char>(c)) &&
            c != '@' && c != '#' && c != '$') {
            return false;
        }
    }
    return true;
}

// ── DSN Parser ─────────────────────────────────────────────────────────
inline DsnComponents parse_dsn(const std::string& dsn) {
    DsnComponents result;
    std::string work = dsn;

    // Temporary dataset
    if (work.size() >= 2 && work[0] == '&' && work[1] == '&') {
        result.is_temporary = true;
        work = work.substr(2);
    }

    // Extract member: QUAL.QUAL(MEMBER)
    auto lp = work.find('(');
    if (lp != std::string::npos) {
        auto rp = work.find(')', lp);
        if (rp == std::string::npos)
            throw std::invalid_argument("DSN missing closing paren: " + dsn);
        std::string inside = work.substr(lp + 1, rp - lp - 1);
        work = work.substr(0, lp);

        // Check for GDG relative: (+1), (-1), (0)
        if (!inside.empty() && (inside[0] == '+' || inside[0] == '-' ||
            (inside[0] == '0' && inside.size() == 1))) {
            result.is_gdg = true;
            result.gdg_relative = std::stoi(inside);
        } else {
            result.member = inside;
        }
    }

    // Split on '.'
    std::vector<std::string> parts;
    std::stringstream ss(work);
    std::string tok;
    while (std::getline(ss, tok, '.')) {
        parts.push_back(tok);
    }

    if (parts.empty())
        throw std::invalid_argument("DSN has no qualifiers: " + dsn);

    result.hlq = parts[0];
    for (size_t i = 1; i < parts.size(); ++i)
        result.qualifiers.push_back(parts[i]);

    return result;
}

// ── DSN validation (full) ──────────────────────────────────────────────
inline bool validate_dsn(const std::string& dsn) {
    try {
        auto comp = parse_dsn(dsn);
        if (!is_valid_qualifier(comp.hlq)) return false;
        for (auto& q : comp.qualifiers) {
            if (!is_valid_qualifier(q)) return false;
        }
        if (!comp.member.empty() && !is_valid_qualifier(comp.member))
            return false;
        return true;
    } catch (...) {
        return false;
    }
}

// ── Full DSN string from components ────────────────────────────────────
inline std::string dsn_to_string(const DsnComponents& c) {
    std::string out;
    if (c.is_temporary) out += "&&";
    out += c.hlq;
    for (auto& q : c.qualifiers) {
        out += '.';
        out += q;
    }
    if (!c.member.empty()) {
        out += '(';
        out += c.member;
        out += ')';
    }
    if (c.is_gdg) {
        out += '(';
        if (c.gdg_relative >= 0) out += '+';
        out += std::to_string(c.gdg_relative);
        out += ')';
    }
    return out;
}

// ── UNIT/VOL resolution ────────────────────────────────────────────────
// Maps a DSN to a filesystem path under a configurable root.
// Pattern: root / HLQ / QUAL1 / QUAL2 / ...
class DsnResolver {
public:
    explicit DsnResolver(const fs::path& root) : root_(root) {}

    fs::path resolve(const std::string& dsn) const {
        auto comp = parse_dsn(dsn);
        fs::path p = root_;
        if (comp.is_temporary) {
            p = fs::temp_directory_path() / "lazarus_temp_dsn";
        }
        p /= comp.hlq;
        for (auto& q : comp.qualifiers) {
            p /= q;
        }
        if (!comp.member.empty()) {
            // PDS: directory for base, file for member
            p /= comp.member;
        }
        return p;
    }

    fs::path resolve_base(const std::string& dsn) const {
        auto comp = parse_dsn(dsn);
        fs::path p = root_;
        if (comp.is_temporary) {
            p = fs::temp_directory_path() / "lazarus_temp_dsn";
        }
        p /= comp.hlq;
        for (auto& q : comp.qualifiers) {
            p /= q;
        }
        return p;
    }

    const fs::path& root() const { return root_; }

private:
    fs::path root_;
};

// ── Concatenation ──────────────────────────────────────────────────────
// Reads sequentially through a list of paths (like DD CONCAT)
class DsnConcatenation {
public:
    void add(const fs::path& path) {
        paths_.push_back(path);
    }

    // Read all lines sequentially across concatenated datasets
    std::vector<std::string> read_all_lines() const {
        std::vector<std::string> lines;
        for (auto& p : paths_) {
            if (!fs::exists(p)) continue;
            std::ifstream in(p);
            std::string line;
            while (std::getline(in, line)) {
                lines.push_back(line);
            }
        }
        return lines;
    }

    // Read all raw bytes concatenated
    std::string read_all_bytes() const {
        std::string result;
        for (auto& p : paths_) {
            if (!fs::exists(p)) continue;
            std::ifstream in(p, std::ios::binary);
            std::ostringstream oss;
            oss << in.rdbuf();
            result += oss.str();
        }
        return result;
    }

    size_t count() const { return paths_.size(); }
    const std::vector<fs::path>& paths() const { return paths_; }

private:
    std::vector<fs::path> paths_;
};

// ── Temporary Dataset ──────────────────────────────────────────────────
class TempDataset {
public:
    explicit TempDataset(const std::string& name) : name_(name) {
        path_ = fs::temp_directory_path() / ("lazarus_tmp_" + name);
    }

    ~TempDataset() {
        cleanup();
    }

    // Non-copyable, movable
    TempDataset(const TempDataset&) = delete;
    TempDataset& operator=(const TempDataset&) = delete;
    TempDataset(TempDataset&& o) noexcept
        : name_(std::move(o.name_)), path_(std::move(o.path_)) {
        o.path_.clear();
    }
    TempDataset& operator=(TempDataset&& o) noexcept {
        if (this != &o) {
            cleanup();
            name_ = std::move(o.name_);
            path_ = std::move(o.path_);
            o.path_.clear();
        }
        return *this;
    }

    const fs::path& path() const { return path_; }
    const std::string& name() const { return name_; }

    void write(const std::string& data) {
        fs::create_directories(path_.parent_path());
        std::ofstream out(path_, std::ios::binary);
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
    }

    std::string read() const {
        if (!fs::exists(path_)) return {};
        std::ifstream in(path_, std::ios::binary);
        std::ostringstream oss;
        oss << in.rdbuf();
        return oss.str();
    }

    void cleanup() {
        if (!path_.empty() && fs::exists(path_)) {
            std::error_code ec;
            fs::remove(path_, ec);
        }
    }

private:
    std::string name_;
    fs::path path_;
};

}} // namespace lazarus::file

#endif // LAZARUS_FILE_DSN_H
