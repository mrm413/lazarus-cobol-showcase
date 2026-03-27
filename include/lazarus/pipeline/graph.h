#ifndef LAZARUS_PIPELINE_GRAPH_H
#define LAZARUS_PIPELINE_GRAPH_H

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <algorithm>
#include <queue>
#include <functional>
#include <cctype>
#include "lazarus/pipeline/detector.h"

namespace lazarus { namespace pipeline {

class DependencyGraph {
    std::unordered_map<std::string, std::vector<std::string>> forward_;   // node -> dependencies
    std::unordered_map<std::string, std::vector<std::string>> reverse_;   // node -> dependents
    std::unordered_set<std::string> nodes_;

    static std::string to_str(std::string_view sv) { return std::string(sv); }

    // Tarjan's SCC internals
    struct TarjanState {
        std::unordered_map<std::string, int> index_map;
        std::unordered_map<std::string, int> lowlink;
        std::unordered_map<std::string, bool> on_stack;
        std::vector<std::string> stack;
        int index = 0;
        std::vector<std::vector<std::string>> components;
    };

    void tarjan_visit(const std::string& v, TarjanState& state) const {
        state.index_map[v] = state.index;
        state.lowlink[v] = state.index;
        ++state.index;
        state.stack.push_back(v);
        state.on_stack[v] = true;

        auto it = forward_.find(v);
        if (it != forward_.end()) {
            for (const auto& w : it->second) {
                if (state.index_map.find(w) == state.index_map.end()) {
                    tarjan_visit(w, state);
                    state.lowlink[v] = std::min(state.lowlink[v], state.lowlink[w]);
                } else if (state.on_stack[w]) {
                    state.lowlink[v] = std::min(state.lowlink[v], state.index_map[w]);
                }
            }
        }

        if (state.lowlink[v] == state.index_map[v]) {
            std::vector<std::string> component;
            while (true) {
                auto w = state.stack.back();
                state.stack.pop_back();
                state.on_stack[w] = false;
                component.push_back(w);
                if (w == v) break;
            }
            std::sort(component.begin(), component.end());
            state.components.push_back(std::move(component));
        }
    }

public:
    void add_node(std::string name) {
        nodes_.insert(name);
        if (forward_.find(name) == forward_.end()) forward_[name] = {};
        if (reverse_.find(name) == reverse_.end()) reverse_[name] = {};
    }

    void add_edge(std::string_view from, std::string_view to) {
        std::string f = to_str(from);
        std::string t = to_str(to);
        add_node(f);
        add_node(t);
        // Avoid duplicates
        auto& fwd = forward_[f];
        if (std::find(fwd.begin(), fwd.end(), t) == fwd.end()) {
            fwd.push_back(t);
        }
        auto& rev = reverse_[t];
        if (std::find(rev.begin(), rev.end(), f) == rev.end()) {
            rev.push_back(f);
        }
    }

    size_t node_count() const { return nodes_.size(); }
    size_t edge_count() const {
        size_t total = 0;
        for (const auto& kv : forward_) total += kv.second.size();
        return total;
    }

    bool has_node(std::string_view name) const {
        return nodes_.find(std::string(name)) != nodes_.end();
    }

    std::vector<std::string> dependents(std::string_view node) const {
        auto it = reverse_.find(to_str(node));
        if (it == reverse_.end()) return {};
        auto result = it->second;
        std::sort(result.begin(), result.end());
        return result;
    }

    std::vector<std::string> dependencies(std::string_view node) const {
        auto it = forward_.find(to_str(node));
        if (it == forward_.end()) return {};
        auto result = it->second;
        std::sort(result.begin(), result.end());
        return result;
    }

    int fan_in(std::string_view node) const {
        auto it = reverse_.find(to_str(node));
        return it != reverse_.end() ? static_cast<int>(it->second.size()) : 0;
    }

    int fan_out(std::string_view node) const {
        auto it = forward_.find(to_str(node));
        return it != forward_.end() ? static_cast<int>(it->second.size()) : 0;
    }

    // Kahn's algorithm
    std::vector<std::string> topological_sort() const {
        std::unordered_map<std::string, int> in_degree;
        for (const auto& n : nodes_) in_degree[n] = 0;
        for (const auto& kv : forward_) {
            for (const auto& dep : kv.second) {
                ++in_degree[dep];
            }
        }

        // Use sorted queue for deterministic output
        std::vector<std::string> zero_in;
        for (const auto& kv : in_degree) {
            if (kv.second == 0) zero_in.push_back(kv.first);
        }
        std::sort(zero_in.begin(), zero_in.end());

        std::vector<std::string> result;
        std::deque<std::string> q(zero_in.begin(), zero_in.end());

        while (!q.empty()) {
            auto node = q.front();
            q.pop_front();
            result.push_back(node);

            auto it = forward_.find(node);
            if (it != forward_.end()) {
                std::vector<std::string> next_ready;
                for (const auto& dep : it->second) {
                    --in_degree[dep];
                    if (in_degree[dep] == 0) {
                        next_ready.push_back(dep);
                    }
                }
                std::sort(next_ready.begin(), next_ready.end());
                for (const auto& n : next_ready) {
                    q.push_back(n);
                }
            }
        }

        return result; // If size < nodes_, there's a cycle
    }

    // DFS-based cycle detection with path reporting
    std::optional<std::vector<std::string>> detect_cycle() const {
        enum Color { WHITE, GRAY, BLACK };
        std::unordered_map<std::string, Color> color;
        std::unordered_map<std::string, std::string> parent;
        for (const auto& n : nodes_) color[n] = WHITE;

        std::vector<std::string> cycle_path;
        bool found = false;

        std::function<void(const std::string&)> dfs = [&](const std::string& u) {
            if (found) return;
            color[u] = GRAY;

            auto it = forward_.find(u);
            if (it != forward_.end()) {
                for (const auto& v : it->second) {
                    if (found) return;
                    if (color[v] == GRAY) {
                        // Found cycle: trace back
                        cycle_path.push_back(v);
                        cycle_path.push_back(u);
                        std::string cur = u;
                        while (cur != v) {
                            auto pit = parent.find(cur);
                            if (pit == parent.end()) break;
                            cur = pit->second;
                            cycle_path.push_back(cur);
                        }
                        std::reverse(cycle_path.begin(), cycle_path.end());
                        found = true;
                        return;
                    }
                    if (color[v] == WHITE) {
                        parent[v] = u;
                        dfs(v);
                    }
                }
            }
            color[u] = BLACK;
        };

        // Visit in sorted order for determinism
        std::vector<std::string> sorted_nodes(nodes_.begin(), nodes_.end());
        std::sort(sorted_nodes.begin(), sorted_nodes.end());
        for (const auto& n : sorted_nodes) {
            if (color[n] == WHITE) {
                dfs(n);
                if (found) return cycle_path;
            }
        }
        return std::nullopt;
    }

    std::vector<std::vector<std::string>> connected_components() const {
        std::unordered_set<std::string> visited;
        std::vector<std::vector<std::string>> components;

        // Build undirected adjacency
        std::unordered_map<std::string, std::unordered_set<std::string>> undirected;
        for (const auto& n : nodes_) undirected[n] = {};
        for (const auto& kv : forward_) {
            for (const auto& dep : kv.second) {
                undirected[kv.first].insert(dep);
                undirected[dep].insert(kv.first);
            }
        }

        std::vector<std::string> sorted_nodes(nodes_.begin(), nodes_.end());
        std::sort(sorted_nodes.begin(), sorted_nodes.end());

        for (const auto& start : sorted_nodes) {
            if (visited.count(start)) continue;
            std::vector<std::string> component;
            std::queue<std::string> bfs;
            bfs.push(start);
            visited.insert(start);
            while (!bfs.empty()) {
                auto cur = bfs.front();
                bfs.pop();
                component.push_back(cur);
                auto& neighbors = undirected[cur];
                std::vector<std::string> sorted_neighbors(neighbors.begin(), neighbors.end());
                std::sort(sorted_neighbors.begin(), sorted_neighbors.end());
                for (const auto& nb : sorted_neighbors) {
                    if (!visited.count(nb)) {
                        visited.insert(nb);
                        bfs.push(nb);
                    }
                }
            }
            std::sort(component.begin(), component.end());
            components.push_back(std::move(component));
        }
        return components;
    }

    std::vector<std::vector<std::string>> strongly_connected_components() const {
        TarjanState state;
        std::vector<std::string> sorted_nodes(nodes_.begin(), nodes_.end());
        std::sort(sorted_nodes.begin(), sorted_nodes.end());

        for (const auto& n : sorted_nodes) {
            if (state.index_map.find(n) == state.index_map.end()) {
                tarjan_visit(n, state);
            }
        }

        std::sort(state.components.begin(), state.components.end());
        return state.components;
    }
};

// Dependency extraction from source content
namespace dep_extract {

inline std::string to_upper(std::string_view sv) {
    std::string result(sv);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

inline std::string trim(std::string_view sv) {
    size_t start = 0;
    while (start < sv.size() && std::isspace(static_cast<unsigned char>(sv[start]))) ++start;
    size_t end = sv.size();
    while (end > start && std::isspace(static_cast<unsigned char>(sv[end - 1]))) --end;
    return std::string(sv.substr(start, end - start));
}

inline std::string extract_word_after(std::string_view line, std::string_view keyword) {
    auto pos = line.find(keyword);
    if (pos == std::string_view::npos) return "";
    pos += keyword.size();
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) ++pos;
    size_t start = pos;
    while (pos < line.size() && !std::isspace(static_cast<unsigned char>(line[pos])) &&
           line[pos] != '.' && line[pos] != ',' && line[pos] != ';' &&
           line[pos] != '\'' && line[pos] != '"') {
        ++pos;
    }
    if (pos > start) return std::string(line.substr(start, pos - start));
    return "";
}

inline std::string extract_quoted_or_word(std::string_view line, std::string_view keyword) {
    auto pos = line.find(keyword);
    if (pos == std::string_view::npos) return "";
    pos += keyword.size();
    while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) ++pos;
    if (pos < line.size() && (line[pos] == '\'' || line[pos] == '"')) {
        char quote = line[pos];
        ++pos;
        size_t start = pos;
        while (pos < line.size() && line[pos] != quote) ++pos;
        return std::string(line.substr(start, pos - start));
    }
    return extract_word_after(line, keyword);
}

struct DepInfo {
    std::string source;
    std::string target;
    std::string type; // "COPY", "CALL", "EXEC PGM", "PROC", "INCLUDE", "BIND"
};

inline std::vector<DepInfo> extract_dependencies(std::string_view filename, std::string_view content,
                                                  SourceLang lang) {
    std::vector<DepInfo> deps;
    std::string fname(filename);
    std::string upper_content = to_upper(content);

    size_t pos = 0;
    while (pos < upper_content.size()) {
        auto eol = upper_content.find('\n', pos);
        if (eol == std::string_view::npos) eol = upper_content.size();
        std::string_view line(upper_content.data() + pos, eol - pos);

        if (lang == SourceLang::COBOL || lang == SourceLang::Copybook) {
            // COPY copybook-name
            auto copy_pos = line.find("COPY ");
            if (copy_pos != std::string_view::npos) {
                std::string target = extract_word_after(line, "COPY ");
                if (!target.empty()) {
                    deps.push_back({fname, target, "COPY"});
                }
            }
            // CALL 'program-name' or CALL program-name
            auto call_pos = line.find("CALL ");
            if (call_pos != std::string_view::npos) {
                std::string target = extract_quoted_or_word(line, "CALL ");
                if (!target.empty()) {
                    deps.push_back({fname, target, "CALL"});
                }
            }
        }

        if (lang == SourceLang::JCL) {
            // EXEC PGM=program
            auto pgm_pos = line.find("PGM=");
            if (pgm_pos != std::string_view::npos) {
                std::string target = extract_word_after(line, "PGM=");
                if (!target.empty()) {
                    deps.push_back({fname, target, "EXEC PGM"});
                }
            }
            // EXEC PROC= or just EXEC procname (if no PGM=)
            auto exec_pos = line.find("EXEC ");
            if (exec_pos != std::string_view::npos && line.find("PGM=") == std::string_view::npos) {
                auto proc_eq = line.find("PROC=");
                if (proc_eq != std::string_view::npos) {
                    std::string target = extract_word_after(line, "PROC=");
                    if (!target.empty()) deps.push_back({fname, target, "PROC"});
                } else {
                    std::string target = extract_word_after(line, "EXEC ");
                    if (!target.empty() && target != "PGM" && target != "PROC") {
                        deps.push_back({fname, target, "PROC"});
                    }
                }
            }
        }

        if (lang == SourceLang::PLI) {
            auto inc_pos = line.find("%INCLUDE");
            if (inc_pos != std::string_view::npos) {
                std::string target = extract_word_after(line, "%INCLUDE");
                if (!target.empty()) {
                    deps.push_back({fname, target, "INCLUDE"});
                }
            }
        }

        if (lang == SourceLang::DB2DDL) {
            auto bind_pos = line.find("BIND ");
            if (bind_pos != std::string_view::npos) {
                std::string target = extract_word_after(line, "BIND ");
                if (!target.empty()) {
                    deps.push_back({fname, target, "BIND"});
                }
            }
        }

        pos = eol + 1;
    }

    return deps;
}

inline void populate_graph(DependencyGraph& graph, const std::vector<std::pair<std::string, std::string>>& files,
                           const std::vector<SourceLang>& langs) {
    for (size_t i = 0; i < files.size(); ++i) {
        graph.add_node(files[i].first);
        auto deps = extract_dependencies(files[i].first, files[i].second,
                                         i < langs.size() ? langs[i] : SourceLang::Unknown);
        for (const auto& d : deps) {
            graph.add_edge(d.source, d.target);
        }
    }
}

} // namespace dep_extract

}} // namespace lazarus::pipeline

#endif // LAZARUS_PIPELINE_GRAPH_H
