#ifndef LAZARUS_JCL_SHELL_TRANSLATOR_H
#define LAZARUS_JCL_SHELL_TRANSLATOR_H

#include "lazarus/jcl/parser.h"
#include "lazarus/jcl/translator.h"
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>

namespace lazarus { namespace jcl {

// Bash script generator from JCL AST
class ShellTranslator {
public:
    explicit ShellTranslator(const JclJob& job) : job_(job) {}

    // Generate complete bash script
    std::string generate() const {
        std::ostringstream out;
        emit_shebang(out);
        emit_header_comment(out);
        emit_error_handling(out);
        emit_dd_variables(out);
        emit_step_functions(out);
        emit_main(out);
        return out.str();
    }

    // Generate just the step functions
    std::string generate_step_functions() const {
        std::ostringstream out;
        emit_step_functions(out);
        return out.str();
    }

    // Generate DD variable declarations
    std::string generate_dd_vars() const {
        std::ostringstream out;
        emit_dd_variables(out);
        return out.str();
    }

    // Generate the main execution flow
    std::string generate_main() const {
        std::ostringstream out;
        emit_main(out);
        return out.str();
    }

    // Map a single DD to a bash variable assignment
    static std::string dd_to_var(const JclDd& dd) {
        if (dd.params.is_dummy) {
            return "DD_" + to_upper_str(dd.name) + "=/dev/null";
        }
        if (dd.params.is_instream) {
            return "DD_" + to_upper_str(dd.name) + "=$(mktemp)";
        }
        if (!dd.params.dsn.empty()) {
            return "DD_" + to_upper_str(dd.name) + "=\"" +
                   Translator::dsn_to_path(dd.params.dsn) + "\"";
        }
        if (!dd.params.sysout.empty()) {
            return "DD_" + to_upper_str(dd.name) + "=/dev/stdout";
        }
        return "DD_" + to_upper_str(dd.name) + "=\"\"";
    }

    // Map PGM= to program invocation
    static std::string pgm_to_command(const std::string& pgm,
                                       const std::map<std::string, std::string>& params) {
        std::string cmd = pgm;
        auto parm_it = params.find("PARM");
        if (parm_it != params.end()) {
            cmd += " " + parm_it->second;
        }
        return cmd;
    }

    // Generate COND test expression for bash
    static std::string cond_to_test(const std::string& cond_str) {
        auto evals = parse_cond_param(cond_str);
        if (evals.empty()) return "true";

        std::ostringstream out;
        for (size_t i = 0; i < evals.size(); ++i) {
            if (i > 0) out << " || ";
            const auto& e = evals[i];
            std::string bash_op;
            if (e.op == "GT") bash_op = "-gt";
            else if (e.op == "GE") bash_op = "-ge";
            else if (e.op == "LT") bash_op = "-lt";
            else if (e.op == "LE") bash_op = "-le";
            else if (e.op == "EQ") bash_op = "-eq";
            else if (e.op == "NE") bash_op = "-ne";
            out << "[ \"$RC_" << to_upper_str(e.step) << "\" " << bash_op
                << " " << e.code << " ]";
        }
        return out.str();
    }

    // Generate DISP handling commands
    static std::string disp_commands(const JclDd& dd, const std::string& var_name,
                                      bool is_cleanup) {
        auto disp = parse_disp_action(dd.params);
        std::ostringstream out;

        if (!is_cleanup) {
            // Pre-step: create directories for NEW
            if (disp.creates_file()) {
                out << "    mkdir -p \"$(dirname \"$" << var_name << "\")\"\n";
            }
        } else {
            // Post-step cleanup
            if (disp.deletes_on_success()) {
                out << "    if [ $? -eq 0 ]; then\n";
                out << "        rm -f \"$" << var_name << "\"\n";
                out << "    fi\n";
            }
        }
        return out.str();
    }

private:
    JclJob job_;

    static std::string to_upper_str(const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return r;
    }

    static std::string to_lower_str(const std::string& s) {
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

    void emit_shebang(std::ostringstream& out) const {
        out << "#!/bin/bash\n";
    }

    void emit_header_comment(std::ostringstream& out) const {
        out << "# Translated from JCL job: " << job_.name << "\n";
        out << "# Steps: " << job_.steps.size() << "\n\n";
    }

    void emit_error_handling(std::ostringstream& out) const {
        out << "set -euo pipefail\n";
        out << "trap 'echo \"JOB " << job_.name
            << " FAILED at step $CURRENT_STEP with RC=$?\" >&2; exit 1' ERR\n";
        out << "CURRENT_STEP=\"INIT\"\n";
        out << "MAX_RC=0\n\n";
    }

    void emit_dd_variables(std::ostringstream& out) const {
        out << "# DD-to-file mappings\n";
        std::set<std::string> emitted;
        for (const auto& step : job_.steps) {
            for (const auto& dd : step.dd_statements) {
                std::string var_name = "DD_" + to_upper_str(dd.name);
                if (emitted.count(var_name)) continue;
                emitted.insert(var_name);
                out << dd_to_var(dd) << "\n";
            }
        }

        // Emit instream data as heredocs
        for (const auto& step : job_.steps) {
            for (const auto& dd : step.dd_statements) {
                if (dd.params.is_instream && !dd.params.instream_data.empty()) {
                    std::string var_name = "DD_" + to_upper_str(dd.name);
                    out << "cat > \"$" << var_name << "\" <<'__INSTREAM_EOF__'\n";
                    for (const auto& line : dd.params.instream_data) {
                        out << line << "\n";
                    }
                    out << "__INSTREAM_EOF__\n";
                }
            }
        }
        out << "\n";
    }

    void emit_step_functions(std::ostringstream& out) const {
        for (const auto& step : job_.steps) {
            std::string fn_name = "step_" + to_lower_str(sanitize_name(step.name));
            out << fn_name << "() {\n";
            out << "    local step_rc=0\n";

            // Pre-step DISP handling
            for (const auto& dd : step.dd_statements) {
                if (!dd.params.dsn.empty() && !dd.params.is_dummy) {
                    auto disp = parse_disp_action(dd.params);
                    if (disp.creates_file()) {
                        std::string path = Translator::dsn_to_path(dd.params.dsn);
                        out << "    mkdir -p \"$(dirname '" << path << "')\"\n";
                    }
                }
            }

            // Program execution
            if (!step.pgm.empty()) {
                std::string cmd = pgm_to_command(step.pgm, step.exec_params);
                out << "    # EXEC PGM=" << step.pgm << "\n";

                // Build I/O redirections from DD statements
                bool has_sysut1 = false;
                bool has_sysut2 = false;
                bool has_sysin = false;
                bool has_sysprint = false;
                for (const auto& dd : step.dd_statements) {
                    std::string n = to_upper_str(dd.name);
                    if (n == "SYSUT1") has_sysut1 = true;
                    if (n == "SYSUT2") has_sysut2 = true;
                    if (n == "SYSIN") has_sysin = true;
                    if (n == "SYSPRINT") has_sysprint = true;
                }

                if (has_sysin && has_sysprint) {
                    out << "    " << cmd << " < \"$DD_SYSIN\" > \"$DD_SYSPRINT\" 2>&1\n";
                } else if (has_sysut1 && has_sysut2) {
                    out << "    " << cmd << " < \"$DD_SYSUT1\" > \"$DD_SYSUT2\"\n";
                } else {
                    out << "    " << cmd << "\n";
                }
                out << "    step_rc=$?\n";
            } else if (!step.proc.empty()) {
                out << "    # EXEC PROC=" << step.proc << "\n";
                out << "    step_rc=0\n";
            }

            // Post-step DISP cleanup
            for (const auto& dd : step.dd_statements) {
                if (!dd.params.dsn.empty() && !dd.params.is_dummy) {
                    auto disp = parse_disp_action(dd.params);
                    if (disp.deletes_on_success()) {
                        out << "    if [ $step_rc -eq 0 ]; then rm -f \"$DD_"
                            << to_upper_str(dd.name) << "\"; fi\n";
                    }
                }
            }

            out << "    return $step_rc\n";
            out << "}\n\n";
        }
    }

    void emit_main(std::ostringstream& out) const {
        out << "# Main execution\n";
        out << "main() {\n";
        out << "    echo \"JOB " << job_.name << " STARTED\"\n";

        for (const auto& step : job_.steps) {
            std::string fn_name = "step_" + to_lower_str(sanitize_name(step.name));
            std::string rc_var = "RC_" + to_upper_str(sanitize_name(step.name));

            auto cond_it = step.exec_params.find("COND");
            if (cond_it != step.exec_params.end()) {
                std::string test = cond_to_test(cond_it->second);
                out << "    CURRENT_STEP=\"" << step.name << "\"\n";
                out << "    if " << test << "; then\n";
                out << "        echo \"STEP " << step.name << " SKIPPED (COND)\"\n";
                out << "        " << rc_var << "=0\n";
                out << "    else\n";
                out << "        " << fn_name << "\n";
                out << "        " << rc_var << "=$?\n";
                out << "    fi\n";
            } else {
                out << "    CURRENT_STEP=\"" << step.name << "\"\n";
                out << "    " << fn_name << "\n";
                out << "    " << rc_var << "=$?\n";
            }
            out << "    if [ \"$" << rc_var << "\" -gt \"$MAX_RC\" ]; then MAX_RC=$"
                << rc_var << "; fi\n";
        }

        out << "    echo \"JOB " << job_.name << " ENDED RC=$MAX_RC\"\n";
        out << "    exit $MAX_RC\n";
        out << "}\n\n";
        out << "main \"$@\"\n";
    }
};

}} // namespace lazarus::jcl

#endif // LAZARUS_JCL_SHELL_TRANSLATOR_H
