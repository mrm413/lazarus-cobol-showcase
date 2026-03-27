#ifndef LAZARUS_PIPELINE_ASSESSMENT_H
#define LAZARUS_PIPELINE_ASSESSMENT_H

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <numeric>
#include <unordered_map>
#include "lazarus/pipeline/detector.h"
#include "lazarus/pipeline/graph.h"

namespace lazarus { namespace pipeline {

enum class RiskCategory { LOW, MEDIUM, HIGH, CRITICAL };

inline const char* risk_category_name(RiskCategory rc) {
    switch (rc) {
        case RiskCategory::LOW:      return "LOW";
        case RiskCategory::MEDIUM:   return "MEDIUM";
        case RiskCategory::HIGH:     return "HIGH";
        case RiskCategory::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

inline RiskCategory categorize_risk(float score) {
    if (score < 25.0f)  return RiskCategory::LOW;
    if (score < 75.0f)  return RiskCategory::MEDIUM;
    if (score < 150.0f) return RiskCategory::HIGH;
    return RiskCategory::CRITICAL;
}

struct ComplexityMetrics {
    int loc = 0;                // Lines of code (excl. blanks/comments)
    int total_lines = 0;
    int blank_lines = 0;
    int comment_lines = 0;
    int mccabe = 1;             // McCabe cyclomatic complexity
    int halstead_operators = 0;
    int halstead_operands = 0;
    float halstead_volume = 0.0f;
};

struct FileAssessment {
    std::string filename;
    SourceLang lang = SourceLang::Unknown;
    ComplexityMetrics metrics;
    float risk_score = 0.0f;
    RiskCategory risk = RiskCategory::LOW;
    int fan_in = 0;
    int fan_out = 0;
    int exec_cics_count = 0;
    int exec_sql_count = 0;
    int call_count = 0;
    std::vector<std::string> dependencies;
};

struct MigrationWave {
    int wave_number = 0;
    std::vector<std::string> files;
    float total_risk = 0.0f;
    float avg_risk = 0.0f;
};

struct PortfolioSummary {
    size_t total_files = 0;
    int total_loc = 0;
    float avg_risk = 0.0f;
    float max_risk = 0.0f;
    std::unordered_map<int, size_t> lang_counts;      // SourceLang -> count
    std::unordered_map<int, size_t> risk_dist;         // RiskCategory -> count
    float estimated_effort_days = 0.0f;
    std::vector<MigrationWave> waves;
};

namespace metrics {

inline bool is_blank_line(std::string_view line) {
    for (char c : line) {
        if (!std::isspace(static_cast<unsigned char>(c))) return false;
    }
    return true;
}

inline bool is_comment_line(std::string_view line, SourceLang lang) {
    // Skip leading whitespace for the check
    size_t i = 0;
    while (i < line.size() && std::isspace(static_cast<unsigned char>(line[i]))) ++i;
    auto trimmed = line.substr(i);

    switch (lang) {
        case SourceLang::COBOL:
        case SourceLang::Copybook:
            // Col 7 = '*' in fixed-format
            if (line.size() >= 7 && (line[6] == '*' || line[6] == '/')) return true;
            return false;

        case SourceLang::JCL:
            // //* is a comment
            if (trimmed.size() >= 3 && trimmed[0] == '/' && trimmed[1] == '/' && trimmed[2] == '*')
                return true;
            return false;

        case SourceLang::REXX:
        case SourceLang::SAS:
        case SourceLang::PLI:
            // /* ... */ style (line-level check only)
            if (trimmed.size() >= 2 && trimmed[0] == '/' && trimmed[1] == '*') return true;
            if (trimmed.size() >= 1 && trimmed[0] == '*') return true;
            return false;

        case SourceLang::HLASM:
            // Column 1 = '*'
            if (!line.empty() && line[0] == '*') return true;
            return false;

        case SourceLang::Easytrieve:
            // * in column 1
            if (!line.empty() && line[0] == '*') return true;
            return false;

        default:
            // Generic: lines starting with * or //
            if (trimmed.size() >= 1 && trimmed[0] == '*') return true;
            if (trimmed.size() >= 2 && trimmed[0] == '/' && trimmed[1] == '/') return true;
            return false;
    }
}

inline ComplexityMetrics compute_metrics(std::string_view content, SourceLang lang) {
    ComplexityMetrics m;
    std::string upper = detail::to_upper(content);

    size_t pos = 0;
    while (pos < content.size()) {
        auto eol = content.find('\n', pos);
        if (eol == std::string_view::npos) eol = content.size();
        auto line = content.substr(pos, eol - pos);

        ++m.total_lines;
        if (is_blank_line(line)) {
            ++m.blank_lines;
        } else if (is_comment_line(line, lang)) {
            ++m.comment_lines;
        } else {
            ++m.loc;
        }
        pos = eol + 1;
    }

    // McCabe: count decision points
    auto count_kw = [&](std::string_view kw) -> int {
        return static_cast<int>(detail::count_occurrences_ci(content, kw));
    };

    m.mccabe = 1; // base

    if (lang == SourceLang::COBOL || lang == SourceLang::Copybook) {
        m.mccabe += count_kw("IF ");
        m.mccabe += count_kw("EVALUATE ");
        m.mccabe += count_kw("WHEN ");
        m.mccabe += count_kw("PERFORM ");
        m.mccabe += count_kw("GO TO ");
        m.mccabe += count_kw("ON SIZE ERROR");
        m.mccabe += count_kw("ON EXCEPTION");
    } else if (lang == SourceLang::JCL) {
        m.mccabe += count_kw("IF ");
        m.mccabe += count_kw("COND=");
    } else if (lang == SourceLang::PLI) {
        m.mccabe += count_kw("IF ");
        m.mccabe += count_kw("DO WHILE");
        m.mccabe += count_kw("DO UNTIL");
        m.mccabe += count_kw("SELECT");
        m.mccabe += count_kw("WHEN ");
        m.mccabe += count_kw("GOTO ");
    } else if (lang == SourceLang::REXX) {
        m.mccabe += count_kw("IF ");
        m.mccabe += count_kw("SELECT");
        m.mccabe += count_kw("WHEN ");
        m.mccabe += count_kw("DO WHILE");
        m.mccabe += count_kw("DO UNTIL");
        m.mccabe += count_kw("SIGNAL ");
    } else {
        // Generic: count IF, DO, WHEN
        m.mccabe += count_kw("IF ");
        m.mccabe += count_kw("DO ");
        m.mccabe += count_kw("WHEN ");
    }

    // Halstead: simplified count of distinct operators/operands
    // Operators: keywords that are actions
    std::vector<std::string_view> operators_list = {
        "IF", "ELSE", "PERFORM", "MOVE", "COMPUTE", "ADD", "SUBTRACT",
        "MULTIPLY", "DIVIDE", "CALL", "GO TO", "EVALUATE", "WHEN",
        "READ", "WRITE", "OPEN", "CLOSE", "DISPLAY", "ACCEPT", "SET"
    };
    std::unordered_set<std::string> distinct_ops;
    for (const auto& op : operators_list) {
        if (detail::count_occurrences_ci(content, op) > 0) {
            distinct_ops.insert(std::string(op));
        }
    }
    m.halstead_operators = static_cast<int>(distinct_ops.size());

    // Operands: rough count of unique identifiers (simplified)
    m.halstead_operands = std::max(m.loc / 3, 1);

    // Halstead volume = N * log2(n) where N = total, n = distinct vocabulary
    int n = m.halstead_operators + m.halstead_operands;
    int vocabulary = static_cast<int>(distinct_ops.size()) + m.halstead_operands;
    if (vocabulary > 0 && n > 0) {
        m.halstead_volume = static_cast<float>(n) * std::log2(static_cast<float>(vocabulary));
    }

    return m;
}

} // namespace metrics

inline float compute_risk_score(const ComplexityMetrics& m, int exec_cics, int exec_sql,
                                 int call_count, int fan_in_val, int /*fan_out_val*/) {
    float score = 0.0f;
    score += static_cast<float>(m.loc) * 0.1f;            // LOC component
    score += static_cast<float>(m.mccabe) * 2.0f;         // Complexity component
    score += static_cast<float>(exec_cics) * 50.0f;       // CICS risk
    score += static_cast<float>(exec_sql) * 30.0f;        // SQL risk
    score += static_cast<float>(call_count) * 10.0f;      // CALL risk
    score += static_cast<float>(fan_in_val) * 5.0f;       // Fan-in risk
    return score;
}

inline FileAssessment assess_file(std::string_view filename, std::string_view content,
                                   SourceLang lang, const DependencyGraph& graph) {
    FileAssessment fa;
    fa.filename = std::string(filename);
    fa.lang = lang;
    fa.metrics = metrics::compute_metrics(content, lang);
    fa.fan_in = graph.has_node(filename) ? graph.fan_in(filename) : 0;
    fa.fan_out = graph.has_node(filename) ? graph.fan_out(filename) : 0;

    // Count EXEC CICS, EXEC SQL, CALL
    fa.exec_cics_count = static_cast<int>(detail::count_occurrences_ci(content, "EXEC CICS"));
    fa.exec_sql_count = static_cast<int>(detail::count_occurrences_ci(content, "EXEC SQL"));
    fa.call_count = static_cast<int>(detail::count_occurrences_ci(content, "CALL "));

    // Dependencies
    if (graph.has_node(filename)) {
        fa.dependencies = graph.dependencies(filename);
    }

    fa.risk_score = compute_risk_score(fa.metrics, fa.exec_cics_count, fa.exec_sql_count,
                                        fa.call_count, fa.fan_in, fa.fan_out);
    fa.risk = categorize_risk(fa.risk_score);
    return fa;
}

inline std::vector<MigrationWave> plan_waves(const std::vector<FileAssessment>& assessments,
                                              const DependencyGraph& graph) {
    auto components = graph.connected_components();
    std::vector<MigrationWave> waves;

    // Build lookup: filename -> assessment
    std::unordered_map<std::string, const FileAssessment*> lookup;
    for (const auto& a : assessments) {
        lookup[a.filename] = &a;
    }

    int wave_num = 1;
    // Sort components by average risk (lowest first)
    std::vector<std::pair<float, size_t>> wave_risk;
    for (size_t i = 0; i < components.size(); ++i) {
        float total = 0.0f;
        int count = 0;
        for (const auto& f : components[i]) {
            auto it = lookup.find(f);
            if (it != lookup.end()) {
                total += it->second->risk_score;
                ++count;
            }
        }
        float avg = count > 0 ? total / static_cast<float>(count) : 0.0f;
        wave_risk.push_back({avg, i});
    }
    std::sort(wave_risk.begin(), wave_risk.end());

    for (const auto& wr : wave_risk) {
        MigrationWave mw;
        mw.wave_number = wave_num++;
        mw.files = components[wr.second];
        mw.total_risk = 0.0f;
        for (const auto& f : mw.files) {
            auto it = lookup.find(f);
            if (it != lookup.end()) mw.total_risk += it->second->risk_score;
        }
        mw.avg_risk = !mw.files.empty() ? mw.total_risk / static_cast<float>(mw.files.size()) : 0.0f;
        waves.push_back(std::move(mw));
    }

    return waves;
}

inline PortfolioSummary summarize(const std::vector<FileAssessment>& assessments,
                                   const DependencyGraph& graph) {
    PortfolioSummary ps;
    ps.total_files = assessments.size();
    float total_risk = 0.0f;
    ps.max_risk = 0.0f;

    for (const auto& a : assessments) {
        ps.total_loc += a.metrics.loc;
        total_risk += a.risk_score;
        if (a.risk_score > ps.max_risk) ps.max_risk = a.risk_score;
        ++ps.lang_counts[static_cast<int>(a.lang)];
        ++ps.risk_dist[static_cast<int>(a.risk)];
    }

    ps.avg_risk = ps.total_files > 0 ? total_risk / static_cast<float>(ps.total_files) : 0.0f;

    // Effort estimate: ~0.5 days per 100 LOC, scaled by average risk
    float risk_multiplier = 1.0f + (ps.avg_risk / 100.0f);
    ps.estimated_effort_days = (static_cast<float>(ps.total_loc) / 100.0f) * 0.5f * risk_multiplier;

    ps.waves = plan_waves(assessments, graph);

    return ps;
}

}} // namespace lazarus::pipeline

#endif // LAZARUS_PIPELINE_ASSESSMENT_H
