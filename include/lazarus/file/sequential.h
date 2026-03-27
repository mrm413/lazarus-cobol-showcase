#ifndef LAZARUS_FILE_SEQUENTIAL_H
#define LAZARUS_FILE_SEQUENTIAL_H
// ============================================================================
// Lazarus C++17 Platform — Sequential File I/O
// Torsova LLC — lazarus-systems.com
// ============================================================================

#include <string>
#include <vector>
#include <stdexcept>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <optional>

namespace lazarus { namespace file {

namespace fs = std::filesystem;

// ── Record format types ────────────────────────────────────────────────
enum class RecFm { F, FB, V, VB };

inline const char* recfm_name(RecFm r) {
    switch (r) {
        case RecFm::F:  return "F";
        case RecFm::FB: return "FB";
        case RecFm::V:  return "V";
        case RecFm::VB: return "VB";
    }
    return "?";
}

// ── DCB (Data Control Block) parameters ────────────────────────────────
struct SeqDcb {
    RecFm recfm = RecFm::FB;
    uint32_t lrecl = 80;     // logical record length
    uint32_t blksize = 0;    // block size (0 = auto)

    uint32_t effective_blksize() const {
        if (blksize > 0) return blksize;
        switch (recfm) {
            case RecFm::F:  return lrecl;
            case RecFm::FB: return lrecl * 10; // default 10 records/block
            case RecFm::V:  return lrecl + 4;
            case RecFm::VB: return (lrecl + 4) * 10;
        }
        return lrecl;
    }

    uint32_t records_per_block() const {
        uint32_t bk = effective_blksize();
        switch (recfm) {
            case RecFm::F:  return 1;
            case RecFm::FB: return bk / lrecl;
            case RecFm::V:  return 1;
            case RecFm::VB: return bk / (lrecl + 4); // approximate
        }
        return 1;
    }
};

// ── RDW (Record Descriptor Word) for V/VB ──────────────────────────────
struct Rdw {
    uint16_t length;   // includes 4-byte RDW
    uint16_t reserved; // always 0
};

inline void write_rdw(std::ostream& out, uint16_t data_len) {
    uint16_t total = static_cast<uint16_t>(data_len + 4);
    uint16_t zero = 0;
    out.write(reinterpret_cast<const char*>(&total), 2);
    out.write(reinterpret_cast<const char*>(&zero), 2);
}

inline uint16_t read_rdw(std::istream& in) {
    uint16_t total = 0;
    uint16_t reserved = 0;
    in.read(reinterpret_cast<char*>(&total), 2);
    in.read(reinterpret_cast<char*>(&reserved), 2);
    if (total < 4)
        throw std::runtime_error("RDW length < 4");
    return static_cast<uint16_t>(total - 4);
}

// ── Sequential File — RAII wrapper ─────────────────────────────────────
class SequentialFile {
public:
    SequentialFile(const fs::path& path, SeqDcb dcb)
        : path_(path), dcb_(dcb) {}

    ~SequentialFile() = default;

    // Non-copyable
    SequentialFile(const SequentialFile&) = delete;
    SequentialFile& operator=(const SequentialFile&) = delete;

    const fs::path& path() const { return path_; }
    const SeqDcb& dcb() const { return dcb_; }

    // ── Write records ──────────────────────────────────────────────────
    void write_records(const std::vector<std::string>& records) {
        fs::create_directories(path_.parent_path());
        std::ofstream out(path_, std::ios::binary | std::ios::trunc);
        if (!out)
            throw std::runtime_error("Cannot open for write: " + path_.string());

        for (auto& rec : records) {
            write_one(out, rec);
        }
    }

    void append_record(const std::string& record) {
        fs::create_directories(path_.parent_path());
        std::ofstream out(path_, std::ios::binary | std::ios::app);
        if (!out)
            throw std::runtime_error("Cannot open for append: " + path_.string());
        write_one(out, record);
    }

    // ── Read all records ───────────────────────────────────────────────
    std::vector<std::string> read_records() const {
        std::ifstream in(path_, std::ios::binary);
        if (!in)
            throw std::runtime_error("Cannot open for read: " + path_.string());

        std::vector<std::string> result;
        while (in.peek() != EOF) {
            auto rec = read_one(in);
            if (rec.has_value())
                result.push_back(std::move(*rec));
        }
        return result;
    }

    // ── Record count ───────────────────────────────────────────────────
    size_t record_count() const {
        return read_records().size();
    }

    // ── Iterator support for range-based for ───────────────────────────
    class RecordIterator {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::string;
        using difference_type = std::ptrdiff_t;
        using pointer = const std::string*;
        using reference = const std::string&;

        RecordIterator() : owner_(nullptr), idx_(0) {}
        RecordIterator(const SequentialFile* owner, size_t idx,
                       std::vector<std::string> recs)
            : owner_(owner), idx_(idx), records_(std::move(recs)) {}

        reference operator*() const { return records_[idx_]; }
        pointer operator->() const { return &records_[idx_]; }

        RecordIterator& operator++() { ++idx_; return *this; }
        RecordIterator operator++(int) {
            auto tmp = *this; ++idx_; return tmp;
        }

        bool operator==(const RecordIterator& o) const {
            return idx_ >= records_.size() && o.idx_ >= o.records_.size();
        }
        bool operator!=(const RecordIterator& o) const {
            return !(*this == o);
        }

    private:
        const SequentialFile* owner_;
        size_t idx_;
        std::vector<std::string> records_;
    };

    RecordIterator begin() const {
        auto recs = read_records();
        return RecordIterator(this, 0, std::move(recs));
    }

    RecordIterator end() const {
        return RecordIterator(this, 0, {});
    }

    // ── Block-level access ─────────────────────────────────────────────
    std::vector<std::vector<std::string>> read_blocks() const {
        auto all = read_records();
        std::vector<std::vector<std::string>> blocks;
        uint32_t rpb = dcb_.records_per_block();
        if (rpb == 0) rpb = 1;

        for (size_t i = 0; i < all.size(); i += rpb) {
            std::vector<std::string> block;
            for (size_t j = i; j < all.size() && j < i + rpb; ++j)
                block.push_back(all[j]);
            blocks.push_back(std::move(block));
        }
        return blocks;
    }

private:
    fs::path path_;
    SeqDcb dcb_;

    void write_one(std::ostream& out, const std::string& rec) const {
        switch (dcb_.recfm) {
            case RecFm::F:
            case RecFm::FB: {
                // Pad or truncate to LRECL
                std::string padded = rec;
                if (padded.size() < dcb_.lrecl)
                    padded.resize(dcb_.lrecl, ' ');
                else if (padded.size() > dcb_.lrecl)
                    padded.resize(dcb_.lrecl);
                out.write(padded.data(), static_cast<std::streamsize>(dcb_.lrecl));
                break;
            }
            case RecFm::V:
            case RecFm::VB: {
                if (rec.size() > dcb_.lrecl)
                    throw std::runtime_error("Record exceeds LRECL=" +
                        std::to_string(dcb_.lrecl));
                write_rdw(out, static_cast<uint16_t>(rec.size()));
                out.write(rec.data(), static_cast<std::streamsize>(rec.size()));
                break;
            }
        }
    }

    std::optional<std::string> read_one(std::istream& in) const {
        switch (dcb_.recfm) {
            case RecFm::F:
            case RecFm::FB: {
                std::string buf(dcb_.lrecl, '\0');
                in.read(buf.data(), static_cast<std::streamsize>(dcb_.lrecl));
                if (in.gcount() < static_cast<std::streamsize>(dcb_.lrecl))
                    return std::nullopt;
                return buf;
            }
            case RecFm::V:
            case RecFm::VB: {
                if (in.peek() == EOF) return std::nullopt;
                uint16_t dlen = read_rdw(in);
                std::string buf(dlen, '\0');
                in.read(buf.data(), dlen);
                if (in.gcount() < dlen)
                    return std::nullopt;
                return buf;
            }
        }
        return std::nullopt;
    }
};

}} // namespace lazarus::file

#endif // LAZARUS_FILE_SEQUENTIAL_H
