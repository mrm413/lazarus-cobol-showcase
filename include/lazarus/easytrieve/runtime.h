#ifndef LAZARUS_EASYTRIEVE_RUNTIME_H
#define LAZARUS_EASYTRIEVE_RUNTIME_H

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <functional>
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <cmath>
#include <iomanip>
#include <ctime>
#include <memory>
#include <tuple>
#include <numeric>
#include <cassert>

namespace lazarus { namespace easytrieve { namespace runtime {

// ============================================================
// Field type conversion utilities
// ============================================================
namespace convert {

inline int32_t alpha_to_int(const std::string& s) {
    if (s.empty()) return 0;
    try {
        // Strip leading/trailing spaces
        size_t start = s.find_first_not_of(' ');
        if (start == std::string::npos) return 0;
        size_t end = s.find_last_not_of(' ');
        return std::stoi(s.substr(start, end - start + 1));
    } catch (...) {
        return 0;
    }
}

inline int64_t alpha_to_int64(const std::string& s) {
    if (s.empty()) return 0;
    try {
        size_t start = s.find_first_not_of(' ');
        if (start == std::string::npos) return 0;
        size_t end = s.find_last_not_of(' ');
        return std::stoll(s.substr(start, end - start + 1));
    } catch (...) {
        return 0;
    }
}

inline double alpha_to_double(const std::string& s) {
    if (s.empty()) return 0.0;
    try {
        size_t start = s.find_first_not_of(' ');
        if (start == std::string::npos) return 0.0;
        size_t end = s.find_last_not_of(' ');
        return std::stod(s.substr(start, end - start + 1));
    } catch (...) {
        return 0.0;
    }
}

inline std::string int_to_alpha(int32_t val, int width) {
    std::string s = std::to_string(val);
    while (static_cast<int>(s.size()) < width) s = "0" + s;
    if (static_cast<int>(s.size()) > width) s = s.substr(s.size() - static_cast<size_t>(width));
    return s;
}

inline std::string int64_to_alpha(int64_t val, int width) {
    std::string s = std::to_string(val);
    while (static_cast<int>(s.size()) < width) s = "0" + s;
    if (static_cast<int>(s.size()) > width) s = s.substr(s.size() - static_cast<size_t>(width));
    return s;
}

inline std::string double_to_alpha(double val, int width, int decimals) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(decimals) << val;
    std::string s = oss.str();
    while (static_cast<int>(s.size()) < width) s = " " + s;
    return s;
}

// Packed decimal (BCD) encoding
inline int64_t packed_to_int64(const char* buf, int bytes) {
    int64_t result = 0;
    for (int i = 0; i < bytes - 1; i++) {
        uint8_t b = static_cast<uint8_t>(buf[i]);
        result = result * 10 + ((b >> 4) & 0x0F);
        result = result * 10 + (b & 0x0F);
    }
    // Last byte: high nibble is digit, low nibble is sign
    uint8_t last = static_cast<uint8_t>(buf[bytes - 1]);
    result = result * 10 + ((last >> 4) & 0x0F);
    uint8_t sign = last & 0x0F;
    if (sign == 0x0D || sign == 0x0B) result = -result; // negative
    return result;
}

inline void int64_to_packed(int64_t val, char* buf, int bytes) {
    bool negative = (val < 0);
    if (negative) val = -val;

    // Clear buffer
    std::memset(buf, 0, static_cast<size_t>(bytes));

    // Fill digits from right to left
    // Last byte: high nibble = last digit, low nibble = sign
    buf[bytes - 1] = static_cast<char>(
        (static_cast<uint8_t>(val % 10) << 4) | (negative ? 0x0D : 0x0C));
    val /= 10;

    for (int i = bytes - 2; i >= 0 && val > 0; i--) {
        uint8_t lo = static_cast<uint8_t>(val % 10);
        val /= 10;
        uint8_t hi = static_cast<uint8_t>(val % 10);
        val /= 10;
        buf[i] = static_cast<char>((hi << 4) | lo);
    }
}

// Binary (big-endian) encoding
inline int32_t binary_to_int32(const char* buf, int bytes) {
    int32_t result = 0;
    for (int i = 0; i < bytes; i++) {
        result = (result << 8) | static_cast<uint8_t>(buf[i]);
    }
    // Sign extend
    if (bytes < 4 && (static_cast<uint8_t>(buf[0]) & 0x80)) {
        for (int i = bytes; i < 4; i++) {
            result |= (0xFF << (i * 8));
        }
    }
    return result;
}

inline void int32_to_binary(int32_t val, char* buf, int bytes) {
    for (int i = bytes - 1; i >= 0; i--) {
        buf[i] = static_cast<char>(val & 0xFF);
        val >>= 8;
    }
}

// Edit mask formatting
inline std::string apply_edit_mask(const std::string& value, const std::string& mask) {
    if (mask.empty()) return value;
    std::string result;
    result.reserve(mask.size());
    size_t vi = 0;
    for (size_t mi = 0; mi < mask.size(); mi++) {
        if (mask[mi] == 'Z' || mask[mi] == '9' || mask[mi] == '#') {
            if (vi < value.size()) {
                if (mask[mi] == 'Z' && value[vi] == '0') {
                    result += ' ';
                } else {
                    result += value[vi];
                }
                vi++;
            } else {
                result += ' ';
            }
        } else {
            result += mask[mi];
        }
    }
    return result;
}

} // namespace convert

// ============================================================
// EzRecord: record buffer with field access by name/position
// ============================================================
class EzRecord {
public:
    explicit EzRecord(int lrecl = 80) : lrecl_(lrecl), buffer_(static_cast<size_t>(lrecl), ' ') {}

    int lrecl() const { return lrecl_; }

    // Define a field by name, position (1-based), length
    void define_field(const std::string& name, int pos, int len) {
        fields_[name] = {pos - 1, len}; // convert to 0-based
    }

    // Get field value as string
    std::string get_alpha(const std::string& name) const {
        auto it = fields_.find(name);
        if (it == fields_.end()) return "";
        auto [offset, len] = it->second;
        if (offset < 0 || offset + len > static_cast<int>(buffer_.size())) return "";
        return buffer_.substr(static_cast<size_t>(offset), static_cast<size_t>(len));
    }

    // Get field as integer
    int32_t get_numeric(const std::string& name) const {
        return convert::alpha_to_int(get_alpha(name));
    }

    // Get field as double
    double get_decimal(const std::string& name, int dec_places) const {
        int64_t raw = convert::alpha_to_int64(get_alpha(name));
        double divisor = std::pow(10.0, dec_places);
        return static_cast<double>(raw) / divisor;
    }

    // Set field from string
    void set_alpha(const std::string& name, const std::string& value) {
        auto it = fields_.find(name);
        if (it == fields_.end()) return;
        auto [offset, len] = it->second;
        std::string padded = value;
        while (static_cast<int>(padded.size()) < len) padded += ' ';
        if (static_cast<int>(padded.size()) > len) padded = padded.substr(0, static_cast<size_t>(len));
        buffer_.replace(static_cast<size_t>(offset), static_cast<size_t>(len), padded);
    }

    // Set field from integer
    void set_numeric(const std::string& name, int32_t value) {
        auto it = fields_.find(name);
        if (it == fields_.end()) return;
        auto [offset, len] = it->second;
        (void)offset;
        set_alpha(name, convert::int_to_alpha(value, len));
    }

    // Direct buffer access
    const std::string& buffer() const { return buffer_; }
    std::string& buffer() { return buffer_; }
    void set_buffer(const std::string& buf) {
        buffer_ = buf;
        if (static_cast<int>(buffer_.size()) < lrecl_) {
            buffer_.resize(static_cast<size_t>(lrecl_), ' ');
        }
    }

    // Field existence check
    bool has_field(const std::string& name) const {
        return fields_.find(name) != fields_.end();
    }

    // Get all field names
    std::vector<std::string> field_names() const {
        std::vector<std::string> names;
        for (const auto& [name, _] : fields_) {
            (void)_;
            names.push_back(name);
        }
        return names;
    }

private:
    int lrecl_;
    std::string buffer_;
    std::map<std::string, std::pair<int, int>> fields_; // name -> (offset, length)
};

// ============================================================
// EzFile: RAII file handler with fixed-length record support
// ============================================================
class EzFile {
public:
    EzFile() : lrecl_(80), eof_(false) {}
    explicit EzFile(int lrecl) : lrecl_(lrecl), eof_(false), record_(lrecl) {}

    ~EzFile() { close(); }

    // Non-copyable
    EzFile(const EzFile&) = delete;
    EzFile& operator=(const EzFile&) = delete;

    // Move OK
    EzFile(EzFile&& other) noexcept
        : stream_(std::move(other.stream_)), lrecl_(other.lrecl_),
          eof_(other.eof_), record_(std::move(other.record_)),
          path_(std::move(other.path_)) {}

    EzFile& operator=(EzFile&& other) noexcept {
        if (this != &other) {
            close();
            stream_ = std::move(other.stream_);
            lrecl_ = other.lrecl_;
            eof_ = other.eof_;
            record_ = std::move(other.record_);
            path_ = std::move(other.path_);
        }
        return *this;
    }

    bool open_read(const std::string& path) {
        path_ = path;
        stream_.open(path, std::ios::in | std::ios::binary);
        eof_ = false;
        return stream_.is_open();
    }

    bool open_write(const std::string& path) {
        path_ = path;
        stream_.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
        eof_ = false;
        return stream_.is_open();
    }

    bool open_update(const std::string& path) {
        path_ = path;
        stream_.open(path, std::ios::in | std::ios::out | std::ios::binary);
        eof_ = false;
        return stream_.is_open();
    }

    void close() {
        if (stream_.is_open()) stream_.close();
    }

    bool is_open() const { return stream_.is_open(); }
    bool is_eof() const { return eof_; }
    int lrecl() const { return lrecl_; }
    const std::string& path() const { return path_; }

    // GET: read next record, return false on EOF
    bool get() {
        if (!stream_.is_open() || eof_) return false;
        std::string buf(static_cast<size_t>(lrecl_), ' ');
        if (!stream_.read(&buf[0], lrecl_)) {
            eof_ = true;
            return false;
        }
        record_.set_buffer(buf);
        return true;
    }

    // PUT: write current record
    bool put() {
        if (!stream_.is_open()) return false;
        const auto& buf = record_.buffer();
        stream_.write(buf.data(), lrecl_);
        return stream_.good();
    }

    // POINT: seek to record by key (for sequential, seek by record number)
    bool point(int record_number) {
        if (!stream_.is_open()) return false;
        auto pos = static_cast<std::streamoff>(record_number) * lrecl_;
        stream_.seekg(pos);
        eof_ = false;
        return stream_.good();
    }

    // Record access
    EzRecord& record() { return record_; }
    const EzRecord& record() const { return record_; }

    // Define a field on the record layout
    void define_field(const std::string& name, int pos, int len) {
        record_.define_field(name, pos, len);
    }

private:
    std::fstream stream_;
    int lrecl_;
    bool eof_;
    EzRecord record_;
    std::string path_;
};

// ============================================================
// SyncProcessor: template-based multi-file synchronized processing
// Performs merge-join on a common key field
// ============================================================
template<typename... Files>
class SyncProcessor;

// Base case: single file
template<typename F>
class SyncProcessor<F> {
public:
    explicit SyncProcessor(F& file, const std::string& key_field)
        : file_(file), key_field_(key_field), active_(false) {}

    bool advance() {
        active_ = file_.get();
        return active_;
    }

    std::string current_key() const {
        if (!active_) return "\xFF"; // high-values for EOF
        return file_.record().get_alpha(key_field_);
    }

    bool is_active() const { return active_; }

    std::string min_key() const { return current_key(); }

    bool any_active() const { return active_; }

    void advance_matching(const std::string& key) {
        if (active_ && current_key() == key) {
            advance();
        }
    }

protected:
    F& file_;
    std::string key_field_;
    bool active_;
};

// Recursive case: multiple files
template<typename F, typename... Rest>
class SyncProcessor<F, Rest...> {
public:
    SyncProcessor(F& file, const std::string& key_field,
                  Rest&... rest_files)
        : head_(file, key_field), tail_(rest_files...) {}

    // Initialize: read first record from each file
    bool initialize() {
        bool h = head_.advance();
        bool t = tail_any_active_after_init();
        return h || t;
    }

    // Get the minimum key across all files
    std::string min_key() const {
        std::string hk = head_.current_key();
        std::string tk = tail_.min_key();
        return (hk <= tk) ? hk : tk;
    }

    // Are any files still active?
    bool any_active() const {
        return head_.is_active() || tail_.any_active();
    }

    // Process one cycle: find min key, call handler, advance matching files
    template<typename Handler>
    bool process_one(Handler& handler) {
        if (!any_active()) return false;
        std::string key = min_key();
        handler(key);
        head_.advance_matching(key);
        tail_.advance_matching(key);
        return any_active();
    }

    // Run full synchronized processing
    template<typename Handler>
    void run(Handler& handler) {
        head_.advance();
        // Initialize tail by advancing
        init_tail();
        while (any_active()) {
            process_one(handler);
        }
    }

private:
    SyncProcessor<F> head_;
    SyncProcessor<Rest...> tail_;

    bool tail_any_active_after_init() {
        // Need to advance tail files
        return tail_.any_active();
    }

    void init_tail() {
        // Initialize remaining files
        if constexpr (sizeof...(Rest) == 1) {
            tail_.advance();
        }
    }
};

// Convenience: two-file sync processor
using SyncPair = SyncProcessor<EzFile, EzFile>;

// ============================================================
// TableLookup: DEFINE TABLE implementation with SEARCH
// ============================================================
class TableLookup {
public:
    TableLookup() = default;

    // Add an entry: key -> vector of values
    void add_entry(const std::string& key, const std::vector<std::string>& values) {
        entries_[key] = values;
    }

    // Add a simple key-value entry
    void add_entry(const std::string& key, const std::string& value) {
        entries_[key] = {value};
    }

    // Search by key, return first value or default
    std::string search(const std::string& key, const std::string& default_val = "") const {
        auto it = entries_.find(key);
        if (it != entries_.end() && !it->second.empty()) {
            return it->second[0];
        }
        return default_val;
    }

    // Search by key, return value at column index
    std::string search(const std::string& key, size_t col_index,
                       const std::string& default_val = "") const {
        auto it = entries_.find(key);
        if (it != entries_.end() && col_index < it->second.size()) {
            return it->second[col_index];
        }
        return default_val;
    }

    // Check if key exists
    bool contains(const std::string& key) const {
        return entries_.find(key) != entries_.end();
    }

    // Number of entries
    size_t size() const { return entries_.size(); }

    // Clear all entries
    void clear() { entries_.clear(); }

    // Load from parallel vectors
    void load(const std::vector<std::string>& keys,
              const std::vector<std::string>& values) {
        size_t n = std::min(keys.size(), values.size());
        for (size_t i = 0; i < n; i++) {
            entries_[keys[i]] = {values[i]};
        }
    }

    // Get all keys
    std::vector<std::string> keys() const {
        std::vector<std::string> result;
        result.reserve(entries_.size());
        for (const auto& [k, v] : entries_) {
            (void)v;
            result.push_back(k);
        }
        return result;
    }

private:
    std::map<std::string, std::vector<std::string>> entries_;
};

// ============================================================
// ReportEngine: page-oriented formatted output
// ============================================================
class ReportEngine {
public:
    explicit ReportEngine(std::ostream& out, int lines_per_page = 60)
        : out_(out), lines_per_page_(lines_per_page),
          line_count_(0), page_count_(0) {}

    // ---- Page control ----
    int lines_per_page() const { return lines_per_page_; }
    int current_line() const { return line_count_; }
    int current_page() const { return page_count_; }

    void set_lines_per_page(int n) { lines_per_page_ = n; }

    // ---- Title/header registration ----
    void add_title(const std::string& title) {
        titles_.push_back(title);
    }

    void add_heading(const std::string& heading) {
        headings_.push_back(heading);
    }

    void set_page_header_callback(std::function<void(std::ostream&, int)> cb) {
        header_callback_ = std::move(cb);
    }

    // ---- Control break support ----
    struct BreakLevel {
        std::string field_name;
        std::string prev_value;
        std::map<std::string, double> accumulators; // field_name -> sum
        bool initialized;

        BreakLevel() : initialized(false) {}
    };

    void add_control_break(const std::string& field_name,
                           const std::vector<std::string>& sum_fields) {
        BreakLevel bl;
        bl.field_name = field_name;
        for (const auto& sf : sum_fields) {
            bl.accumulators[sf] = 0.0;
        }
        break_levels_.push_back(std::move(bl));
    }

    // Check and process control breaks, return vector of break level indices that fired
    std::vector<int> check_breaks(const std::map<std::string, std::string>& current_values) {
        std::vector<int> fired;
        for (int i = 0; i < static_cast<int>(break_levels_.size()); i++) {
            auto& bl = break_levels_[static_cast<size_t>(i)];
            auto it = current_values.find(bl.field_name);
            if (it == current_values.end()) continue;

            if (!bl.initialized) {
                bl.prev_value = it->second;
                bl.initialized = true;
                continue;
            }

            if (it->second != bl.prev_value) {
                fired.push_back(i);
                bl.prev_value = it->second;
            }
        }
        return fired;
    }

    // Accumulate sum fields
    void accumulate(const std::map<std::string, double>& values) {
        for (auto& bl : break_levels_) {
            for (auto& [field, acc] : bl.accumulators) {
                auto it = values.find(field);
                if (it != values.end()) {
                    acc += it->second;
                }
            }
        }
        // Grand totals
        for (const auto& [field, val] : values) {
            grand_totals_[field] += val;
        }
    }

    // Get subtotal for a break level and field, then reset
    double get_subtotal(int level, const std::string& field) {
        if (level < 0 || level >= static_cast<int>(break_levels_.size())) return 0.0;
        auto& bl = break_levels_[static_cast<size_t>(level)];
        auto it = bl.accumulators.find(field);
        if (it == bl.accumulators.end()) return 0.0;
        double val = it->second;
        it->second = 0.0;
        return val;
    }

    // Get grand total for a field
    double get_grand_total(const std::string& field) const {
        auto it = grand_totals_.find(field);
        return (it != grand_totals_.end()) ? it->second : 0.0;
    }

    // ---- Output methods ----
    void check_page_break() {
        if (line_count_ >= lines_per_page_ || page_count_ == 0) {
            new_page();
        }
    }

    void new_page() {
        page_count_++;
        line_count_ = 0;

        // Print page header
        if (header_callback_) {
            header_callback_(out_, page_count_);
        }

        for (const auto& t : titles_) {
            print_line(t);
        }

        // Date/time stamp
        print_line(get_date_stamp() + std::string(20, ' ') + "PAGE " + std::to_string(page_count_));

        for (const auto& h : headings_) {
            print_line(h);
        }

        // Separator line
        print_line(std::string(80, '-'));
    }

    void print_line(const std::string& line) {
        out_ << line << "\n";
        line_count_++;
    }

    void print_formatted(const std::vector<std::pair<std::string, int>>& columns) {
        check_page_break();
        std::ostringstream line;
        for (const auto& [val, width] : columns) {
            if (width > 0) {
                line << std::setw(width) << std::left << val;
            } else {
                line << std::setw(-width) << std::right << val;
            }
        }
        print_line(line.str());
    }

    void print_subtotal_line(int level, const std::string& label) {
        if (level < 0 || level >= static_cast<int>(break_levels_.size())) return;
        auto& bl = break_levels_[static_cast<size_t>(level)];
        std::ostringstream line;
        line << label;
        for (const auto& [field, acc] : bl.accumulators) {
            line << "  " << field << ": " << std::fixed << std::setprecision(2) << acc;
        }
        print_line(line.str());
        // Reset accumulators for this level
        for (auto& [f, a] : bl.accumulators) {
            (void)f;
            a = 0.0;
        }
    }

    void print_grand_totals(const std::string& label) {
        std::ostringstream line;
        line << label;
        for (const auto& [field, total] : grand_totals_) {
            line << "  " << field << ": " << std::fixed << std::setprecision(2) << total;
        }
        print_line(line.str());
    }

    // Reset for new report
    void reset() {
        line_count_ = 0;
        page_count_ = 0;
        for (auto& bl : break_levels_) {
            bl.initialized = false;
            for (auto& [f, a] : bl.accumulators) {
                (void)f;
                a = 0.0;
            }
        }
        grand_totals_.clear();
    }

    // ---- Date/time utilities ----
    static std::string get_date_stamp() {
        std::time_t t = std::time(nullptr);
        std::tm tm_buf{};
#ifdef _WIN32
        localtime_s(&tm_buf, &t);
#else
        localtime_r(&t, &tm_buf);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &tm_buf);
        return std::string(buf);
    }

private:
    std::ostream& out_;
    int lines_per_page_;
    int line_count_;
    int page_count_;
    std::vector<std::string> titles_;
    std::vector<std::string> headings_;
    std::function<void(std::ostream&, int)> header_callback_;
    std::vector<BreakLevel> break_levels_;
    std::map<std::string, double> grand_totals_;
};

}}} // namespace lazarus::easytrieve::runtime

#endif // LAZARUS_EASYTRIEVE_RUNTIME_H
