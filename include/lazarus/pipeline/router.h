#ifndef LAZARUS_PIPELINE_ROUTER_H
#define LAZARUS_PIPELINE_ROUTER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include "lazarus/pipeline/detector.h"

namespace lazarus { namespace pipeline {

struct TranspileRequest {
    SourceLang lang = SourceLang::Unknown;
    std::string filename;
    std::string content;
};

struct TranspileResult {
    bool success = false;
    std::string output;
    std::string error;
    SourceLang lang = SourceLang::Unknown;
    std::string filename;
};

struct RouteStats {
    size_t total_routed = 0;
    size_t total_success = 0;
    size_t total_failure = 0;
    std::unordered_map<int, size_t> per_lang_count;
    std::unordered_map<int, size_t> per_lang_success;

    void record(SourceLang lang, bool success) {
        ++total_routed;
        int key = static_cast<int>(lang);
        ++per_lang_count[key];
        if (success) {
            ++total_success;
            ++per_lang_success[key];
        } else {
            ++total_failure;
        }
    }

    size_t count_for(SourceLang lang) const {
        auto it = per_lang_count.find(static_cast<int>(lang));
        return it != per_lang_count.end() ? it->second : 0;
    }

    size_t success_for(SourceLang lang) const {
        auto it = per_lang_success.find(static_cast<int>(lang));
        return it != per_lang_success.end() ? it->second : 0;
    }

    float success_rate() const {
        return total_routed > 0 ? static_cast<float>(total_success) / static_cast<float>(total_routed) : 0.0f;
    }

    float success_rate_for(SourceLang lang) const {
        size_t c = count_for(lang);
        size_t s = success_for(lang);
        return c > 0 ? static_cast<float>(s) / static_cast<float>(c) : 0.0f;
    }
};

class Router {
public:
    using Handler = std::function<TranspileResult(const TranspileRequest&)>;

private:
    std::unordered_map<int, Handler> handlers_;
    RouteStats stats_;

    static TranspileResult default_handler(const TranspileRequest& req) {
        return {
            true,
            "// Placeholder transpilation for " + std::string(source_lang_name(req.lang)) +
                "\n// Source: " + req.filename +
                "\n// Lines: " + std::to_string(std::count(req.content.begin(), req.content.end(), '\n') + 1) +
                "\n",
            "",
            req.lang,
            req.filename
        };
    }

public:
    void register_handler(SourceLang lang, Handler handler) {
        handlers_[static_cast<int>(lang)] = std::move(handler);
    }

    bool has_handler(SourceLang lang) const {
        return handlers_.find(static_cast<int>(lang)) != handlers_.end();
    }

    TranspileResult route(const TranspileRequest& req) {
        TranspileResult result;

        if (req.lang == SourceLang::Unknown) {
            result.success = false;
            result.error = "Unknown source language for file: " + req.filename +
                           ". Suggestion: specify the language explicitly or check the file content.";
            result.lang = req.lang;
            result.filename = req.filename;
            stats_.record(req.lang, false);
            return result;
        }

        auto it = handlers_.find(static_cast<int>(req.lang));
        if (it != handlers_.end()) {
            result = it->second(req);
        } else {
            result = default_handler(req);
        }
        result.lang = req.lang;
        result.filename = req.filename;
        stats_.record(req.lang, result.success);
        return result;
    }

    std::vector<TranspileResult> route_batch(const std::vector<TranspileRequest>& reqs) {
        std::vector<TranspileResult> results;
        results.reserve(reqs.size());
        for (const auto& req : reqs) {
            results.push_back(route(req));
        }
        return results;
    }

    const RouteStats& stats() const { return stats_; }

    void reset_stats() { stats_ = RouteStats{}; }

    std::vector<SourceLang> registered_languages() const {
        std::vector<SourceLang> langs;
        for (const auto& kv : handlers_) {
            langs.push_back(static_cast<SourceLang>(kv.first));
        }
        return langs;
    }
};

}} // namespace lazarus::pipeline

#endif // LAZARUS_PIPELINE_ROUTER_H
