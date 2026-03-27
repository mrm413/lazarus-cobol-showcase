#ifndef LAZARUS_REXX_RUNTIME_H
#define LAZARUS_REXX_RUNTIME_H

#include "lazarus/rexx/ast.h"
#include "lazarus/rexx/builtins.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace lazarus { namespace rexx {

// ── Stem variable (compound symbol) ────────────────────────────────────────
class RexxStem {
public:
    RexxValue default_value;

    RexxStem() : default_value(RexxValue("")) {}
    explicit RexxStem(const RexxValue& def) : default_value(def) {}

    // Build compound key from parts: "stem.a.b.c" -> "A.B.C"
    static std::string build_key(const std::vector<std::string>& parts) {
        std::string key;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i > 0) key += '.';
            std::string p = parts[i];
            std::transform(p.begin(), p.end(), p.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
            key += p;
        }
        return key;
    }

    void set(const std::string& key, const RexxValue& val) {
        data_[key] = val;
    }

    RexxValue get(const std::string& key) const {
        auto it = data_.find(key);
        if (it != data_.end()) return it->second;
        return default_value;
    }

    bool exists(const std::string& key) const {
        return data_.count(key) > 0;
    }

    void drop(const std::string& key) {
        data_.erase(key);
    }

    void drop_all() {
        data_.clear();
    }

    size_t size() const { return data_.size(); }

    const std::unordered_map<std::string, RexxValue>& entries() const {
        return data_;
    }

private:
    std::unordered_map<std::string, RexxValue> data_;
};

// ── External Data Queue ────────────────────────────────────────────────────
class ExternalQueue {
public:
    // PUSH (LIFO — add to front)
    void push(const RexxValue& val) {
        queue_.push_front(val);
    }

    // QUEUE (FIFO — add to back)
    void queue(const RexxValue& val) {
        queue_.push_back(val);
    }

    // PULL — remove from front
    RexxValue pull() {
        if (queue_.empty()) return RexxValue(std::string(""));
        RexxValue v = queue_.front();
        queue_.pop_front();
        return v;
    }

    int queued() const { return static_cast<int>(queue_.size()); }
    bool empty() const { return queue_.empty(); }
    void clear() { queue_.clear(); }

private:
    std::deque<RexxValue> queue_;
};

// ── TRACE settings ─────────────────────────────────────────────────────────
enum class TraceLevel {
    All,        // A — all clauses
    Commands,   // C — commands
    Error,      // E — errors
    Failure,    // F — failures
    Intermediates, // I — intermediate results
    Labels,     // L — labels
    Normal,     // N — normal (default)
    Off,        // O — off
    Results,    // R — results
    Scan        // S — scan
};

inline TraceLevel parse_trace_level(const std::string& s) {
    if (s.empty()) return TraceLevel::Normal;
    char c = static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    switch (c) {
        case 'A': return TraceLevel::All;
        case 'C': return TraceLevel::Commands;
        case 'E': return TraceLevel::Error;
        case 'F': return TraceLevel::Failure;
        case 'I': return TraceLevel::Intermediates;
        case 'L': return TraceLevel::Labels;
        case 'N': return TraceLevel::Normal;
        case 'O': return TraceLevel::Off;
        case 'R': return TraceLevel::Results;
        case 'S': return TraceLevel::Scan;
        default:  return TraceLevel::Normal;
    }
}

inline std::string trace_level_name(TraceLevel t) {
    switch (t) {
        case TraceLevel::All:           return "A";
        case TraceLevel::Commands:      return "C";
        case TraceLevel::Error:         return "E";
        case TraceLevel::Failure:       return "F";
        case TraceLevel::Intermediates: return "I";
        case TraceLevel::Labels:        return "L";
        case TraceLevel::Normal:        return "N";
        case TraceLevel::Off:           return "O";
        case TraceLevel::Results:       return "R";
        case TraceLevel::Scan:          return "S";
    }
    return "N";
}

// ── Condition trap ─────────────────────────────────────────────────────────
struct ConditionTrap {
    bool enabled = false;
    bool is_signal = false;   // true=SIGNAL, false=CALL
    std::string label;
};

// ── ADDRESS routing ────────────────────────────────────────────────────────
using AddressHandler = std::function<int(const std::string& command)>;

// ── EXECIO operations ──────────────────────────────────────────────────────
struct ExecioResult {
    int rc = 0;
    std::vector<std::string> lines;
};

inline ExecioResult execio_diskr(const std::string& filename, int count = -1) {
    ExecioResult result;
    std::ifstream file(filename);
    if (!file.is_open()) { result.rc = 2; return result; }
    std::string line;
    int read = 0;
    while (std::getline(file, line)) {
        result.lines.push_back(line);
        ++read;
        if (count >= 0 && read >= count) break;
    }
    return result;
}

inline int execio_diskw(const std::string& filename,
                         const std::vector<std::string>& lines,
                         bool append = false) {
    auto mode = append ? std::ios::app : std::ios::trunc;
    std::ofstream file(filename, mode);
    if (!file.is_open()) return 2;
    for (const auto& line : lines) {
        file << line << "\n";
    }
    return 0;
}

// ── REXX Runtime Environment ───────────────────────────────────────────────
class RexxRuntime {
public:
    // Variable management
    void set_var(const std::string& name, const RexxValue& val) {
        std::string upper = to_upper_str(name);
        variables_[upper] = val;
    }

    RexxValue get_var(const std::string& name) const {
        std::string upper = to_upper_str(name);
        auto it = variables_.find(upper);
        if (it != variables_.end()) return it->second;
        // NOVALUE: return the variable name uppercased
        return RexxValue(upper);
    }

    bool var_exists(const std::string& name) const {
        std::string upper = to_upper_str(name);
        return variables_.count(upper) > 0;
    }

    void drop_var(const std::string& name) {
        std::string upper = to_upper_str(name);
        variables_.erase(upper);
    }

    // Stem management
    RexxStem& get_stem(const std::string& name) {
        std::string upper = to_upper_str(name);
        return stems_[upper];
    }

    void set_stem_value(const std::string& stem, const std::vector<std::string>& tail,
                        const RexxValue& val) {
        std::string upper = to_upper_str(stem);
        std::string key = RexxStem::build_key(tail);
        stems_[upper].set(key, val);
    }

    RexxValue get_stem_value(const std::string& stem,
                              const std::vector<std::string>& tail) const {
        std::string upper = to_upper_str(stem);
        auto it = stems_.find(upper);
        if (it == stems_.end()) {
            // Return "STEM.TAIL" (unset compound)
            return RexxValue(upper + "." + RexxStem::build_key(tail));
        }
        return it->second.get(RexxStem::build_key(tail));
    }

    // External queue
    ExternalQueue& get_queue() { return queue_; }

    // ADDRESS routing
    void register_address(const std::string& env, AddressHandler handler) {
        std::string upper = to_upper_str(env);
        address_handlers_[upper] = std::move(handler);
    }

    int execute_address(const std::string& env, const std::string& command) {
        std::string upper = to_upper_str(env);
        auto it = address_handlers_.find(upper);
        if (it != address_handlers_.end()) {
            return it->second(command);
        }
        return -1; // No handler
    }

    std::string current_address() const { return current_address_; }
    void set_current_address(const std::string& env) {
        current_address_ = to_upper_str(env);
    }

    // TRACE
    TraceLevel trace_level() const { return trace_; }
    void set_trace(TraceLevel t) { trace_ = t; }
    void set_trace(const std::string& s) { trace_ = parse_trace_level(s); }

    // NUMERIC settings
    int numeric_digits() const { return numeric_digits_; }
    int numeric_fuzz() const { return numeric_fuzz_; }
    std::string numeric_form() const { return numeric_form_; }
    void set_numeric_digits(int d) { numeric_digits_ = d; }
    void set_numeric_fuzz(int f) { numeric_fuzz_ = f; }
    void set_numeric_form(const std::string& f) { numeric_form_ = f; }

    // Condition traps
    void set_condition_trap(ConditionType ct, bool is_signal, const std::string& label) {
        auto& trap = traps_[ct];
        trap.enabled = true;
        trap.is_signal = is_signal;
        trap.label = label;
    }

    void clear_condition_trap(ConditionType ct) {
        traps_[ct].enabled = false;
    }

    bool has_condition_trap(ConditionType ct) const {
        auto it = traps_.find(ct);
        return it != traps_.end() && it->second.enabled;
    }

    const ConditionTrap& get_condition_trap(ConditionType ct) const {
        static ConditionTrap empty;
        auto it = traps_.find(ct);
        if (it != traps_.end()) return it->second;
        return empty;
    }

    // INTERPRET: simple dispatch table for dynamic eval
    using InterpretHandler = std::function<RexxValue(const std::string&)>;

    void register_interpret_handler(InterpretHandler handler) {
        interpret_handler_ = std::move(handler);
    }

    RexxValue interpret(const std::string& code) {
        if (interpret_handler_) return interpret_handler_(code);
        // Minimal: try to evaluate as SAY literal
        return RexxValue(code);
    }

    // EXECIO interface
    ExecioResult execio_read(const std::string& filename, int count = -1) {
        return execio_diskr(filename, count);
    }

    int execio_write(const std::string& filename,
                     const std::vector<std::string>& lines,
                     bool append = false) {
        return execio_diskw(filename, lines, append);
    }

    // Subroutine/label tracking
    void register_label(const std::string& name, size_t index) {
        std::string upper = to_upper_str(name);
        labels_[upper] = index;
    }

    bool has_label(const std::string& name) const {
        return labels_.count(to_upper_str(name)) > 0;
    }

    size_t get_label_index(const std::string& name) const {
        auto it = labels_.find(to_upper_str(name));
        if (it != labels_.end()) return it->second;
        throw std::runtime_error("Label not found: " + name);
    }

    // Output capture for testing
    std::vector<std::string>& output() { return output_; }
    void say(const std::string& text) { output_.push_back(text); }

    // PARSE template application
    void apply_template(const std::string& source,
                        const std::vector<TemplateItem>& tmpl) {
        size_t pos = 0;
        for (size_t i = 0; i < tmpl.size(); ++i) {
            const auto& item = tmpl[i];
            switch (item.type) {
                case TemplateItemType::Variable: {
                    // Find delimiter from next item
                    size_t end = source.size();
                    if (i + 1 < tmpl.size()) {
                        const auto& next = tmpl[i + 1];
                        if (next.type == TemplateItemType::LiteralDelim) {
                            size_t found = source.find(next.value, pos);
                            if (found != std::string::npos) end = found;
                        } else if (next.type == TemplateItemType::Variable ||
                                   next.type == TemplateItemType::Period) {
                            // Word-split: take next word
                            size_t ws = source.find(' ', pos);
                            if (ws != std::string::npos) end = ws;
                        } else if (next.type == TemplateItemType::Positional) {
                            end = static_cast<size_t>(next.position - 1);
                            if (end > source.size()) end = source.size();
                        }
                    }
                    // For the last variable with no delimiter, get remaining
                    std::string val = (pos < source.size()) ?
                        source.substr(pos, end - pos) : "";
                    // Strip leading/trailing spaces for word parse
                    while (!val.empty() && val.front() == ' ') val.erase(val.begin());
                    while (!val.empty() && val.back() == ' ') val.pop_back();
                    set_var(item.value, RexxValue(val));
                    pos = end;
                    // Skip past delimiter space
                    if (pos < source.size() && source[pos] == ' ') ++pos;
                    break;
                }
                case TemplateItemType::LiteralDelim: {
                    size_t found = source.find(item.value, pos);
                    if (found != std::string::npos) {
                        pos = found + item.value.size();
                    }
                    break;
                }
                case TemplateItemType::Positional: {
                    pos = static_cast<size_t>(item.position - 1);
                    if (pos > source.size()) pos = source.size();
                    break;
                }
                case TemplateItemType::RelativePos: {
                    if (item.position >= 0)
                        pos += static_cast<size_t>(item.position);
                    else {
                        size_t abs_off = static_cast<size_t>(-item.position);
                        pos = (pos >= abs_off) ? pos - abs_off : 0;
                    }
                    break;
                }
                case TemplateItemType::Period: {
                    // Placeholder: skip a word
                    size_t ws = source.find(' ', pos);
                    pos = (ws != std::string::npos) ? ws + 1 : source.size();
                    break;
                }
            }
        }
    }

private:
    std::unordered_map<std::string, RexxValue> variables_;
    std::unordered_map<std::string, RexxStem> stems_;
    ExternalQueue queue_;
    std::unordered_map<std::string, AddressHandler> address_handlers_;
    std::string current_address_ = "SYSTEM";
    TraceLevel trace_ = TraceLevel::Normal;
    int numeric_digits_ = 9;
    int numeric_fuzz_ = 0;
    std::string numeric_form_ = "SCIENTIFIC";
    std::unordered_map<ConditionType, ConditionTrap> traps_;
    InterpretHandler interpret_handler_;
    std::unordered_map<std::string, size_t> labels_;
    std::vector<std::string> output_;

    static std::string to_upper_str(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return r;
    }
};

}} // namespace lazarus::rexx

#endif // LAZARUS_REXX_RUNTIME_H
