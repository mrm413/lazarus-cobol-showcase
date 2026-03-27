#ifndef LAZARUS_DB2_RECORD_READER_H
#define LAZARUS_DB2_RECORD_READER_H

#include "ebcdic.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

// Platform-specific memory mapping
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace lazarus { namespace db2 {

// ==================================================================
//  Memory-mapped file
// ==================================================================

class MappedFile {
public:
    MappedFile() = default;

    explicit MappedFile(const std::string& path) {
        open(path);
    }

    ~MappedFile() {
        close();
    }

    MappedFile(const MappedFile&) = delete;
    MappedFile& operator=(const MappedFile&) = delete;

    MappedFile(MappedFile&& other) noexcept
        : data_(other.data_), size_(other.size_)
#ifdef _WIN32
        , file_handle_(other.file_handle_), mapping_handle_(other.mapping_handle_)
#else
        , fd_(other.fd_)
#endif
    {
        other.data_ = nullptr;
        other.size_ = 0;
#ifdef _WIN32
        other.file_handle_ = INVALID_HANDLE_VALUE;
        other.mapping_handle_ = nullptr;
#else
        other.fd_ = -1;
#endif
    }

    MappedFile& operator=(MappedFile&& other) noexcept {
        if (this != &other) {
            close();
            data_ = other.data_;
            size_ = other.size_;
#ifdef _WIN32
            file_handle_ = other.file_handle_;
            mapping_handle_ = other.mapping_handle_;
            other.file_handle_ = INVALID_HANDLE_VALUE;
            other.mapping_handle_ = nullptr;
#else
            fd_ = other.fd_;
            other.fd_ = -1;
#endif
            other.data_ = nullptr;
            other.size_ = 0;
        }
        return *this;
    }

    void open(const std::string& path) {
        close();
#ifdef _WIN32
        file_handle_ = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                   nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file_handle_ == INVALID_HANDLE_VALUE) {
            throw std::runtime_error("Cannot open file: " + path);
        }
        LARGE_INTEGER fsize;
        if (!GetFileSizeEx(file_handle_, &fsize)) {
            CloseHandle(file_handle_);
            file_handle_ = INVALID_HANDLE_VALUE;
            throw std::runtime_error("Cannot get file size: " + path);
        }
        size_ = static_cast<std::size_t>(fsize.QuadPart);
        if (size_ == 0) {
            CloseHandle(file_handle_);
            file_handle_ = INVALID_HANDLE_VALUE;
            return;
        }
        mapping_handle_ = CreateFileMappingA(file_handle_, nullptr, PAGE_READONLY, 0, 0, nullptr);
        if (!mapping_handle_) {
            CloseHandle(file_handle_);
            file_handle_ = INVALID_HANDLE_VALUE;
            throw std::runtime_error("Cannot create file mapping: " + path);
        }
        data_ = static_cast<const uint8_t*>(MapViewOfFile(mapping_handle_, FILE_MAP_READ, 0, 0, 0));
        if (!data_) {
            CloseHandle(mapping_handle_);
            CloseHandle(file_handle_);
            mapping_handle_ = nullptr;
            file_handle_ = INVALID_HANDLE_VALUE;
            throw std::runtime_error("Cannot map file: " + path);
        }
#else
        fd_ = ::open(path.c_str(), O_RDONLY);
        if (fd_ < 0) throw std::runtime_error("Cannot open file: " + path);
        struct stat st;
        if (fstat(fd_, &st) != 0) {
            ::close(fd_); fd_ = -1;
            throw std::runtime_error("Cannot stat file: " + path);
        }
        size_ = static_cast<std::size_t>(st.st_size);
        if (size_ == 0) { ::close(fd_); fd_ = -1; return; }
        data_ = static_cast<const uint8_t*>(mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0));
        if (data_ == MAP_FAILED) {
            data_ = nullptr;
            ::close(fd_); fd_ = -1;
            throw std::runtime_error("Cannot mmap file: " + path);
        }
#endif
    }

    void close() {
#ifdef _WIN32
        if (data_) { UnmapViewOfFile(data_); data_ = nullptr; }
        if (mapping_handle_) { CloseHandle(mapping_handle_); mapping_handle_ = nullptr; }
        if (file_handle_ != INVALID_HANDLE_VALUE) { CloseHandle(file_handle_); file_handle_ = INVALID_HANDLE_VALUE; }
#else
        if (data_ && size_ > 0) { munmap(const_cast<uint8_t*>(data_), size_); }
        data_ = nullptr;
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
#endif
        size_ = 0;
    }

    const uint8_t* data() const { return data_; }
    std::size_t size() const { return size_; }
    bool is_open() const { return data_ != nullptr || (size_ == 0
#ifdef _WIN32
        && file_handle_ != INVALID_HANDLE_VALUE
#else
        && fd_ >= 0
#endif
    ); }

private:
    const uint8_t* data_ = nullptr;
    std::size_t size_ = 0;
#ifdef _WIN32
    HANDLE file_handle_ = INVALID_HANDLE_VALUE;
    HANDLE mapping_handle_ = nullptr;
#else
    int fd_ = -1;
#endif
};

// ==================================================================
//  Record view
// ==================================================================

class RecordView {
public:
    RecordView() = default;
    RecordView(const uint8_t* data, std::size_t length) : data_(data), length_(length) {}

    const uint8_t* data() const { return data_; }
    std::size_t length() const { return length_; }
    bool empty() const { return length_ == 0; }

    // Extract field by position and length (0-based offset)
    std::string_view field(std::size_t offset, std::size_t len) const {
        if (offset >= length_) return {};
        std::size_t actual_len = std::min(len, length_ - offset);
        return {reinterpret_cast<const char*>(data_ + offset), actual_len};
    }

    // Extract field as EBCDIC-decoded ASCII string
    std::string field_ascii(std::size_t offset, std::size_t len) const {
        if (offset >= length_) return {};
        std::size_t actual_len = std::min(len, length_ - offset);
        return ebcdic::ebcdic_to_ascii(data_ + offset, actual_len);
    }

    // Extract packed decimal field
    double field_packed(std::size_t offset, std::size_t len, int scale = 0) const {
        if (offset + len > length_) return 0.0;
        return ebcdic::packed_decimal_decode(data_ + offset, len, scale);
    }

    int64_t field_packed_int(std::size_t offset, std::size_t len) const {
        if (offset + len > length_) return 0;
        return ebcdic::packed_decimal_decode_int(data_ + offset, len);
    }

    // Extract binary integer
    int16_t field_int16(std::size_t offset) const {
        if (offset + 2 > length_) return 0;
        return ebcdic::binary_to_int16(data_ + offset);
    }

    int32_t field_int32(std::size_t offset) const {
        if (offset + 4 > length_) return 0;
        return ebcdic::binary_to_int32(data_ + offset);
    }

    int64_t field_int64(std::size_t offset) const {
        if (offset + 8 > length_) return 0;
        return ebcdic::binary_to_int64(data_ + offset);
    }

    // Extract zoned decimal
    int64_t field_zoned(std::size_t offset, std::size_t len) const {
        if (offset + len > length_) return 0;
        return ebcdic::zoned_decimal_decode(data_ + offset, len);
    }

    // Raw byte access
    uint8_t operator[](std::size_t idx) const {
        if (idx >= length_) return 0;
        return data_[idx];
    }

private:
    const uint8_t* data_ = nullptr;
    std::size_t length_ = 0;
};

// ==================================================================
//  Record iterator (used by both Fixed and Variable readers)
// ==================================================================

class RecordIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = RecordView;
    using difference_type = std::ptrdiff_t;
    using pointer = const RecordView*;
    using reference = const RecordView&;

    RecordIterator() = default;

    RecordIterator(const uint8_t* data, std::size_t total_size,
                   std::size_t record_len, bool variable, std::size_t pos = 0)
        : data_(data), total_size_(total_size), record_len_(record_len),
          variable_(variable), pos_(pos) {
        advance_to_record();
    }

    reference operator*() const { return current_; }
    pointer operator->() const { return &current_; }

    RecordIterator& operator++() {
        if (variable_) {
            pos_ += current_.length() + 4; // skip RDW + data
        } else {
            pos_ += record_len_;
        }
        advance_to_record();
        return *this;
    }

    RecordIterator operator++(int) {
        auto tmp = *this;
        ++(*this);
        return tmp;
    }

    bool operator==(const RecordIterator& o) const { return pos_ == o.pos_ && data_ == o.data_; }
    bool operator!=(const RecordIterator& o) const { return !(*this == o); }

private:
    void advance_to_record() {
        if (!data_ || pos_ >= total_size_) {
            pos_ = total_size_;
            current_ = {};
            return;
        }
        if (variable_) {
            if (pos_ + 4 > total_size_) {
                pos_ = total_size_;
                current_ = {};
                return;
            }
            // RDW: 2-byte big-endian length (includes 4-byte RDW), 2 reserved bytes
            uint16_t rdw_len = static_cast<uint16_t>(
                (static_cast<uint16_t>(data_[pos_]) << 8) | data_[pos_ + 1]);
            if (rdw_len < 4) {
                pos_ = total_size_;
                current_ = {};
                return;
            }
            std::size_t data_len = rdw_len - 4;
            if (pos_ + rdw_len > total_size_) {
                pos_ = total_size_;
                current_ = {};
                return;
            }
            current_ = RecordView(data_ + pos_ + 4, data_len);
        } else {
            if (pos_ + record_len_ > total_size_) {
                pos_ = total_size_;
                current_ = {};
                return;
            }
            current_ = RecordView(data_ + pos_, record_len_);
        }
    }

    const uint8_t* data_ = nullptr;
    std::size_t total_size_ = 0;
    std::size_t record_len_ = 0;
    bool variable_ = false;
    std::size_t pos_ = 0;
    RecordView current_;
};

// ==================================================================
//  Fixed-length record reader
// ==================================================================

class FixedRecordReader {
public:
    FixedRecordReader(const uint8_t* data, std::size_t size, std::size_t record_len)
        : data_(data), size_(size), record_len_(record_len) {
        if (record_len_ == 0) throw std::invalid_argument("Record length must be > 0");
    }

    explicit FixedRecordReader(const std::vector<uint8_t>& data, std::size_t record_len)
        : FixedRecordReader(data.data(), data.size(), record_len) {}

    std::size_t record_count() const {
        return record_len_ > 0 ? size_ / record_len_ : 0;
    }

    RecordView record(std::size_t index) const {
        std::size_t offset = index * record_len_;
        if (offset + record_len_ > size_) return {};
        return RecordView(data_ + offset, record_len_);
    }

    RecordIterator begin() const { return RecordIterator(data_, size_, record_len_, false, 0); }
    RecordIterator end() const { return RecordIterator(data_, size_, record_len_, false, size_); }

private:
    const uint8_t* data_;
    std::size_t size_;
    std::size_t record_len_;
};

// ==================================================================
//  Variable-length record reader (RDW format)
// ==================================================================

class VariableRecordReader {
public:
    VariableRecordReader(const uint8_t* data, std::size_t size)
        : data_(data), size_(size) {}

    explicit VariableRecordReader(const std::vector<uint8_t>& data)
        : VariableRecordReader(data.data(), data.size()) {}

    // Count records by scanning RDWs
    std::size_t record_count() const {
        std::size_t count = 0;
        std::size_t pos = 0;
        while (pos + 4 <= size_) {
            uint16_t rdw_len = static_cast<uint16_t>(
                (static_cast<uint16_t>(data_[pos]) << 8) | data_[pos + 1]);
            if (rdw_len < 4 || pos + rdw_len > size_) break;
            ++count;
            pos += rdw_len;
        }
        return count;
    }

    RecordView record(std::size_t index) const {
        std::size_t pos = 0;
        std::size_t current = 0;
        while (pos + 4 <= size_) {
            uint16_t rdw_len = static_cast<uint16_t>(
                (static_cast<uint16_t>(data_[pos]) << 8) | data_[pos + 1]);
            if (rdw_len < 4 || pos + rdw_len > size_) break;
            if (current == index) {
                return RecordView(data_ + pos + 4, rdw_len - 4);
            }
            pos += rdw_len;
            ++current;
        }
        return {};
    }

    RecordIterator begin() const { return RecordIterator(data_, size_, 0, true, 0); }
    RecordIterator end() const { return RecordIterator(data_, size_, 0, true, size_); }

private:
    const uint8_t* data_;
    std::size_t size_;
};

// ==================================================================
//  Block Descriptor Word (BDW) support
// ==================================================================

class BlockedRecordReader {
public:
    // BDW: 4-byte header per block. First 2 bytes = block length (big-endian, includes BDW).
    // Within each block, records have RDW format.
    BlockedRecordReader(const uint8_t* data, std::size_t size)
        : data_(data), size_(size) {
        scan_blocks();
    }

    explicit BlockedRecordReader(const std::vector<uint8_t>& data)
        : BlockedRecordReader(data.data(), data.size()) {}

    std::size_t block_count() const { return blocks_.size(); }

    std::size_t record_count() const {
        std::size_t total = 0;
        for (const auto& blk : blocks_) {
            VariableRecordReader vrr(blk.data, blk.size);
            total += vrr.record_count();
        }
        return total;
    }

    // Get all records flattened across blocks
    std::vector<RecordView> all_records() const {
        std::vector<RecordView> records;
        for (const auto& blk : blocks_) {
            VariableRecordReader vrr(blk.data, blk.size);
            for (auto it = vrr.begin(); it != vrr.end(); ++it) {
                records.push_back(*it);
            }
        }
        return records;
    }

private:
    struct Block {
        const uint8_t* data;
        std::size_t size;
    };

    void scan_blocks() {
        std::size_t pos = 0;
        while (pos + 4 <= size_) {
            uint16_t bdw_len = static_cast<uint16_t>(
                (static_cast<uint16_t>(data_[pos]) << 8) | data_[pos + 1]);
            if (bdw_len < 4 || pos + bdw_len > size_) break;
            // Block data starts after BDW (4 bytes)
            blocks_.push_back({data_ + pos + 4, static_cast<std::size_t>(bdw_len - 4)});
            pos += bdw_len;
        }
    }

    const uint8_t* data_;
    std::size_t size_;
    std::vector<Block> blocks_;
};

// ==================================================================
//  File-based readers (convenience wrappers)
// ==================================================================

inline std::vector<uint8_t> read_binary_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("Cannot open file: " + path);
    auto size = f.tellg();
    f.seekg(0);
    std::vector<uint8_t> data(static_cast<std::size_t>(size));
    f.read(reinterpret_cast<char*>(data.data()), size);
    return data;
}

}} // namespace lazarus::db2

#endif // LAZARUS_DB2_RECORD_READER_H
