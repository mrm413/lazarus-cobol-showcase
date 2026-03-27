#ifndef LAZARUS_FILE_VSAM_H
#define LAZARUS_FILE_VSAM_H
// ============================================================================
// Lazarus C++17 Platform — VSAM Implementation
// Torsova LLC — lazarus-systems.com
// ============================================================================

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <algorithm>
#include <optional>
#include <functional>
#include <sstream>
#include <cassert>

namespace lazarus { namespace file {

namespace fs = std::filesystem;

// ── VSAM return codes ──────────────────────────────────────────────────
enum class VsamRc {
    OK = 0,
    NOT_FOUND = 8,
    DUPLICATE_KEY = 8,
    END_OF_FILE = 4,
    INVALID_REQUEST = 12,
    IO_ERROR = 16
};

// ── Cluster definition parameters ──────────────────────────────────────
struct ClusterDef {
    std::string name;
    uint32_t key_offset = 0;
    uint32_t key_length = 0;
    uint32_t record_size = 0;       // 0 = variable
    uint32_t max_record_size = 32760;
    bool reuse = false;
    bool unique = true;
};

// ── Browse state ───────────────────────────────────────────────────────
template<typename KeyType>
struct BrowseState {
    bool active = false;
    KeyType current;
    bool forward = true;
};

// ── KSDS: Key-Sequenced Data Set ───────────────────────────────────────
class KsdsFile {
public:
    KsdsFile(const fs::path& path, const ClusterDef& def)
        : path_(path), def_(def) {
        load();
    }

    ~KsdsFile() { save(); }

    KsdsFile(const KsdsFile&) = delete;
    KsdsFile& operator=(const KsdsFile&) = delete;

    // ── CRUD operations ────────────────────────────────────────────────
    VsamRc put(const std::string& key, const std::string& record) {
        if (def_.unique && records_.count(key))
            return VsamRc::DUPLICATE_KEY;
        records_[key] = record;
        update_alt_indexes(key, record);
        return VsamRc::OK;
    }

    VsamRc get(const std::string& key, std::string& record) const {
        auto it = records_.find(key);
        if (it == records_.end()) return VsamRc::NOT_FOUND;
        record = it->second;
        return VsamRc::OK;
    }

    VsamRc update(const std::string& key, const std::string& record) {
        auto it = records_.find(key);
        if (it == records_.end()) return VsamRc::NOT_FOUND;
        remove_alt_index_entries(key, it->second);
        it->second = record;
        update_alt_indexes(key, record);
        return VsamRc::OK;
    }

    VsamRc erase(const std::string& key) {
        auto it = records_.find(key);
        if (it == records_.end()) return VsamRc::NOT_FOUND;
        remove_alt_index_entries(key, it->second);
        records_.erase(it);
        return VsamRc::OK;
    }

    // ── Point: position for subsequent browse ──────────────────────────
    VsamRc point(const std::string& key) {
        browse_.active = true;
        browse_.current = key;
        browse_.forward = true;
        return VsamRc::OK;
    }

    // ── Browse operations ──────────────────────────────────────────────
    VsamRc startbr(const std::string& key) {
        browse_.active = true;
        browse_.current = key;
        browse_.forward = true;
        return VsamRc::OK;
    }

    VsamRc readnext(std::string& key, std::string& record) {
        if (!browse_.active) return VsamRc::INVALID_REQUEST;
        auto it = records_.lower_bound(browse_.current);
        if (it == records_.end()) {
            browse_.active = false;
            return VsamRc::END_OF_FILE;
        }
        key = it->first;
        record = it->second;
        ++it;
        if (it != records_.end())
            browse_.current = it->first;
        else
            browse_.current = key + '\xFF'; // past end
        return VsamRc::OK;
    }

    VsamRc readprev(std::string& key, std::string& record) {
        if (!browse_.active) return VsamRc::INVALID_REQUEST;
        auto it = records_.upper_bound(browse_.current);
        if (it == records_.begin()) {
            browse_.active = false;
            return VsamRc::END_OF_FILE;
        }
        --it;
        key = it->first;
        record = it->second;
        if (it != records_.begin()) {
            auto prev = it;
            --prev;
            browse_.current = prev->first;
        } else {
            browse_.current.clear();
        }
        return VsamRc::OK;
    }

    VsamRc endbr() {
        browse_.active = false;
        return VsamRc::OK;
    }

    // ── Alternate index ────────────────────────────────────────────────
    using AltKeyExtractor = std::function<std::string(const std::string& record)>;

    void define_alt_index(const std::string& index_name,
                          AltKeyExtractor extractor,
                          bool unique_key = false) {
        AltIndex idx;
        idx.name = index_name;
        idx.extractor = std::move(extractor);
        idx.unique_key = unique_key;
        alt_indexes_[index_name] = std::move(idx);
        rebuild_alt_index(index_name);
    }

    VsamRc get_by_alt(const std::string& index_name,
                      const std::string& alt_key,
                      std::string& key,
                      std::string& record) const {
        auto ait = alt_indexes_.find(index_name);
        if (ait == alt_indexes_.end()) return VsamRc::INVALID_REQUEST;
        auto kit = ait->second.index.find(alt_key);
        if (kit == ait->second.index.end()) return VsamRc::NOT_FOUND;
        key = kit->second;
        return get(key, record);
    }

    size_t size() const { return records_.size(); }
    bool empty() const { return records_.empty(); }

    const std::map<std::string, std::string>& data() const { return records_; }

    void flush() { save(); }

private:
    fs::path path_;
    ClusterDef def_;
    std::map<std::string, std::string> records_;
    BrowseState<std::string> browse_;

    struct AltIndex {
        std::string name;
        AltKeyExtractor extractor;
        bool unique_key = false;
        std::map<std::string, std::string> index; // alt_key -> primary_key
    };
    std::map<std::string, AltIndex> alt_indexes_;

    void update_alt_indexes(const std::string& pkey,
                            const std::string& record) {
        for (auto& [name, idx] : alt_indexes_) {
            std::string akey = idx.extractor(record);
            if (!akey.empty())
                idx.index[akey] = pkey;
        }
    }

    void remove_alt_index_entries(const std::string& pkey,
                                  const std::string& record) {
        for (auto& [name, idx] : alt_indexes_) {
            std::string akey = idx.extractor(record);
            auto it = idx.index.find(akey);
            if (it != idx.index.end() && it->second == pkey)
                idx.index.erase(it);
        }
    }

    void rebuild_alt_index(const std::string& index_name) {
        auto& idx = alt_indexes_[index_name];
        idx.index.clear();
        for (auto& [pkey, rec] : records_) {
            std::string akey = idx.extractor(rec);
            if (!akey.empty())
                idx.index[akey] = pkey;
        }
    }

    void save() {
        if (path_.empty()) return;
        fs::create_directories(path_.parent_path());
        std::ofstream out(path_, std::ios::binary);
        if (!out) return;
        for (auto& [k, v] : records_) {
            uint32_t klen = static_cast<uint32_t>(k.size());
            uint32_t vlen = static_cast<uint32_t>(v.size());
            out.write(reinterpret_cast<const char*>(&klen), 4);
            out.write(k.data(), static_cast<std::streamsize>(klen));
            out.write(reinterpret_cast<const char*>(&vlen), 4);
            out.write(v.data(), static_cast<std::streamsize>(vlen));
        }
    }

    void load() {
        if (!fs::exists(path_)) return;
        std::ifstream in(path_, std::ios::binary);
        if (!in) return;
        while (in.peek() != EOF) {
            uint32_t klen = 0, vlen = 0;
            in.read(reinterpret_cast<char*>(&klen), 4);
            if (in.gcount() < 4) break;
            std::string key(klen, '\0');
            in.read(key.data(), static_cast<std::streamsize>(klen));
            in.read(reinterpret_cast<char*>(&vlen), 4);
            if (in.gcount() < 4) break;
            std::string val(vlen, '\0');
            in.read(val.data(), static_cast<std::streamsize>(vlen));
            records_[key] = val;
        }
    }
};

// ── ESDS: Entry-Sequenced Data Set ─────────────────────────────────────
class EsdsFile {
public:
    EsdsFile(const fs::path& path, const ClusterDef& def)
        : path_(path), def_(def), next_rba_(0) {
        load();
    }

    ~EsdsFile() = default;

    EsdsFile(const EsdsFile&) = delete;
    EsdsFile& operator=(const EsdsFile&) = delete;

    // PUT: append-only, returns RBA
    std::pair<VsamRc, uint64_t> put(const std::string& record) {
        uint64_t rba = next_rba_;
        entries_.push_back({rba, record});
        rba_index_[rba] = entries_.size() - 1;
        next_rba_ = rba + 4 + record.size(); // 4-byte length prefix + data
        save_append(record);
        return {VsamRc::OK, rba};
    }

    // GET by RBA
    VsamRc get(uint64_t rba, std::string& record) const {
        auto it = rba_index_.find(rba);
        if (it == rba_index_.end()) return VsamRc::NOT_FOUND;
        record = entries_[it->second].data;
        return VsamRc::OK;
    }

    // Browse: sequential read
    VsamRc startbr() {
        browse_pos_ = 0;
        return VsamRc::OK;
    }

    VsamRc readnext(uint64_t& rba, std::string& record) {
        if (browse_pos_ >= entries_.size()) return VsamRc::END_OF_FILE;
        rba = entries_[browse_pos_].rba;
        record = entries_[browse_pos_].data;
        ++browse_pos_;
        return VsamRc::OK;
    }

    VsamRc endbr() {
        browse_pos_ = 0;
        return VsamRc::OK;
    }

    // No DELETE allowed on ESDS — entry-sequenced is append-only
    size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }

private:
    struct Entry {
        uint64_t rba;
        std::string data;
    };

    fs::path path_;
    ClusterDef def_;
    std::vector<Entry> entries_;
    std::map<uint64_t, size_t> rba_index_;
    uint64_t next_rba_;
    size_t browse_pos_ = 0;

    void save_append(const std::string& record) {
        fs::create_directories(path_.parent_path());
        std::ofstream out(path_, std::ios::binary | std::ios::app);
        uint32_t len = static_cast<uint32_t>(record.size());
        out.write(reinterpret_cast<const char*>(&len), 4);
        out.write(record.data(), static_cast<std::streamsize>(len));
    }

    void load() {
        if (!fs::exists(path_)) return;
        std::ifstream in(path_, std::ios::binary);
        if (!in) return;
        uint64_t rba = 0;
        while (in.peek() != EOF) {
            uint32_t len = 0;
            in.read(reinterpret_cast<char*>(&len), 4);
            if (in.gcount() < 4) break;
            std::string data(len, '\0');
            in.read(data.data(), static_cast<std::streamsize>(len));
            if (in.gcount() < static_cast<std::streamsize>(len)) break;
            entries_.push_back({rba, data});
            rba_index_[rba] = entries_.size() - 1;
            rba = rba + 4 + len;
        }
        next_rba_ = rba;
    }
};

// ── RRDS: Relative Record Data Set ─────────────────────────────────────
class RrdsFile {
public:
    RrdsFile(const fs::path& path, const ClusterDef& def)
        : path_(path), def_(def) {
        if (def_.record_size == 0)
            throw std::invalid_argument("RRDS requires fixed record_size");
        load();
    }

    ~RrdsFile() { save(); }

    RrdsFile(const RrdsFile&) = delete;
    RrdsFile& operator=(const RrdsFile&) = delete;

    // PUT at slot number (1-based)
    VsamRc put(uint32_t slot, const std::string& record) {
        if (slot == 0) return VsamRc::INVALID_REQUEST;
        if (record.size() > def_.record_size)
            return VsamRc::INVALID_REQUEST;

        ensure_slot(slot);
        if (occupied_[slot - 1])
            return VsamRc::DUPLICATE_KEY;
        std::string padded = record;
        padded.resize(def_.record_size, '\0');
        slots_[slot - 1] = padded;
        occupied_[slot - 1] = true;
        return VsamRc::OK;
    }

    // GET from slot
    VsamRc get(uint32_t slot, std::string& record) const {
        if (slot == 0 || slot > slots_.size())
            return VsamRc::NOT_FOUND;
        if (!occupied_[slot - 1])
            return VsamRc::NOT_FOUND;
        record = slots_[slot - 1];
        return VsamRc::OK;
    }

    // UPDATE at slot
    VsamRc update(uint32_t slot, const std::string& record) {
        if (slot == 0 || slot > slots_.size())
            return VsamRc::NOT_FOUND;
        if (!occupied_[slot - 1])
            return VsamRc::NOT_FOUND;
        std::string padded = record;
        padded.resize(def_.record_size, '\0');
        slots_[slot - 1] = padded;
        return VsamRc::OK;
    }

    // DELETE slot
    VsamRc erase(uint32_t slot) {
        if (slot == 0 || slot > slots_.size())
            return VsamRc::NOT_FOUND;
        if (!occupied_[slot - 1])
            return VsamRc::NOT_FOUND;
        occupied_[slot - 1] = false;
        slots_[slot - 1].clear();
        return VsamRc::OK;
    }

    // Browse sequential
    VsamRc startbr() {
        browse_pos_ = 0;
        return VsamRc::OK;
    }

    VsamRc readnext(uint32_t& slot, std::string& record) {
        while (browse_pos_ < slots_.size()) {
            if (occupied_[browse_pos_]) {
                slot = static_cast<uint32_t>(browse_pos_ + 1);
                record = slots_[browse_pos_];
                ++browse_pos_;
                return VsamRc::OK;
            }
            ++browse_pos_;
        }
        return VsamRc::END_OF_FILE;
    }

    VsamRc endbr() {
        browse_pos_ = 0;
        return VsamRc::OK;
    }

    size_t size() const {
        size_t n = 0;
        for (auto o : occupied_) if (o) ++n;
        return n;
    }

    size_t capacity() const { return slots_.size(); }
    bool empty() const { return size() == 0; }

    void flush() { save(); }

private:
    fs::path path_;
    ClusterDef def_;
    std::vector<std::string> slots_;
    std::vector<bool> occupied_;
    size_t browse_pos_ = 0;

    void ensure_slot(uint32_t slot) {
        if (slot > slots_.size()) {
            slots_.resize(slot);
            occupied_.resize(slot, false);
        }
    }

    void save() {
        if (path_.empty()) return;
        fs::create_directories(path_.parent_path());
        std::ofstream out(path_, std::ios::binary);
        if (!out) return;
        uint32_t count = static_cast<uint32_t>(slots_.size());
        uint32_t recsz = def_.record_size;
        out.write(reinterpret_cast<const char*>(&count), 4);
        out.write(reinterpret_cast<const char*>(&recsz), 4);
        for (size_t i = 0; i < slots_.size(); ++i) {
            uint8_t occ = occupied_[i] ? 1 : 0;
            out.write(reinterpret_cast<const char*>(&occ), 1);
            if (occupied_[i]) {
                std::string padded = slots_[i];
                padded.resize(recsz, '\0');
                out.write(padded.data(), static_cast<std::streamsize>(recsz));
            }
        }
    }

    void load() {
        if (!fs::exists(path_)) return;
        std::ifstream in(path_, std::ios::binary);
        if (!in) return;
        uint32_t count = 0, recsz = 0;
        in.read(reinterpret_cast<char*>(&count), 4);
        in.read(reinterpret_cast<char*>(&recsz), 4);
        slots_.resize(count);
        occupied_.resize(count, false);
        for (uint32_t i = 0; i < count; ++i) {
            uint8_t occ = 0;
            in.read(reinterpret_cast<char*>(&occ), 1);
            occupied_[i] = (occ != 0);
            if (occupied_[i]) {
                slots_[i].resize(recsz, '\0');
                in.read(slots_[i].data(), static_cast<std::streamsize>(recsz));
            }
        }
    }
};

// ── REPRO: Copy utility ────────────────────────────────────────────────
// Template-based copy between VSAM file types with optional filter

// KSDS -> KSDS copy
inline size_t repro_ksds_to_ksds(
    KsdsFile& source, KsdsFile& target,
    std::function<bool(const std::string&, const std::string&)> filter = nullptr)
{
    size_t count = 0;
    for (auto& [key, rec] : source.data()) {
        if (filter && !filter(key, rec)) continue;
        auto rc = target.put(key, rec);
        if (rc == VsamRc::OK) ++count;
    }
    return count;
}

// KSDS -> sequential (writes key|record lines)
inline size_t repro_ksds_to_stream(
    KsdsFile& source, std::ostream& out,
    std::function<bool(const std::string&, const std::string&)> filter = nullptr)
{
    size_t count = 0;
    for (auto& [key, rec] : source.data()) {
        if (filter && !filter(key, rec)) continue;
        out << key << "|" << rec << "\n";
        ++count;
    }
    return count;
}

}} // namespace lazarus::file

#endif // LAZARUS_FILE_VSAM_H
