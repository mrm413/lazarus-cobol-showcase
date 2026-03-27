#ifndef LAZARUS_JCL_TRANSLATOR_H
#define LAZARUS_JCL_TRANSLATOR_H

#include "lazarus/jcl/parser.h"
#include "lazarus/jcl/proc_resolver.h"
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>

namespace lazarus { namespace jcl {

// Result of translating a step
struct StepResult {
    int rc;
    std::string output_path;
};

// Evaluation of COND parameter for step skipping
struct CondEval {
    int code;
    std::string op;     // GT, GE, LT, LE, EQ, NE
    std::string step;

    bool should_skip(int step_rc) const {
        if (op == "GT") return step_rc > code;
        if (op == "GE") return step_rc >= code;
        if (op == "LT") return step_rc < code;
        if (op == "LE") return step_rc <= code;
        if (op == "EQ") return step_rc == code;
        if (op == "NE") return step_rc != code;
        return false;
    }
};

inline std::vector<std::string> split_csv(const std::string& s) {
    std::vector<std::string> parts;
    size_t start = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        if (i == s.size() || s[i] == ',') {
            std::string part = s.substr(start, i - start);
            // trim
            while (!part.empty() && part.front() == ' ') part.erase(part.begin());
            while (!part.empty() && part.back() == ' ') part.pop_back();
            parts.push_back(part);
            start = i + 1;
        }
    }
    return parts;
}

// Parse a COND string like "(4,LT,STEP1)" into CondEval
inline std::vector<CondEval> parse_cond_param(const std::string& cond_str) {
    std::vector<CondEval> evals;
    std::string s = cond_str;

    // Strip outer parens
    while (!s.empty() && s.front() == '(') s = s.substr(1);
    while (!s.empty() && s.back() == ')') s.pop_back();

    // Check for nested conditions
    if (!s.empty() && s.front() == '(') {
        // Multiple: (4,LT,STEP1),(0,NE,STEP2)
        size_t pos = 0;
        while (pos < s.size()) {
            if (s[pos] == '(') {
                size_t end = s.find(')', pos);
                if (end != std::string::npos) {
                    std::string inner = s.substr(pos + 1, end - pos - 1);
                    auto parts = split_csv(inner);
                    if (parts.size() >= 3) {
                        CondEval ce;
                        try { ce.code = std::stoi(parts[0]); } catch (...) { ce.code = 0; }
                        ce.op = to_upper(parts[1]);
                        ce.step = parts[2];
                        evals.push_back(ce);
                    }
                    pos = end + 1;
                } else break;
            } else ++pos;
        }
    } else {
        auto parts = split_csv(s);
        if (parts.size() >= 3) {
            CondEval ce;
            try { ce.code = std::stoi(parts[0]); } catch (...) { ce.code = 0; }
            ce.op = to_upper(parts[1]);
            ce.step = parts[2];
            evals.push_back(ce);
        }
    }
    return evals;
}

// Maps DISP status to filesystem operations
struct DispAction {
    std::string status;    // NEW, OLD, SHR, MOD
    std::string normal;    // CATLG, KEEP, DELETE, PASS, UNCATLG
    std::string abnormal;  // CATLG, KEEP, DELETE, UNCATLG

    bool creates_file() const { return status == "NEW"; }
    bool deletes_on_success() const { return normal == "DELETE"; }
    bool deletes_on_failure() const { return abnormal == "DELETE"; }
    bool keeps_on_success() const {
        return normal == "CATLG" || normal == "KEEP" || normal == "PASS" || normal.empty();
    }
};

inline DispAction parse_disp_action(const DdParams& dd) {
    DispAction da;
    da.status = dd.disp_status.empty() ? "NEW" : dd.disp_status;
    da.normal = dd.disp_normal.empty() ? "KEEP" : dd.disp_normal;
    da.abnormal = dd.disp_abnormal.empty() ? da.normal : dd.disp_abnormal;
    return da;
}

// C++17 code generator
class Translator {
public:
    explicit Translator(const JclJob& job) : job_(job) {}

    // Set the namespace for generated code
    void set_namespace(const std::string& ns) { namespace_ = ns; }

    // Set output directory for generated files
    void set_output_dir(const std::string& dir) { output_dir_ = dir; }

    // Generate complete C++17 source for the job
    std::string generate() const {
        std::ostringstream out;
        emit_header(out);
        emit_class_open(out);
        emit_dd_mappings(out);
        emit_step_methods(out);
        emit_execute(out);
        emit_class_close(out);
        return out.str();
    }

    // Generate just the execute() method body for inspection
    std::string generate_execute_body() const {
        std::ostringstream out;
        emit_execute_body(out, "    ");
        return out.str();
    }

    // Generate DD->path mapping code
    std::string generate_dd_map() const {
        std::ostringstream out;
        for (const auto& step : job_.steps) {
            for (const auto& dd : step.dd_statements) {
                if (!dd.params.dsn.empty() && !dd.params.is_dummy && !dd.params.is_instream) {
                    out << "dd_map[\"" << dd.name << "\"] = \""
                        << dsn_to_path(dd.params.dsn) << "\";\n";
                }
            }
        }
        return out.str();
    }

    // Convert DSN to filesystem path
    static std::string dsn_to_path(const std::string& dsn) {
        std::string path = dsn;
        // Replace dots with path separators
        for (auto& c : path) {
            if (c == '.') c = '/';
        }
        // Lowercase
        std::transform(path.begin(), path.end(), path.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return path;
    }

    // Generate DISP RAII wrapper code
    static std::string generate_disp_raii(const std::string& varname,
                                           const std::string& path,
                                           const DispAction& disp) {
        std::ostringstream out;
        if (disp.creates_file()) {
            out << "// DISP=(NEW," << disp.normal << "," << disp.abnormal << ")\n";
            out << "std::filesystem::path " << varname << "(\"" << path << "\");\n";
            out << "auto " << varname << "_guard = lazarus::make_disp_guard(\n"
                << "    " << varname << ",\n"
                << "    /*keep_on_success=*/" << (disp.keeps_on_success() ? "true" : "false") << ",\n"
                << "    /*delete_on_failure=*/" << (disp.deletes_on_failure() ? "true" : "false")
                << ");\n";
        } else {
            out << "// DISP=(" << disp.status << "," << disp.normal << "," << disp.abnormal << ")\n";
            out << "std::filesystem::path " << varname << "(\"" << path << "\");\n";
        }
        return out.str();
    }

    // Evaluate COND for step skipping - returns code fragment
    static std::string generate_cond_check(const std::string& cond_str,
                                            const std::string& step_name) {
        auto evals = parse_cond_param(cond_str);
        if (evals.empty()) return "";

        std::ostringstream out;
        out << "// COND check for " << step_name << "\n";
        out << "if (";
        for (size_t i = 0; i < evals.size(); ++i) {
            if (i > 0) out << " || ";
            const auto& e = evals[i];
            std::string cpp_op;
            if (e.op == "GT") cpp_op = ">";
            else if (e.op == "GE") cpp_op = ">=";
            else if (e.op == "LT") cpp_op = "<";
            else if (e.op == "LE") cpp_op = "<=";
            else if (e.op == "EQ") cpp_op = "==";
            else if (e.op == "NE") cpp_op = "!=";
            out << "rc_map[\"" << e.step << "\"] " << cpp_op << " " << e.code;
        }
        out << ") {\n";
        out << "    // Skip step " << step_name << "\n";
        out << "} else {\n";
        out << "    auto [rc, path] = step_" << to_lower(step_name) << "();\n";
        out << "    rc_map[\"" << step_name << "\"] = rc;\n";
        out << "}\n";
        return out.str();
    }

private:
    JclJob job_;
    std::string namespace_ = "lazarus::generated";
    std::string output_dir_ = "./output";

    static std::string to_lower(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return r;
    }

    static std::string sanitize_name(const std::string& name) {
        std::string r;
        for (char c : name) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') r += c;
            else r += '_';
        }
        return r;
    }

    void emit_header(std::ostringstream& out) const {
        out << "#pragma once\n";
        out << "#include <string>\n";
        out << "#include <map>\n";
        out << "#include <tuple>\n";
        out << "#include <filesystem>\n";
        out << "#include <fstream>\n";
        out << "#include <cstdlib>\n";
        out << "#include <stdexcept>\n\n";
    }

    void emit_class_open(std::ostringstream& out) const {
        std::string class_name = "Job_" + sanitize_name(job_.name);
        out << "namespace " << namespace_ << " {\n\n";
        out << "class " << class_name << " {\n";
        out << "public:\n";
        out << "    using StepResult = std::tuple<int, std::string>;\n\n";
    }

    void emit_dd_mappings(std::ostringstream& out) const {
        out << "    std::map<std::string, std::filesystem::path> dd_map;\n";
        out << "    std::map<std::string, int> rc_map;\n\n";

        out << "    void init_dd_map() {\n";
        for (const auto& step : job_.steps) {
            for (const auto& dd : step.dd_statements) {
                if (!dd.params.dsn.empty() && !dd.params.is_dummy && !dd.params.is_instream) {
                    out << "        dd_map[\"" << step.name << "." << dd.name
                        << "\"] = \"" << dsn_to_path(dd.params.dsn) << "\";\n";
                }
            }
        }
        out << "    }\n\n";
    }

    void emit_step_methods(std::ostringstream& out) const {
        for (const auto& step : job_.steps) {
            std::string method_name = "step_" + to_lower(sanitize_name(step.name));
            out << "    StepResult " << method_name << "() {\n";

            // DISP handling for DDs
            for (const auto& dd : step.dd_statements) {
                if (dd.params.is_dummy) {
                    out << "        // " << dd.name << " DD DUMMY\n";
                    continue;
                }
                if (dd.params.is_instream) {
                    out << "        // " << dd.name << " DD * (instream data)\n";
                    continue;
                }
                if (!dd.params.dsn.empty()) {
                    auto disp = parse_disp_action(dd.params);
                    std::string path = dsn_to_path(dd.params.dsn);
                    out << "        // " << dd.name << " DISP=("
                        << disp.status << "," << disp.normal << "," << disp.abnormal << ")\n";
                    out << "        std::filesystem::path dd_" << to_lower(dd.name)
                        << "(\"" << path << "\");\n";
                    if (disp.creates_file()) {
                        out << "        std::filesystem::create_directories(dd_"
                            << to_lower(dd.name) << ".parent_path());\n";
                    }
                }
            }

            // Program execution
            if (!step.pgm.empty()) {
                out << "        // EXEC PGM=" << step.pgm << "\n";
                out << "        int rc = 0; // " << step.pgm << " execution\n";
            } else if (!step.proc.empty()) {
                out << "        // EXEC PROC=" << step.proc << "\n";
                out << "        int rc = 0; // PROC call\n";
            } else {
                out << "        int rc = 0;\n";
            }

            out << "        return {rc, \"\"};\n";
            out << "    }\n\n";
        }
    }

    void emit_execute(std::ostringstream& out) const {
        out << "    int execute() {\n";
        out << "        init_dd_map();\n";
        emit_execute_body(out, "        ");
        out << "        // Return highest RC\n";
        out << "        int max_rc = 0;\n";
        out << "        for (const auto& [step, rc] : rc_map) {\n";
        out << "            if (rc > max_rc) max_rc = rc;\n";
        out << "        }\n";
        out << "        return max_rc;\n";
        out << "    }\n";
    }

    void emit_execute_body(std::ostringstream& out, const std::string& indent) const {
        for (const auto& step : job_.steps) {
            std::string method_name = "step_" + to_lower(sanitize_name(step.name));
            auto cond_it = step.exec_params.find("COND");

            if (cond_it != step.exec_params.end()) {
                auto evals = parse_cond_param(cond_it->second);
                if (!evals.empty()) {
                    out << indent << "// COND check for " << step.name << "\n";
                    out << indent << "{\n";
                    out << indent << "    bool skip = false;\n";
                    for (const auto& e : evals) {
                        std::string cpp_op;
                        if (e.op == "GT") cpp_op = ">";
                        else if (e.op == "GE") cpp_op = ">=";
                        else if (e.op == "LT") cpp_op = "<";
                        else if (e.op == "LE") cpp_op = "<=";
                        else if (e.op == "EQ") cpp_op = "==";
                        else if (e.op == "NE") cpp_op = "!=";
                        out << indent << "    if (rc_map.count(\"" << e.step
                            << "\") && rc_map[\"" << e.step << "\"] "
                            << cpp_op << " " << e.code << ") skip = true;\n";
                    }
                    out << indent << "    if (!skip) {\n";
                    out << indent << "        auto [rc, path] = " << method_name << "();\n";
                    out << indent << "        rc_map[\"" << step.name << "\"] = rc;\n";
                    out << indent << "    }\n";
                    out << indent << "}\n";
                    continue;
                }
            }

            out << indent << "{\n";
            out << indent << "    auto [rc, path] = " << method_name << "();\n";
            out << indent << "    rc_map[\"" << step.name << "\"] = rc;\n";
            out << indent << "}\n";
        }
    }

    void emit_class_close(std::ostringstream& out) const {
        out << "}; // class\n\n";
        out << "} // namespace " << namespace_ << "\n";
    }
};

}} // namespace lazarus::jcl

#endif // LAZARUS_JCL_TRANSLATOR_H
