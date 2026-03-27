#ifndef LAZARUS_PARITY_COMPARATOR_H
#define LAZARUS_PARITY_COMPARATOR_H

#include <string>
#include <string_view>
#include <vector>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <optional>
#include <functional>
#include <limits>

namespace lazarus { namespace parity {

// ---------------------------------------------------------------------------
// COBOL data types for field-level comparison
// ---------------------------------------------------------------------------
enum class DataType {
    DISPLAY,    // PIC 9/X DISPLAY
    COMP,       // binary integer (COMP / COMP-4)
    COMP3,      // packed decimal (BCD)
    COMP5,      // native binary integer
    FLOAT,      // COMP-1 / COMP-2 floating point
    ALPHA,      // PIC A alphabetic
    GROUP,      // group-level (recursive)
    NATIONAL    // PIC N (UTF-16 / DBCS)
};

inline std::string data_type_str(DataType dt) {
    switch (dt) {
        case DataType::DISPLAY:  return "DISPLAY";
        case DataType::COMP:     return "COMP";
        case DataType::COMP3:    return "COMP3";
        case DataType::COMP5:    return "COMP5";
        case DataType::FLOAT:    return "FLOAT";
        case DataType::ALPHA:    return "ALPHA";
        case DataType::GROUP:    return "GROUP";
        case DataType::NATIONAL: return "NATIONAL";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Whitespace normalization options
// ---------------------------------------------------------------------------
struct NormalizationOptions {
    bool trim_trailing       = true;
    bool trim_leading        = false;
    bool normalize_internal  = false;   // collapse runs of spaces to one
    bool ignore_case         = false;
    bool strip_leading_zeros = false;
    bool normalize_sign      = false;   // treat +0 == -0, leading + == unsigned
    bool normalize_decimal   = false;   // "1.0" == "1.00"
};

// ---------------------------------------------------------------------------
// String utilities
// ---------------------------------------------------------------------------
namespace detail {

inline std::string trim_right(std::string_view sv) {
    auto end = sv.find_last_not_of(" \t\r\n");
    if (end == std::string_view::npos) return "";
    return std::string(sv.substr(0, end + 1));
}

inline std::string trim_left(std::string_view sv) {
    auto start = sv.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) return "";
    return std::string(sv.substr(start));
}

inline std::string trim(std::string_view sv) {
    return trim_left(trim_right(sv));
}

inline std::string normalize_whitespace(std::string_view sv) {
    std::string result;
    result.reserve(sv.size());
    bool last_space = false;
    for (char c : sv) {
        if (c == ' ' || c == '\t') {
            if (!last_space) { result += ' '; last_space = true; }
        } else {
            result += c;
            last_space = false;
        }
    }
    return result;
}

inline std::string to_lower(std::string_view sv) {
    std::string s(sv);
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

inline std::string strip_leading_zeros(std::string_view sv) {
    // Preserve sign
    std::string_view num = sv;
    std::string sign;
    if (!num.empty() && (num[0] == '+' || num[0] == '-')) {
        sign = std::string(1, num[0]);
        num = num.substr(1);
    }
    // Strip zeros but keep at least one digit
    auto pos = num.find_first_not_of('0');
    if (pos == std::string_view::npos) return sign + "0";
    // Check if next is a decimal point -- keep one leading zero
    if (num[pos] == '.') {
        if (pos == 0) return sign + std::string(num);
        return sign + "0" + std::string(num.substr(pos));
    }
    return sign + std::string(num.substr(pos));
}

inline std::string normalize_sign(std::string_view sv) {
    if (sv.empty()) return "";
    std::string s(sv);
    // Remove leading +
    if (s[0] == '+') s = s.substr(1);
    // Treat -0, -0.0, -0.00 etc as unsigned zero
    if (s[0] == '-') {
        std::string_view rest(s.c_str() + 1, s.size() - 1);
        bool all_zero = true;
        for (char c : rest) {
            if (c != '0' && c != '.' && c != ' ') { all_zero = false; break; }
        }
        if (all_zero) return std::string(rest);
    }
    return s;
}

inline std::string normalize_decimal(std::string_view sv) {
    // Remove trailing zeros after decimal point
    std::string s(sv);
    auto dot = s.find('.');
    if (dot == std::string::npos) return s;
    auto last_nonzero = s.find_last_not_of('0');
    if (last_nonzero == dot) {
        return s.substr(0, dot); // remove decimal entirely (e.g. "1." -> "1")
    }
    return s.substr(0, last_nonzero + 1);
}

inline std::string apply_normalization(std::string_view sv,
                                        const NormalizationOptions& opts) {
    std::string s(sv);
    if (opts.trim_trailing)       s = trim_right(s);
    if (opts.trim_leading)        s = trim_left(s);
    if (opts.normalize_internal)  s = normalize_whitespace(s);
    if (opts.strip_leading_zeros) s = strip_leading_zeros(s);
    if (opts.normalize_sign)      s = normalize_sign(s);
    if (opts.normalize_decimal)   s = normalize_decimal(s);
    if (opts.ignore_case)         s = to_lower(s);
    return s;
}

// Parse integer from string view (ignore non-digit except sign)
inline int64_t parse_integer(std::string_view sv) {
    std::string cleaned;
    cleaned.reserve(sv.size());
    for (size_t i = 0; i < sv.size(); ++i) {
        char c = sv[i];
        if (c == '-' || c == '+') {
            if (i == 0) cleaned += c;
        } else if (c >= '0' && c <= '9') {
            cleaned += c;
        }
    }
    if (cleaned.empty() || cleaned == "-" || cleaned == "+") return 0;
    return std::stoll(cleaned);
}

// Parse double from string view
inline double parse_double(std::string_view sv) {
    std::string s(sv);
    // Remove spaces
    s.erase(std::remove(s.begin(), s.end(), ' '), s.end());
    if (s.empty()) return 0.0;
    return std::stod(s);
}

// COMP-3 packed decimal: each byte has 2 digits except last nibble is sign
// Compare as string representation after unpacking
inline std::string unpack_comp3_hex(std::string_view hex_str) {
    // Input: hex string representing packed bytes e.g. "12345C" = +12345
    if (hex_str.empty()) return "0";
    // Last nibble is sign: C/A/F = positive, D/B = negative
    char sign_nibble = static_cast<char>(
        std::toupper(static_cast<unsigned char>(hex_str.back())));
    bool negative = (sign_nibble == 'D' || sign_nibble == 'B');
    std::string digits;
    for (size_t i = 0; i < hex_str.size() - 1; ++i) {
        char c = hex_str[i];
        if (c >= '0' && c <= '9') digits += c;
        else if ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) {
            // Treat hex A-F in digit positions as literal values 10-15
            // But for packed decimal, digit nibbles should only be 0-9
            digits += c;
        }
    }
    // Strip leading zeros
    auto pos = digits.find_first_not_of('0');
    if (pos == std::string::npos) return "0";
    digits = digits.substr(pos);
    return (negative ? "-" : "") + digits;
}

} // namespace detail

// ---------------------------------------------------------------------------
// Template-specialized field comparison
// ---------------------------------------------------------------------------
template<DataType T>
bool compare_field(std::string_view expected, std::string_view actual,
                   double tolerance = 0.0);

// DISPLAY: string comparison with trailing space normalization
template<>
inline bool compare_field<DataType::DISPLAY>(
        std::string_view expected, std::string_view actual, double /*tolerance*/)
{
    return detail::trim_right(expected) == detail::trim_right(actual);
}

// COMP: integer comparison (exact)
template<>
inline bool compare_field<DataType::COMP>(
        std::string_view expected, std::string_view actual, double /*tolerance*/)
{
    return detail::parse_integer(expected) == detail::parse_integer(actual);
}

// COMP-3: packed decimal comparison (penny-exact)
template<>
inline bool compare_field<DataType::COMP3>(
        std::string_view expected, std::string_view actual, double /*tolerance*/)
{
    // For COMP-3, we compare the decimal string representation exactly
    // Both inputs should be decimal string representations
    auto norm_expected = detail::strip_leading_zeros(detail::trim(expected));
    auto norm_actual   = detail::strip_leading_zeros(detail::trim(actual));
    return norm_expected == norm_actual;
}

// COMP-5: native binary integer (exact)
template<>
inline bool compare_field<DataType::COMP5>(
        std::string_view expected, std::string_view actual, double /*tolerance*/)
{
    return detail::parse_integer(expected) == detail::parse_integer(actual);
}

// FLOAT: epsilon comparison with configurable tolerance
template<>
inline bool compare_field<DataType::FLOAT>(
        std::string_view expected, std::string_view actual, double tolerance)
{
    double e = detail::parse_double(expected);
    double a = detail::parse_double(actual);
    if (tolerance <= 0.0) {
        tolerance = std::numeric_limits<double>::epsilon() * 100.0;
    }
    return std::fabs(e - a) <= tolerance;
}

// ALPHA: case-sensitive with padding normalization
template<>
inline bool compare_field<DataType::ALPHA>(
        std::string_view expected, std::string_view actual, double /*tolerance*/)
{
    // COBOL ALPHA fields are right-padded with spaces
    return detail::trim_right(expected) == detail::trim_right(actual);
}

// GROUP: recursive field comparison (delegates to per-line)
template<>
inline bool compare_field<DataType::GROUP>(
        std::string_view expected, std::string_view actual, double /*tolerance*/)
{
    // For GROUP, compare as multi-line text with trailing whitespace normalization
    std::string exp_str{expected};
    std::string act_str{actual};
    std::istringstream exp_stream{exp_str};
    std::istringstream act_stream{act_str};
    std::string exp_line, act_line;

    while (std::getline(exp_stream, exp_line)) {
        if (!std::getline(act_stream, act_line)) return false;
        if (detail::trim_right(exp_line) != detail::trim_right(act_line))
            return false;
    }
    // Check no extra lines in actual
    return !std::getline(act_stream, act_line);
}

// NATIONAL: UTF-16/DBCS comparison (compare bytes after trim)
template<>
inline bool compare_field<DataType::NATIONAL>(
        std::string_view expected, std::string_view actual, double /*tolerance*/)
{
    return detail::trim_right(expected) == detail::trim_right(actual);
}

// ---------------------------------------------------------------------------
// Runtime dispatch variant
// ---------------------------------------------------------------------------
inline bool compare_field_dynamic(DataType dt,
                                  std::string_view expected,
                                  std::string_view actual,
                                  double tolerance = 0.0)
{
    switch (dt) {
        case DataType::DISPLAY:  return compare_field<DataType::DISPLAY>(expected, actual, tolerance);
        case DataType::COMP:     return compare_field<DataType::COMP>(expected, actual, tolerance);
        case DataType::COMP3:    return compare_field<DataType::COMP3>(expected, actual, tolerance);
        case DataType::COMP5:    return compare_field<DataType::COMP5>(expected, actual, tolerance);
        case DataType::FLOAT:    return compare_field<DataType::FLOAT>(expected, actual, tolerance);
        case DataType::ALPHA:    return compare_field<DataType::ALPHA>(expected, actual, tolerance);
        case DataType::GROUP:    return compare_field<DataType::GROUP>(expected, actual, tolerance);
        case DataType::NATIONAL: return compare_field<DataType::NATIONAL>(expected, actual, tolerance);
    }
    return false;
}

// ---------------------------------------------------------------------------
// FieldDiff -- result of comparing one field
// ---------------------------------------------------------------------------
struct FieldDiff {
    std::string field_name;
    DataType    data_type    = DataType::DISPLAY;
    std::string expected;
    std::string actual;
    bool        matched      = false;
    size_t      line_number  = 0;
    std::string detail;      // human-readable explanation if mismatch
};

// ---------------------------------------------------------------------------
// CopybookField -- describes one field in a copybook layout
// ---------------------------------------------------------------------------
struct CopybookField {
    std::string name;
    DataType    type       = DataType::DISPLAY;
    size_t      offset     = 0;  // byte offset in record
    size_t      length     = 0;  // byte length
    double      tolerance  = 0.0;
};

// ---------------------------------------------------------------------------
// Line-by-line output comparison
// ---------------------------------------------------------------------------
struct LineComparisonResult {
    size_t total_lines     = 0;
    size_t matched_lines   = 0;
    size_t mismatched_lines = 0;
    size_t extra_expected  = 0;  // lines only in expected
    size_t extra_actual    = 0;  // lines only in actual
    std::vector<FieldDiff> diffs;

    bool all_match() const noexcept {
        return mismatched_lines == 0 && extra_expected == 0 && extra_actual == 0;
    }
    double match_rate() const noexcept {
        return total_lines > 0
            ? static_cast<double>(matched_lines) / static_cast<double>(total_lines) * 100.0
            : 100.0;
    }
};

inline LineComparisonResult compare_lines(
        std::string_view expected,
        std::string_view actual,
        const NormalizationOptions& opts = {})
{
    LineComparisonResult result;
    std::string exp_str{expected};
    std::string act_str{actual};
    std::istringstream exp_stream{exp_str};
    std::istringstream act_stream{act_str};
    std::string exp_line, act_line;
    size_t line_no = 0;

    while (true) {
        bool has_exp = static_cast<bool>(std::getline(exp_stream, exp_line));
        bool has_act = static_cast<bool>(std::getline(act_stream, act_line));

        if (!has_exp && !has_act) break;
        ++line_no;
        ++result.total_lines;

        if (!has_exp) {
            ++result.extra_actual;
            FieldDiff d;
            d.field_name = "line_" + std::to_string(line_no);
            d.line_number = line_no;
            d.expected = "";
            d.actual = act_line;
            d.matched = false;
            d.detail = "extra line in actual output";
            result.diffs.push_back(std::move(d));
            continue;
        }
        if (!has_act) {
            ++result.extra_expected;
            FieldDiff d;
            d.field_name = "line_" + std::to_string(line_no);
            d.line_number = line_no;
            d.expected = exp_line;
            d.actual = "";
            d.matched = false;
            d.detail = "missing line in actual output";
            result.diffs.push_back(std::move(d));
            continue;
        }

        auto norm_exp = detail::apply_normalization(exp_line, opts);
        auto norm_act = detail::apply_normalization(act_line, opts);

        if (norm_exp == norm_act) {
            ++result.matched_lines;
        } else {
            ++result.mismatched_lines;
            FieldDiff d;
            d.field_name = "line_" + std::to_string(line_no);
            d.line_number = line_no;
            d.expected = exp_line;
            d.actual = act_line;
            d.matched = false;
            d.detail = "content mismatch";
            result.diffs.push_back(std::move(d));
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// Field-by-field comparison with copybook layout
// ---------------------------------------------------------------------------
inline std::vector<FieldDiff> compare_record(
        std::string_view expected_record,
        std::string_view actual_record,
        const std::vector<CopybookField>& layout)
{
    std::vector<FieldDiff> diffs;
    diffs.reserve(layout.size());

    for (auto& field : layout) {
        FieldDiff d;
        d.field_name = field.name;
        d.data_type = field.type;

        // Extract substrings (handle short records gracefully)
        std::string_view exp_val = (field.offset < expected_record.size())
            ? expected_record.substr(field.offset,
                std::min(field.length, expected_record.size() - field.offset))
            : std::string_view{};

        std::string_view act_val = (field.offset < actual_record.size())
            ? actual_record.substr(field.offset,
                std::min(field.length, actual_record.size() - field.offset))
            : std::string_view{};

        d.expected = std::string(exp_val);
        d.actual   = std::string(act_val);
        d.matched  = compare_field_dynamic(field.type, exp_val, act_val, field.tolerance);

        if (!d.matched) {
            d.detail = "field mismatch: expected [" + d.expected +
                       "] actual [" + d.actual + "]";
        }

        diffs.push_back(std::move(d));
    }

    return diffs;
}

// ---------------------------------------------------------------------------
// Numeric formatting normalization
// ---------------------------------------------------------------------------
inline std::string normalize_numeric(std::string_view sv) {
    NormalizationOptions opts;
    opts.trim_trailing = true;
    opts.trim_leading = true;
    opts.strip_leading_zeros = true;
    opts.normalize_sign = true;
    opts.normalize_decimal = true;
    return detail::apply_normalization(sv, opts);
}

}} // namespace lazarus::parity

#endif // LAZARUS_PARITY_COMPARATOR_H
