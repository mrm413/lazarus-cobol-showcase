#ifndef LAZARUS_PLI_RUNTIME_H
#define LAZARUS_PLI_RUNTIME_H

#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <map>
#include <fstream>
#include <cstdio>
#include <algorithm>
#include <memory>
#include <cmath>
#include <cstring>

namespace lazarus { namespace pli { namespace runtime {

// ---------- PL/I Condition classes ----------

class PliCondition : public std::runtime_error {
public:
    int code;
    std::string condition_name;
    PliCondition(const std::string& name, int c, const std::string& msg)
        : std::runtime_error(msg), code(c), condition_name(name) {}
};

class PliError : public PliCondition {
public:
    explicit PliError(const std::string& msg = "ERROR condition")
        : PliCondition("ERROR", 1000, msg) {}
};

class PliEndFile : public PliCondition {
public:
    std::string file_name;
    explicit PliEndFile(const std::string& fname = "")
        : PliCondition("ENDFILE", 70, "End of file on " + fname), file_name(fname) {}
};

class PliEndPage : public PliCondition {
public:
    explicit PliEndPage() : PliCondition("ENDPAGE", 71, "End of page") {}
};

class PliKey : public PliCondition {
public:
    std::string key_value;
    explicit PliKey(const std::string& k = "")
        : PliCondition("KEY", 50, "KEY condition: " + k), key_value(k) {}
};

class PliConversion : public PliCondition {
public:
    std::string source;
    explicit PliConversion(const std::string& s = "")
        : PliCondition("CONVERSION", 600, "Conversion error: " + s), source(s) {}
};

class PliFixedOverflow : public PliCondition {
public:
    PliFixedOverflow() : PliCondition("FIXEDOVERFLOW", 310, "Fixed-point overflow") {}
};

class PliOverflow : public PliCondition {
public:
    PliOverflow() : PliCondition("OVERFLOW", 300, "Floating-point overflow") {}
};

class PliUnderflow : public PliCondition {
public:
    PliUnderflow() : PliCondition("UNDERFLOW", 320, "Floating-point underflow") {}
};

class PliZeroDivide : public PliCondition {
public:
    PliZeroDivide() : PliCondition("ZERODIVIDE", 320, "Zero divide") {}
};

class PliSubscriptRange : public PliCondition {
public:
    PliSubscriptRange() : PliCondition("SUBSCRIPTRANGE", 520, "Subscript out of range") {}
};

class PliStringRange : public PliCondition {
public:
    PliStringRange() : PliCondition("STRINGRANGE", 500, "String range error") {}
};

class PliSize : public PliCondition {
public:
    PliSize() : PliCondition("SIZE", 340, "Size condition") {}
};

class PliArea : public PliCondition {
public:
    PliArea() : PliCondition("AREA", 200, "Area condition") {}
};

class PliStorage : public PliCondition {
public:
    PliStorage() : PliCondition("STORAGE", 830, "Storage condition") {}
};

class PliUndefinedFile : public PliCondition {
public:
    std::string file_name;
    explicit PliUndefinedFile(const std::string& fname = "")
        : PliCondition("UNDEFINEDFILE", 80, "Undefined file: " + fname), file_name(fname) {}
};

class PliName : public PliCondition {
public:
    PliName() : PliCondition("NAME", 15, "Name condition") {}
};

class PliRecord : public PliCondition {
public:
    PliRecord() : PliCondition("RECORD", 21, "Record condition") {}
};

// ---------- Handler stack (thread_local for dynamic scope) ----------

using ConditionHandler = std::function<void(const PliCondition&)>;

struct HandlerEntry {
    std::string condition_name;
    ConditionHandler handler;
};

inline std::vector<HandlerEntry>& handler_stack() {
    static thread_local std::vector<HandlerEntry> stack;
    return stack;
}

inline void push_handler(const std::string& condition, ConditionHandler handler) {
    handler_stack().push_back({condition, std::move(handler)});
}

inline void pop_handler(const std::string& condition) {
    auto& stack = handler_stack();
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        if (it->condition_name == condition) {
            stack.erase((it + 1).base());
            return;
        }
    }
}

inline bool dispatch_condition(const PliCondition& cond) {
    auto& stack = handler_stack();
    for (auto it = stack.rbegin(); it != stack.rend(); ++it) {
        if (it->condition_name == cond.condition_name || it->condition_name == "ERROR") {
            it->handler(cond);
            return true;
        }
    }
    return false;
}

inline void signal_condition(const std::string& condition_name) {
    std::string upper = condition_name;
    for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    if (upper == "ERROR") throw PliError();
    if (upper == "ENDFILE") throw PliEndFile();
    if (upper == "ENDPAGE") throw PliEndPage();
    if (upper == "KEY") throw PliKey();
    if (upper == "CONVERSION") throw PliConversion();
    if (upper == "FIXEDOVERFLOW") throw PliFixedOverflow();
    if (upper == "OVERFLOW") throw PliOverflow();
    if (upper == "UNDERFLOW") throw PliUnderflow();
    if (upper == "ZERODIVIDE") throw PliZeroDivide();
    if (upper == "SUBSCRIPTRANGE") throw PliSubscriptRange();
    if (upper == "STRINGRANGE") throw PliStringRange();
    if (upper == "SIZE") throw PliSize();
    if (upper == "AREA") throw PliArea();
    if (upper == "STORAGE") throw PliStorage();
    if (upper == "UNDEFINEDFILE") throw PliUndefinedFile();
    if (upper == "NAME") throw PliName();
    if (upper == "RECORD") throw PliRecord();

    throw PliError("Unknown condition: " + condition_name);
}

// ---------- Format engine ----------

class FormatEngine {
public:
    // Format A(w): character field
    static std::string format_A(const std::string& val, int w = 0) {
        if (w <= 0) return val;
        if (static_cast<int>(val.size()) >= w) return val.substr(0, static_cast<size_t>(w));
        return val + std::string(static_cast<size_t>(w) - val.size(), ' ');
    }

    // Format F(w,d): fixed-point numeric
    static std::string format_F(double val, int w, int d = 0) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(d) << val;
        std::string s = oss.str();
        if (w > 0 && static_cast<int>(s.size()) < w) {
            s = std::string(static_cast<size_t>(w) - s.size(), ' ') + s;
        }
        return s;
    }

    // Format E(w,d,s): floating-point scientific
    static std::string format_E(double val, int w, int d = 6, int /*s*/ = 0) {
        std::ostringstream oss;
        oss << std::scientific << std::setprecision(d) << val;
        std::string str = oss.str();
        if (w > 0 && static_cast<int>(str.size()) < w) {
            str = std::string(static_cast<size_t>(w) - str.size(), ' ') + str;
        }
        return str;
    }

    // Format B(w): bit string
    static std::string format_B(int64_t val, int w = 32) {
        std::string bits;
        for (int i = w - 1; i >= 0; i--) {
            bits += ((val >> i) & 1) ? '1' : '0';
        }
        return bits;
    }

    // Format P'picture': picture editing
    static std::string format_P(double val, const std::string& pic) {
        // Simplified picture formatting
        std::ostringstream oss;
        int decimal_places = 0;
        bool found_v = false;
        for (char c : pic) {
            if (c == 'V' || c == 'v') found_v = true;
            else if (found_v && c == '9') decimal_places++;
        }
        oss << std::fixed << std::setprecision(decimal_places) << val;
        return oss.str();
    }

    // Format X(n): insert n spaces
    static std::string format_X(int n) {
        return std::string(static_cast<size_t>(n), ' ');
    }

    // SKIP(n): newlines
    static std::string format_SKIP(int n = 1) {
        return std::string(static_cast<size_t>(n), '\n');
    }

    // COLUMN(n): spaces to column
    static std::string format_COLUMN(int n, int current_col = 0) {
        if (n <= current_col) return "";
        return std::string(static_cast<size_t>(n - current_col), ' ');
    }
};

// ---------- PL/I File I/O ----------

enum class FileMode { STREAM_INPUT, STREAM_OUTPUT, STREAM_PRINT, RECORD_INPUT, RECORD_OUTPUT, RECORD_UPDATE };

class PliFile {
public:
    std::string name;
    FileMode mode = FileMode::STREAM_INPUT;
    bool is_open = false;
    int line_number = 0;
    int page_number = 1;
    int column = 0;
    int page_size = 60;

    // Stream I/O buffer
    std::string stream_buffer;
    size_t read_pos = 0;

    // Record I/O
    std::vector<std::string> records;
    size_t record_pos = 0;

    // Keyed records
    std::map<std::string, std::string> keyed_records;

    void open(FileMode m) {
        mode = m;
        is_open = true;
        line_number = 0;
        page_number = 1;
        column = 0;
        read_pos = 0;
        record_pos = 0;
    }

    void close() {
        is_open = false;
    }

    // Stream output
    void put_chars(const std::string& s) {
        stream_buffer += s;
        for (char c : s) {
            if (c == '\n') { line_number++; column = 0; }
            else column++;
        }
    }

    // Stream input (from buffer)
    std::string get_chars(int n) {
        if (read_pos >= stream_buffer.size()) throw PliEndFile(name);
        size_t avail = stream_buffer.size() - read_pos;
        size_t count = std::min(static_cast<size_t>(n), avail);
        std::string result = stream_buffer.substr(read_pos, count);
        read_pos += count;
        return result;
    }

    // Stream list-directed input: read next token (space/comma separated)
    std::string get_list_item() {
        // Skip whitespace and commas
        while (read_pos < stream_buffer.size() &&
               (stream_buffer[read_pos] == ' ' || stream_buffer[read_pos] == ',' ||
                stream_buffer[read_pos] == '\n' || stream_buffer[read_pos] == '\t')) {
            read_pos++;
        }
        if (read_pos >= stream_buffer.size()) throw PliEndFile(name);

        std::string result;
        if (stream_buffer[read_pos] == '\'') {
            // Quoted string
            read_pos++;
            while (read_pos < stream_buffer.size() && stream_buffer[read_pos] != '\'') {
                result += stream_buffer[read_pos++];
            }
            if (read_pos < stream_buffer.size()) read_pos++;
        } else {
            while (read_pos < stream_buffer.size() &&
                   stream_buffer[read_pos] != ' ' && stream_buffer[read_pos] != ',' &&
                   stream_buffer[read_pos] != '\n') {
                result += stream_buffer[read_pos++];
            }
        }
        return result;
    }

    // Record I/O
    void write_record(const std::string& rec) {
        records.push_back(rec);
    }

    void write_record_keyed(const std::string& key, const std::string& rec) {
        keyed_records[key] = rec;
    }

    std::string read_record() {
        if (record_pos >= records.size()) throw PliEndFile(name);
        return records[record_pos++];
    }

    std::string read_record_keyed(const std::string& key) {
        auto it = keyed_records.find(key);
        if (it == keyed_records.end()) throw PliKey(key);
        return it->second;
    }

    void rewrite_record(const std::string& rec) {
        if (record_pos > 0 && record_pos <= records.size()) {
            records[record_pos - 1] = rec;
        }
    }

    void delete_record() {
        if (record_pos > 0 && record_pos <= records.size()) {
            records.erase(records.begin() + static_cast<std::ptrdiff_t>(record_pos - 1));
            record_pos--;
        }
    }

    void delete_record_keyed(const std::string& key) {
        keyed_records.erase(key);
    }
};

// Global file table
inline std::map<std::string, PliFile>& file_table() {
    static thread_local std::map<std::string, PliFile> table;
    return table;
}

inline PliFile& get_file(const std::string& name) {
    auto& table = file_table();
    auto it = table.find(name);
    if (it == table.end()) {
        table[name].name = name;
    }
    return table[name];
}

// PUT STRING / GET STRING support
class StringStream {
public:
    std::string buffer;
    size_t read_pos = 0;

    void put(const std::string& s) { buffer += s; }

    std::string get_list_item() {
        while (read_pos < buffer.size() &&
               (buffer[read_pos] == ' ' || buffer[read_pos] == ',' ||
                buffer[read_pos] == '\n')) {
            read_pos++;
        }
        if (read_pos >= buffer.size()) return "";
        std::string result;
        while (read_pos < buffer.size() && buffer[read_pos] != ' ' &&
               buffer[read_pos] != ',' && buffer[read_pos] != '\n') {
            result += buffer[read_pos++];
        }
        return result;
    }

    std::string str() const { return buffer; }
    void clear() { buffer.clear(); read_pos = 0; }
};

// ---------- PUT LIST implementation ----------

inline std::string put_list_format(int64_t val) {
    return std::to_string(val);
}

inline std::string put_list_format(double val) {
    std::ostringstream oss;
    oss << val;
    return oss.str();
}

inline std::string put_list_format(const std::string& val) {
    return val;
}

// ---------- Safe division ----------

inline double safe_divide(double a, double b) {
    if (b == 0.0) throw PliZeroDivide();
    double result = a / b;
    if (std::isinf(result)) throw PliOverflow();
    return result;
}

inline int64_t safe_divide(int64_t a, int64_t b) {
    if (b == 0) throw PliZeroDivide();
    return a / b;
}

// ---------- Subscript check ----------

inline void check_subscript(int idx, int lower, int upper) {
    if (idx < lower || idx > upper) throw PliSubscriptRange();
}

// ---------- String range check ----------

inline void check_string_range(int pos, int len, int str_len) {
    if (pos < 1 || pos > str_len || pos + len - 1 > str_len) throw PliStringRange();
}

}}} // namespace lazarus::pli::runtime

#endif // LAZARUS_PLI_RUNTIME_H
