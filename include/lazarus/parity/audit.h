#ifndef LAZARUS_PARITY_AUDIT_H
#define LAZARUS_PARITY_AUDIT_H

#include <string>
#include <vector>
#include <sstream>
#include <cstddef>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <cmath>

#include "lazarus/parity/comparator.h"

namespace lazarus { namespace parity {

// ---------------------------------------------------------------------------
// Audit status for a single field check
// ---------------------------------------------------------------------------
enum class AuditStatus { PASS, FAIL, WARN };

inline std::string audit_status_str(AuditStatus s) {
    switch (s) {
        case AuditStatus::PASS: return "PASS";
        case AuditStatus::FAIL: return "FAIL";
        case AuditStatus::WARN: return "WARN";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Failure categories
// ---------------------------------------------------------------------------
enum class FailureCategory {
    NONE,
    WHITESPACE,       // cosmetic: trailing spaces, padding differences
    NUMERIC_PRECISION,// floating-point rounding
    DATA_LOSS,        // actual is truncated or missing data
    STRUCTURAL,       // different number of fields/lines, layout mismatch
    TYPE_MISMATCH,    // data type interpretation differs
    ENCODING          // character encoding / EBCDIC-ASCII issues
};

inline std::string failure_category_str(FailureCategory fc) {
    switch (fc) {
        case FailureCategory::NONE:              return "NONE";
        case FailureCategory::WHITESPACE:        return "WHITESPACE";
        case FailureCategory::NUMERIC_PRECISION: return "NUMERIC_PRECISION";
        case FailureCategory::DATA_LOSS:         return "DATA_LOSS";
        case FailureCategory::STRUCTURAL:        return "STRUCTURAL";
        case FailureCategory::TYPE_MISMATCH:     return "TYPE_MISMATCH";
        case FailureCategory::ENCODING:          return "ENCODING";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// AuditEntry -- one field comparison result
// ---------------------------------------------------------------------------
struct AuditEntry {
    std::string      field_name;
    std::string      expected_value;
    std::string      actual_value;
    DataType         data_type      = DataType::DISPLAY;
    AuditStatus      status         = AuditStatus::PASS;
    FailureCategory  category       = FailureCategory::NONE;
    size_t           line_number    = 0;
    std::string      detail;
};

// ---------------------------------------------------------------------------
// Auto-categorize a failure based on expected vs actual
// ---------------------------------------------------------------------------
inline FailureCategory categorize_failure(
        std::string_view expected,
        std::string_view actual,
        DataType dt)
{
    if (expected == actual) return FailureCategory::NONE;

    // Check whitespace-only difference
    auto exp_trimmed = detail::trim(expected);
    auto act_trimmed = detail::trim(actual);
    if (exp_trimmed == act_trimmed) return FailureCategory::WHITESPACE;

    // Check numeric precision
    if (dt == DataType::FLOAT || dt == DataType::COMP3) {
        try {
            double e = detail::parse_double(expected);
            double a = detail::parse_double(actual);
            if (std::fabs(e - a) < 0.01) return FailureCategory::NUMERIC_PRECISION;
        } catch (...) {}
    }

    // Data loss: actual is shorter (truncated)
    if (actual.empty() || actual.size() < expected.size() / 2) {
        return FailureCategory::DATA_LOSS;
    }

    // Structural: different line counts
    auto count_lines = [](std::string_view sv) {
        size_t n = 1;
        for (char c : sv) if (c == '\n') ++n;
        return n;
    };
    if (count_lines(expected) != count_lines(actual)) {
        return FailureCategory::STRUCTURAL;
    }

    // Default to type mismatch
    return FailureCategory::TYPE_MISMATCH;
}

// ---------------------------------------------------------------------------
// AuditSummary -- aggregate statistics
// ---------------------------------------------------------------------------
struct AuditSummary {
    size_t total_fields = 0;
    size_t passed       = 0;
    size_t failed       = 0;
    size_t warnings     = 0;
    double pass_rate    = 0.0;

    // Per-category failure counts
    size_t whitespace_failures   = 0;
    size_t precision_failures    = 0;
    size_t data_loss_failures    = 0;
    size_t structural_failures   = 0;
    size_t type_mismatch_failures = 0;
    size_t encoding_failures     = 0;
};

// ---------------------------------------------------------------------------
// AuditLog -- collection of entries with statistics and serialization
// ---------------------------------------------------------------------------
class AuditLog {
public:
    AuditLog() = default;

    // Append an entry with auto-categorization
    void append(const std::string& field_name,
                std::string_view expected,
                std::string_view actual,
                DataType dt = DataType::DISPLAY,
                size_t line_number = 0)
    {
        AuditEntry entry;
        entry.field_name     = field_name;
        entry.expected_value = std::string(expected);
        entry.actual_value   = std::string(actual);
        entry.data_type      = dt;
        entry.line_number    = line_number;

        bool match = compare_field_dynamic(dt, expected, actual);
        if (match) {
            entry.status   = AuditStatus::PASS;
            entry.category = FailureCategory::NONE;
        } else {
            auto cat = categorize_failure(expected, actual, dt);
            // Whitespace-only = WARN, everything else = FAIL
            if (cat == FailureCategory::WHITESPACE) {
                entry.status = AuditStatus::WARN;
            } else {
                entry.status = AuditStatus::FAIL;
            }
            entry.category = cat;
            entry.detail   = "expected [" + std::string(expected) +
                             "] got [" + std::string(actual) + "]";
        }

        entries_.push_back(std::move(entry));
    }

    // Append a pre-built entry
    void append(AuditEntry entry) {
        entries_.push_back(std::move(entry));
    }

    // Access entries
    const std::vector<AuditEntry>& entries() const noexcept { return entries_; }
    size_t size() const noexcept { return entries_.size(); }
    bool empty() const noexcept { return entries_.empty(); }
    const AuditEntry& operator[](size_t i) const { return entries_[i]; }

    // Compute summary
    AuditSummary summary() const {
        AuditSummary s;
        s.total_fields = entries_.size();
        for (auto& e : entries_) {
            switch (e.status) {
                case AuditStatus::PASS: ++s.passed; break;
                case AuditStatus::FAIL: ++s.failed; break;
                case AuditStatus::WARN: ++s.warnings; break;
            }
            switch (e.category) {
                case FailureCategory::WHITESPACE:        ++s.whitespace_failures; break;
                case FailureCategory::NUMERIC_PRECISION: ++s.precision_failures; break;
                case FailureCategory::DATA_LOSS:         ++s.data_loss_failures; break;
                case FailureCategory::STRUCTURAL:        ++s.structural_failures; break;
                case FailureCategory::TYPE_MISMATCH:     ++s.type_mismatch_failures; break;
                case FailureCategory::ENCODING:          ++s.encoding_failures; break;
                case FailureCategory::NONE: break;
            }
        }
        s.pass_rate = s.total_fields > 0
            ? static_cast<double>(s.passed) / static_cast<double>(s.total_fields) * 100.0
            : 100.0;
        return s;
    }

    // Clear all entries
    void clear() { entries_.clear(); }

    // Filter entries by status
    std::vector<AuditEntry> failures() const {
        std::vector<AuditEntry> result;
        for (auto& e : entries_) {
            if (e.status == AuditStatus::FAIL) result.push_back(e);
        }
        return result;
    }

    std::vector<AuditEntry> warnings() const {
        std::vector<AuditEntry> result;
        for (auto& e : entries_) {
            if (e.status == AuditStatus::WARN) result.push_back(e);
        }
        return result;
    }

    // ------------------------------------------------------------------
    // JSON serialization
    // ------------------------------------------------------------------
    static std::string escape_json(std::string_view sv) {
        std::string out;
        out.reserve(sv.size() + 8);
        for (char c : sv) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:   out += c;      break;
            }
        }
        return out;
    }

    std::string to_json() const {
        std::ostringstream os;
        auto s = summary();

        os << "{\n";
        os << "  \"summary\": {\n";
        os << "    \"total_fields\": " << s.total_fields << ",\n";
        os << "    \"passed\": " << s.passed << ",\n";
        os << "    \"failed\": " << s.failed << ",\n";
        os << "    \"warnings\": " << s.warnings << ",\n";
        os << "    \"pass_rate\": " << std::fixed << std::setprecision(1)
           << s.pass_rate << "\n";
        os << "  },\n";
        os << "  \"entries\": [\n";

        for (size_t i = 0; i < entries_.size(); ++i) {
            auto& e = entries_[i];
            os << "    {\n";
            os << "      \"field_name\": \"" << escape_json(e.field_name) << "\",\n";
            os << "      \"expected\": \"" << escape_json(e.expected_value) << "\",\n";
            os << "      \"actual\": \"" << escape_json(e.actual_value) << "\",\n";
            os << "      \"data_type\": \"" << data_type_str(e.data_type) << "\",\n";
            os << "      \"status\": \"" << audit_status_str(e.status) << "\",\n";
            os << "      \"category\": \"" << failure_category_str(e.category) << "\",\n";
            os << "      \"line_number\": " << e.line_number << ",\n";
            os << "      \"detail\": \"" << escape_json(e.detail) << "\"\n";
            os << "    }";
            if (i + 1 < entries_.size()) os << ",";
            os << "\n";
        }

        os << "  ]\n";
        os << "}";
        return os.str();
    }

    // ------------------------------------------------------------------
    // JSON deserialization (simple parser for our own format)
    // ------------------------------------------------------------------
    static AuditLog from_json(const std::string& json) {
        AuditLog log;
        // Parse entries from our known JSON format
        // Find each entry block
        size_t pos = 0;
        while ((pos = json.find("\"field_name\"", pos)) != std::string::npos) {
            AuditEntry entry;
            auto extract_string = [&](const std::string& key, size_t start) -> std::string {
                auto kpos = json.find("\"" + key + "\"", start);
                if (kpos == std::string::npos || kpos > start + 500) return "";
                auto colon = json.find(':', kpos + key.size() + 2);
                if (colon == std::string::npos) return "";
                auto quote1 = json.find('"', colon + 1);
                if (quote1 == std::string::npos) return "";
                std::string result;
                for (size_t i = quote1 + 1; i < json.size(); ++i) {
                    if (json[i] == '\\' && i + 1 < json.size()) {
                        char next = json[i + 1];
                        if (next == '"') { result += '"'; ++i; }
                        else if (next == '\\') { result += '\\'; ++i; }
                        else if (next == 'n') { result += '\n'; ++i; }
                        else if (next == 'r') { result += '\r'; ++i; }
                        else if (next == 't') { result += '\t'; ++i; }
                        else { result += json[i]; }
                    } else if (json[i] == '"') {
                        break;
                    } else {
                        result += json[i];
                    }
                }
                return result;
            };

            auto extract_number = [&](const std::string& key, size_t start) -> size_t {
                auto kpos = json.find("\"" + key + "\"", start);
                if (kpos == std::string::npos || kpos > start + 500) return 0;
                auto colon = json.find(':', kpos + key.size() + 2);
                if (colon == std::string::npos) return 0;
                std::string num;
                for (size_t i = colon + 1; i < json.size(); ++i) {
                    if (json[i] >= '0' && json[i] <= '9') num += json[i];
                    else if (!num.empty()) break;
                }
                return num.empty() ? 0 : std::stoull(num);
            };

            size_t block_start = pos > 20 ? pos - 20 : 0;

            entry.field_name     = extract_string("field_name", block_start);
            entry.expected_value = extract_string("expected", block_start);
            entry.actual_value   = extract_string("actual", block_start);

            auto dt_str = extract_string("data_type", block_start);
            if (dt_str == "COMP")    entry.data_type = DataType::COMP;
            else if (dt_str == "COMP3")   entry.data_type = DataType::COMP3;
            else if (dt_str == "COMP5")   entry.data_type = DataType::COMP5;
            else if (dt_str == "FLOAT")   entry.data_type = DataType::FLOAT;
            else if (dt_str == "ALPHA")   entry.data_type = DataType::ALPHA;
            else if (dt_str == "GROUP")   entry.data_type = DataType::GROUP;
            else if (dt_str == "NATIONAL")entry.data_type = DataType::NATIONAL;
            else entry.data_type = DataType::DISPLAY;

            auto st_str = extract_string("status", block_start);
            if (st_str == "FAIL")      entry.status = AuditStatus::FAIL;
            else if (st_str == "WARN") entry.status = AuditStatus::WARN;
            else entry.status = AuditStatus::PASS;

            auto cat_str = extract_string("category", block_start);
            if (cat_str == "WHITESPACE")            entry.category = FailureCategory::WHITESPACE;
            else if (cat_str == "NUMERIC_PRECISION")entry.category = FailureCategory::NUMERIC_PRECISION;
            else if (cat_str == "DATA_LOSS")        entry.category = FailureCategory::DATA_LOSS;
            else if (cat_str == "STRUCTURAL")       entry.category = FailureCategory::STRUCTURAL;
            else if (cat_str == "TYPE_MISMATCH")    entry.category = FailureCategory::TYPE_MISMATCH;
            else if (cat_str == "ENCODING")         entry.category = FailureCategory::ENCODING;
            else entry.category = FailureCategory::NONE;

            entry.line_number = extract_number("line_number", block_start);
            entry.detail      = extract_string("detail", block_start);

            log.append(std::move(entry));
            pos += 10; // advance past this entry
        }
        return log;
    }

    // ------------------------------------------------------------------
    // Diff generation: show expected vs actual with context
    // ------------------------------------------------------------------
    std::string generate_diff(size_t context_lines = 3) const {
        std::ostringstream os;
        os << "=== Parity Audit Diff ===\n";
        auto s = summary();
        os << "Total: " << s.total_fields
           << " | Pass: " << s.passed
           << " | Fail: " << s.failed
           << " | Warn: " << s.warnings
           << " | Rate: " << std::fixed << std::setprecision(1)
           << s.pass_rate << "%\n";
        os << "========================\n\n";

        (void)context_lines; // context_lines reserved for multi-line diffs

        for (auto& e : entries_) {
            if (e.status == AuditStatus::PASS) continue;
            os << (e.status == AuditStatus::FAIL ? "FAIL" : "WARN");
            os << " [" << data_type_str(e.data_type) << "] ";
            os << e.field_name;
            if (e.line_number > 0) os << " (line " << e.line_number << ")";
            os << "\n";
            os << "  - expected: \"" << e.expected_value << "\"\n";
            os << "  + actual:   \"" << e.actual_value << "\"\n";
            if (!e.detail.empty()) {
                os << "  category: " << failure_category_str(e.category) << "\n";
            }
            os << "\n";
        }

        return os.str();
    }

private:
    std::vector<AuditEntry> entries_;
};

}} // namespace lazarus::parity

#endif // LAZARUS_PARITY_AUDIT_H
