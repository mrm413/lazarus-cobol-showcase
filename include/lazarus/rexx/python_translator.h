#ifndef LAZARUS_REXX_PYTHON_TRANSLATOR_H
#define LAZARUS_REXX_PYTHON_TRANSLATOR_H

#include "lazarus/rexx/ast.h"
#include "lazarus/rexx/lexer.h"
#include "lazarus/rexx/parser.h"
#include "lazarus/rexx/builtins.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace lazarus { namespace rexx {

class PythonTranslator {
public:
    PythonTranslator() {
        // Built-in function mapping REXX -> Python
        fn_map_["LENGTH"]    = "len";
        fn_map_["SUBSTR"]    = "rexx_substr";
        fn_map_["POS"]       = "rexx_pos";
        fn_map_["LEFT"]      = "rexx_left";
        fn_map_["RIGHT"]     = "rexx_right";
        fn_map_["COPIES"]    = "rexx_copies";
        fn_map_["STRIP"]     = "rexx_strip";
        fn_map_["TRANSLATE"] = "rexx_translate";
        fn_map_["REVERSE"]   = "rexx_reverse";
        fn_map_["WORDS"]     = "rexx_words";
        fn_map_["WORD"]      = "rexx_word";
        fn_map_["ABS"]       = "abs";
        fn_map_["MAX"]       = "max";
        fn_map_["MIN"]       = "min";
        fn_map_["RANDOM"]    = "rexx_random";
        fn_map_["DATATYPE"]  = "rexx_datatype";
        fn_map_["CENTER"]    = "rexx_center";
        fn_map_["CENTRE"]    = "rexx_center";
        fn_map_["SPACE"]     = "rexx_space";
        fn_map_["OVERLAY"]   = "rexx_overlay";
        fn_map_["INSERT"]    = "rexx_insert";
        fn_map_["DELSTR"]    = "rexx_delstr";
        fn_map_["VERIFY"]    = "rexx_verify";
        fn_map_["SIGN"]      = "rexx_sign";
        fn_map_["TRUNC"]     = "rexx_trunc";
        fn_map_["FORMAT"]    = "rexx_format";
        fn_map_["D2X"]       = "rexx_d2x";
        fn_map_["X2D"]       = "rexx_x2d";
        fn_map_["C2X"]       = "rexx_c2x";
        fn_map_["X2C"]       = "rexx_x2c";
        fn_map_["D2C"]       = "rexx_d2c";
        fn_map_["C2D"]       = "rexx_c2d";
        fn_map_["B2X"]       = "rexx_b2x";
        fn_map_["X2B"]       = "rexx_x2b";
    }

    std::string translate(const Program& prog) {
        lines_.clear();
        indent_ = 0;
        emit_header();
        for (const auto& stmt : prog.statements) {
            translate_stmt(stmt);
        }
        return join_lines();
    }

    // Convenience: translate from source
    std::string translate_source(const std::string& source) {
        Lexer lexer(source);
        auto tokens = lexer.tokenize();
        Parser parser(tokens);
        auto prog = parser.parse();
        return translate(prog);
    }

private:
    std::vector<std::string> lines_;
    int indent_ = 0;
    std::unordered_map<std::string, std::string> fn_map_;
    std::unordered_set<std::string> stem_names_;
    bool in_procedure_ = false;
    std::vector<std::string> expose_vars_;

    void emit(const std::string& line) {
        lines_.push_back(std::string(static_cast<size_t>(indent_ * 4), ' ') + line);
    }

    void emit_raw(const std::string& line) {
        lines_.push_back(line);
    }

    std::string join_lines() const {
        std::string result;
        for (size_t i = 0; i < lines_.size(); ++i) {
            if (i > 0) result += "\n";
            result += lines_[i];
        }
        return result;
    }

    void emit_header() {
        emit_raw("import subprocess");
        emit_raw("import sys");
        emit_raw("import random");
        emit_raw("");
        emit_raw("# REXX runtime support");
        emit_raw("rexx_queue = []");
        emit_raw("");
    }

    // ── Statement translation ──────────────────────────────────────────
    void translate_stmt(const StmtPtr& stmt) {
        if (!stmt) return;
        switch (stmt->type) {
            case StmtType::Say:       translate_say(stmt); break;
            case StmtType::Pull:      translate_pull(stmt); break;
            case StmtType::Push:      translate_push(stmt); break;
            case StmtType::Queue:     translate_queue(stmt); break;
            case StmtType::Assign:    translate_assign(stmt); break;
            case StmtType::If:        translate_if(stmt); break;
            case StmtType::Select:    translate_select(stmt); break;
            case StmtType::Do:        translate_do(stmt); break;
            case StmtType::Call:      translate_call(stmt); break;
            case StmtType::Return:    translate_return(stmt); break;
            case StmtType::Exit:      translate_exit(stmt); break;
            case StmtType::Signal:    translate_signal(stmt); break;
            case StmtType::Address:   translate_address(stmt); break;
            case StmtType::Parse:     translate_parse(stmt); break;
            case StmtType::Drop:      translate_drop(stmt); break;
            case StmtType::Nop:       emit("pass"); break;
            case StmtType::Iterate:   emit("continue"); break;
            case StmtType::Leave:     emit("break"); break;
            case StmtType::Interpret: translate_interpret(stmt); break;
            case StmtType::Numeric:   translate_numeric(stmt); break;
            case StmtType::Trace:     translate_trace(stmt); break;
            case StmtType::Procedure: translate_procedure(stmt); break;
            case StmtType::Label:     translate_label(stmt); break;
            case StmtType::Block:
                for (const auto& s : stmt->block_stmts) translate_stmt(s);
                break;
        }
    }

    void translate_say(const StmtPtr& stmt) {
        emit("print(" + translate_expr(stmt->expr) + ")");
    }

    void translate_pull(const StmtPtr& stmt) {
        if (stmt->tmpl.size() == 1 &&
            stmt->tmpl[0].type == TemplateItemType::Variable) {
            emit(py_varname(stmt->tmpl[0].value) + " = input().upper()");
        } else {
            emit("_rexx_input = input().upper()");
            emit_parse_template("_rexx_input", stmt->tmpl);
        }
    }

    void translate_push(const StmtPtr& stmt) {
        emit("rexx_queue.insert(0, " + translate_expr(stmt->expr) + ")");
    }

    void translate_queue(const StmtPtr& stmt) {
        emit("rexx_queue.append(" + translate_expr(stmt->expr) + ")");
    }

    void translate_assign(const StmtPtr& stmt) {
        if (!stmt->assign_stem.empty()) {
            // Stem assignment -> dict
            std::string stem = py_varname(stmt->assign_stem);
            std::string key = translate_stem_key(stmt->assign_tail);
            stem_names_.insert(stem);
            emit(stem + "[" + key + "] = " + translate_expr(stmt->assign_value));
        } else {
            emit(py_varname(stmt->assign_target) + " = " +
                 translate_expr(stmt->assign_value));
        }
    }

    void translate_if(const StmtPtr& stmt) {
        emit("if " + translate_expr(stmt->condition) + ":");
        ++indent_;
        if (stmt->then_branch) {
            if (stmt->then_branch->type == StmtType::Nop) emit("pass");
            else translate_stmt(stmt->then_branch);
        } else {
            emit("pass");
        }
        --indent_;
        if (stmt->else_branch) {
            emit("else:");
            ++indent_;
            if (stmt->else_branch->type == StmtType::Nop) emit("pass");
            else translate_stmt(stmt->else_branch);
            --indent_;
        }
    }

    void translate_select(const StmtPtr& stmt) {
        bool first = true;
        for (const auto& wc : stmt->when_clauses) {
            std::string kw = first ? "if " : "elif ";
            emit(kw + translate_expr(wc.condition) + ":");
            ++indent_;
            for (const auto& s : wc.body) translate_stmt(s);
            if (wc.body.empty()) emit("pass");
            --indent_;
            first = false;
        }
        if (!stmt->otherwise_body.empty()) {
            emit("else:");
            ++indent_;
            for (const auto& s : stmt->otherwise_body) translate_stmt(s);
            --indent_;
        }
    }

    void translate_do(const StmtPtr& stmt) {
        switch (stmt->do_variant) {
            case DoVariant::Simple: {
                // Simple block — just emit body
                for (const auto& s : stmt->do_body) translate_stmt(s);
                break;
            }
            case DoVariant::Counted: {
                emit("for _rexx_i in range(" + translate_expr(stmt->do_count) + "):");
                ++indent_;
                emit_do_body(stmt->do_body);
                --indent_;
                break;
            }
            case DoVariant::Controlled: {
                std::string var = py_varname(stmt->do_var);
                std::string start = translate_expr(stmt->do_start);
                std::string end_e = translate_expr(stmt->do_end);
                std::string step = stmt->do_step ? translate_expr(stmt->do_step) : "1";
                emit("for " + var + " in range(" + start + ", " + end_e + " + 1, " + step + "):");
                ++indent_;
                emit_do_body(stmt->do_body);
                --indent_;
                break;
            }
            case DoVariant::While: {
                emit("while " + translate_expr(stmt->do_condition) + ":");
                ++indent_;
                emit_do_body(stmt->do_body);
                --indent_;
                break;
            }
            case DoVariant::Until: {
                emit("while True:");
                ++indent_;
                emit_do_body(stmt->do_body);
                emit("if " + translate_expr(stmt->do_condition) + ":");
                ++indent_;
                emit("break");
                --indent_;
                --indent_;
                break;
            }
            case DoVariant::Forever: {
                emit("while True:");
                ++indent_;
                emit_do_body(stmt->do_body);
                --indent_;
                break;
            }
        }
    }

    void emit_do_body(const std::vector<StmtPtr>& body) {
        if (body.empty()) {
            emit("pass");
            return;
        }
        for (const auto& s : body) translate_stmt(s);
    }

    void translate_call(const StmtPtr& stmt) {
        if (stmt->is_on) {
            emit("# CALL ON " + condition_name(stmt->cond_type));
            return;
        }
        if (stmt->is_off) {
            emit("# CALL OFF " + condition_name(stmt->cond_type));
            return;
        }
        std::string args;
        for (size_t i = 0; i < stmt->call_args.size(); ++i) {
            if (i > 0) args += ", ";
            args += translate_expr(stmt->call_args[i]);
        }
        emit(py_varname(stmt->target_name) + "(" + args + ")");
    }

    void translate_return(const StmtPtr& stmt) {
        if (stmt->expr) {
            emit("return " + translate_expr(stmt->expr));
        } else {
            emit("return");
        }
    }

    void translate_exit(const StmtPtr& stmt) {
        if (stmt->expr) {
            emit("sys.exit(" + translate_expr(stmt->expr) + ")");
        } else {
            emit("sys.exit(0)");
        }
    }

    void translate_signal(const StmtPtr& stmt) {
        if (stmt->is_on) {
            // SIGNAL ON condition -> try/except
            emit("# SIGNAL ON " + condition_name(stmt->cond_type));
            return;
        }
        if (stmt->is_off) {
            emit("# SIGNAL OFF " + condition_name(stmt->cond_type));
            return;
        }
        // SIGNAL label -> raise / goto equivalent
        emit("raise Exception('SIGNAL " + stmt->target_name + "')");
    }

    void translate_address(const StmtPtr& stmt) {
        if (stmt->address_command) {
            emit("subprocess.run(" + translate_expr(stmt->address_command) +
                 ", shell=True)");
        } else {
            emit("# ADDRESS " + stmt->environment);
        }
    }

    void translate_parse(const StmtPtr& stmt) {
        std::string source_expr;
        switch (stmt->parse_source) {
            case ParseSource::Arg:
                source_expr = "' '.join(sys.argv[1:])";
                break;
            case ParseSource::Pull:
                source_expr = "input().upper()";
                break;
            case ParseSource::Var:
                source_expr = py_varname(stmt->parse_var_name);
                break;
            case ParseSource::Value:
                source_expr = "''";
                break;
            case ParseSource::Source:
                source_expr = "'Python " + std::string("COMMAND") + "'";
                break;
            case ParseSource::Version:
                source_expr = "'REXX-Lazarus 1.0'";
                break;
            case ParseSource::LinIn:
                source_expr = "input()";
                break;
        }
        emit("_rexx_parse_src = str(" + source_expr + ")");
        emit_parse_template("_rexx_parse_src", stmt->tmpl);
    }

    void emit_parse_template(const std::string& src_var,
                              const std::vector<TemplateItem>& tmpl) {
        if (tmpl.empty()) return;

        // Simple word-split for all-variable templates
        bool all_vars = true;
        bool has_literal = false;
        for (const auto& item : tmpl) {
            if (item.type != TemplateItemType::Variable &&
                item.type != TemplateItemType::Period) {
                all_vars = false;
            }
            if (item.type == TemplateItemType::LiteralDelim) has_literal = true;
        }

        if (all_vars) {
            // Word split
            emit("_rexx_words = " + src_var + ".split()");
            int idx = 0;
            for (size_t i = 0; i < tmpl.size(); ++i) {
                if (tmpl[i].type == TemplateItemType::Variable) {
                    if (i == tmpl.size() - 1) {
                        // Last var gets rest
                        emit(py_varname(tmpl[i].value) + " = ' '.join(_rexx_words[" +
                             std::to_string(idx) + ":])");
                    } else {
                        emit(py_varname(tmpl[i].value) + " = _rexx_words[" +
                             std::to_string(idx) + "] if len(_rexx_words) > " +
                             std::to_string(idx) + " else ''");
                    }
                    ++idx;
                } else if (tmpl[i].type == TemplateItemType::Period) {
                    ++idx; // skip placeholder
                }
            }
            return;
        }

        if (has_literal) {
            // Literal delimiter parsing
            std::string pos_var = "_rexx_pos";
            emit(pos_var + " = 0");
            for (size_t i = 0; i < tmpl.size(); ++i) {
                if (tmpl[i].type == TemplateItemType::Variable) {
                    // Find next literal delimiter
                    std::string end_expr;
                    bool found_delim = false;
                    for (size_t j = i + 1; j < tmpl.size(); ++j) {
                        if (tmpl[j].type == TemplateItemType::LiteralDelim) {
                            end_expr = src_var + ".find('" +
                                       escape_py_str(tmpl[j].value) + "', " + pos_var + ")";
                            found_delim = true;
                            break;
                        }
                    }
                    if (found_delim) {
                        emit("_rexx_end = " + end_expr);
                        emit("if _rexx_end == -1: _rexx_end = len(" + src_var + ")");
                        emit(py_varname(tmpl[i].value) + " = " + src_var +
                             "[" + pos_var + ":_rexx_end]");
                        emit(pos_var + " = _rexx_end");
                    } else {
                        emit(py_varname(tmpl[i].value) + " = " + src_var +
                             "[" + pos_var + ":]");
                    }
                } else if (tmpl[i].type == TemplateItemType::LiteralDelim) {
                    emit("_rexx_delim = " + src_var + ".find('" +
                         escape_py_str(tmpl[i].value) + "', " + pos_var + ")");
                    emit("if _rexx_delim >= 0: " + pos_var + " = _rexx_delim + " +
                         std::to_string(tmpl[i].value.size()));
                } else if (tmpl[i].type == TemplateItemType::Positional) {
                    emit(pos_var + " = " + std::to_string(tmpl[i].position - 1));
                }
            }
            return;
        }

        // Positional parsing
        emit("_rexx_pos = 0");
        for (const auto& item : tmpl) {
            if (item.type == TemplateItemType::Variable) {
                emit(py_varname(item.value) + " = " + src_var + "[_rexx_pos:]");
            } else if (item.type == TemplateItemType::Positional) {
                emit("_rexx_pos = " + std::to_string(item.position - 1));
            } else if (item.type == TemplateItemType::RelativePos) {
                emit("_rexx_pos += " + std::to_string(item.position));
            }
        }
    }

    void translate_drop(const StmtPtr& stmt) {
        for (const auto& var : stmt->drop_vars) {
            emit("try:\n" + std::string(static_cast<size_t>((indent_ + 1) * 4), ' ') +
                 "del " + py_varname(var) + "\n" +
                 std::string(static_cast<size_t>(indent_ * 4), ' ') +
                 "except NameError:\n" +
                 std::string(static_cast<size_t>((indent_ + 1) * 4), ' ') + "pass");
        }
    }

    void translate_interpret(const StmtPtr& stmt) {
        emit("exec(" + translate_expr(stmt->expr) + ")");
    }

    void translate_numeric(const StmtPtr& stmt) {
        std::string setting;
        switch (stmt->numeric_setting) {
            case NumericSetting::Digits: setting = "DIGITS"; break;
            case NumericSetting::Fuzz:   setting = "FUZZ"; break;
            case NumericSetting::Form:   setting = "FORM"; break;
        }
        emit("# NUMERIC " + setting + " " +
             (stmt->numeric_value ? translate_expr(stmt->numeric_value) : ""));
    }

    void translate_trace(const StmtPtr& stmt) {
        emit("# TRACE " + stmt->trace_setting);
    }

    void translate_procedure(const StmtPtr& stmt) {
        if (!stmt->expose_vars.empty()) {
            for (const auto& var : stmt->expose_vars) {
                emit("nonlocal " + py_varname(var));
            }
        }
        in_procedure_ = true;
        expose_vars_ = stmt->expose_vars;
    }

    void translate_label(const StmtPtr& stmt) {
        emit("");
        emit("def " + py_varname(stmt->label_name) + "():");
        ++indent_;
    }

    // ── Expression translation ─────────────────────────────────────────
    std::string translate_expr(const ExprPtr& expr) {
        if (!expr) return "''";
        switch (expr->type) {
            case ExprType::Literal:
                return quote_py_literal(expr->value);

            case ExprType::Variable:
                return py_varname(expr->value);

            case ExprType::StemAccess: {
                std::string stem = py_varname(expr->stem_name);
                stem_names_.insert(stem);
                std::string key;
                for (size_t i = 0; i < expr->tail_exprs.size(); ++i) {
                    if (i > 0) key += " + '.' + ";
                    key += "str(" + translate_expr(expr->tail_exprs[i]) + ")";
                }
                return stem + ".get(" + key + ", '')";
            }

            case ExprType::BinaryOp:
                return translate_binop(expr);

            case ExprType::UnaryOp:
                if (expr->un_op == UnOp::Negate)
                    return "-(" + translate_expr(expr->operand) + ")";
                return "not (" + translate_expr(expr->operand) + ")";

            case ExprType::FunctionCall:
                return translate_func_call(expr);

            case ExprType::Comparison:
                return translate_comparison(expr);

            case ExprType::Parenthesized:
                return "(" + translate_expr(expr->operand) + ")";
        }
        return "''";
    }

    std::string translate_binop(const ExprPtr& expr) {
        std::string l = translate_expr(expr->left);
        std::string r = translate_expr(expr->right);
        switch (expr->bin_op) {
            case BinOp::Add:         return "(" + l + " + " + r + ")";
            case BinOp::Sub:         return "(" + l + " - " + r + ")";
            case BinOp::Mul:         return "(" + l + " * " + r + ")";
            case BinOp::Div:         return "(" + l + " / " + r + ")";
            case BinOp::IntDiv:      return "(" + l + " // " + r + ")";
            case BinOp::Mod:         return "(" + l + " % " + r + ")";
            case BinOp::Power:       return "(" + l + " ** " + r + ")";
            case BinOp::Concat:      return "str(" + l + ") + str(" + r + ")";
            case BinOp::ConcatSpace: return "str(" + l + ") + ' ' + str(" + r + ")";
            case BinOp::LogicalAnd:  return "(" + l + " and " + r + ")";
            case BinOp::LogicalOr:   return "(" + l + " or " + r + ")";
            default: return l;
        }
    }

    std::string translate_comparison(const ExprPtr& expr) {
        std::string l = translate_expr(expr->left);
        std::string r = translate_expr(expr->right);
        switch (expr->bin_op) {
            case BinOp::Eq:       return "(" + l + " == " + r + ")";
            case BinOp::Ne:       return "(" + l + " != " + r + ")";
            case BinOp::Lt:       return "(" + l + " < " + r + ")";
            case BinOp::Gt:       return "(" + l + " > " + r + ")";
            case BinOp::Le:       return "(" + l + " <= " + r + ")";
            case BinOp::Ge:       return "(" + l + " >= " + r + ")";
            case BinOp::StrictEq: return "(str(" + l + ") == str(" + r + "))";
            case BinOp::StrictNe: return "(str(" + l + ") != str(" + r + "))";
            case BinOp::StrictLt: return "(str(" + l + ") < str(" + r + "))";
            case BinOp::StrictGt: return "(str(" + l + ") > str(" + r + "))";
            default: return l;
        }
    }

    std::string translate_func_call(const ExprPtr& expr) {
        std::string name = expr->func_name;
        std::string upper_name = name;
        std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

        std::string py_name;
        auto it = fn_map_.find(upper_name);
        if (it != fn_map_.end()) {
            py_name = it->second;
        } else {
            py_name = py_varname(name);
        }

        std::string args;
        for (size_t i = 0; i < expr->args.size(); ++i) {
            if (i > 0) args += ", ";
            args += translate_expr(expr->args[i]);
        }
        return py_name + "(" + args + ")";
    }

public:
    // ── Helpers (public for testability) ────────────────────────────────
    static std::string py_varname(const std::string& name) {
        std::string result;
        for (char c : name) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
                result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            } else {
                result += '_';
            }
        }
        // Avoid Python keywords
        if (result == "in" || result == "is" || result == "or" ||
            result == "and" || result == "not" || result == "for" ||
            result == "if" || result == "else" || result == "while" ||
            result == "class" || result == "def" || result == "return" ||
            result == "import" || result == "from" || result == "pass" ||
            result == "break" || result == "continue" || result == "try" ||
            result == "except" || result == "with" || result == "as") {
            result = "rexx_" + result;
        }
        return result;
    }

private:
    static std::string quote_py_literal(const std::string& val) {
        // Check if numeric
        bool is_num = !val.empty();
        bool has_dot = false;
        size_t start = 0;
        if (!val.empty() && (val[0] == '-' || val[0] == '+')) start = 1;
        if (start >= val.size()) is_num = false;
        for (size_t i = start; i < val.size() && is_num; ++i) {
            if (val[i] == '.') {
                if (has_dot) is_num = false;
                has_dot = true;
            } else if (!std::isdigit(static_cast<unsigned char>(val[i]))) {
                is_num = false;
            }
        }
        if (is_num && !val.empty()) return val;
        return "'" + escape_py_str(val) + "'";
    }

    static std::string escape_py_str(const std::string& s) {
        std::string result;
        for (char c : s) {
            switch (c) {
                case '\\': result += "\\\\"; break;
                case '\'': result += "\\'"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c; break;
            }
        }
        return result;
    }

    std::string translate_stem_key(const std::vector<ExprPtr>& tails) {
        std::string key;
        for (size_t i = 0; i < tails.size(); ++i) {
            if (i > 0) key += " + '.' + ";
            key += "str(" + translate_expr(tails[i]) + ")";
        }
        return key;
    }
};

}} // namespace lazarus::rexx

#endif // LAZARUS_REXX_PYTHON_TRANSLATOR_H
