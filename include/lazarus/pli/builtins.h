#ifndef LAZARUS_PLI_BUILTINS_H
#define LAZARUS_PLI_BUILTINS_H

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <complex>
#include <climits>

namespace lazarus { namespace pli { namespace builtins {

// ---------- String functions ----------

inline std::string SUBSTR(const std::string& s, int pos, int len = -1) {
    if (pos < 1) pos = 1;
    size_t start = static_cast<size_t>(pos - 1); // PL/I is 1-based
    if (start >= s.size()) return "";
    if (len < 0) return s.substr(start);
    return s.substr(start, static_cast<size_t>(len));
}

inline int INDEX(const std::string& haystack, const std::string& needle) {
    auto pos = haystack.find(needle);
    if (pos == std::string::npos) return 0;
    return static_cast<int>(pos) + 1; // 1-based
}

inline int LENGTH(const std::string& s) {
    return static_cast<int>(s.size());
}

inline std::string TRANSLATE(const std::string& s, const std::string& to, const std::string& from) {
    std::string result = s;
    for (auto& c : result) {
        auto pos = from.find(c);
        if (pos != std::string::npos && pos < to.size()) {
            c = to[pos];
        }
    }
    return result;
}

inline int VERIFY(const std::string& s, const std::string& valid) {
    for (size_t i = 0; i < s.size(); i++) {
        if (valid.find(s[i]) == std::string::npos) {
            return static_cast<int>(i) + 1;
        }
    }
    return 0;
}

inline std::string REPEAT(const std::string& s, int n) {
    std::string result;
    result.reserve(s.size() * static_cast<size_t>(n + 1));
    for (int i = 0; i <= n; i++) { // PL/I REPEAT gives n+1 copies
        result += s;
    }
    return result;
}

inline std::string REVERSE(const std::string& s) {
    return std::string(s.rbegin(), s.rend());
}

inline std::string LEFT(const std::string& s, int len, char pad = ' ') {
    if (len <= 0) return "";
    std::string result = s;
    if (static_cast<int>(result.size()) < len) {
        result.append(static_cast<size_t>(len) - result.size(), pad);
    } else {
        result.resize(static_cast<size_t>(len));
    }
    return result;
}

inline std::string RIGHT(const std::string& s, int len, char pad = ' ') {
    if (len <= 0) return "";
    if (static_cast<int>(s.size()) >= len) {
        return s.substr(s.size() - static_cast<size_t>(len));
    }
    return std::string(static_cast<size_t>(len) - s.size(), pad) + s;
}

inline std::string CENTER(const std::string& s, int len, char pad = ' ') {
    if (len <= 0) return "";
    if (static_cast<int>(s.size()) >= len) {
        return s.substr(0, static_cast<size_t>(len));
    }
    size_t total_pad = static_cast<size_t>(len) - s.size();
    size_t left_pad = total_pad / 2;
    size_t right_pad = total_pad - left_pad;
    return std::string(left_pad, pad) + s + std::string(right_pad, pad);
}

inline std::string TRIM(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

inline std::string COPY(const std::string& s, int n) {
    std::string result;
    result.reserve(s.size() * static_cast<size_t>(n));
    for (int i = 0; i < n; i++) result += s;
    return result;
}

inline std::string CHAR_fn(int code) {
    return std::string(1, static_cast<char>(code));
}

inline std::string BIT_fn(int val, int len = 32) {
    std::string result;
    for (int i = len - 1; i >= 0; i--) {
        result += ((val >> i) & 1) ? '1' : '0';
    }
    return result;
}

inline std::string COLLATE() {
    std::string result(256, '\0');
    for (int i = 0; i < 256; i++) result[static_cast<size_t>(i)] = static_cast<char>(i);
    return result;
}

inline std::string HIGH(int n) {
    return std::string(static_cast<size_t>(n), '\xFF');
}

inline std::string LOW(int n) {
    return std::string(static_cast<size_t>(n), '\x00');
}

inline std::string MPSTR(const std::string& s, const std::string& from_type, const std::string& /*to_type*/) {
    // Simplified: in real PL/I this converts between character sets
    (void)from_type;
    return s;
}

// ---------- Math functions ----------

inline double ABS_fn(double x) { return std::abs(x); }
inline int64_t MOD(int64_t a, int64_t b) { return (b != 0) ? (a % b) : 0; }
inline double CEIL(double x) { return std::ceil(x); }
inline double FLOOR(double x) { return std::floor(x); }

inline double ROUND(double x, int places = 0) {
    double factor = std::pow(10.0, places);
    return std::round(x * factor) / factor;
}

inline double TRUNC(double x) { return (x >= 0) ? std::floor(x) : std::ceil(x); }

inline int SIGN(double x) {
    if (x > 0) return 1;
    if (x < 0) return -1;
    return 0;
}

template<typename T, typename... Ts>
inline T MIN_fn(T first, Ts... rest) {
    if constexpr (sizeof...(rest) == 0) {
        return first;
    } else {
        T rest_min = MIN_fn(rest...);
        return (first < rest_min) ? first : rest_min;
    }
}

template<typename T, typename... Ts>
inline T MAX_fn(T first, Ts... rest) {
    if constexpr (sizeof...(rest) == 0) {
        return first;
    } else {
        T rest_max = MAX_fn(rest...);
        return (first > rest_max) ? first : rest_max;
    }
}

inline double SQRT_fn(double x) { return std::sqrt(x); }
inline double LOG_fn(double x) { return std::log(x); }
inline double LOG2(double x) { return std::log2(x); }
inline double LOG10(double x) { return std::log10(x); }
inline double EXP_fn(double x) { return std::exp(x); }
inline double SIN_fn(double x) { return std::sin(x); }
inline double COS_fn(double x) { return std::cos(x); }
inline double TAN_fn(double x) { return std::tan(x); }
inline double ATAN_fn(double x) { return std::atan(x); }
inline double ATAND(double x) { return std::atan(x) * 180.0 / 3.14159265358979323846; }
inline double ASIN_fn(double x) { return std::asin(x); }
inline double ACOS_fn(double x) { return std::acos(x); }

// ---------- Conversion functions ----------

inline int64_t BINARY(const std::string& bits) {
    int64_t result = 0;
    for (char c : bits) {
        result = (result << 1) | (c == '1' ? 1 : 0);
    }
    return result;
}

inline int64_t DECIMAL(int64_t val) { return val; }

inline int64_t FIXED_fn(double x) { return static_cast<int64_t>(x); }
inline double FLOAT_fn(int64_t x) { return static_cast<double>(x); }

inline std::string CHAR_conv(int64_t x) { return std::to_string(x); }
inline std::string CHAR_conv_f(double x) {
    std::ostringstream oss;
    oss << x;
    return oss.str();
}

inline std::string BIT_conv(int64_t x, int len = 32) {
    return BIT_fn(static_cast<int>(x), len);
}

inline std::complex<double> COMPLEX_fn(double real, double imag) {
    return {real, imag};
}

inline double REAL(std::complex<double> c) { return c.real(); }
inline double IMAG(std::complex<double> c) { return c.imag(); }

inline bool BOOL_fn(int64_t x) { return x != 0; }
inline bool BOOL_str(const std::string& s) { return !s.empty() && s != "0"; }

// UNSPEC: reinterpret as bit string
inline std::string UNSPEC(int64_t x) {
    return BIT_fn(static_cast<int>(x), 64);
}

// ---------- Array functions ----------

inline double SUM(const std::vector<double>& arr) {
    return std::accumulate(arr.begin(), arr.end(), 0.0);
}

inline double PROD(const std::vector<double>& arr) {
    return std::accumulate(arr.begin(), arr.end(), 1.0, std::multiplies<double>());
}

inline double POLY(const std::vector<double>& coeffs, double x) {
    // Horner's method: coeffs[0] + coeffs[1]*x + coeffs[2]*x^2 + ...
    double result = 0.0;
    double xn = 1.0;
    for (size_t i = 0; i < coeffs.size(); i++) {
        result += coeffs[i] * xn;
        xn *= x;
    }
    return result;
}

inline int LBOUND(int /*dim*/ = 1) { return 1; } // Default lower bound in PL/I

inline int HBOUND(const std::vector<double>& arr, int /*dim*/ = 1) {
    return static_cast<int>(arr.size());
}

inline int DIM_fn(const std::vector<double>& arr, int /*dim*/ = 1) {
    return static_cast<int>(arr.size());
}

inline int ALLOCATION(bool is_allocated) {
    return is_allocated ? 1 : 0;
}

// ---------- Storage functions ----------

inline uintptr_t ADDR(const void* p) { return reinterpret_cast<uintptr_t>(p); }

inline void* NULL_ptr() { return nullptr; }
inline void* SYSNULL() { return nullptr; }

inline size_t SIZE(size_t bytes) { return bytes; }
inline size_t CURRENTSIZE(size_t bytes) { return bytes; }
inline size_t STORAGE(size_t bytes) { return bytes; }

inline uintptr_t BINARYVALUE(const void* p) { return reinterpret_cast<uintptr_t>(p); }
inline void* POINTERVALUE(uintptr_t v) { return reinterpret_cast<void*>(v); }

// ---------- Date/Time functions ----------

inline std::string DATE() {
    time_t now = time(nullptr);
    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &now);
#else
    localtime_r(&now, &tm_buf);
#endif
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d%02d%02d",
                  (tm_buf.tm_year + 1900) % 100, tm_buf.tm_mon + 1, tm_buf.tm_mday);
    return buf;
}

inline std::string TIME() {
    time_t now = time(nullptr);
    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &now);
#else
    localtime_r(&now, &tm_buf);
#endif
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d%02d%02d000",
                  tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    return buf;
}

inline std::string DATETIME() {
    time_t now = time(nullptr);
    struct tm tm_buf;
#ifdef _WIN32
    localtime_s(&tm_buf, &now);
#else
    localtime_r(&now, &tm_buf);
#endif
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d%02d%02d%02d000",
                  tm_buf.tm_year + 1900, tm_buf.tm_mon + 1, tm_buf.tm_mday,
                  tm_buf.tm_hour, tm_buf.tm_min, tm_buf.tm_sec);
    return buf;
}

// DAYS: convert date to Lilian day number (days since Jan 1, 1600)
inline int64_t DAYS(const std::string& date_str) {
    // Expects YYYYMMDD format
    if (date_str.size() < 8) return 0;
    int y = std::stoi(date_str.substr(0, 4));
    int m = std::stoi(date_str.substr(4, 2));
    int d = std::stoi(date_str.substr(6, 2));

    // Lilian day calculation (days since Oct 15, 1582)
    // Simplified: use Julian day number offset
    if (m <= 2) { y--; m += 12; }
    int A = y / 100;
    int B = 2 - A + A / 4;
    int64_t jd = static_cast<int64_t>(365.25 * (y + 4716)) +
                 static_cast<int64_t>(30.6001 * (m + 1)) + d + B - 1524;
    // Lilian day = JD - 2299160 (offset to Oct 15, 1582)
    return jd - 2299160;
}

inline int64_t SECS(const std::string& time_str) {
    // Expects HHMMSS format
    if (time_str.size() < 6) return 0;
    int h = std::stoi(time_str.substr(0, 2));
    int m = std::stoi(time_str.substr(2, 2));
    int s = std::stoi(time_str.substr(4, 2));
    return static_cast<int64_t>(h) * 3600 + static_cast<int64_t>(m) * 60 + s;
}

inline std::string DAYSTODATE(int64_t lilian) {
    // Convert Lilian day back to YYYYMMDD
    int64_t jd = lilian + 2299160;
    int64_t l = jd + 68569;
    int64_t n = 4 * l / 146097;
    l = l - (146097 * n + 3) / 4;
    int64_t i = 4000 * (l + 1) / 1461001;
    l = l - 1461 * i / 4 + 31;
    int64_t j = 80 * l / 2447;
    int d = static_cast<int>(l - 2447 * j / 80);
    l = j / 11;
    int m = static_cast<int>(j + 2 - 12 * l);
    int y = static_cast<int>(100 * (n - 49) + i + l);

    char buf[16];
    std::snprintf(buf, sizeof(buf), "%04d%02d%02d", y, m, d);
    return buf;
}

inline std::string SECSTOTIME(int64_t secs) {
    int h = static_cast<int>(secs / 3600);
    int m = static_cast<int>((secs % 3600) / 60);
    int s = static_cast<int>(secs % 60);
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02d%02d%02d", h, m, s);
    return buf;
}

// ---------- Condition query functions ----------

struct ConditionInfo {
    int oncode = 0;
    std::string onchar;
    std::string onsource;
    std::string onloc;
    std::string onfile;
    std::string onkey;
    std::string datafield;
};

// Thread-local condition info for ON conditions
inline ConditionInfo& current_condition_info() {
    static thread_local ConditionInfo info;
    return info;
}

inline int ONCODE() { return current_condition_info().oncode; }
inline std::string ONCHAR() { return current_condition_info().onchar; }
inline std::string ONSOURCE() { return current_condition_info().onsource; }
inline std::string ONLOC() { return current_condition_info().onloc; }
inline std::string ONFILE() { return current_condition_info().onfile; }
inline std::string ONKEY() { return current_condition_info().onkey; }
inline std::string DATAFIELD() { return current_condition_info().datafield; }

// ---------- Builtin registry for name lookups ----------

enum class BuiltinKind {
    SUBSTR, INDEX, LENGTH, TRANSLATE, VERIFY, REPEAT, REVERSE,
    LEFT, RIGHT, CENTER, TRIM, COPY, CHAR, BIT, COLLATE, MPSTR,
    HIGH, LOW, UNSPEC,
    ABS, MOD, CEIL, FLOOR, ROUND, TRUNC, SIGN, MIN, MAX,
    SQRT, LOG, LOG2, LOG10, EXP, SIN, COS, TAN, ATAN, ATAND, ASIN, ACOS,
    BINARY, DECIMAL, FIXED, FLOAT, CHAR_CONV, BIT_CONV, COMPLEX, REAL, IMAG, BOOL,
    SUM, PROD, POLY, LBOUND, HBOUND, DIM, ALLOCATION,
    ADDR, NULL_B, BINARYVALUE, POINTERVALUE, SYSNULL, SIZE, CURRENTSIZE, STORAGE,
    DATE, TIME, DATETIME, DAYS, SECS, DAYSTODATE, SECSTOTIME,
    ONCODE, ONCHAR, ONSOURCE, ONLOC, ONFILE, ONKEY, DATAFIELD,
    UNKNOWN
};

struct BuiltinEntry {
    const char* name;
    BuiltinKind kind;
};

constexpr BuiltinEntry kBuiltins[] = {
    {"SUBSTR", BuiltinKind::SUBSTR}, {"INDEX", BuiltinKind::INDEX},
    {"LENGTH", BuiltinKind::LENGTH}, {"TRANSLATE", BuiltinKind::TRANSLATE},
    {"VERIFY", BuiltinKind::VERIFY}, {"REPEAT", BuiltinKind::REPEAT},
    {"REVERSE", BuiltinKind::REVERSE}, {"LEFT", BuiltinKind::LEFT},
    {"RIGHT", BuiltinKind::RIGHT}, {"CENTER", BuiltinKind::CENTER},
    {"CENTRE", BuiltinKind::CENTER}, {"TRIM", BuiltinKind::TRIM},
    {"COPY", BuiltinKind::COPY}, {"CHAR", BuiltinKind::CHAR},
    {"BIT", BuiltinKind::BIT}, {"COLLATE", BuiltinKind::COLLATE},
    {"MPSTR", BuiltinKind::MPSTR}, {"HIGH", BuiltinKind::HIGH},
    {"LOW", BuiltinKind::LOW}, {"UNSPEC", BuiltinKind::UNSPEC},
    {"ABS", BuiltinKind::ABS}, {"MOD", BuiltinKind::MOD},
    {"CEIL", BuiltinKind::CEIL}, {"FLOOR", BuiltinKind::FLOOR},
    {"ROUND", BuiltinKind::ROUND}, {"TRUNC", BuiltinKind::TRUNC},
    {"SIGN", BuiltinKind::SIGN}, {"MIN", BuiltinKind::MIN},
    {"MAX", BuiltinKind::MAX}, {"SQRT", BuiltinKind::SQRT},
    {"LOG", BuiltinKind::LOG}, {"LOG2", BuiltinKind::LOG2},
    {"LOG10", BuiltinKind::LOG10}, {"EXP", BuiltinKind::EXP},
    {"SIN", BuiltinKind::SIN}, {"COS", BuiltinKind::COS},
    {"TAN", BuiltinKind::TAN}, {"ATAN", BuiltinKind::ATAN},
    {"ATAND", BuiltinKind::ATAND}, {"ASIN", BuiltinKind::ASIN},
    {"ACOS", BuiltinKind::ACOS},
    {"BINARY", BuiltinKind::BINARY}, {"DECIMAL", BuiltinKind::DECIMAL},
    {"FIXED", BuiltinKind::FIXED}, {"FLOAT", BuiltinKind::FLOAT},
    {"COMPLEX", BuiltinKind::COMPLEX}, {"REAL", BuiltinKind::REAL},
    {"IMAG", BuiltinKind::IMAG}, {"BOOL", BuiltinKind::BOOL},
    {"SUM", BuiltinKind::SUM}, {"PROD", BuiltinKind::PROD},
    {"POLY", BuiltinKind::POLY}, {"LBOUND", BuiltinKind::LBOUND},
    {"HBOUND", BuiltinKind::HBOUND}, {"DIM", BuiltinKind::DIM},
    {"ALLOCATION", BuiltinKind::ALLOCATION},
    {"ADDR", BuiltinKind::ADDR}, {"NULL", BuiltinKind::NULL_B},
    {"BINARYVALUE", BuiltinKind::BINARYVALUE},
    {"POINTERVALUE", BuiltinKind::POINTERVALUE},
    {"SYSNULL", BuiltinKind::SYSNULL},
    {"SIZE", BuiltinKind::SIZE}, {"CURRENTSIZE", BuiltinKind::CURRENTSIZE},
    {"STORAGE", BuiltinKind::STORAGE},
    {"DATE", BuiltinKind::DATE}, {"TIME", BuiltinKind::TIME},
    {"DATETIME", BuiltinKind::DATETIME}, {"DAYS", BuiltinKind::DAYS},
    {"SECS", BuiltinKind::SECS}, {"DAYSTODATE", BuiltinKind::DAYSTODATE},
    {"SECSTOTIME", BuiltinKind::SECSTOTIME},
    {"ONCODE", BuiltinKind::ONCODE}, {"ONCHAR", BuiltinKind::ONCHAR},
    {"ONSOURCE", BuiltinKind::ONSOURCE}, {"ONLOC", BuiltinKind::ONLOC},
    {"ONFILE", BuiltinKind::ONFILE}, {"ONKEY", BuiltinKind::ONKEY},
    {"DATAFIELD", BuiltinKind::DATAFIELD},
};

inline BuiltinKind lookup_builtin(const std::string& name) {
    std::string upper = name;
    for (auto& ch : upper) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    for (const auto& entry : kBuiltins) {
        if (upper == entry.name) return entry.kind;
    }
    return BuiltinKind::UNKNOWN;
}

inline bool is_builtin(const std::string& name) {
    return lookup_builtin(name) != BuiltinKind::UNKNOWN;
}

}}} // namespace lazarus::pli::builtins

#endif // LAZARUS_PLI_BUILTINS_H
