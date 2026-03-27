#ifndef LAZARUS_EASYTRIEVE_PARSER_H
#define LAZARUS_EASYTRIEVE_PARSER_H

#include "lazarus/easytrieve/lexer.h"
#include "lazarus/easytrieve/ast.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <memory>

namespace lazarus { namespace easytrieve {

class ParseError : public std::runtime_error {
public:
    int line, col;
    ParseError(const std::string& msg, int ln, int cl)
        : std::runtime_error("Parse error at " + std::to_string(ln) + ":" +
                             std::to_string(cl) + ": " + msg),
          line(ln), col(cl) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}

    Program parse() {
        Program prog;
        while (!at_end()) {
            parse_top_level(prog);
        }
        return prog;
    }

    // Parse a single FILE declaration from token stream
    FileDecl parse_file_decl() {
        expect(TokenType::KW_FILE);
        FileDecl fd;
        fd.name = expect_identifier();

        // Parse file options
        while (!at_end() && !is_top_level_start()) {
            if (match(TokenType::KW_LRECL)) {
                fd.lrecl = expect_number();
            } else if (match(TokenType::KW_VIRTUAL)) {
                fd.is_virtual = true;
            } else if (match(TokenType::KW_INDEXED)) {
                fd.is_indexed = true;
            } else if (match(TokenType::KW_KEY)) {
                expect(TokenType::LPAREN);
                fd.key_field = expect_identifier();
                expect(TokenType::RPAREN);
            } else if (current_type() == TokenType::IDENTIFIER) {
                // Could be a field definition
                break;
            } else {
                break;
            }
        }

        // Parse field definitions that follow the FILE declaration
        while (!at_end() && !is_top_level_start()) {
            if (current_type() == TokenType::IDENTIFIER) {
                fd.fields.push_back(parse_field_def());
            } else if (current_type() == TokenType::KW_RECORD) {
                fd.records.push_back(parse_record_decl(fd.name));
            } else {
                break;
            }
        }

        return fd;
    }

    // Parse a single W (working storage) field
    WorkingField parse_working_field() {
        WorkingField wf;
        wf.name = expect_identifier();
        wf.length = expect_number();
        wf.type = parse_field_type();
        // Optional decimal places
        if (!at_end() && current_type() == TokenType::NUMERIC_LITERAL) {
            wf.decimals = expect_number();
        }
        // Optional initial value
        if (!at_end() && match(TokenType::KW_MASK)) {
            expect(TokenType::LPAREN);
            wf.mask = current().value;
            advance();
            expect(TokenType::RPAREN);
        }
        if (!at_end() && (current_type() == TokenType::STRING_LITERAL ||
                          current_type() == TokenType::NUMERIC_LITERAL)) {
            wf.initial_value = current().value;
            advance();
        }
        return wf;
    }

    // Parse W declarations block
    std::vector<WorkingField> parse_w_block() {
        expect(TokenType::KW_W);
        std::vector<WorkingField> fields;
        while (!at_end() && !is_top_level_start()) {
            if (current_type() == TokenType::IDENTIFIER) {
                fields.push_back(parse_working_field());
            } else {
                break;
            }
        }
        return fields;
    }

    // Parse JOB activity
    JobActivity parse_job() {
        expect(TokenType::KW_JOB);
        JobActivity job;
        job.name = "JOB";

        // Optional job name or INPUT file
        if (!at_end() && current_type() == TokenType::IDENTIFIER) {
            std::string name = expect_identifier();
            if (!at_end() && current_type() == TokenType::IDENTIFIER) {
                job.name = name;
                job.input_file = expect_identifier();
            } else {
                job.input_file = name;
            }
        }

        // Parse statements until next top-level
        while (!at_end() && !is_top_level_start()) {
            auto stmt = parse_statement();
            if (stmt) job.statements.push_back(std::move(stmt));
        }
        return job;
    }

    // Parse SORT activity
    SortActivity parse_sort() {
        expect(TokenType::KW_SORT);
        SortActivity sort;
        sort.name = "SORT";
        sort.input_file = expect_identifier();

        // Parse sort specification
        if (!at_end() && match(TokenType::KW_TO)) {
            sort.output_file = expect_identifier();
        }

        // Parse USING / sort fields
        while (!at_end() && current_type() == TokenType::IDENTIFIER && !is_top_level_start()) {
            sort.sort_fields.push_back(current().value);
            advance();
            // Optional D for descending
            if (!at_end() && current_type() == TokenType::IDENTIFIER && current().value == "D") {
                sort.ascending.push_back(false);
                advance();
            } else {
                sort.ascending.push_back(true);
            }
        }

        return sort;
    }

    // Parse REPORT activity
    ReportActivity parse_report() {
        expect(TokenType::KW_REPORT);
        ReportActivity rpt;
        rpt.name = "REPORT";

        if (!at_end() && current_type() == TokenType::IDENTIFIER) {
            rpt.name = expect_identifier();
        }

        // Parse report sections
        while (!at_end() && !is_top_level_start()) {
            if (match(TokenType::KW_SEQUENCE)) {
                rpt.sequence_field = expect_identifier();
            } else if (match(TokenType::KW_CONTROL)) {
                auto cb = parse_control_break();
                rpt.control_breaks.push_back(std::move(cb));
            } else if (match(TokenType::KW_TITLE)) {
                auto title = parse_title_line();
                rpt.titles.push_back(std::move(title));
            } else if (match(TokenType::KW_HEADING)) {
                auto heading = parse_heading_line();
                rpt.headings.push_back(std::move(heading));
            } else if (match(TokenType::KW_LINE)) {
                auto detail = parse_detail_line();
                rpt.detail_lines.push_back(std::move(detail));
            } else if (match(TokenType::KW_SUM)) {
                // SUM field at report level
                if (!rpt.control_breaks.empty()) {
                    SumField sf;
                    sf.field_name = expect_identifier();
                    rpt.control_breaks.back().sums.push_back(std::move(sf));
                }
            } else {
                break;
            }
        }

        return rpt;
    }

    // Parse DEFINE TABLE
    TableDecl parse_define_table() {
        expect(TokenType::KW_DEFINE);
        TableDecl td;
        td.name = expect_identifier();

        // Parse key fields
        while (!at_end() && current_type() == TokenType::IDENTIFIER && !is_top_level_start()) {
            FieldDecl fd;
            fd.name = expect_identifier();
            if (!at_end() && current_type() == TokenType::NUMERIC_LITERAL) {
                fd.length = expect_number();
            }
            if (!at_end() && is_field_type_token()) {
                fd.type = parse_field_type();
            }
            td.key_fields.push_back(std::move(fd));

            if (match(TokenType::COMMA)) continue;
            break;
        }

        return td;
    }

    // Parse PROC declaration
    ProcDecl parse_proc() {
        expect(TokenType::KW_PROC);
        ProcDecl pd;
        if (!at_end() && current_type() == TokenType::IDENTIFIER) {
            pd.name = expect_identifier();
        }

        // Optional PARM list
        if (match(TokenType::KW_PARM)) {
            expect(TokenType::LPAREN);
            while (!at_end() && current_type() != TokenType::RPAREN) {
                pd.parameters.push_back(expect_identifier());
                if (!match(TokenType::COMMA)) break;
            }
            expect(TokenType::RPAREN);
        }

        // Parse body
        while (!at_end() && current_type() != TokenType::KW_END_PROC &&
               current_type() != TokenType::KW_END && !is_top_level_start()) {
            auto stmt = parse_statement();
            if (stmt) pd.body.push_back(std::move(stmt));
        }

        if (match(TokenType::KW_END_PROC)) { /* consume */ }
        else { match(TokenType::KW_END); }

        return pd;
    }

    // Parse a SEARCH TABLE statement
    std::shared_ptr<Statement> parse_search_stmt() {
        SearchStmt ss;
        ss.table_name = expect_identifier();
        if (!at_end() && (current_type() == TokenType::IDENTIFIER ||
                          current_type() == TokenType::STRING_LITERAL ||
                          current_type() == TokenType::NUMERIC_LITERAL)) {
            ss.search_key = parse_expression();
        }
        return make_stmt(std::move(ss));
    }

    // Parse a single statement
    std::shared_ptr<Statement> parse_statement() {
        if (at_end()) return nullptr;

        // Check for label prefix
        std::string label;
        if (current_type() == TokenType::IDENTIFIER) {
            // Lookahead for DOT after identifier to detect label
            size_t saved = pos_;
            std::string maybe_label = current().value;
            advance();
            if (!at_end() && current_type() == TokenType::DOT) {
                label = maybe_label;
                advance(); // consume dot
            } else {
                pos_ = saved; // restore
            }
        }

        auto type = current_type();

        if (type == TokenType::KW_IF)       return parse_if_stmt(label);
        if (type == TokenType::KW_DO)       return parse_do_stmt(label);
        if (type == TokenType::KW_PERFORM) {
            advance();
            PerformStmt ps;
            ps.proc_name = expect_identifier();
            return make_stmt(std::move(ps), label);
        }
        if (type == TokenType::KW_GOTO || type == TokenType::KW_GO) {
            advance();
            if (type == TokenType::KW_GO) match(TokenType::KW_TO);
            GoToStmt gs;
            gs.label = expect_identifier();
            return make_stmt(std::move(gs), label);
        }
        if (type == TokenType::KW_GET) {
            advance();
            GetStmt gs;
            gs.file_name = expect_identifier();
            return make_stmt(std::move(gs), label);
        }
        if (type == TokenType::KW_PUT) {
            advance();
            PutStmt ps;
            ps.file_name = expect_identifier();
            return make_stmt(std::move(ps), label);
        }
        if (type == TokenType::KW_POINT) {
            advance();
            PointStmt ps;
            ps.file_name = expect_identifier();
            ps.key_value = parse_expression();
            return make_stmt(std::move(ps), label);
        }
        if (type == TokenType::KW_READ) {
            advance();
            ReadStmt rs;
            rs.file_name = expect_identifier();
            return make_stmt(std::move(rs), label);
        }
        if (type == TokenType::KW_WRITE) {
            advance();
            WriteStmt ws;
            ws.file_name = expect_identifier();
            return make_stmt(std::move(ws), label);
        }
        if (type == TokenType::KW_DISPLAY) {
            advance();
            DisplayStmt ds;
            while (!at_end() && current_type() != TokenType::KW_IF &&
                   !is_top_level_start() && !is_statement_start()) {
                ds.items.push_back(parse_expression());
            }
            return make_stmt(std::move(ds), label);
        }
        if (type == TokenType::KW_MOVE) {
            advance();
            MoveStmt ms;
            ms.source = parse_expression();
            match(TokenType::KW_TO);
            ms.target = parse_expression();
            return make_stmt(std::move(ms), label);
        }
        if (type == TokenType::KW_SEARCH) {
            advance();
            return parse_search_stmt();
        }
        if (type == TokenType::KW_STOP) {
            advance();
            return make_stmt(StopStmt{}, label);
        }
        if (type == TokenType::KW_PRINT) {
            advance();
            PrintStmt ps;
            if (!at_end() && current_type() == TokenType::IDENTIFIER) {
                ps.report_name = expect_identifier();
            }
            return make_stmt(std::move(ps), label);
        }

        // Assignment: FIELD = expr
        if (type == TokenType::IDENTIFIER) {
            size_t saved = pos_;
            auto target = parse_expression();
            if (!at_end() && current_type() == TokenType::EQUALS) {
                advance();
                MoveStmt ms;
                ms.source = parse_expression();
                ms.target = std::move(target);
                return make_stmt(std::move(ms), label);
            }
            pos_ = saved;
        }

        // Skip unknown
        advance();
        return nullptr;
    }

    // Parse expression
    Expression parse_expression() {
        auto left = parse_primary_expr();

        // Check for arithmetic operator
        while (!at_end()) {
            ArithOp op;
            if (current_type() == TokenType::PLUS)       { op = ArithOp::Add; }
            else if (current_type() == TokenType::MINUS)  { op = ArithOp::Sub; }
            else if (current_type() == TokenType::STAR)   { op = ArithOp::Mul; }
            else if (current_type() == TokenType::SLASH)  { op = ArithOp::Div; }
            else break;

            advance();
            auto right = parse_primary_expr();
            left = make_arithmetic(
                std::make_shared<Expression>(std::move(left)),
                op,
                std::make_shared<Expression>(std::move(right))
            );
        }

        return left;
    }

    // Parse condition
    Condition parse_condition() {
        auto left = parse_simple_condition();

        while (!at_end()) {
            if (match(TokenType::KW_AND)) {
                auto right = parse_simple_condition();
                left = make_and(std::move(left), std::move(right));
            } else if (match(TokenType::KW_OR)) {
                auto right = parse_simple_condition();
                left = make_or(std::move(left), std::move(right));
            } else {
                break;
            }
        }

        return left;
    }

private:
    const std::vector<Token>& tokens_;
    size_t pos_;

    // ---- Navigation ----
    bool at_end() const {
        return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::END_OF_INPUT;
    }

    const Token& current() const {
        static Token eof(TokenType::END_OF_INPUT, "", 0, 0);
        return (pos_ < tokens_.size()) ? tokens_[pos_] : eof;
    }

    TokenType current_type() const { return current().type; }

    void advance() {
        if (pos_ < tokens_.size()) pos_++;
    }

    bool match(TokenType t) {
        if (!at_end() && current_type() == t) {
            advance();
            return true;
        }
        return false;
    }

    void expect(TokenType t) {
        if (at_end() || current_type() != t) {
            throw ParseError(
                std::string("Expected ") + token_type_name(t) + " but got " +
                token_type_name(current_type()) + " ('" + current().value + "')",
                current().line, current().column
            );
        }
        advance();
    }

    std::string expect_identifier() {
        if (at_end()) {
            throw ParseError("Expected identifier at end of input",
                             current().line, current().column);
        }
        // Accept keywords as identifiers in certain contexts
        std::string val = current().value;
        if (current_type() == TokenType::IDENTIFIER ||
            is_keyword_usable_as_ident(current_type())) {
            advance();
            return val;
        }
        throw ParseError("Expected identifier but got " +
                         std::string(token_type_name(current_type())),
                         current().line, current().column);
    }

    int expect_number() {
        if (at_end() || current_type() != TokenType::NUMERIC_LITERAL) {
            throw ParseError("Expected numeric literal",
                             current().line, current().column);
        }
        int val = std::stoi(current().value);
        advance();
        return val;
    }

    bool is_keyword_usable_as_ident(TokenType t) const {
        // Some keywords can appear as identifiers (file names, field names)
        return t == TokenType::KW_KEY || t == TokenType::KW_LINE ||
               t == TokenType::KW_TITLE || t == TokenType::KW_TYPE_A ||
               t == TokenType::KW_TYPE_N || t == TokenType::KW_TYPE_P ||
               t == TokenType::KW_TYPE_B || t == TokenType::KW_TYPE_U ||
               t == TokenType::KW_END || t == TokenType::IDENTIFIER;
    }

    bool is_top_level_start() const {
        auto t = current_type();
        return t == TokenType::KW_FILE || t == TokenType::KW_JOB ||
               t == TokenType::KW_SORT || t == TokenType::KW_REPORT ||
               t == TokenType::KW_W || t == TokenType::KW_DEFINE ||
               t == TokenType::KW_PROC || t == TokenType::KW_MACRO;
    }

    bool is_statement_start() const {
        auto t = current_type();
        return t == TokenType::KW_IF || t == TokenType::KW_DO ||
               t == TokenType::KW_PERFORM || t == TokenType::KW_GO ||
               t == TokenType::KW_GOTO || t == TokenType::KW_STOP ||
               t == TokenType::KW_GET || t == TokenType::KW_PUT ||
               t == TokenType::KW_POINT || t == TokenType::KW_READ ||
               t == TokenType::KW_WRITE || t == TokenType::KW_DISPLAY ||
               t == TokenType::KW_MOVE || t == TokenType::KW_SEARCH ||
               t == TokenType::KW_PRINT || t == TokenType::KW_ELSE ||
               t == TokenType::KW_END_IF || t == TokenType::KW_END_DO;
    }

    bool is_field_type_token() const {
        auto t = current_type();
        return t == TokenType::KW_TYPE_A || t == TokenType::KW_TYPE_N ||
               t == TokenType::KW_TYPE_P || t == TokenType::KW_TYPE_B ||
               t == TokenType::KW_TYPE_U;
    }

    // ---- Top-level parsing ----
    void parse_top_level(Program& prog) {
        auto type = current_type();
        if (type == TokenType::KW_FILE) {
            prog.files.push_back(parse_file_decl());
        } else if (type == TokenType::KW_W) {
            auto fields = parse_w_block();
            for (auto& f : fields) {
                prog.working_fields.push_back(std::move(f));
            }
        } else if (type == TokenType::KW_DEFINE) {
            prog.tables.push_back(parse_define_table());
        } else if (type == TokenType::KW_JOB) {
            prog.jobs.push_back(parse_job());
        } else if (type == TokenType::KW_SORT) {
            prog.sorts.push_back(parse_sort());
        } else if (type == TokenType::KW_REPORT) {
            prog.reports.push_back(parse_report());
        } else if (type == TokenType::KW_PROC) {
            prog.procs.push_back(parse_proc());
        } else {
            advance(); // skip unrecognized
        }
    }

    // ---- Field definition ----
    FieldDecl parse_field_def() {
        FieldDecl fd;
        fd.name = expect_identifier();
        fd.position = expect_number();
        fd.length = expect_number();
        fd.type = parse_field_type();
        // Optional decimal places
        if (!at_end() && current_type() == TokenType::NUMERIC_LITERAL) {
            fd.decimals = expect_number();
        }
        // Optional HEADING
        if (!at_end() && match(TokenType::KW_HEADING)) {
            expect(TokenType::LPAREN);
            if (!at_end() && current_type() == TokenType::STRING_LITERAL) {
                fd.heading = current().value;
                advance();
            }
            expect(TokenType::RPAREN);
        }
        return fd;
    }

    RecordDecl parse_record_decl(const std::string& file_name) {
        expect(TokenType::KW_RECORD);
        RecordDecl rd;
        rd.file_name = file_name;
        rd.name = expect_identifier();

        while (!at_end() && current_type() == TokenType::IDENTIFIER && !is_top_level_start()) {
            rd.fields.push_back(parse_field_def());
        }
        return rd;
    }

    FieldType parse_field_type() {
        if (match(TokenType::KW_TYPE_A)) return FieldType::Alpha;
        if (match(TokenType::KW_TYPE_N)) return FieldType::Numeric;
        if (match(TokenType::KW_TYPE_P)) return FieldType::Packed;
        if (match(TokenType::KW_TYPE_B)) return FieldType::Binary;
        if (match(TokenType::KW_TYPE_U)) return FieldType::Unsigned;
        return FieldType::Alpha; // default
    }

    // ---- Report elements ----
    ControlBreak parse_control_break() {
        ControlBreak cb;
        cb.field_name = expect_identifier();
        // Optional level
        if (!at_end() && current_type() == TokenType::NUMERIC_LITERAL) {
            cb.level = expect_number();
        }
        return cb;
    }

    TitleLine parse_title_line() {
        TitleLine tl;
        tl.line_number = 1;
        if (!at_end() && current_type() == TokenType::NUMERIC_LITERAL) {
            tl.line_number = expect_number();
        }
        while (!at_end() && !is_top_level_start() && !is_report_section_start()) {
            tl.elements.push_back(parse_expression());
        }
        return tl;
    }

    HeadingLine parse_heading_line() {
        HeadingLine hl;
        hl.line_number = 1;
        if (!at_end() && current_type() == TokenType::NUMERIC_LITERAL) {
            hl.line_number = expect_number();
        }
        while (!at_end() && !is_top_level_start() && !is_report_section_start()) {
            hl.elements.push_back(parse_expression());
        }
        return hl;
    }

    DetailLine parse_detail_line() {
        DetailLine dl;
        if (!at_end() && current_type() == TokenType::IDENTIFIER) {
            dl.name = expect_identifier();
        }
        while (!at_end() && !is_top_level_start() && !is_report_section_start()) {
            dl.elements.push_back(parse_expression());
        }
        return dl;
    }

    bool is_report_section_start() const {
        auto t = current_type();
        return t == TokenType::KW_SEQUENCE || t == TokenType::KW_CONTROL ||
               t == TokenType::KW_TITLE || t == TokenType::KW_HEADING ||
               t == TokenType::KW_LINE || t == TokenType::KW_SUM;
    }

    // ---- Statement parsing ----
    std::shared_ptr<Statement> parse_if_stmt(const std::string& label) {
        expect(TokenType::KW_IF);
        IfStmt is;
        is.condition = parse_condition();

        // Parse then branch
        while (!at_end() && current_type() != TokenType::KW_ELSE &&
               current_type() != TokenType::KW_END_IF && !is_top_level_start()) {
            auto stmt = parse_statement();
            if (stmt) is.then_branch.push_back(std::move(stmt));
        }

        // Parse else branch
        if (match(TokenType::KW_ELSE)) {
            while (!at_end() && current_type() != TokenType::KW_END_IF &&
                   !is_top_level_start()) {
                auto stmt = parse_statement();
                if (stmt) is.else_branch.push_back(std::move(stmt));
            }
        }

        match(TokenType::KW_END_IF);
        return make_stmt(std::move(is), label);
    }

    std::shared_ptr<Statement> parse_do_stmt(const std::string& label) {
        expect(TokenType::KW_DO);
        DoStmt ds;

        // DO WHILE condition
        if (current_type() == TokenType::IDENTIFIER && current().value == "WHILE") {
            advance();
            ds.is_while = true;
            ds.while_condition = parse_condition();
        }

        // Parse body
        while (!at_end() && current_type() != TokenType::KW_END_DO && !is_top_level_start()) {
            auto stmt = parse_statement();
            if (stmt) ds.body.push_back(std::move(stmt));
        }

        match(TokenType::KW_END_DO);
        return make_stmt(std::move(ds), label);
    }

    // ---- Expression parsing ----
    Expression parse_primary_expr() {
        if (at_end()) return make_literal("", false);

        if (current_type() == TokenType::STRING_LITERAL) {
            std::string val = current().value;
            advance();
            return make_literal(val, false);
        }

        if (current_type() == TokenType::NUMERIC_LITERAL) {
            std::string val = current().value;
            advance();
            return make_literal(val, true);
        }

        if (current_type() == TokenType::LPAREN) {
            advance();
            auto expr = parse_expression();
            match(TokenType::RPAREN);
            return expr;
        }

        if (current_type() == TokenType::IDENTIFIER || is_keyword_usable_as_ident(current_type())) {
            std::string name = current().value;
            advance();
            // Check for qualified reference (FILE:FIELD)
            if (!at_end() && current_type() == TokenType::COLON) {
                advance();
                std::string field = expect_identifier();
                return make_field_ref(field, name);
            }
            return make_field_ref(name);
        }

        // Fallback
        std::string val = current().value;
        advance();
        return make_literal(val, false);
    }

    // ---- Condition parsing ----
    Condition parse_simple_condition() {
        if (match(TokenType::KW_NOT)) {
            auto inner = parse_simple_condition();
            return make_not(std::move(inner));
        }

        auto left_expr = parse_expression();

        // RANGE check
        if (match(TokenType::KW_RANGE)) {
            auto lo = parse_expression();
            auto hi = parse_expression();
            return make_range(std::move(left_expr), std::move(lo), std::move(hi));
        }

        // MASK check
        if (match(TokenType::KW_MASK)) {
            std::string pattern;
            if (!at_end() && current_type() == TokenType::STRING_LITERAL) {
                pattern = current().value;
                advance();
            }
            return make_mask(std::move(left_expr), pattern);
        }

        // Comparison operator
        CompareOp op = CompareOp::Eq;
        bool found_op = false;

        if (match(TokenType::KW_EQ) || match(TokenType::EQUALS)) {
            op = CompareOp::Eq; found_op = true;
        } else if (match(TokenType::KW_NE)) {
            op = CompareOp::Ne; found_op = true;
        } else if (match(TokenType::KW_LT)) {
            op = CompareOp::Lt; found_op = true;
        } else if (match(TokenType::KW_LE)) {
            op = CompareOp::Le; found_op = true;
        } else if (match(TokenType::KW_GT)) {
            op = CompareOp::Gt; found_op = true;
        } else if (match(TokenType::KW_GE)) {
            op = CompareOp::Ge; found_op = true;
        }

        if (found_op) {
            auto right_expr = parse_expression();
            return make_compare(std::move(left_expr), op, std::move(right_expr));
        }

        // Default: field existence check (treat as EQ TRUE)
        return make_compare(std::move(left_expr), CompareOp::Ne,
                           make_literal("", false));
    }
};

}} // namespace lazarus::easytrieve

#endif // LAZARUS_EASYTRIEVE_PARSER_H
