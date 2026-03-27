#ifndef LAZARUS_JCL_PROC_RESOLVER_H
#define LAZARUS_JCL_PROC_RESOLVER_H

#include "lazarus/jcl/parser.h"
#include <string>
#include <map>
#include <vector>
#include <stdexcept>
#include <regex>
#include <functional>

namespace lazarus { namespace jcl {

// Scope for symbolic parameter resolution
struct SymbolScope {
    std::string name;  // scope name (proc name or "JOB")
    std::map<std::string, std::string> symbols;
};

class ProcResolver {
public:
    using ProcLibrary = std::map<std::string, JclProc>;

    ProcResolver() = default;

    // Register a cataloged procedure
    void add_proc(const std::string& name, const JclProc& proc) {
        std::string upper = to_upper(name);
        proc_lib_[upper] = proc;
    }

    // Register a proc from JCL text
    void add_proc_from_text(const std::string& name, std::string_view jcl_text) {
        Lexer lex(jcl_text);
        Parser parser(lex.tokens());
        JclProc proc = parser.parse_proc();
        proc.name = name;
        add_proc(name, proc);
    }

    // Resolve all proc calls in a job, expanding steps and substituting symbols
    JclJob resolve(const JclJob& job,
                   const std::map<std::string, std::string>& overrides = {}) {
        JclJob resolved = job;
        resolved.steps.clear();

        // Build job-level symbol scope
        SymbolScope job_scope;
        job_scope.name = "JOB";
        job_scope.symbols = overrides;
        scope_stack_.clear();
        scope_stack_.push_back(job_scope);

        for (const auto& step : job.steps) {
            if (step.is_proc_call) {
                auto expanded = expand_proc_call(step);
                for (auto& s : expanded) {
                    resolved.steps.push_back(std::move(s));
                }
            } else {
                auto resolved_step = resolve_step_symbols(step);
                resolved.steps.push_back(std::move(resolved_step));
            }
        }

        scope_stack_.pop_back();
        return resolved;
    }

    // Simple symbolic substitution in a string
    static std::string substitute(const std::string& text,
                                   const std::map<std::string, std::string>& symbols) {
        std::string result = text;
        // Process &VAR and &VAR. patterns (dot is consumed if present)
        for (const auto& [var, val] : symbols) {
            // Try &VAR. first (dot-terminated)
            std::string pat_dot = "&" + var + ".";
            size_t pos = 0;
            while ((pos = result.find(pat_dot, pos)) != std::string::npos) {
                result.replace(pos, pat_dot.size(), val);
                pos += val.size();
            }
            // Then &VAR (non-dot, must be followed by non-alnum or end)
            std::string pat = "&" + var;
            pos = 0;
            while ((pos = result.find(pat, pos)) != std::string::npos) {
                size_t after = pos + pat.size();
                if (after < result.size() && (std::isalnum(static_cast<unsigned char>(result[after]))
                    || result[after] == '_')) {
                    pos = after;
                    continue;
                }
                result.replace(pos, pat.size(), val);
                pos += val.size();
            }
        }
        return result;
    }

    // Resolve referback: *.stepname.ddname -> actual DSN
    static std::string resolve_referback(const std::string& ref,
                                          const std::vector<JclStep>& steps) {
        // Format: *.STEPNAME.DDNAME
        if (ref.size() < 3 || ref[0] != '*' || ref[1] != '.') return ref;

        std::string path = ref.substr(2);
        size_t dot = path.find('.');
        if (dot == std::string::npos) return ref;

        std::string step_name = to_upper(path.substr(0, dot));
        std::string dd_name = to_upper(path.substr(dot + 1));

        for (const auto& step : steps) {
            if (to_upper(step.name) == step_name) {
                for (const auto& dd : step.dd_statements) {
                    if (to_upper(dd.name) == dd_name) {
                        return dd.params.dsn;
                    }
                }
            }
        }
        return ref; // Unresolved
    }

    // Resolve referbacks in all DD statements across all steps
    static void resolve_all_referbacks(std::vector<JclStep>& steps) {
        for (auto& step : steps) {
            for (auto& dd : step.dd_statements) {
                if (dd.params.is_referback && !dd.params.referback.empty()) {
                    std::string resolved = resolve_referback(dd.params.referback, steps);
                    if (resolved != dd.params.referback) {
                        dd.params.dsn = resolved;
                        dd.params.is_referback = false;
                    }
                }
            }
        }
    }

    const ProcLibrary& proc_library() const { return proc_lib_; }

private:
    ProcLibrary proc_lib_;
    std::vector<SymbolScope> scope_stack_;

    // Merge all symbol scopes (innermost takes precedence)
    std::map<std::string, std::string> merged_symbols() const {
        std::map<std::string, std::string> merged;
        for (const auto& scope : scope_stack_) {
            for (const auto& [k, v] : scope.symbols) {
                merged[k] = v;
            }
        }
        return merged;
    }

    std::string resolve_string(const std::string& s) const {
        return substitute(s, merged_symbols());
    }

    JclDd resolve_dd_symbols(const JclDd& dd) const {
        JclDd resolved = dd;
        resolved.params.dsn = resolve_string(dd.params.dsn);
        resolved.params.disp = resolve_string(dd.params.disp);
        resolved.params.space = resolve_string(dd.params.space);
        resolved.params.dcb = resolve_string(dd.params.dcb);
        resolved.params.sysout = resolve_string(dd.params.sysout);
        resolved.params.unit = resolve_string(dd.params.unit);
        resolved.params.vol = resolve_string(dd.params.vol);
        resolved.params.referback = resolve_string(dd.params.referback);
        for (auto& [k, v] : resolved.params.extra_params) {
            v = resolve_string(v);
        }
        return resolved;
    }

    JclStep resolve_step_symbols(const JclStep& step) const {
        JclStep resolved = step;
        resolved.pgm = resolve_string(step.pgm);
        resolved.proc = resolve_string(step.proc);
        for (auto& [k, v] : resolved.exec_params) {
            v = resolve_string(v);
        }
        resolved.dd_statements.clear();
        for (const auto& dd : step.dd_statements) {
            resolved.dd_statements.push_back(resolve_dd_symbols(dd));
        }
        return resolved;
    }

    std::vector<JclStep> expand_proc_call(const JclStep& call_step) {
        std::string proc_name = to_upper(call_step.proc);
        auto it = proc_lib_.find(proc_name);
        if (it == proc_lib_.end()) {
            // Unknown proc: return as-is with a single step
            return {call_step};
        }

        const JclProc& proc = it->second;

        // Build scope: start with PROC defaults, then layer EXEC overrides
        // but these go BELOW the JOB scope (job-level overrides win)
        SymbolScope proc_scope;
        proc_scope.name = proc_name;
        proc_scope.symbols = proc.defaults;

        // Apply overrides from EXEC params
        for (const auto& [k, v] : call_step.exec_params) {
            proc_scope.symbols[k] = v;
        }

        // Also apply job-level overrides (from scope_stack_[0]) into proc scope
        // so they take precedence over proc defaults
        if (!scope_stack_.empty()) {
            for (const auto& [k, v] : scope_stack_[0].symbols) {
                proc_scope.symbols[k] = v;
            }
        }

        scope_stack_.push_back(proc_scope);

        std::vector<JclStep> expanded;
        for (const auto& proc_step : proc.steps) {
            JclStep resolved = resolve_step_symbols(proc_step);
            // Prefix step name with caller's step name for qualification
            if (!call_step.name.empty() && !resolved.name.empty()) {
                resolved.name = call_step.name + "." + resolved.name;
            }

            // Apply DD overrides from the calling step
            apply_dd_overrides(resolved, call_step.dd_statements);

            expanded.push_back(std::move(resolved));
        }

        scope_stack_.pop_back();
        return expanded;
    }

    void apply_dd_overrides(JclStep& step, const std::vector<JclDd>& overrides) {
        for (const auto& ovr : overrides) {
            bool found = false;
            for (auto& dd : step.dd_statements) {
                if (to_upper(dd.name) == to_upper(ovr.name)) {
                    // Override: merge non-empty params
                    if (!ovr.params.dsn.empty()) dd.params.dsn = ovr.params.dsn;
                    if (!ovr.params.disp.empty()) dd.params.disp = ovr.params.disp;
                    if (!ovr.params.space.empty()) dd.params.space = ovr.params.space;
                    if (!ovr.params.dcb.empty()) dd.params.dcb = ovr.params.dcb;
                    if (!ovr.params.sysout.empty()) dd.params.sysout = ovr.params.sysout;
                    dd.is_override = true;
                    found = true;
                    break;
                }
            }
            if (!found) {
                // Add new DD
                JclDd new_dd = ovr;
                new_dd.is_override = true;
                step.dd_statements.push_back(new_dd);
            }
        }
    }
};

}} // namespace lazarus::jcl

#endif // LAZARUS_JCL_PROC_RESOLVER_H
