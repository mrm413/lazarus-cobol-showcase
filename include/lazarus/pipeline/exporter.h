#ifndef LAZARUS_PIPELINE_EXPORTER_H
#define LAZARUS_PIPELINE_EXPORTER_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "lazarus/pipeline/detector.h"
#include "lazarus/pipeline/assessment.h"
#include "lazarus/pipeline/graph.h"
#include "lazarus/pipeline/orchestrator.h"

namespace lazarus { namespace pipeline {

enum class ExportFormat { JSON, CSV, HTML };

inline const char* export_format_name(ExportFormat f) {
    switch (f) {
        case ExportFormat::JSON: return "JSON";
        case ExportFormat::CSV:  return "CSV";
        case ExportFormat::HTML: return "HTML";
    }
    return "UNKNOWN";
}

namespace json_detail {

inline std::string escape_json(const std::string& s) {
    std::string result;
    result.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"':  result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:   result += c; break;
        }
    }
    return result;
}

inline std::string to_json_string(const std::string& s) {
    return "\"" + escape_json(s) + "\"";
}

} // namespace json_detail

class Exporter {
public:

    static std::string export_json(const PipelineState& state) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "{\n";

        // Summary
        oss << "  \"summary\": {\n";
        oss << "    \"total_files\": " << state.summary.total_files << ",\n";
        oss << "    \"total_loc\": " << state.summary.total_loc << ",\n";
        oss << "    \"avg_risk\": " << state.summary.avg_risk << ",\n";
        oss << "    \"max_risk\": " << state.summary.max_risk << ",\n";
        oss << "    \"estimated_effort_days\": " << state.summary.estimated_effort_days << ",\n";

        // Language counts
        oss << "    \"languages\": {";
        bool first_lang = true;
        for (const auto& kv : state.summary.lang_counts) {
            if (!first_lang) oss << ",";
            first_lang = false;
            oss << "\n      " << json_detail::to_json_string(source_lang_name(static_cast<SourceLang>(kv.first)))
                << ": " << kv.second;
        }
        oss << "\n    },\n";

        // Risk distribution
        oss << "    \"risk_distribution\": {";
        bool first_risk = true;
        for (const auto& kv : state.summary.risk_dist) {
            if (!first_risk) oss << ",";
            first_risk = false;
            oss << "\n      " << json_detail::to_json_string(risk_category_name(static_cast<RiskCategory>(kv.first)))
                << ": " << kv.second;
        }
        oss << "\n    }\n";
        oss << "  },\n";

        // Files
        oss << "  \"files\": [\n";
        for (size_t i = 0; i < state.results.size(); ++i) {
            const auto& r = state.results[i];
            if (i > 0) oss << ",\n";
            oss << "    {\n";
            oss << "      \"filename\": " << json_detail::to_json_string(r.filename) << ",\n";
            if (r.detection) {
                oss << "      \"language\": " << json_detail::to_json_string(source_lang_name(r.detection->lang)) << ",\n";
                oss << "      \"confidence\": " << r.detection->confidence << ",\n";
            }
            if (r.assessment) {
                oss << "      \"loc\": " << r.assessment->metrics.loc << ",\n";
                oss << "      \"mccabe\": " << r.assessment->metrics.mccabe << ",\n";
                oss << "      \"risk_score\": " << r.assessment->risk_score << ",\n";
                oss << "      \"risk_category\": " << json_detail::to_json_string(risk_category_name(r.assessment->risk)) << ",\n";
                oss << "      \"fan_in\": " << r.assessment->fan_in << ",\n";
                oss << "      \"fan_out\": " << r.assessment->fan_out << ",\n";
                oss << "      \"exec_cics\": " << r.assessment->exec_cics_count << ",\n";
                oss << "      \"exec_sql\": " << r.assessment->exec_sql_count << ",\n";

                // Dependencies array
                oss << "      \"dependencies\": [";
                for (size_t d = 0; d < r.assessment->dependencies.size(); ++d) {
                    if (d > 0) oss << ", ";
                    oss << json_detail::to_json_string(r.assessment->dependencies[d]);
                }
                oss << "],\n";
            }
            oss << "      \"success\": " << (r.success ? "true" : "false") << "\n";
            oss << "    }";
        }
        oss << "\n  ],\n";

        // Dependency graph
        oss << "  \"dependency_graph\": {\n";
        oss << "    \"node_count\": " << state.graph.node_count() << ",\n";
        oss << "    \"edge_count\": " << state.graph.edge_count() << "\n";
        oss << "  },\n";

        // Migration waves
        oss << "  \"migration_waves\": [\n";
        for (size_t w = 0; w < state.summary.waves.size(); ++w) {
            const auto& wave = state.summary.waves[w];
            if (w > 0) oss << ",\n";
            oss << "    {\n";
            oss << "      \"wave\": " << wave.wave_number << ",\n";
            oss << "      \"files\": [";
            for (size_t f = 0; f < wave.files.size(); ++f) {
                if (f > 0) oss << ", ";
                oss << json_detail::to_json_string(wave.files[f]);
            }
            oss << "],\n";
            oss << "      \"total_risk\": " << wave.total_risk << ",\n";
            oss << "      \"avg_risk\": " << wave.avg_risk << "\n";
            oss << "    }";
        }
        oss << "\n  ]\n";

        oss << "}\n";
        return oss.str();
    }

    static std::string export_csv(const PipelineState& state) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);
        oss << "filename,language,confidence,loc,mccabe,risk_score,risk_category,fan_in,fan_out,exec_cics,exec_sql,success\n";

        for (const auto& r : state.results) {
            oss << json_detail::escape_json(r.filename) << ",";
            if (r.detection) {
                oss << source_lang_name(r.detection->lang) << ","
                    << r.detection->confidence << ",";
            } else {
                oss << "Unknown,0.00,";
            }
            if (r.assessment) {
                oss << r.assessment->metrics.loc << ","
                    << r.assessment->metrics.mccabe << ","
                    << r.assessment->risk_score << ","
                    << risk_category_name(r.assessment->risk) << ","
                    << r.assessment->fan_in << ","
                    << r.assessment->fan_out << ","
                    << r.assessment->exec_cics_count << ","
                    << r.assessment->exec_sql_count << ",";
            } else {
                oss << "0,0,0.00,LOW,0,0,0,0,";
            }
            oss << (r.success ? "true" : "false") << "\n";
        }
        return oss.str();
    }

    static std::string export_html(const PipelineState& state) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2);

        oss << "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n";
        oss << "<meta charset=\"UTF-8\">\n";
        oss << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
        oss << "<title>Migration Assessment Report</title>\n";
        oss << "<style>\n";
        oss << "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', sans-serif; margin: 2em; background: #f5f5f5; }\n";
        oss << "h1, h2, h3 { color: #1a1a2e; }\n";
        oss << ".dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 1em; margin: 1em 0; }\n";
        oss << ".card { background: white; padding: 1.5em; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
        oss << ".card h3 { margin-top: 0; font-size: 0.9em; color: #666; text-transform: uppercase; }\n";
        oss << ".card .value { font-size: 2em; font-weight: bold; color: #16213e; }\n";
        oss << "table { width: 100%; border-collapse: collapse; background: white; border-radius: 8px; overflow: hidden; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }\n";
        oss << "th { background: #16213e; color: white; padding: 0.8em; text-align: left; }\n";
        oss << "td { padding: 0.6em 0.8em; border-bottom: 1px solid #eee; }\n";
        oss << "tr:hover { background: #f0f4ff; }\n";
        oss << ".risk-low { color: #28a745; font-weight: bold; }\n";
        oss << ".risk-medium { color: #ffc107; font-weight: bold; }\n";
        oss << ".risk-high { color: #fd7e14; font-weight: bold; }\n";
        oss << ".risk-critical { color: #dc3545; font-weight: bold; }\n";
        oss << ".wave { background: white; padding: 1em; margin: 0.5em 0; border-radius: 8px; border-left: 4px solid #16213e; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }\n";
        oss << ".chart-data { display: none; }\n";
        oss << "</style>\n</head>\n<body>\n";

        oss << "<h1>Migration Assessment Report</h1>\n";

        // Dashboard cards
        oss << "<div class=\"dashboard\">\n";
        oss << "<div class=\"card\"><h3>Total Files</h3><div class=\"value\">"
            << state.summary.total_files << "</div></div>\n";
        oss << "<div class=\"card\"><h3>Total LOC</h3><div class=\"value\">"
            << state.summary.total_loc << "</div></div>\n";
        oss << "<div class=\"card\"><h3>Avg Risk</h3><div class=\"value\">"
            << state.summary.avg_risk << "</div></div>\n";
        oss << "<div class=\"card\"><h3>Est. Effort (days)</h3><div class=\"value\">"
            << state.summary.estimated_effort_days << "</div></div>\n";
        oss << "</div>\n";

        // Language distribution (data for D3.js)
        oss << "<h2>Language Distribution</h2>\n";
        oss << "<div class=\"chart-data\" id=\"lang-data\" data-chart=\"pie\">\n";
        for (const auto& kv : state.summary.lang_counts) {
            oss << source_lang_name(static_cast<SourceLang>(kv.first)) << ":" << kv.second << "\n";
        }
        oss << "</div>\n";

        // Risk distribution
        oss << "<h2>Risk Distribution</h2>\n";
        oss << "<div class=\"chart-data\" id=\"risk-data\" data-chart=\"histogram\">\n";
        for (const auto& kv : state.summary.risk_dist) {
            oss << risk_category_name(static_cast<RiskCategory>(kv.first)) << ":" << kv.second << "\n";
        }
        oss << "</div>\n";

        // File inventory table
        oss << "<h2>File Inventory</h2>\n";
        oss << "<table>\n<thead><tr>";
        oss << "<th>File</th><th>Language</th><th>LOC</th><th>McCabe</th>";
        oss << "<th>Risk Score</th><th>Risk</th><th>Fan-In</th><th>Fan-Out</th>";
        oss << "<th>CICS</th><th>SQL</th><th>Status</th>";
        oss << "</tr></thead>\n<tbody>\n";

        for (const auto& r : state.results) {
            oss << "<tr>";
            oss << "<td>" << r.filename << "</td>";
            if (r.detection) {
                oss << "<td>" << source_lang_name(r.detection->lang) << "</td>";
            } else {
                oss << "<td>Unknown</td>";
            }
            if (r.assessment) {
                std::string risk_class;
                switch (r.assessment->risk) {
                    case RiskCategory::LOW:      risk_class = "risk-low"; break;
                    case RiskCategory::MEDIUM:    risk_class = "risk-medium"; break;
                    case RiskCategory::HIGH:      risk_class = "risk-high"; break;
                    case RiskCategory::CRITICAL:  risk_class = "risk-critical"; break;
                }
                oss << "<td>" << r.assessment->metrics.loc << "</td>";
                oss << "<td>" << r.assessment->metrics.mccabe << "</td>";
                oss << "<td>" << r.assessment->risk_score << "</td>";
                oss << "<td class=\"" << risk_class << "\">"
                    << risk_category_name(r.assessment->risk) << "</td>";
                oss << "<td>" << r.assessment->fan_in << "</td>";
                oss << "<td>" << r.assessment->fan_out << "</td>";
                oss << "<td>" << r.assessment->exec_cics_count << "</td>";
                oss << "<td>" << r.assessment->exec_sql_count << "</td>";
            } else {
                oss << "<td>-</td><td>-</td><td>-</td><td>-</td><td>-</td><td>-</td><td>-</td><td>-</td>";
            }
            oss << "<td>" << (r.success ? "OK" : "FAIL") << "</td>";
            oss << "</tr>\n";
        }
        oss << "</tbody></table>\n";

        // Dependency graph data
        oss << "<h2>Dependency Graph</h2>\n";
        oss << "<div class=\"chart-data\" id=\"dep-data\" data-chart=\"graph\">\n";
        oss << "nodes:" << state.graph.node_count() << "\n";
        oss << "edges:" << state.graph.edge_count() << "\n";
        oss << "</div>\n";

        // Migration waves
        oss << "<h2>Migration Waves</h2>\n";
        for (const auto& wave : state.summary.waves) {
            oss << "<div class=\"wave\">\n";
            oss << "<h3>Wave " << wave.wave_number << "</h3>\n";
            oss << "<p>Files: " << wave.files.size()
                << " | Total Risk: " << wave.total_risk
                << " | Avg Risk: " << wave.avg_risk << "</p>\n";
            oss << "<ul>\n";
            for (const auto& f : wave.files) {
                oss << "<li>" << f << "</li>\n";
            }
            oss << "</ul>\n</div>\n";
        }

        oss << "</body>\n</html>\n";
        return oss.str();
    }

    static std::string export_report(const PipelineState& state, ExportFormat format) {
        switch (format) {
            case ExportFormat::JSON: return export_json(state);
            case ExportFormat::CSV:  return export_csv(state);
            case ExportFormat::HTML: return export_html(state);
        }
        return "";
    }
};

}} // namespace lazarus::pipeline

#endif // LAZARUS_PIPELINE_EXPORTER_H
