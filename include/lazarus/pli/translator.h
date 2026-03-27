#ifndef LAZARUS_PLI_TRANSLATOR_H
#define LAZARUS_PLI_TRANSLATOR_H

#include "lazarus/pli/ast.h"
#include "lazarus/pli/builtins.h"
#include <string>
#include <sstream>
#include <map>
#include <set>
#include <vector>
#include <algorithm>

namespace lazarus { namespace pli {

class Translator {
public:
    Translator() = default;

    // Translate a full program
    std::string translate(const Program& prog) {
        std::ostringstream out;

        // Emit headers
        out << "#include <cstdint>\n";
        out << "#include <string>\n";
        out << "#include <iostream>\n";
        out << "#include <cmath>\n";
        out << "#include <vector>\n";
        out << "#include <stdexcept>\n";
        out << "#include <functional>\n";
        out << "#include <bitset>\n";
        out << "#include <complex>\n\n";

        // Forward declarations for structures
        for (const auto& decl : prog.declarations) {
            if (auto* sd = std::get_if<StructureDecl>(&decl->node)) {
                emit_structure_forward(out, *sd);
            }
        }

        // Emit declarations
        for (const auto& decl : prog.declarations) {
            emit_declaration(out, *decl, 0);
        }

        // Main function
        if (prog.options_main) {
            out << "\nint main() {\n";
        } else if (!prog.name.empty()) {
            out << "\nvoid " << prog.name << "() {\n";
        }

        // Emit statements
        for (const auto& stmt : prog.statements) {
            emit_statement(out, *stmt, 1);
        }

        if (prog.options_main || !prog.name.empty()) {
            if (prog.options_main) out << "    return 0;\n";
            out << "}\n";
        }

        return out.str();
    }

    // Translate a single expression to C++
    std::string translate_expr(const ExprPtr& expr) {
        std::ostringstream out;
        emit_expression(out, *expr);
        return out.str();
    }

    // Translate a single statement to C++
    std::string translate_stmt(const StmtPtr& stmt) {
        std::ostringstream out;
        emit_statement(out, *stmt, 0);
        return out.str();
    }

    // Translate a single declaration to C++
    std::string translate_decl(const DeclPtr& decl) {
        std::ostringstream out;
        emit_declaration(out, *decl, 0);
        return out.str();
    }

    // Map PL/I type to C++ type string
    static std::string map_type(const DataAttributes& attrs) {
        switch (attrs.base_type) {
            case BaseType::FIXED:
                if (attrs.scale == Scale::BINARY) {
                    if (attrs.precision <= 15) return "int16_t";
                    if (attrs.precision <= 31) return "int32_t";
                    return "int64_t";
                }
                // FIXED DECIMAL -> packed decimal / int64
                return "int64_t";
            case BaseType::FLOAT:
                if (attrs.precision > 0 && attrs.precision <= 6) return "float";
                return "double";
            case BaseType::CHAR:
                if (attrs.varying) return "std::string";
                if (attrs.char_length > 0) {
                    return "std::string";  // Could use std::array<char,N> for FIXED
                }
                return "std::string";
            case BaseType::BIT:
                if (attrs.bit_length > 0) {
                    return "std::bitset<" + std::to_string(attrs.bit_length) + ">";
                }
                return "std::bitset<1>";
            case BaseType::POINTER:
                return "void*";
            case BaseType::ENTRY:
                return "std::function<void()>";
            case BaseType::FILE:
                return "/* PliFile */ int";
            case BaseType::LABEL:
                return "/* label */ int";
            case BaseType::AREA:
                return "std::vector<uint8_t>";
            case BaseType::OFFSET:
                return "ptrdiff_t";
            case BaseType::PICTURE:
                return "std::string";
            case BaseType::COMPLEX:
                return "std::complex<double>";
            case BaseType::UNKNOWN:
                return "int64_t";
        }
        return "int64_t";
    }

private:
    int indent_level_ = 0;

    std::string indent(int level) const {
        return std::string(static_cast<size_t>(level) * 4, ' ');
    }

    // ---------- Declaration emission ----------

    void emit_structure_forward(std::ostringstream& out, const StructureDecl& sd) {
        out << "struct " << sd.name << ";\n";
    }

    void emit_declaration(std::ostringstream& out, const Declaration& decl, int level) {
        std::visit([&](const auto& d) { emit_decl_impl(out, d, level); }, decl.node);
    }

    void emit_decl_impl(std::ostringstream& out, const ScalarDecl& d, int level) {
        out << indent(level) << map_type(d.attrs) << " " << d.name;
        if (d.attrs.is_initial) {
            out << " = " << format_initial(d.attrs);
        }
        out << ";\n";
    }

    void emit_decl_impl(std::ostringstream& out, const ArrayDecl& d, int level) {
        out << indent(level) << "std::vector<" << map_type(d.attrs) << "> " << d.name;
        if (!d.dimensions.empty()) {
            int total = 1;
            for (const auto& dim : d.dimensions) {
                total *= (dim.upper - dim.lower + 1);
            }
            out << "(" << total << ")";
        }
        out << ";\n";
    }

    void emit_decl_impl(std::ostringstream& out, const StructureDecl& d, int level) {
        out << indent(level) << "struct " << d.name << " {\n";
        for (const auto& member : d.members) {
            out << indent(level + 1) << map_type(member.attrs) << " " << member.name;
            if (!member.dimensions.empty()) {
                // Array member
                int total = 1;
                for (const auto& dim : member.dimensions) {
                    total *= (dim.upper - dim.lower + 1);
                }
                out << "[" << total << "]";
            }
            out << ";\n";
        }
        out << indent(level) << "};\n";
    }

    void emit_decl_impl(std::ostringstream& out, const EntryDecl& d, int level) {
        out << indent(level) << "std::function<";
        if (d.returns.returns_specified) {
            out << map_type(d.returns);
        } else {
            out << "void";
        }
        out << "(";
        for (size_t i = 0; i < d.param_types.size(); i++) {
            if (i > 0) out << ", ";
            out << map_type(d.param_types[i]);
        }
        out << ")> " << d.name << ";\n";
    }

    void emit_decl_impl(std::ostringstream& out, const LabelDecl& d, int level) {
        out << indent(level) << "// label: " << d.name << "\n";
    }

    void emit_decl_impl(std::ostringstream& out, const FileDecl& d, int level) {
        out << indent(level) << "// PL/I file: " << d.name << "\n";
    }

    void emit_decl_impl(std::ostringstream& out, const ProcedureDecl& d, int level) {
        std::string ret_type = "void";
        if (d.attrs.returns_specified) {
            ret_type = map_type(d.attrs);
        }
        out << indent(level) << ret_type << " " << d.name << "(";
        for (size_t i = 0; i < d.params.size(); i++) {
            if (i > 0) out << ", ";
            out << "auto " << d.params[i];
        }
        out << ") {\n";
        for (const auto& ld : d.local_decls) {
            emit_declaration(out, *ld, level + 1);
        }
        for (const auto& s : d.body) {
            emit_statement(out, *s, level + 1);
        }
        out << indent(level) << "}\n";
    }

    std::string format_initial(const DataAttributes& attrs) {
        if (attrs.initial_value.empty()) {
            if (attrs.base_type == BaseType::CHAR) return "\"\"";
            return "0";
        }
        if (attrs.base_type == BaseType::CHAR) {
            return "\"" + attrs.initial_value + "\"";
        }
        return attrs.initial_value;
    }

    // ---------- Statement emission ----------

    void emit_statement(std::ostringstream& out, const Statement& stmt, int level) {
        std::visit([&](const auto& s) { emit_stmt_impl(out, s, level); }, stmt.node);
    }

    void emit_stmt_impl(std::ostringstream& out, const AssignmentStmt& s, int level) {
        out << indent(level);
        emit_expression(out, *s.target);
        out << " = ";
        emit_expression(out, *s.value);
        out << ";\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const IfStmt& s, int level) {
        out << indent(level) << "if (";
        emit_expression(out, *s.condition);
        out << ") {\n";
        for (const auto& stmt : s.then_body) {
            emit_statement(out, *stmt, level + 1);
        }
        if (!s.else_body.empty()) {
            out << indent(level) << "} else {\n";
            for (const auto& stmt : s.else_body) {
                emit_statement(out, *stmt, level + 1);
            }
        }
        out << indent(level) << "}\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const SelectStmt& s, int level) {
        if (s.expr) {
            // SELECT (expr); WHEN ...
            out << indent(level) << "{\n";
            out << indent(level + 1) << "auto _sel = ";
            emit_expression(out, *s.expr);
            out << ";\n";
            for (size_t i = 0; i < s.whens.size(); i++) {
                out << indent(level + 1) << (i == 0 ? "if" : "else if") << " (";
                for (size_t j = 0; j < s.whens[i].values.size(); j++) {
                    if (j > 0) out << " || ";
                    out << "_sel == ";
                    emit_expression(out, *s.whens[i].values[j]);
                }
                out << ") {\n";
                for (const auto& stmt : s.whens[i].body) {
                    emit_statement(out, *stmt, level + 2);
                }
                out << indent(level + 1) << "}\n";
            }
            if (!s.otherwise_body.empty()) {
                out << indent(level + 1) << "else {\n";
                for (const auto& stmt : s.otherwise_body) {
                    emit_statement(out, *stmt, level + 2);
                }
                out << indent(level + 1) << "}\n";
            }
            out << indent(level) << "}\n";
        } else {
            // SELECT; WHEN (cond) ...
            for (size_t i = 0; i < s.whens.size(); i++) {
                out << indent(level) << (i == 0 ? "if" : "else if") << " (";
                for (size_t j = 0; j < s.whens[i].values.size(); j++) {
                    if (j > 0) out << " || ";
                    emit_expression(out, *s.whens[i].values[j]);
                }
                out << ") {\n";
                for (const auto& stmt : s.whens[i].body) {
                    emit_statement(out, *stmt, level + 1);
                }
                out << indent(level) << "}\n";
            }
            if (!s.otherwise_body.empty()) {
                out << indent(level) << "else {\n";
                for (const auto& stmt : s.otherwise_body) {
                    emit_statement(out, *stmt, level + 1);
                }
                out << indent(level) << "}\n";
            }
        }
    }

    void emit_stmt_impl(std::ostringstream& out, const DoLoopStmt& s, int level) {
        switch (s.kind) {
            case DoLoopStmt::SIMPLE:
                out << indent(level) << "{\n";
                break;
            case DoLoopStmt::ITERATIVE:
                out << indent(level) << "for (auto " << s.variable << " = ";
                if (s.from) emit_expression(out, *s.from);
                else out << "0";
                out << "; " << s.variable << " <= ";
                if (s.to) emit_expression(out, *s.to);
                else out << "0";
                out << "; " << s.variable << " += ";
                if (s.by) emit_expression(out, *s.by);
                else out << "1";
                out << ") {\n";
                break;
            case DoLoopStmt::WHILE:
                out << indent(level) << "while (";
                if (s.while_cond) emit_expression(out, *s.while_cond);
                out << ") {\n";
                break;
            case DoLoopStmt::UNTIL:
                out << indent(level) << "do {\n";
                break;
            case DoLoopStmt::FOREVER:
                out << indent(level) << "for (;;) {\n";
                break;
        }
        for (const auto& stmt : s.body) {
            emit_statement(out, *stmt, level + 1);
        }
        if (s.kind == DoLoopStmt::UNTIL) {
            out << indent(level) << "} while (!(";
            if (s.until_cond) emit_expression(out, *s.until_cond);
            out << "));\n";
        } else {
            out << indent(level) << "}\n";
        }
    }

    void emit_stmt_impl(std::ostringstream& out, const CallStmt& s, int level) {
        out << indent(level) << s.target << "(";
        for (size_t i = 0; i < s.args.size(); i++) {
            if (i > 0) out << ", ";
            emit_expression(out, *s.args[i]);
        }
        out << ");\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const ReturnStmt& s, int level) {
        out << indent(level) << "return";
        if (s.value) {
            out << " ";
            emit_expression(out, *s.value);
        }
        out << ";\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const PutListStmt& s, int level) {
        out << indent(level);
        if (s.skip) out << "std::cout << '\\n';\n" << indent(level);
        out << "std::cout";
        for (const auto& item : s.items) {
            out << " << ";
            emit_expression(out, *item);
        }
        out << " << '\\n';\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const PutEditStmt& s, int level) {
        // Simplified: emit as printf-style
        out << indent(level) << "std::cout";
        for (const auto& item : s.items) {
            out << " << ";
            emit_expression(out, *item);
        }
        out << ";\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const GetListStmt& s, int level) {
        for (const auto& target : s.targets) {
            out << indent(level) << "std::cin >> ";
            emit_expression(out, *target);
            out << ";\n";
        }
    }

    void emit_stmt_impl(std::ostringstream& out, const GetEditStmt& s, int level) {
        for (const auto& target : s.targets) {
            out << indent(level) << "std::cin >> ";
            emit_expression(out, *target);
            out << ";\n";
        }
    }

    void emit_stmt_impl(std::ostringstream& out, const OpenStmt& s, int level) {
        out << indent(level) << "// OPEN FILE(" << s.file_name << ")\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const CloseStmt& s, int level) {
        out << indent(level) << "// CLOSE FILE(" << s.file_name << ")\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const ReadStmt& s, int level) {
        out << indent(level) << "// READ FILE(" << s.file_name << ")";
        if (s.into) { out << " INTO(...)"; }
        out << "\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const WriteStmt& s, int level) {
        out << indent(level) << "// WRITE FILE(" << s.file_name << ")";
        if (s.from) { out << " FROM(...)"; }
        out << "\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const RewriteStmt& s, int level) {
        out << indent(level) << "// REWRITE FILE(" << s.file_name << ")\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const DeleteStmt& s, int level) {
        out << indent(level) << "// DELETE FILE(" << s.file_name << ")\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const AllocateStmt& s, int level) {
        out << indent(level) << s.variable << " = new auto(" << s.variable << ");\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const FreeStmt& s, int level) {
        out << indent(level) << "delete " << s.variable << ";\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const OnStmt& s, int level) {
        if (s.is_system) {
            out << indent(level) << "// ON " << s.condition << " SYSTEM;\n";
        } else {
            out << indent(level) << "try {\n";
            out << indent(level) << "} catch (...) {\n";
            for (const auto& stmt : s.body) {
                emit_statement(out, *stmt, level + 1);
            }
            out << indent(level) << "}\n";
        }
    }

    void emit_stmt_impl(std::ostringstream& out, const SignalStmt& s, int level) {
        out << indent(level) << "throw std::runtime_error(\"" << s.condition << "\");\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const RevertStmt& s, int level) {
        out << indent(level) << "// REVERT " << s.condition << "\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const StopStmt&, int level) {
        out << indent(level) << "return 0;\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const DisplayStmt& s, int level) {
        out << indent(level) << "std::cout";
        for (const auto& item : s.items) {
            out << " << ";
            emit_expression(out, *item);
        }
        out << " << std::endl;\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const GoToStmt& s, int level) {
        out << indent(level) << "goto " << s.label << ";\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const NullStmt&, int level) {
        out << indent(level) << "/* null */;\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const LeaveStmt&, int level) {
        out << indent(level) << "break;\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const IterateStmt&, int level) {
        out << indent(level) << "continue;\n";
    }

    void emit_stmt_impl(std::ostringstream& out, const EntryStmt& s, int level) {
        out << indent(level) << "// ENTRY " << s.name << "\n";
    }

    // ---------- Expression emission ----------

    void emit_expression(std::ostringstream& out, const Expression& expr) {
        std::visit([&](const auto& e) { emit_expr_impl(out, e); }, expr.node);
    }

    void emit_expr_impl(std::ostringstream& out, const LiteralExpr& e) {
        switch (e.kind) {
            case LiteralExpr::INTEGER: out << e.value; break;
            case LiteralExpr::FLOAT_LIT: out << e.value; break;
            case LiteralExpr::STRING: out << "\"" << e.value << "\""; break;
            case LiteralExpr::BIT_STRING: out << "std::bitset<" << e.value.size() << ">(\"" << e.value << "\")"; break;
            case LiteralExpr::COMPLEX_LIT: out << "std::complex<double>(0, " << e.value << ")"; break;
        }
    }

    void emit_expr_impl(std::ostringstream& out, const IdentifierExpr& e) {
        out << e.name;
    }

    void emit_expr_impl(std::ostringstream& out, const BinaryOpExpr& e) {
        out << "(";
        emit_expression(out, *e.left);
        switch (e.op) {
            case BinOp::ADD: out << " + "; break;
            case BinOp::SUB: out << " - "; break;
            case BinOp::MUL: out << " * "; break;
            case BinOp::DIV: out << " / "; break;
            case BinOp::POWER: out << ", "; emit_expression(out, *e.right); out << ")"; return; // need pow()
            case BinOp::CONCAT: out << " + "; break; // string concat
            case BinOp::EQ: out << " == "; break;
            case BinOp::NE: out << " != "; break;
            case BinOp::LT: out << " < "; break;
            case BinOp::GT: out << " > "; break;
            case BinOp::LE: out << " <= "; break;
            case BinOp::GE: out << " >= "; break;
            case BinOp::AND: out << " && "; break;
            case BinOp::OR: out << " || "; break;
            case BinOp::NOT: out << " ^ "; break; // shouldn't happen as binary
        }
        emit_expression(out, *e.right);
        out << ")";
    }

    void emit_expr_impl(std::ostringstream& out, const UnaryOpExpr& e) {
        switch (e.op) {
            case UnaryOp::NEGATE: out << "(-"; break;
            case UnaryOp::NOT: out << "(!"; break;
            case UnaryOp::PLUS: out << "(+"; break;
        }
        emit_expression(out, *e.operand);
        out << ")";
    }

    void emit_expr_impl(std::ostringstream& out, const FunctionCallExpr& e) {
        out << e.name << "(";
        for (size_t i = 0; i < e.args.size(); i++) {
            if (i > 0) out << ", ";
            emit_expression(out, *e.args[i]);
        }
        out << ")";
    }

    void emit_expr_impl(std::ostringstream& out, const SubscriptExpr& e) {
        emit_expression(out, *e.base);
        out << "[";
        for (size_t i = 0; i < e.indices.size(); i++) {
            if (i > 0) out << "][";
            emit_expression(out, *e.indices[i]);
        }
        out << "]";
    }

    void emit_expr_impl(std::ostringstream& out, const MemberAccessExpr& e) {
        emit_expression(out, *e.object);
        out << "." << e.member;
    }

    void emit_expr_impl(std::ostringstream& out, const BuiltinCallExpr& e) {
        // Map builtin names to C++ equivalents
        out << e.name << "(";
        for (size_t i = 0; i < e.args.size(); i++) {
            if (i > 0) out << ", ";
            emit_expression(out, *e.args[i]);
        }
        out << ")";
    }
};

// ---------- Preprocessor directive translator ----------

inline std::string translate_preprocessor(const std::string& directive, const std::string& args) {
    std::string upper = directive;
    for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    if (upper == "%INCLUDE") return "#include \"" + args + "\"";
    if (upper == "%IF") return "#if " + args;
    if (upper == "%THEN") return "";
    if (upper == "%ELSE") return "#else";
    if (upper == "%DECLARE" || upper == "%DCL") return "constexpr auto " + args + ";";
    if (upper == "%REPLACE") return "constexpr auto " + args + ";";
    if (upper == "%ACTIVATE") return "#define " + args;
    if (upper == "%DEACTIVATE") return "#undef " + args;
    if (upper == "%DO") return "{";
    if (upper == "%END") return "}";

    return "// " + directive + " " + args;
}

}} // namespace lazarus::pli

#endif // LAZARUS_PLI_TRANSLATOR_H
