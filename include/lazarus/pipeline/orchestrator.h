#ifndef LAZARUS_PIPELINE_ORCHESTRATOR_H
#define LAZARUS_PIPELINE_ORCHESTRATOR_H

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <unordered_map>
#include <sstream>
#include "lazarus/pipeline/detector.h"
#include "lazarus/pipeline/router.h"
#include "lazarus/pipeline/graph.h"
#include "lazarus/pipeline/assessment.h"

namespace lazarus { namespace pipeline {

using ProgressCallback = std::function<void(int current, int total,
                                            const std::string& item, const std::string& status)>;

struct PipelineConfig {
    std::string input_dir;
    std::string output_dir;
    std::vector<SourceLang> language_filters;  // empty = all
    bool stop_on_error = false;
    ProgressCallback on_progress;

    bool is_lang_filtered(SourceLang lang) const {
        if (language_filters.empty()) return false; // no filter = accept all
        for (auto l : language_filters) {
            if (l == lang) return false; // in filter list = accepted
        }
        return true; // not in filter list = filtered out
    }
};

enum class StageStatus { PENDING, RUNNING, SUCCESS, FAILED, SKIPPED };

inline const char* stage_status_name(StageStatus s) {
    switch (s) {
        case StageStatus::PENDING: return "PENDING";
        case StageStatus::RUNNING: return "RUNNING";
        case StageStatus::SUCCESS: return "SUCCESS";
        case StageStatus::FAILED:  return "FAILED";
        case StageStatus::SKIPPED: return "SKIPPED";
    }
    return "UNKNOWN";
}

struct FileResult {
    std::string filename;
    std::optional<Detection> detection;
    std::optional<FileAssessment> assessment;
    std::optional<TranspileResult> transpile_result;
    std::string error;
    bool success = true;
};

struct PipelineState {
    std::string stage;
    int files_processed = 0;
    int files_total = 0;
    std::vector<FileResult> results;
    DependencyGraph graph;
    PortfolioSummary summary;
    bool completed = false;

    // Serialization: simple key-value checkpoint format
    std::string serialize() const {
        std::ostringstream oss;
        oss << "STAGE=" << stage << "\n";
        oss << "FILES_PROCESSED=" << files_processed << "\n";
        oss << "FILES_TOTAL=" << files_total << "\n";
        oss << "COMPLETED=" << (completed ? "true" : "false") << "\n";
        oss << "RESULT_COUNT=" << results.size() << "\n";
        for (size_t i = 0; i < results.size(); ++i) {
            oss << "RESULT_" << i << "_FILE=" << results[i].filename << "\n";
            oss << "RESULT_" << i << "_SUCCESS=" << (results[i].success ? "true" : "false") << "\n";
            if (results[i].detection) {
                oss << "RESULT_" << i << "_LANG=" << source_lang_name(results[i].detection->lang) << "\n";
                oss << "RESULT_" << i << "_CONF=" << results[i].detection->confidence << "\n";
            }
        }
        return oss.str();
    }

    static PipelineState deserialize(const std::string& data) {
        PipelineState state;
        std::istringstream iss(data);
        std::string line;
        std::unordered_map<std::string, std::string> kv;

        while (std::getline(iss, line)) {
            auto eq = line.find('=');
            if (eq != std::string::npos) {
                kv[line.substr(0, eq)] = line.substr(eq + 1);
            }
        }

        if (kv.count("STAGE")) state.stage = kv["STAGE"];
        if (kv.count("FILES_PROCESSED")) state.files_processed = std::stoi(kv["FILES_PROCESSED"]);
        if (kv.count("FILES_TOTAL")) state.files_total = std::stoi(kv["FILES_TOTAL"]);
        if (kv.count("COMPLETED")) state.completed = (kv["COMPLETED"] == "true");

        int result_count = 0;
        if (kv.count("RESULT_COUNT")) result_count = std::stoi(kv["RESULT_COUNT"]);

        for (int i = 0; i < result_count; ++i) {
            FileResult fr;
            std::string prefix = "RESULT_" + std::to_string(i) + "_";
            if (kv.count(prefix + "FILE")) fr.filename = kv[prefix + "FILE"];
            if (kv.count(prefix + "SUCCESS")) fr.success = (kv[prefix + "SUCCESS"] == "true");
            state.results.push_back(fr);
        }

        return state;
    }
};

class Orchestrator {
    PipelineConfig config_;
    Router router_;
    PipelineState state_;

    void notify(int current, int total, const std::string& item, const std::string& status) {
        if (config_.on_progress) {
            config_.on_progress(current, total, item, status);
        }
    }

public:
    explicit Orchestrator(PipelineConfig config) : config_(std::move(config)) {}

    Router& router() { return router_; }
    const PipelineState& state() const { return state_; }

    void set_config(PipelineConfig config) { config_ = std::move(config); }

    // Run pipeline on a list of files (filename, content pairs)
    PipelineState run(const std::vector<std::pair<std::string, std::string>>& files) {
        state_ = PipelineState{};
        state_.files_total = static_cast<int>(files.size());

        // Stage 1: Detection
        state_.stage = "detect";
        notify(0, state_.files_total, "", "Starting detection");

        std::vector<Detection> detections;
        for (int i = 0; i < static_cast<int>(files.size()); ++i) {
            FileResult fr;
            fr.filename = files[static_cast<size_t>(i)].first;

            auto det = detect(files[static_cast<size_t>(i)].first, files[static_cast<size_t>(i)].second);
            fr.detection = det;
            detections.push_back(det);

            // Check language filter
            if (config_.is_lang_filtered(det.lang)) {
                fr.success = true;
                fr.error = "Filtered out (lang=" + std::string(source_lang_name(det.lang)) + ")";
            }

            state_.results.push_back(std::move(fr));
            notify(i + 1, state_.files_total, files[static_cast<size_t>(i)].first, "detected");
        }

        // Stage 2: Dependency graph
        state_.stage = "graph";
        notify(0, state_.files_total, "", "Building dependency graph");

        for (int i = 0; i < static_cast<int>(files.size()); ++i) {
            auto si = static_cast<size_t>(i);
            state_.graph.add_node(files[si].first);
            auto deps = dep_extract::extract_dependencies(
                files[si].first, files[si].second, detections[si].lang);
            for (const auto& d : deps) {
                state_.graph.add_edge(d.source, d.target);
            }
        }

        // Stage 3: Assessment
        state_.stage = "assess";
        notify(0, state_.files_total, "", "Assessing files");

        std::vector<FileAssessment> assessments;
        for (int i = 0; i < static_cast<int>(files.size()); ++i) {
            auto si = static_cast<size_t>(i);
            if (config_.is_lang_filtered(detections[si].lang)) continue;

            auto assessment = assess_file(files[si].first, files[si].second,
                                           detections[si].lang, state_.graph);
            assessments.push_back(assessment);
            state_.results[si].assessment = assessment;
            notify(i + 1, state_.files_total, files[si].first, "assessed");
        }

        // Stage 4: Route and transpile
        state_.stage = "transpile";
        notify(0, state_.files_total, "", "Transpiling");

        // Build order from topological sort
        auto topo = state_.graph.topological_sort();

        // Index files by name
        std::unordered_map<std::string, size_t> file_idx;
        for (size_t i = 0; i < files.size(); ++i) {
            file_idx[files[i].first] = i;
        }

        int transpiled = 0;
        for (const auto& name : topo) {
            auto it = file_idx.find(name);
            if (it == file_idx.end()) continue;
            size_t idx = it->second;

            if (config_.is_lang_filtered(detections[idx].lang)) continue;

            TranspileRequest req;
            req.lang = detections[idx].lang;
            req.filename = files[idx].first;
            req.content = files[idx].second;

            auto result = router_.route(req);
            state_.results[idx].transpile_result = result;

            if (!result.success) {
                state_.results[idx].success = false;
                state_.results[idx].error = result.error;
                if (config_.stop_on_error) {
                    state_.stage = "error";
                    return state_;
                }
            }

            ++transpiled;
            notify(transpiled, state_.files_total, name, result.success ? "transpiled" : "failed");
        }

        // Stage 5: Summary
        state_.stage = "report";
        state_.summary = summarize(assessments, state_.graph);
        state_.files_processed = transpiled;
        state_.completed = true;

        notify(state_.files_total, state_.files_total, "", "Pipeline complete");
        return state_;
    }
};

}} // namespace lazarus::pipeline

#endif // LAZARUS_PIPELINE_ORCHESTRATOR_H
