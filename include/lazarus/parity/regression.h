#ifndef LAZARUS_PARITY_REGRESSION_H
#define LAZARUS_PARITY_REGRESSION_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <ctime>

namespace lazarus { namespace parity {

// ---------------------------------------------------------------------------
// Baseline status for a program
// ---------------------------------------------------------------------------
enum class BaselineStatus { PASS, FAIL, SKIP };

inline std::string baseline_status_str(BaselineStatus s) {
    switch (s) {
        case BaselineStatus::PASS: return "PASS";
        case BaselineStatus::FAIL: return "FAIL";
        case BaselineStatus::SKIP: return "SKIP";
    }
    return "UNKNOWN";
}

inline BaselineStatus parse_baseline_status(const std::string& s) {
    if (s == "PASS") return BaselineStatus::PASS;
    if (s == "FAIL") return BaselineStatus::FAIL;
    return BaselineStatus::SKIP;
}

// ---------------------------------------------------------------------------
// RegressionBaseline -- stored set of known results per program
// ---------------------------------------------------------------------------
class RegressionBaseline {
public:
    RegressionBaseline() = default;

    // Set status for a program
    void set(const std::string& program, BaselineStatus status) {
        entries_[program] = status;
    }

    // Get status (SKIP if not found)
    BaselineStatus get(const std::string& program) const {
        auto it = entries_.find(program);
        return it != entries_.end() ? it->second : BaselineStatus::SKIP;
    }

    // Check if program exists in baseline
    bool contains(const std::string& program) const {
        return entries_.find(program) != entries_.end();
    }

    // Number of entries
    size_t size() const noexcept { return entries_.size(); }
    bool empty() const noexcept { return entries_.empty(); }

    // All entries
    const std::map<std::string, BaselineStatus>& entries() const noexcept {
        return entries_;
    }

    // Count by status
    size_t count_pass() const {
        size_t n = 0;
        for (auto& [k, v] : entries_) { (void)k; if (v == BaselineStatus::PASS) ++n; }
        return n;
    }
    size_t count_fail() const {
        size_t n = 0;
        for (auto& [k, v] : entries_) { (void)k; if (v == BaselineStatus::FAIL) ++n; }
        return n;
    }

    // ------------------------------------------------------------------
    // Save to file: simple text format "program_name STATUS"
    // ------------------------------------------------------------------
    bool save(const std::string& path) const {
        std::ofstream ofs(path);
        if (!ofs.is_open()) return false;
        ofs << "# Parity Regression Baseline\n";
        ofs << "# Format: program_name STATUS\n";
        for (auto& [prog, status] : entries_) {
            ofs << prog << " " << baseline_status_str(status) << "\n";
        }
        return ofs.good();
    }

    // Save to string (for testing without filesystem)
    std::string save_to_string() const {
        std::ostringstream os;
        os << "# Parity Regression Baseline\n";
        os << "# Format: program_name STATUS\n";
        for (auto& [prog, status] : entries_) {
            os << prog << " " << baseline_status_str(status) << "\n";
        }
        return os.str();
    }

    // ------------------------------------------------------------------
    // Load from file
    // ------------------------------------------------------------------
    static RegressionBaseline load(const std::string& path) {
        RegressionBaseline baseline;
        std::ifstream ifs(path);
        if (!ifs.is_open()) return baseline;
        return load_from_string(std::string(
            std::istreambuf_iterator<char>(ifs),
            std::istreambuf_iterator<char>()));
    }

    // Load from string (for testing)
    static RegressionBaseline load_from_string(const std::string& content) {
        RegressionBaseline baseline;
        std::istringstream is(content);
        std::string line;
        while (std::getline(is, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') continue;
            // Parse "program_name STATUS"
            std::istringstream ls(line);
            std::string prog, status_str;
            if (ls >> prog >> status_str) {
                baseline.set(prog, parse_baseline_status(status_str));
            }
        }
        return baseline;
    }

private:
    std::map<std::string, BaselineStatus> entries_;
};

// ---------------------------------------------------------------------------
// RegressionResult -- outcome of comparing current run against baseline
// ---------------------------------------------------------------------------
struct RegressionResult {
    std::vector<std::string> new_failures;    // PASS->FAIL (regression)
    std::vector<std::string> fixed;           // FAIL->PASS (improvement)
    std::vector<std::string> stable_failures; // FAIL->FAIL (known)
    std::vector<std::string> stable_passes;   // PASS->PASS (stable)
    std::vector<std::string> new_programs;    // not in baseline

    bool has_regressions() const noexcept { return !new_failures.empty(); }
    size_t total() const noexcept {
        return new_failures.size() + fixed.size() +
               stable_failures.size() + stable_passes.size() +
               new_programs.size();
    }
};

// ---------------------------------------------------------------------------
// Compare current run results against a baseline
// ---------------------------------------------------------------------------
inline RegressionResult compare_against_baseline(
        const RegressionBaseline& baseline,
        const std::map<std::string, BaselineStatus>& current)
{
    RegressionResult result;

    for (auto& [prog, cur_status] : current) {
        if (!baseline.contains(prog)) {
            result.new_programs.push_back(prog);
            continue;
        }
        auto prev = baseline.get(prog);
        if (prev == BaselineStatus::PASS && cur_status == BaselineStatus::FAIL) {
            result.new_failures.push_back(prog);
        } else if (prev == BaselineStatus::FAIL && cur_status == BaselineStatus::PASS) {
            result.fixed.push_back(prog);
        } else if (prev == BaselineStatus::FAIL && cur_status == BaselineStatus::FAIL) {
            result.stable_failures.push_back(prog);
        } else if (prev == BaselineStatus::PASS && cur_status == BaselineStatus::PASS) {
            result.stable_passes.push_back(prog);
        }
    }

    return result;
}

// ---------------------------------------------------------------------------
// RegressionReport -- human-readable report
// ---------------------------------------------------------------------------
inline std::string generate_regression_report(const RegressionResult& rr) {
    std::ostringstream os;

    os << "=== Regression Report ===\n";
    os << "Total programs: " << rr.total() << "\n\n";

    if (!rr.new_failures.empty()) {
        os << "!!! NEW FAILURES (REGRESSIONS) !!!\n";
        for (auto& p : rr.new_failures) {
            os << "  REGRESSION: " << p << " (was PASS, now FAIL)\n";
        }
        os << "\n";
    }

    if (!rr.fixed.empty()) {
        os << "+++ FIXED (IMPROVEMENTS) +++\n";
        for (auto& p : rr.fixed) {
            os << "  FIXED: " << p << " (was FAIL, now PASS)\n";
        }
        os << "\n";
    }

    if (!rr.stable_failures.empty()) {
        os << "--- Known Failures (stable) ---\n";
        for (auto& p : rr.stable_failures) {
            os << "  KNOWN_FAIL: " << p << "\n";
        }
        os << "\n";
    }

    if (!rr.new_programs.empty()) {
        os << "*** New Programs ***\n";
        for (auto& p : rr.new_programs) {
            os << "  NEW: " << p << "\n";
        }
        os << "\n";
    }

    os << "Summary: "
       << rr.stable_passes.size() << " stable passes, "
       << rr.fixed.size() << " fixed, "
       << rr.new_failures.size() << " regressions, "
       << rr.stable_failures.size() << " known failures, "
       << rr.new_programs.size() << " new\n";

    return os.str();
}

// ---------------------------------------------------------------------------
// TrendPoint -- pass rate at a point in time
// ---------------------------------------------------------------------------
struct TrendPoint {
    std::string timestamp;  // ISO 8601
    size_t      total    = 0;
    size_t      passed   = 0;
    size_t      failed   = 0;
    double      pass_rate = 0.0;
};

// ---------------------------------------------------------------------------
// TrendTracker -- track pass rates over multiple runs
// ---------------------------------------------------------------------------
class TrendTracker {
public:
    TrendTracker() = default;

    void record(size_t total, size_t passed, const std::string& timestamp = "") {
        TrendPoint pt;
        pt.timestamp = timestamp.empty() ? current_timestamp() : timestamp;
        pt.total = total;
        pt.passed = passed;
        pt.failed = total >= passed ? total - passed : 0;
        pt.pass_rate = total > 0
            ? static_cast<double>(passed) / static_cast<double>(total) * 100.0
            : 0.0;
        points_.push_back(pt);
    }

    const std::vector<TrendPoint>& points() const noexcept { return points_; }
    size_t size() const noexcept { return points_.size(); }
    bool empty() const noexcept { return points_.empty(); }

    // Latest pass rate
    double latest_pass_rate() const {
        return points_.empty() ? 0.0 : points_.back().pass_rate;
    }

    // Direction: positive = improving, negative = regressing, 0 = stable/insufficient
    double trend_direction() const {
        if (points_.size() < 2) return 0.0;
        return points_.back().pass_rate - points_[points_.size() - 2].pass_rate;
    }

    // Serialize to text
    std::string save_to_string() const {
        std::ostringstream os;
        os << "# Parity Trend Data\n";
        os << "# timestamp total passed failed pass_rate\n";
        for (auto& pt : points_) {
            os << pt.timestamp << " "
               << pt.total << " "
               << pt.passed << " "
               << pt.failed << " "
               << std::fixed << std::setprecision(1) << pt.pass_rate << "\n";
        }
        return os.str();
    }

    // Deserialize from text
    static TrendTracker load_from_string(const std::string& content) {
        TrendTracker tracker;
        std::istringstream is(content);
        std::string line;
        while (std::getline(is, line)) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream ls(line);
            TrendPoint pt;
            ls >> pt.timestamp >> pt.total >> pt.passed >> pt.failed >> pt.pass_rate;
            if (!pt.timestamp.empty()) {
                tracker.points_.push_back(pt);
            }
        }
        return tracker;
    }

private:
    std::vector<TrendPoint> points_;

    static std::string current_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto t   = std::chrono::system_clock::to_time_t(now);
        std::tm tm_buf{};
#ifdef _WIN32
        localtime_s(&tm_buf, &t);
#else
        localtime_r(&t, &tm_buf);
#endif
        char buf[32];
        std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm_buf);
        return std::string(buf);
    }
};

}} // namespace lazarus::parity

#endif // LAZARUS_PARITY_REGRESSION_H
