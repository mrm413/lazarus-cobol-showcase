#ifndef LAZARUS_PARITY_RUNNER_H
#define LAZARUS_PARITY_RUNNER_H

#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <future>
#include <mutex>
#include <cstdio>
#include <cstring>
#include <array>
#include <algorithm>
#include <sstream>
#include <thread>

namespace lazarus { namespace parity {

// ---------------------------------------------------------------------------
// SubprocessResult -- captured output of a single process execution
// ---------------------------------------------------------------------------
struct SubprocessResult {
    int         exit_code      = -1;
    std::string stdout_capture;
    std::string stderr_capture;
    double      elapsed_ms     = 0.0;
    bool        timed_out      = false;

    bool ok() const noexcept { return exit_code == 0 && !timed_out; }
};

// ---------------------------------------------------------------------------
// run_subprocess -- execute a command, capture stdout, enforce timeout
// ---------------------------------------------------------------------------
inline SubprocessResult run_subprocess(
        const std::string& command,
        std::chrono::milliseconds timeout = std::chrono::milliseconds(30000))
{
    SubprocessResult result;
    auto t0 = std::chrono::steady_clock::now();

    // Redirect stderr to stdout so we capture both via popen
    std::string cmd = command + " 2>&1";

#ifdef _WIN32
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.c_str(), "r");
#endif

    if (!pipe) {
        result.exit_code = -1;
        result.stderr_capture = "failed to open pipe";
        return result;
    }

    // Read in a separate thread so we can enforce timeout
    std::string captured;
    std::atomic<bool> done{false};

    auto reader = std::async(std::launch::async, [&]() {
        std::array<char, 4096> buf{};
        while (!done.load(std::memory_order_relaxed)) {
            size_t n = std::fread(buf.data(), 1, buf.size(), pipe);
            if (n == 0) break;
            captured.append(buf.data(), n);
        }
    });

    // Wait for reader up to timeout
    auto status = reader.wait_for(timeout);
    auto t1 = std::chrono::steady_clock::now();
    result.elapsed_ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();

    if (status == std::future_status::timeout) {
        result.timed_out = true;
        done.store(true, std::memory_order_relaxed);
        // Still close the pipe to reap the process
    }

    reader.wait(); // ensure thread finishes before closing pipe

#ifdef _WIN32
    int rc = _pclose(pipe);
#else
    int rc = pclose(pipe);
#endif

    result.exit_code = rc;
    result.stdout_capture = captured;

    return result;
}

// ---------------------------------------------------------------------------
// Comparison status for a run pair
// ---------------------------------------------------------------------------
enum class ComparisonStatus { MATCH, MISMATCH, COBOL_FAIL, CPP_FAIL, BOTH_FAIL, TIMEOUT };

inline std::string comparison_status_str(ComparisonStatus s) {
    switch (s) {
        case ComparisonStatus::MATCH:      return "MATCH";
        case ComparisonStatus::MISMATCH:   return "MISMATCH";
        case ComparisonStatus::COBOL_FAIL: return "COBOL_FAIL";
        case ComparisonStatus::CPP_FAIL:   return "CPP_FAIL";
        case ComparisonStatus::BOTH_FAIL:  return "BOTH_FAIL";
        case ComparisonStatus::TIMEOUT:    return "TIMEOUT";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// RunResult -- outcome of running one COBOL + C++ pair
// ---------------------------------------------------------------------------
struct RunResult {
    std::string       program_name;
    SubprocessResult  cobol_result;
    SubprocessResult  cpp_result;
    ComparisonStatus  comparison_status = ComparisonStatus::MISMATCH;
};

// ---------------------------------------------------------------------------
// ProgressCallback -- invoked as each pair completes
// ---------------------------------------------------------------------------
using ProgressCallback =
    std::function<void(size_t current, size_t total,
                       const std::string& program_name,
                       ComparisonStatus status)>;

// ---------------------------------------------------------------------------
// ProgramPair -- describes one COBOL + C++ executable pair
// ---------------------------------------------------------------------------
struct ProgramPair {
    std::string name;
    std::string cobol_command;
    std::string cpp_command;
};

// ---------------------------------------------------------------------------
// ParallelRunner -- execute pairs side-by-side using std::async
// ---------------------------------------------------------------------------
class ParallelRunner {
public:
    explicit ParallelRunner(
            std::chrono::milliseconds timeout = std::chrono::milliseconds(30000))
        : timeout_(timeout) {}

    void set_timeout(std::chrono::milliseconds ms) { timeout_ = ms; }
    std::chrono::milliseconds timeout() const noexcept { return timeout_; }

    // Run a single pair
    RunResult run_pair(const ProgramPair& pair) const {
        RunResult rr;
        rr.program_name = pair.name;

        // Launch both in parallel
        auto cobol_future = std::async(std::launch::async, [&]() {
            return run_subprocess(pair.cobol_command, timeout_);
        });
        auto cpp_future = std::async(std::launch::async, [&]() {
            return run_subprocess(pair.cpp_command, timeout_);
        });

        rr.cobol_result = cobol_future.get();
        rr.cpp_result   = cpp_future.get();

        // Determine comparison status
        if (rr.cobol_result.timed_out || rr.cpp_result.timed_out) {
            rr.comparison_status = ComparisonStatus::TIMEOUT;
        } else if (!rr.cobol_result.ok() && !rr.cpp_result.ok()) {
            rr.comparison_status = ComparisonStatus::BOTH_FAIL;
        } else if (!rr.cobol_result.ok()) {
            rr.comparison_status = ComparisonStatus::COBOL_FAIL;
        } else if (!rr.cpp_result.ok()) {
            rr.comparison_status = ComparisonStatus::CPP_FAIL;
        } else if (rr.cobol_result.stdout_capture == rr.cpp_result.stdout_capture) {
            rr.comparison_status = ComparisonStatus::MATCH;
        } else {
            rr.comparison_status = ComparisonStatus::MISMATCH;
        }

        return rr;
    }

    // Batch execution with optional progress callback
    std::vector<RunResult> run_batch(
            const std::vector<ProgramPair>& pairs,
            ProgressCallback progress = nullptr) const
    {
        std::vector<RunResult> results;
        results.reserve(pairs.size());
        std::mutex mtx;
        size_t completed = 0;

        // Execute pairs with std::async
        std::vector<std::future<RunResult>> futures;
        futures.reserve(pairs.size());

        for (auto& pair : pairs) {
            futures.push_back(std::async(std::launch::async, [&, this]() {
                return run_pair(pair);
            }));
        }

        for (size_t i = 0; i < futures.size(); ++i) {
            auto rr = futures[i].get();
            {
                std::lock_guard<std::mutex> lock(mtx);
                ++completed;
                results.push_back(rr);
            }
            if (progress) {
                progress(completed, pairs.size(),
                         rr.program_name, rr.comparison_status);
            }
        }

        return results;
    }

    // Build pairs from directory listing (simple convention-based)
    static std::vector<ProgramPair> scan_directory(
            const std::string& cobol_dir,
            const std::string& cpp_dir,
            const std::vector<std::string>& program_names)
    {
        std::vector<ProgramPair> pairs;
        pairs.reserve(program_names.size());
        for (auto& name : program_names) {
            ProgramPair pp;
            pp.name = name;
            pp.cobol_command = cobol_dir + "/" + name;
            pp.cpp_command   = cpp_dir + "/" + name;
            pairs.push_back(std::move(pp));
        }
        return pairs;
    }

    // Summary statistics
    struct BatchSummary {
        size_t total     = 0;
        size_t matched   = 0;
        size_t mismatch  = 0;
        size_t failures  = 0;
        size_t timeouts  = 0;
        double pass_rate = 0.0;
    };

    static BatchSummary summarize(const std::vector<RunResult>& results) {
        BatchSummary s;
        s.total = results.size();
        for (auto& r : results) {
            switch (r.comparison_status) {
                case ComparisonStatus::MATCH:      ++s.matched;  break;
                case ComparisonStatus::MISMATCH:   ++s.mismatch; break;
                case ComparisonStatus::TIMEOUT:    ++s.timeouts; break;
                default:                           ++s.failures; break;
            }
        }
        s.pass_rate = s.total > 0
            ? static_cast<double>(s.matched) / static_cast<double>(s.total) * 100.0
            : 0.0;
        return s;
    }

private:
    std::chrono::milliseconds timeout_;
};

}} // namespace lazarus::parity

#endif // LAZARUS_PARITY_RUNNER_H
