#ifndef LAZARUS_EASYTRIEVE_TRANSLATOR_H
#define LAZARUS_EASYTRIEVE_TRANSLATOR_H

#include "lazarus/easytrieve/ast.h"
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <iomanip>

namespace lazarus { namespace easytrieve {

// ============================================================
// Macro preprocessor for #SET, #IF, #ELSE, #END
// ============================================================
class MacroProcessor {
public:
    void define(const std::string& name, const std::string& value) {
        symbols_[name] = value;
    }

    std::string get(const std::string& name) const {
        auto it = symbols_.find(name);
        return (it != symbols_.end()) ? it->second : "";
    }

    bool is_defined(const std::string& name) const {
        return symbols_.find(name) != symbols_.end();
    }

    // Process macro directives in source text
    std::string expand(const std::string& source) const {
        std::istringstream stream(source);
        std::string line;
        std::ostringstream result;
        bool skip = false;
        bool in_if = false;

        while (std::getline(stream, line)) {
            std::string trimmed = trim(line);

            if (starts_with(trimmed, "#SET")) {
                // #SET name value
                auto parts = split_first_two(trimmed.substr(4));
                if (parts.size() >= 2) {
                    // const_cast to allow mutation in expand
                    const_cast<MacroProcessor*>(this)->symbols_[parts[0]] = parts[1];
                }
                continue;
            }

            if (starts_with(trimmed, "#IF")) {
                in_if = true;
                auto name = trim(trimmed.substr(3));
                skip = !is_defined(name);
                continue;
            }

            if (starts_with(trimmed, "#ELSE")) {
                if (in_if) skip = !skip;
                continue;
            }

            if (starts_with(trimmed, "#END")) {
                in_if = false;
                skip = false;
                continue;
            }

            if (!skip) {
                // Substitute %name% references
                std::string expanded = substitute_vars(line);
                result << expanded << "\n";
            }
        }

        return result.str();
    }

private:
    std::map<std::string, std::string> symbols_;

    std::string substitute_vars(const std::string& line) const {
        std::string result;
        size_t i = 0;
        while (i < line.size()) {
            if (line[i] == '%' && i + 1 < line.size()) {
                size_t end = line.find('%', i + 1);
                if (end != std::string::npos) {
                    std::string name = line.substr(i + 1, end - i - 1);
                    auto it = symbols_.find(name);
                    if (it != symbols_.end()) {
                        result += it->second;
                        i = end + 1;
                        continue;
                    }
                }
            }
            result += line[i];
            ++i;
        }
        return result;
    }

    static std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) return "";
        size_t end = s.find_last_not_of(" \t\r\n");
        return s.substr(start, end - start + 1);
    }

    static bool starts_with(const std::string& s, const std::string& prefix) {
        return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
    }

    static std::vector<std::string> split_first_two(const std::string& s) {
        std::vector<std::string> parts;
        std::string trimmed = trim(s);
        size_t sp = trimmed.find_first_of(" \t");
        if (sp == std::string::npos) {
            if (!trimmed.empty()) parts.push_back(trimmed);
            return parts;
        }
        parts.push_back(trimmed.substr(0, sp));
        std::string rest = trim(trimmed.substr(sp));
        if (!rest.empty()) parts.push_back(rest);
        return parts;
    }
};

// ============================================================
// Easytrieve to C++17 Translator
// ============================================================
class Translator {
public:
    Translator() : indent_(0) {}

    // Translate a full program
    std::string translate(const Program& prog) {
        out_.str("");
        out_.clear();

        emit_header();

        // File declarations -> structs + file classes
        for (const auto& f : prog.files) {
            translate_file_decl(f);
        }

        // Working storage -> member variables
        if (!prog.working_fields.empty()) {
            emit_line("// Working storage fields");
            emit_line("struct WorkingStorage {");
            indent_++;
            for (const auto& wf : prog.working_fields) {
                translate_working_field(wf);
            }
            indent_--;
            emit_line("};");
            emit_line("");
        }

        // Table declarations -> maps
        for (const auto& td : prog.tables) {
            translate_table_decl(td);
        }

        // Proc declarations -> functions
        for (const auto& pd : prog.procs) {
            translate_proc(pd);
        }

        // Report activities -> report classes
        for (const auto& rpt : prog.reports) {
            translate_report(rpt);
        }

        // Sort activities
        for (const auto& s : prog.sorts) {
            translate_sort(s);
        }

        // JOB activities -> main processing
        if (!prog.jobs.empty()) {
            emit_line("int main() {");
            indent_++;
            if (!prog.working_fields.empty()) {
                emit_line("WorkingStorage ws;");
            }
            for (const auto& job : prog.jobs) {
                translate_job(job);
            }
            emit_line("return 0;");
            indent_--;
            emit_line("}");
        }

        return out_.str();
    }

    // Translate just a FILE declaration
    std::string translate_file_only(const FileDecl& fd) {
        out_.str("");
        out_.clear();
        translate_file_decl(fd);
        return out_.str();
    }

    // Translate just field declarations to struct members
    std::string translate_fields(const std::vector<FieldDecl>& fields) {
        out_.str("");
        out_.clear();
        for (const auto& f : fields) {
            emit_line(field_to_cpp_type(f) + " " + sanitize_name(f.name) + ";");
        }
        return out_.str();
    }

    // Translate a single statement
    std::string translate_statement(const Statement& stmt) {
        out_.str("");
        out_.clear();
        translate_stmt(stmt);
        return out_.str();
    }

    // Translate an expression to C++ string
    std::string translate_expr(const Expression& expr) {
        return expr_to_cpp(expr);
    }

    // Translate a condition to C++ string
    std::string translate_cond(const Condition& cond) {
        return condition_to_cpp(cond);
    }

    // Translate a JOB to C++ code
    std::string translate_job_only(const JobActivity& job) {
        out_.str("");
        out_.clear();
        translate_job(job);
        return out_.str();
    }

    // Translate SORT to C++ code
    std::string translate_sort_only(const SortActivity& sort) {
        out_.str("");
        out_.clear();
        translate_sort(sort);
        return out_.str();
    }

    // Translate REPORT to C++ code
    std::string translate_report_only(const ReportActivity& rpt) {
        out_.str("");
        out_.clear();
        translate_report(rpt);
        return out_.str();
    }

    // Access the macro processor
    MacroProcessor& macros() { return macros_; }
    const MacroProcessor& macros() const { return macros_; }

private:
    std::ostringstream out_;
    int indent_;
    MacroProcessor macros_;

    // ---- Output helpers ----
    void emit_line(const std::string& line) {
        for (int i = 0; i < indent_; i++) out_ << "    ";
        out_ << line << "\n";
    }

    void emit_raw(const std::string& text) {
        out_ << text;
    }

    void emit_header() {
        emit_line("#include <string>");
        emit_line("#include <vector>");
        emit_line("#include <fstream>");
        emit_line("#include <iostream>");
        emit_line("#include <map>");
        emit_line("#include <algorithm>");
        emit_line("#include <cstring>");
        emit_line("#include <iomanip>");
        emit_line("");
    }

    // ---- Name sanitization ----
    static std::string sanitize_name(const std::string& name) {
        std::string result;
        result.reserve(name.size());
        for (char c : name) {
            if (c == '-') result += '_';
            else if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
                result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return result;
    }

    // ---- Type mapping ----
    static std::string field_to_cpp_type(const FieldDecl& f) {
        switch (f.type) {
            case FieldType::Alpha:
                return "std::string";
            case FieldType::Numeric:
            case FieldType::Unsigned:
                if (f.decimals > 0) return "double";
                if (f.length > 9) return "int64_t";
                return "int32_t";
            case FieldType::Packed:
                if (f.decimals > 0) return "double";
                return "int64_t";
            case FieldType::Binary:
                if (f.length <= 2) return "int16_t";
                if (f.length <= 4) return "int32_t";
                return "int64_t";
        }
        return "std::string";
    }

    static std::string working_field_to_cpp_type(const WorkingField& wf) {
        switch (wf.type) {
            case FieldType::Alpha:
                return "std::string";
            case FieldType::Numeric:
            case FieldType::Unsigned:
                if (wf.decimals > 0) return "double";
                if (wf.length > 9) return "int64_t";
                return "int32_t";
            case FieldType::Packed:
                if (wf.decimals > 0) return "double";
                return "int64_t";
            case FieldType::Binary:
                if (wf.length <= 2) return "int16_t";
                if (wf.length <= 4) return "int32_t";
                return "int64_t";
        }
        return "std::string";
    }

    static std::string default_initializer(const WorkingField& wf) {
        if (!wf.initial_value.empty()) {
            if (wf.type == FieldType::Alpha) {
                return "\"" + wf.initial_value + "\"";
            }
            return wf.initial_value;
        }
        switch (wf.type) {
            case FieldType::Alpha: return "\"\"";
            case FieldType::Numeric:
            case FieldType::Unsigned:
            case FieldType::Packed:
            case FieldType::Binary:
                return "0";
        }
        return "\"\"";
    }

    // ---- FILE declaration ----
    void translate_file_decl(const FileDecl& fd) {
        std::string sname = sanitize_name(fd.name);

        // Record struct
        emit_line("struct " + sname + "_record {");
        indent_++;
        for (const auto& f : fd.fields) {
            std::string cpp_type = field_to_cpp_type(f);
            emit_line(cpp_type + " " + sanitize_name(f.name) + ";");
        }
        for (const auto& rec : fd.records) {
            for (const auto& f : rec.fields) {
                std::string cpp_type = field_to_cpp_type(f);
                emit_line(cpp_type + " " + sanitize_name(f.name) + ";");
            }
        }
        indent_--;
        emit_line("};");
        emit_line("");

        // File class
        emit_line("class " + sname + "_file {");
        emit_line("public:");
        indent_++;
        emit_line(sname + "_record current;");

        if (fd.is_virtual) {
            emit_line("// VIRTUAL file (in-memory)");
            emit_line("std::vector<" + sname + "_record> records;");
        } else {
            emit_line("std::fstream stream;");
            emit_line("static constexpr int LRECL = " + std::to_string(fd.lrecl) + ";");
        }

        // Open method
        emit_line("");
        emit_line("bool open(const std::string& path) {");
        indent_++;
        if (fd.is_virtual) {
            emit_line("(void)path;");
            emit_line("return true;");
        } else {
            emit_line("stream.open(path, std::ios::in | std::ios::out | std::ios::binary);");
            emit_line("return stream.is_open();");
        }
        indent_--;
        emit_line("}");

        // Get (read) method
        emit_line("");
        emit_line("bool get() {");
        indent_++;
        if (fd.is_virtual) {
            emit_line("return false; // virtual files use direct access");
        } else {
            emit_line("std::string buf(LRECL, ' ');");
            emit_line("if (!stream.read(&buf[0], LRECL)) return false;");
            translate_record_unpack(fd);
            emit_line("return true;");
        }
        indent_--;
        emit_line("}");

        // Put (write) method
        emit_line("");
        emit_line("void put() {");
        indent_++;
        if (!fd.is_virtual) {
            emit_line("std::string buf(LRECL, ' ');");
            translate_record_pack(fd);
            emit_line("stream.write(buf.data(), LRECL);");
        } else {
            emit_line("records.push_back(current);");
        }
        indent_--;
        emit_line("}");

        // Close method
        emit_line("");
        emit_line("void close() {");
        indent_++;
        if (!fd.is_virtual) {
            emit_line("if (stream.is_open()) stream.close();");
        }
        indent_--;
        emit_line("}");

        indent_--;
        emit_line("};");
        emit_line("");
    }

    void translate_record_unpack(const FileDecl& fd) {
        for (const auto& f : fd.fields) {
            std::string sn = sanitize_name(f.name);
            if (f.type == FieldType::Alpha) {
                emit_line("current." + sn + " = buf.substr(" +
                         std::to_string(f.position - 1) + ", " +
                         std::to_string(f.length) + ");");
            } else {
                emit_line("current." + sn + " = std::stoi(buf.substr(" +
                         std::to_string(f.position - 1) + ", " +
                         std::to_string(f.length) + "));");
            }
        }
    }

    void translate_record_pack(const FileDecl& fd) {
        for (const auto& f : fd.fields) {
            std::string sn = sanitize_name(f.name);
            if (f.type == FieldType::Alpha) {
                emit_line("buf.replace(" + std::to_string(f.position - 1) + ", " +
                         std::to_string(f.length) + ", current." + sn + ".substr(0, " +
                         std::to_string(f.length) + "));");
            } else {
                emit_line("{");
                indent_++;
                emit_line("auto s = std::to_string(current." + sn + ");");
                emit_line("while (static_cast<int>(s.size()) < " + std::to_string(f.length) +
                         ") s = \"0\" + s;");
                emit_line("buf.replace(" + std::to_string(f.position - 1) + ", " +
                         std::to_string(f.length) + ", s.substr(0, " +
                         std::to_string(f.length) + "));");
                indent_--;
                emit_line("}");
            }
        }
    }

    // ---- Working field ----
    void translate_working_field(const WorkingField& wf) {
        std::string cpp_type = working_field_to_cpp_type(wf);
        std::string init = default_initializer(wf);
        emit_line(cpp_type + " " + sanitize_name(wf.name) + " = " + init + ";");
    }

    // ---- Table declaration ----
    void translate_table_decl(const TableDecl& td) {
        std::string sname = sanitize_name(td.name);
        emit_line("std::map<std::string, std::string> " + sname + "_table;");
        emit_line("");
    }

    // ---- PROC ----
    void translate_proc(const ProcDecl& pd) {
        std::string sname = sanitize_name(pd.name);
        std::string params;
        for (size_t i = 0; i < pd.parameters.size(); i++) {
            if (i > 0) params += ", ";
            params += "const std::string& " + sanitize_name(pd.parameters[i]);
        }
        emit_line("void " + sname + "(" + params + ") {");
        indent_++;
        for (const auto& stmt : pd.body) {
            translate_stmt(*stmt);
        }
        indent_--;
        emit_line("}");
        emit_line("");
    }

    // ---- JOB activity ----
    void translate_job(const JobActivity& job) {
        if (!job.input_file.empty()) {
            std::string fname = sanitize_name(job.input_file);
            emit_line(fname + "_file " + fname + ";");
            emit_line("// Process file: " + job.input_file);
            emit_line("while (" + fname + ".get()) {");
            indent_++;
        }

        for (const auto& stmt : job.statements) {
            translate_stmt(*stmt);
        }

        if (!job.input_file.empty()) {
            indent_--;
            emit_line("}");
        }
    }

    // ---- SORT activity ----
    void translate_sort(const SortActivity& sort) {
        std::string in_name = sanitize_name(sort.input_file);
        emit_line("// SORT " + sort.input_file);
        emit_line("{");
        indent_++;
        emit_line("auto& records = " + in_name + ".records;");
        emit_line("std::sort(records.begin(), records.end(),");
        indent_++;
        emit_line("[](const auto& a, const auto& b) {");
        indent_++;
        for (size_t i = 0; i < sort.sort_fields.size(); i++) {
            std::string fn = sanitize_name(sort.sort_fields[i]);
            bool asc = (i < sort.ascending.size()) ? sort.ascending[i] : true;
            if (asc) {
                emit_line("if (a." + fn + " != b." + fn + ") return a." + fn + " < b." + fn + ";");
            } else {
                emit_line("if (a." + fn + " != b." + fn + ") return a." + fn + " > b." + fn + ";");
            }
        }
        emit_line("return false;");
        indent_--;
        emit_line("});");
        indent_--;
        indent_--;
        emit_line("}");
        emit_line("");
    }

    // ---- REPORT activity ----
    void translate_report(const ReportActivity& rpt) {
        std::string sname = sanitize_name(rpt.name);
        emit_line("class " + sname + "_report {");
        emit_line("public:");
        indent_++;
        emit_line("std::ostream& out;");
        emit_line("int line_count = 0;");
        emit_line("int page_count = 0;");
        emit_line("static constexpr int LINES_PER_PAGE = " +
                 std::to_string(rpt.lines_per_page) + ";");
        emit_line("");

        // Control break totals
        for (const auto& cb : rpt.control_breaks) {
            std::string cbname = sanitize_name(cb.field_name);
            emit_line("std::string prev_" + cbname + ";");
            for (const auto& sf : cb.sums) {
                emit_line("double sum_" + sanitize_name(sf.field_name) + " = 0.0;");
            }
        }
        // Grand totals
        for (const auto& cb : rpt.control_breaks) {
            for (const auto& sf : cb.sums) {
                emit_line("double grand_" + sanitize_name(sf.field_name) + " = 0.0;");
            }
        }

        emit_line("");
        emit_line("explicit " + sname + "_report(std::ostream& os) : out(os) {}");

        // print_headers
        emit_line("");
        emit_line("void print_headers() {");
        indent_++;
        emit_line("page_count++;");
        emit_line("line_count = 0;");
        for (const auto& tl : rpt.titles) {
            emit_line("out << ");
            for (size_t i = 0; i < tl.elements.size(); i++) {
                if (i > 0) emit_raw(" << \" \" << ");
                emit_raw(expr_to_cpp(tl.elements[i]));
            }
            emit_raw(" << \"\\n\";\n");
        }
        for (const auto& hl : rpt.headings) {
            emit_line("out << ");
            for (size_t i = 0; i < hl.elements.size(); i++) {
                if (i > 0) emit_raw(" << \" \" << ");
                emit_raw(expr_to_cpp(hl.elements[i]));
            }
            emit_raw(" << \"\\n\";\n");
        }
        indent_--;
        emit_line("}");

        // check_page_break
        emit_line("");
        emit_line("void check_page_break() {");
        indent_++;
        emit_line("if (line_count >= LINES_PER_PAGE || page_count == 0) {");
        indent_++;
        emit_line("print_headers();");
        indent_--;
        emit_line("}");
        indent_--;
        emit_line("}");

        // print_detail
        emit_line("");
        emit_line("template<typename Record>");
        emit_line("void print_detail(const Record& rec) {");
        indent_++;
        emit_line("check_page_break();");
        emit_line("line_count++;");
        emit_line("(void)rec;");
        indent_--;
        emit_line("}");

        // check_control_break
        emit_line("");
        emit_line("template<typename Record>");
        emit_line("bool check_control_break(const Record& /*rec*/) {");
        indent_++;
        emit_line("return false; // control break logic");
        indent_--;
        emit_line("}");

        // print_totals
        emit_line("");
        emit_line("void print_totals() {");
        indent_++;
        for (const auto& cb : rpt.control_breaks) {
            for (const auto& sf : cb.sums) {
                emit_line("out << \"Total: \" << grand_" + sanitize_name(sf.field_name) +
                         " << \"\\n\";");
            }
        }
        indent_--;
        emit_line("}");

        indent_--;
        emit_line("};");
        emit_line("");
    }

    // ---- Statement translation ----
    void translate_stmt(const Statement& stmt) {
        if (!stmt.label.empty()) {
            // Decrease indent for label, then restore
            int saved = indent_;
            indent_ = 0;
            emit_line(sanitize_name(stmt.label) + ":");
            indent_ = saved;
        }

        std::visit([this](const auto& s) { translate_stmt_impl(s); }, stmt.value);
    }

    void translate_stmt_impl(const IfStmt& s) {
        emit_line("if (" + condition_to_cpp(s.condition) + ") {");
        indent_++;
        for (const auto& st : s.then_branch) translate_stmt(*st);
        indent_--;
        if (!s.else_branch.empty()) {
            emit_line("} else {");
            indent_++;
            for (const auto& st : s.else_branch) translate_stmt(*st);
            indent_--;
        }
        emit_line("}");
    }

    void translate_stmt_impl(const DoStmt& s) {
        if (s.is_while && s.while_condition) {
            emit_line("while (" + condition_to_cpp(*s.while_condition) + ") {");
        } else {
            emit_line("while (true) {");
        }
        indent_++;
        for (const auto& st : s.body) translate_stmt(*st);
        indent_--;
        emit_line("}");
    }

    void translate_stmt_impl(const PerformStmt& s) {
        emit_line(sanitize_name(s.proc_name) + "();");
    }

    void translate_stmt_impl(const GoToStmt& s) {
        emit_line("goto " + sanitize_name(s.label) + ";");
    }

    void translate_stmt_impl(const GetStmt& s) {
        emit_line("if (!" + sanitize_name(s.file_name) + ".get()) break;");
    }

    void translate_stmt_impl(const PutStmt& s) {
        emit_line(sanitize_name(s.file_name) + ".put();");
    }

    void translate_stmt_impl(const PointStmt& s) {
        emit_line(sanitize_name(s.file_name) + ".point(" + expr_to_cpp(s.key_value) + ");");
    }

    void translate_stmt_impl(const ReadStmt& s) {
        emit_line(sanitize_name(s.file_name) + ".get();");
    }

    void translate_stmt_impl(const WriteStmt& s) {
        emit_line(sanitize_name(s.file_name) + ".put();");
    }

    void translate_stmt_impl(const DisplayStmt& s) {
        std::string line = "std::cout";
        for (const auto& item : s.items) {
            line += " << " + expr_to_cpp(item);
        }
        if (s.newline) line += " << \"\\n\"";
        line += ";";
        emit_line(line);
    }

    void translate_stmt_impl(const MoveStmt& s) {
        emit_line(expr_to_cpp(s.target) + " = " + expr_to_cpp(s.source) + ";");
    }

    void translate_stmt_impl(const SearchStmt& s) {
        std::string tname = sanitize_name(s.table_name);
        emit_line("{");
        indent_++;
        emit_line("auto it = " + tname + "_table.find(" + expr_to_cpp(s.search_key) + ");");
        emit_line("if (it != " + tname + "_table.end()) {");
        indent_++;
        if (!s.result_field.empty()) {
            emit_line(sanitize_name(s.result_field) + " = it->second;");
        }
        indent_--;
        emit_line("}");
        indent_--;
        emit_line("}");
    }

    void translate_stmt_impl(const StopStmt&) {
        emit_line("return;");
    }

    void translate_stmt_impl(const PrintStmt& s) {
        std::string rname = sanitize_name(s.report_name);
        emit_line(rname + "_rpt.print_detail(current);");
    }

    // ---- Expression to C++ ----
    std::string expr_to_cpp(const Expression& expr) const {
        if (expr.is_field_ref()) {
            const auto& fr = expr.as_field_ref();
            if (!fr.file_name.empty()) {
                return sanitize_name(fr.file_name) + ".current." + sanitize_name(fr.field_name);
            }
            return sanitize_name(fr.field_name);
        }
        if (expr.is_literal()) {
            const auto& lit = expr.as_literal();
            if (lit.is_numeric) return lit.value;
            return "\"" + lit.value + "\"";
        }
        if (expr.is_arithmetic()) {
            const auto& ar = expr.as_arithmetic();
            return "(" + expr_to_cpp(*ar.left) + " " +
                   arith_op_symbol(ar.op) + " " +
                   expr_to_cpp(*ar.right) + ")";
        }
        if (expr.is_concatenation()) {
            const auto& cat = expr.as_concatenation();
            std::string result = "(std::string()";
            for (const auto& part : cat.parts) {
                result += " + " + expr_to_cpp(*part);
            }
            result += ")";
            return result;
        }
        return "\"\"";
    }

    // ---- Condition to C++ ----
    std::string condition_to_cpp(const Condition& cond) const {
        if (cond.is_compare()) {
            const auto& cmp = std::get<Compare>(cond.value);
            return "(" + expr_to_cpp(cmp.left) + " " +
                   compare_op_cpp(cmp.op) + " " +
                   expr_to_cpp(cmp.right) + ")";
        }
        if (cond.is_range()) {
            const auto& rng = std::get<Range>(cond.value);
            std::string f = expr_to_cpp(rng.field);
            return "(" + f + " >= " + expr_to_cpp(rng.low) + " && " +
                   f + " <= " + expr_to_cpp(rng.high) + ")";
        }
        if (cond.is_mask()) {
            const auto& m = std::get<Mask>(cond.value);
            return "mask_match(" + expr_to_cpp(m.field) + ", \"" + m.pattern + "\")";
        }
        if (std::holds_alternative<std::shared_ptr<ConditionAnd>>(cond.value)) {
            const auto& ca = std::get<std::shared_ptr<ConditionAnd>>(cond.value);
            return "(" + condition_to_cpp(ca->left) + " && " +
                   condition_to_cpp(ca->right) + ")";
        }
        if (std::holds_alternative<std::shared_ptr<ConditionOr>>(cond.value)) {
            const auto& co = std::get<std::shared_ptr<ConditionOr>>(cond.value);
            return "(" + condition_to_cpp(co->left) + " || " +
                   condition_to_cpp(co->right) + ")";
        }
        if (std::holds_alternative<std::shared_ptr<ConditionNot>>(cond.value)) {
            const auto& cn = std::get<std::shared_ptr<ConditionNot>>(cond.value);
            return "(!" + condition_to_cpp(cn->operand) + ")";
        }
        return "true";
    }
};

}} // namespace lazarus::easytrieve

#endif // LAZARUS_EASYTRIEVE_TRANSLATOR_H
