#ifndef LAZARUS_PARITY_REPORT_H
#define LAZARUS_PARITY_REPORT_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <functional>
#include <cstddef>

#include "lazarus/parity/runner.h"
#include "lazarus/parity/comparator.h"
#include "lazarus/parity/audit.h"
#include "lazarus/parity/regression.h"

namespace lazarus { namespace parity {

// ---------------------------------------------------------------------------
// HTML template fragments
// ---------------------------------------------------------------------------
constexpr auto HTML_HEAD = R"html(<!DOCTYPE html><html><head>
<style>body{font-family:sans-serif;margin:2em}table{border-collapse:collapse;width:100%}
th,td{border:1px solid #ddd;padding:8px;text-align:left}
.pass{background:#e6ffe6}.fail{background:#ffe6e6}.warn{background:#fff3cd}
h1{color:#333}h2{color:#555;border-bottom:2px solid #007bff}</style>
</head><body>)html";

constexpr auto HTML_TAIL = R"html(</body></html>)html";

// ---------------------------------------------------------------------------
// Report filter mode
// ---------------------------------------------------------------------------
enum class ReportFilter { ALL, FAILURES_ONLY, REGRESSIONS_ONLY };

// ---------------------------------------------------------------------------
// ReportEntry -- one program's results for reporting
// ---------------------------------------------------------------------------
struct ReportEntry {
    std::string      program_name;
    ComparisonStatus status         = ComparisonStatus::MISMATCH;
    size_t           field_count    = 0;
    size_t           failures       = 0;
    double           elapsed_ms     = 0.0;
    bool             is_regression  = false;
    bool             is_fixed       = false;
    std::string      diff_detail;  // HTML-safe diff for expandable view
};

// ---------------------------------------------------------------------------
// ReportData -- collected data for a report
// ---------------------------------------------------------------------------
struct ReportData {
    std::string                title        = "Parity Test Report";
    std::string                timestamp;
    std::vector<ReportEntry>   entries;
    AuditSummary               audit_summary;
    std::optional<RegressionResult> regression;
    std::vector<TrendPoint>    trend_points;

    // Computed stats
    size_t total_programs() const { return entries.size(); }

    size_t passed_programs() const {
        size_t n = 0;
        for (auto& e : entries)
            if (e.status == ComparisonStatus::MATCH) ++n;
        return n;
    }

    double pass_rate() const {
        return entries.empty() ? 100.0
            : static_cast<double>(passed_programs()) /
              static_cast<double>(entries.size()) * 100.0;
    }
};

// ---------------------------------------------------------------------------
// HTML escaping
// ---------------------------------------------------------------------------
inline std::string html_escape(std::string_view sv) {
    std::string out;
    out.reserve(sv.size() + 16);
    for (char c : sv) {
        switch (c) {
            case '&':  out += "&amp;";  break;
            case '<':  out += "&lt;";   break;
            case '>':  out += "&gt;";   break;
            case '"':  out += "&quot;"; break;
            case '\'': out += "&#39;";  break;
            default:   out += c;        break;
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// CSS class for comparison status
// ---------------------------------------------------------------------------
inline std::string status_css_class(ComparisonStatus s) {
    switch (s) {
        case ComparisonStatus::MATCH:    return "pass";
        case ComparisonStatus::TIMEOUT:  return "warn";
        default:                         return "fail";
    }
}

// ---------------------------------------------------------------------------
// Filter entries based on mode
// ---------------------------------------------------------------------------
inline std::vector<ReportEntry> filter_entries(
        const std::vector<ReportEntry>& entries, ReportFilter filter)
{
    if (filter == ReportFilter::ALL) return entries;

    std::vector<ReportEntry> result;
    for (auto& e : entries) {
        if (filter == ReportFilter::FAILURES_ONLY &&
            e.status != ComparisonStatus::MATCH) {
            result.push_back(e);
        } else if (filter == ReportFilter::REGRESSIONS_ONLY && e.is_regression) {
            result.push_back(e);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Generate HTML report
// ---------------------------------------------------------------------------
inline std::string generate_html_report(
        const ReportData& data,
        ReportFilter filter = ReportFilter::ALL)
{
    auto filtered = filter_entries(data.entries, filter);

    std::ostringstream os;
    os << HTML_HEAD;

    // Title
    os << "<h1>" << html_escape(data.title) << "</h1>\n";
    if (!data.timestamp.empty()) {
        os << "<p>Generated: " << html_escape(data.timestamp) << "</p>\n";
    }

    // Summary dashboard
    os << "<h2>Summary</h2>\n";
    os << "<table>\n";
    os << "<tr><th>Metric</th><th>Value</th></tr>\n";
    os << "<tr><td>Total Programs</td><td>" << data.total_programs() << "</td></tr>\n";
    os << "<tr><td>Passed</td><td>" << data.passed_programs() << "</td></tr>\n";
    os << "<tr class=\""
       << (data.pass_rate() >= 100.0 ? "pass" : (data.pass_rate() >= 80.0 ? "warn" : "fail"))
       << "\"><td>Pass Rate</td><td>"
       << std::fixed << std::setprecision(1) << data.pass_rate() << "%</td></tr>\n";

    // Failure categories from audit summary
    os << "<tr><td>Whitespace Issues</td><td>" << data.audit_summary.whitespace_failures << "</td></tr>\n";
    os << "<tr><td>Precision Issues</td><td>" << data.audit_summary.precision_failures << "</td></tr>\n";
    os << "<tr><td>Data Loss Issues</td><td>" << data.audit_summary.data_loss_failures << "</td></tr>\n";
    os << "<tr><td>Structural Issues</td><td>" << data.audit_summary.structural_failures << "</td></tr>\n";
    os << "</table>\n";

    // Regression summary
    if (data.regression.has_value()) {
        auto& rr = data.regression.value();
        os << "<h2>Regression Analysis</h2>\n";
        os << "<table>\n";
        os << "<tr><th>Category</th><th>Count</th></tr>\n";
        os << "<tr class=\"fail\"><td>New Failures</td><td>"
           << rr.new_failures.size() << "</td></tr>\n";
        os << "<tr class=\"pass\"><td>Fixed</td><td>"
           << rr.fixed.size() << "</td></tr>\n";
        os << "<tr><td>Stable Failures</td><td>"
           << rr.stable_failures.size() << "</td></tr>\n";
        os << "<tr><td>Stable Passes</td><td>"
           << rr.stable_passes.size() << "</td></tr>\n";
        os << "<tr><td>New Programs</td><td>"
           << rr.new_programs.size() << "</td></tr>\n";
        os << "</table>\n";
    }

    // Trend
    if (!data.trend_points.empty()) {
        os << "<h2>Trend</h2>\n";
        os << "<table>\n";
        os << "<tr><th>Run</th><th>Total</th><th>Passed</th><th>Rate</th></tr>\n";
        for (auto& pt : data.trend_points) {
            os << "<tr><td>" << html_escape(pt.timestamp) << "</td>"
               << "<td>" << pt.total << "</td>"
               << "<td>" << pt.passed << "</td>"
               << "<td>" << std::fixed << std::setprecision(1)
               << pt.pass_rate << "%</td></tr>\n";
        }
        os << "</table>\n";
    }

    // Detail table
    os << "<h2>Program Results</h2>\n";
    os << "<table>\n";
    os << "<tr><th>Program</th><th>Status</th><th>Fields</th>"
       << "<th>Failures</th><th>Time (ms)</th><th>Notes</th></tr>\n";

    for (auto& e : filtered) {
        os << "<tr class=\"" << status_css_class(e.status) << "\">";
        os << "<td>" << html_escape(e.program_name) << "</td>";
        os << "<td>" << comparison_status_str(e.status) << "</td>";
        os << "<td>" << e.field_count << "</td>";
        os << "<td>" << e.failures << "</td>";
        os << "<td>" << std::fixed << std::setprecision(1) << e.elapsed_ms << "</td>";

        os << "<td>";
        if (e.is_regression) os << "<strong>REGRESSION</strong> ";
        if (e.is_fixed)      os << "<strong>FIXED</strong> ";
        os << "</td>";
        os << "</tr>\n";

        // Expandable diff detail
        if (!e.diff_detail.empty()) {
            os << "<tr class=\"" << status_css_class(e.status) << "\">"
               << "<td colspan=\"6\"><pre>"
               << html_escape(e.diff_detail)
               << "</pre></td></tr>\n";
        }
    }

    os << "</table>\n";
    os << HTML_TAIL;

    return os.str();
}

// ---------------------------------------------------------------------------
// Generate CSV report
// ---------------------------------------------------------------------------
inline std::string generate_csv_report(
        const ReportData& data,
        ReportFilter filter = ReportFilter::ALL)
{
    auto filtered = filter_entries(data.entries, filter);

    std::ostringstream os;
    // Header
    os << "program_name,status,field_count,failures,elapsed_ms,is_regression,is_fixed\n";

    for (auto& e : filtered) {
        os << "\"" << e.program_name << "\","
           << comparison_status_str(e.status) << ","
           << e.field_count << ","
           << e.failures << ","
           << std::fixed << std::setprecision(1) << e.elapsed_ms << ","
           << (e.is_regression ? "true" : "false") << ","
           << (e.is_fixed ? "true" : "false") << "\n";
    }

    return os.str();
}

// ---------------------------------------------------------------------------
// Generate JSON report
// ---------------------------------------------------------------------------
inline std::string generate_json_report(
        const ReportData& data,
        ReportFilter filter = ReportFilter::ALL)
{
    auto filtered = filter_entries(data.entries, filter);

    std::ostringstream os;
    os << "{\n";
    os << "  \"title\": \"" << AuditLog::escape_json(data.title) << "\",\n";
    os << "  \"timestamp\": \"" << AuditLog::escape_json(data.timestamp) << "\",\n";
    os << "  \"summary\": {\n";
    os << "    \"total_programs\": " << data.total_programs() << ",\n";
    os << "    \"passed\": " << data.passed_programs() << ",\n";
    os << "    \"pass_rate\": " << std::fixed << std::setprecision(1)
       << data.pass_rate() << "\n";
    os << "  },\n";

    os << "  \"results\": [\n";
    for (size_t i = 0; i < filtered.size(); ++i) {
        auto& e = filtered[i];
        os << "    {\n";
        os << "      \"program_name\": \"" << AuditLog::escape_json(e.program_name) << "\",\n";
        os << "      \"status\": \"" << comparison_status_str(e.status) << "\",\n";
        os << "      \"field_count\": " << e.field_count << ",\n";
        os << "      \"failures\": " << e.failures << ",\n";
        os << "      \"elapsed_ms\": " << std::fixed << std::setprecision(1) << e.elapsed_ms << ",\n";
        os << "      \"is_regression\": " << (e.is_regression ? "true" : "false") << ",\n";
        os << "      \"is_fixed\": " << (e.is_fixed ? "true" : "false") << "\n";
        os << "    }";
        if (i + 1 < filtered.size()) os << ",";
        os << "\n";
    }
    os << "  ]\n";
    os << "}";

    return os.str();
}

}} // namespace lazarus::parity

#endif // LAZARUS_PARITY_REPORT_H
