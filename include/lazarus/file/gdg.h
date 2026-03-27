#ifndef LAZARUS_FILE_GDG_H
#define LAZARUS_FILE_GDG_H
// ============================================================================
// Lazarus C++17 Platform — Generation Data Group Management
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
#include <iomanip>
#include <optional>
#include <functional>
#include <regex>
#include <cstdio>

namespace lazarus { namespace file {

namespace fs = std::filesystem;

// ── Scratch policy ─────────────────────────────────────────────────────
enum class GdgScratch { SCRATCH, NOSCRATCH };

// ── Generation naming: HLQ.BASE.GnnnnVnn ───────────────────────────────
inline std::string format_generation_name(const std::string& base,
                                          uint32_t gen_num,
                                          uint32_t version = 0) {
    std::ostringstream oss;
    oss << base << ".G"
        << std::setw(4) << std::setfill('0') << gen_num
        << "V" << std::setw(2) << std::setfill('0') << version;
    return oss.str();
}

// Parse GnnnnVnn suffix -> (gen_num, version)
inline bool parse_generation_suffix(const std::string& name,
                                    uint32_t& gen_num,
                                    uint32_t& version) {
    // Find last component
    auto dot = name.rfind('.');
    if (dot == std::string::npos) return false;
    std::string suffix = name.substr(dot + 1);
    if (suffix.size() < 7) return false; // GnnnnVnn minimum
    if (suffix[0] != 'G') return false;
    auto vpos = suffix.find('V');
    if (vpos == std::string::npos || vpos < 2) return false;
    try {
        gen_num = static_cast<uint32_t>(std::stoul(suffix.substr(1, vpos - 1)));
        version = static_cast<uint32_t>(std::stoul(suffix.substr(vpos + 1)));
        return true;
    } catch (...) {
        return false;
    }
}

// ── Generation entry in catalog ────────────────────────────────────────
struct GdgEntry {
    uint32_t gen_num;
    uint32_t version;
    std::string full_name;
    fs::path data_path;
    bool active = true;
};

// ── GDG Base — directory-backed generation catalog ─────────────────────
class GdgBase {
public:
    GdgBase(const std::string& base_name,
            const fs::path& catalog_dir,
            uint32_t limit = 255,
            GdgScratch scratch = GdgScratch::SCRATCH)
        : base_name_(base_name)
        , catalog_dir_(catalog_dir)
        , limit_(limit)
        , scratch_(scratch)
        , next_gen_(1) {
        fs::create_directories(catalog_dir_);
        load_catalog();
    }

    const std::string& base_name() const { return base_name_; }
    uint32_t limit() const { return limit_; }
    GdgScratch scratch_policy() const { return scratch_; }

    // Current number of active generations
    size_t generation_count() const {
        size_t count = 0;
        for (auto& e : entries_)
            if (e.active) ++count;
        return count;
    }

    // All active entries (oldest first)
    std::vector<GdgEntry> active_entries() const {
        std::vector<GdgEntry> result;
        for (auto& e : entries_)
            if (e.active) result.push_back(e);
        return result;
    }

    // Resolve relative generation: 0 = current, -1 = previous, +1 = new
    std::optional<GdgEntry> resolve_relative(int rel) const {
        auto active = active_entries();
        if (active.empty() && rel <= 0) return std::nullopt;
        if (rel > 0) return std::nullopt; // +1 means "allocate new"
        int idx = static_cast<int>(active.size()) - 1 + rel;
        if (idx < 0 || idx >= static_cast<int>(active.size()))
            return std::nullopt;
        return active[static_cast<size_t>(idx)];
    }

    // Resolve absolute generation number
    std::optional<GdgEntry> resolve_absolute(uint32_t gen_num) const {
        for (auto& e : entries_) {
            if (e.gen_num == gen_num && e.active)
                return e;
        }
        return std::nullopt;
    }

    // Allocate a new generation (+1)
    GdgEntry allocate_new() {
        GdgEntry entry;
        entry.gen_num = next_gen_++;
        entry.version = 0;
        entry.full_name = format_generation_name(base_name_, entry.gen_num,
                                                  entry.version);
        entry.data_path = catalog_dir_ / (entry.full_name);
        entry.active = true;

        entries_.push_back(entry);
        enforce_limit();
        save_catalog();
        return entry;
    }

    // Roll back (deactivate) the newest generation
    bool rollback_latest() {
        for (auto it = entries_.rbegin(); it != entries_.rend(); ++it) {
            if (it->active) {
                it->active = false;
                if (scratch_ == GdgScratch::SCRATCH) {
                    std::error_code ec;
                    fs::remove_all(it->data_path, ec);
                }
                save_catalog();
                return true;
            }
        }
        return false;
    }

    // Get path for generation(0) — current
    fs::path current_path() const {
        auto e = resolve_relative(0);
        if (!e) throw std::runtime_error("No current GDG generation");
        return e->data_path;
    }

private:
    std::string base_name_;
    fs::path catalog_dir_;
    uint32_t limit_;
    GdgScratch scratch_;
    uint32_t next_gen_;
    std::vector<GdgEntry> entries_;

    fs::path catalog_file() const {
        return catalog_dir_ / (base_name_ + ".gdgcat");
    }

    void enforce_limit() {
        // Count active
        size_t count = 0;
        for (auto& e : entries_)
            if (e.active) ++count;

        // Roll off oldest when over limit
        for (auto& e : entries_) {
            if (count <= limit_) break;
            if (e.active) {
                e.active = false;
                if (scratch_ == GdgScratch::SCRATCH) {
                    std::error_code ec;
                    fs::remove_all(e.data_path, ec);
                }
                --count;
            }
        }
    }

    void save_catalog() {
        std::ofstream out(catalog_file());
        out << "NEXT=" << next_gen_ << "\n";
        for (auto& e : entries_) {
            out << (e.active ? "A" : "I") << "|"
                << e.gen_num << "|"
                << e.version << "|"
                << e.full_name << "|"
                << e.data_path.string() << "\n";
        }
    }

    void load_catalog() {
        auto cf = catalog_file();
        if (!fs::exists(cf)) return;

        std::ifstream in(cf);
        std::string line;

        // First line: NEXT=n
        if (std::getline(in, line)) {
            if (line.substr(0, 5) == "NEXT=")
                next_gen_ = static_cast<uint32_t>(
                    std::stoul(line.substr(5)));
        }

        entries_.clear();
        while (std::getline(in, line)) {
            if (line.empty()) continue;
            GdgEntry e;
            std::stringstream ss(line);
            std::string tok;

            std::getline(ss, tok, '|');
            e.active = (tok == "A");

            std::getline(ss, tok, '|');
            e.gen_num = static_cast<uint32_t>(std::stoul(tok));

            std::getline(ss, tok, '|');
            e.version = static_cast<uint32_t>(std::stoul(tok));

            std::getline(ss, tok, '|');
            e.full_name = tok;

            std::getline(ss, tok, '|');
            e.data_path = tok;

            entries_.push_back(e);
        }
    }
};

// ── GDG Generation — RAII lifecycle ────────────────────────────────────
class GdgGeneration {
public:
    GdgGeneration(GdgBase& base)
        : base_(base), committed_(false) {
        entry_ = base_.allocate_new();
        // Create the data file location
        fs::create_directories(entry_.data_path.parent_path());
    }

    ~GdgGeneration() {
        if (!committed_) {
            rollback();
        }
    }

    // Non-copyable
    GdgGeneration(const GdgGeneration&) = delete;
    GdgGeneration& operator=(const GdgGeneration&) = delete;

    void commit() { committed_ = true; }
    void rollback() {
        if (!committed_) {
            base_.rollback_latest();
            committed_ = true; // prevent double rollback
        }
    }

    const GdgEntry& entry() const { return entry_; }
    const fs::path& path() const { return entry_.data_path; }
    const std::string& name() const { return entry_.full_name; }

    // Write data to this generation
    void write(const std::string& data) {
        std::ofstream out(entry_.data_path, std::ios::binary);
        out.write(data.data(), static_cast<std::streamsize>(data.size()));
    }

    std::string read() const {
        std::ifstream in(entry_.data_path, std::ios::binary);
        std::ostringstream oss;
        oss << in.rdbuf();
        return oss.str();
    }

private:
    GdgBase& base_;
    GdgEntry entry_;
    bool committed_;
};

}} // namespace lazarus::file

#endif // LAZARUS_FILE_GDG_H
