#ifndef LAZARUS_REXX_PARSER_H
#define LAZARUS_REXX_PARSER_H

#include "lazarus/rexx/ast.h"
#include "lazarus/rexx/lexer.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <cctype>

namespace lazarus { namespace rexx {

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens) : tokens_(tokens) {}

    Program parse() {
        Program prog;
        skip_newlines();
        while (!at_end()) {
            auto stmt = parse_statement();
            if (stmt) prog.statements.push_back(std::move(stmt));
            skip_newlines_and_semicolons();
        }
        return prog;
    }

private:
    const std::vector<Token>& tokens_;
    size_t pos_ = 0;

    // ── Token navigation ───────────────────────────────────────────────
    const Token& current() const {
        if (pos_ < tokens_.size()) return tokens_[pos_];
        static Token eof(TokenType::END_OF_INPUT, "", 0, 0);
        return eof;
    }

    const Token& peek(size_t ahead = 1) const {
        size_t idx = pos_ + ahead;
        if (idx < tokens_.size()) return tokens_[idx];
        static Token eof(TokenType::END_OF_INPUT, "", 0, 0);
        return eof;
    }

    bool at_end() const {
        return pos_ >= tokens_.size() || current().type == TokenType::END_OF_INPUT;
    }

    void advance_tok() {
        if (pos_ < tokens_.size()) ++pos_;
    }

    bool check(TokenType t) const { return current().type == t; }

    bool match(TokenType t) {
        if (check(t)) { advance_tok(); return true; }
        return false;
    }

    Token expect(TokenType t) {
        if (check(t)) {
            Token tok = current();
            advance_tok();
            return tok;
        }
        throw std::runtime_error(
            std::string("Expected ") + token_type_name(t) +
            " but got " + token_type_name(current().type) +
            " ('" + current().value + "') at line " +
            std::to_string(current().line));
    }

    void skip_newlines() {
        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
            advance_tok();
    }

    void skip_newlines_and_semicolons() {
        while (check(TokenType::NEWLINE) || check(TokenType::SEMICOLON))
            advance_tok();
    }

    bool is_statement_terminator() const {
        return at_end() || check(TokenType::NEWLINE) ||
               check(TokenType::SEMICOLON) || check(TokenType::END) ||
               check(TokenType::ELSE) || check(TokenType::WHEN) ||
               check(TokenType::OTHERWISE) || check(TokenType::THEN);
    }

    std::string upper_val(const Token& t) const {
        std::string s = t.value;
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
        return s;
    }

    bool is_keyword(const std::string& upper) const {
        for (const auto& kw : kRexxKeywords) {
            if (upper == kw.keyword) return true;
        }
        return false;
    }

    // ── Statement parsing ──────────────────────────────────────────────
    StmtPtr parse_statement() {
        skip_newlines();
        if (at_end()) return nullptr;

        // Label
        if (check(TokenType::LABEL)) {
            auto lab = Stmt::make_label(current().value);
            advance_tok();
            return lab;
        }

        TokenType tt = current().type;
        switch (tt) {
            case TokenType::SAY:       return parse_say();
            case TokenType::PULL:      return parse_pull();
            case TokenType::PUSH:      return parse_push();
            case TokenType::QUEUE:     return parse_queue();
            case TokenType::IF:        return parse_if();
            case TokenType::DO:        return parse_do();
            case TokenType::SELECT:    return parse_select();
            case TokenType::CALL:      return parse_call();
            case TokenType::RETURN:    return parse_return();
            case TokenType::EXIT:      return parse_exit();
            case TokenType::SIGNAL:    return parse_signal();
            case TokenType::ADDRESS:   return parse_address();
            case TokenType::PARSE:     return parse_parse();
            case TokenType::ARG:       return parse_arg();
            case TokenType::DROP:      return parse_drop();
            case TokenType::NOP:       advance_tok(); return Stmt::make_nop();
            case TokenType::ITERATE:   return parse_iterate();
            case TokenType::LEAVE:     return parse_leave();
            case TokenType::INTERPRET: return parse_interpret();
            case TokenType::NUMERIC:   return parse_numeric();
            case TokenType::TRACE:     return parse_trace();
            case TokenType::PROCEDURE: return parse_procedure();
            default: break;
        }

        // Assignment or expression statement: ident = expr
        if (check(TokenType::IDENTIFIER)) {
            return parse_assignment_or_expr();
        }

        // Skip unknown token
        advance_tok();
        return nullptr;
    }

    // SAY expr
    StmtPtr parse_say() {
        advance_tok(); // consume SAY
        if (is_statement_terminator())
            return Stmt::make_say(Expr::make_literal(""));
        auto e = parse_expression();
        return Stmt::make_say(std::move(e));
    }

    // PULL template
    StmtPtr parse_pull() {
        advance_tok(); // consume PULL
        auto tmpl = parse_template();
        return Stmt::make_pull(std::move(tmpl));
    }

    // PUSH expr
    StmtPtr parse_push() {
        advance_tok();
        if (is_statement_terminator())
            return Stmt::make_push(Expr::make_literal(""));
        return Stmt::make_push(parse_expression());
    }

    // QUEUE expr
    StmtPtr parse_queue() {
        advance_tok();
        if (is_statement_terminator())
            return Stmt::make_queue(Expr::make_literal(""));
        return Stmt::make_queue(parse_expression());
    }

    // IF cond THEN stmt [ELSE stmt]
    StmtPtr parse_if() {
        advance_tok(); // consume IF
        auto cond = parse_expression();
        skip_newlines();
        expect(TokenType::THEN);
        skip_newlines();
        StmtPtr then_b;
        if (check(TokenType::NOP)) {
            advance_tok();
            then_b = Stmt::make_nop();
        } else if (check(TokenType::DO)) {
            then_b = parse_do();
        } else {
            then_b = parse_statement();
        }
        skip_newlines();
        StmtPtr else_b = nullptr;
        if (match(TokenType::ELSE)) {
            skip_newlines();
            if (check(TokenType::NOP)) {
                advance_tok();
                else_b = Stmt::make_nop();
            } else if (check(TokenType::DO)) {
                else_b = parse_do();
            } else {
                else_b = parse_statement();
            }
        }
        return Stmt::make_if(std::move(cond), std::move(then_b), std::move(else_b));
    }

    // DO variants
    StmtPtr parse_do() {
        advance_tok(); // consume DO
        skip_newlines();

        // DO FOREVER
        if (check(TokenType::FOREVER)) {
            advance_tok();
            auto body = parse_do_body();
            return Stmt::make_do_forever(std::move(body));
        }

        // DO WHILE cond
        if (check(TokenType::WHILE)) {
            advance_tok();
            auto cond = parse_expression();
            auto body = parse_do_body();
            return Stmt::make_do_while(std::move(cond), std::move(body));
        }

        // DO UNTIL cond
        if (check(TokenType::UNTIL)) {
            advance_tok();
            auto cond = parse_expression();
            auto body = parse_do_body();
            return Stmt::make_do_until(std::move(cond), std::move(body));
        }

        // DO i = start TO end [BY step]
        if (check(TokenType::IDENTIFIER) && peek().type == TokenType::EQUAL) {
            std::string var = current().value;
            advance_tok(); // identifier
            advance_tok(); // =
            auto start = parse_expression();
            expect(TokenType::TO);
            auto end_e = parse_expression();
            ExprPtr step = nullptr;
            if (match(TokenType::BY)) {
                step = parse_expression();
            }
            auto body = parse_do_body();
            return Stmt::make_do_controlled(var, std::move(start),
                                             std::move(end_e), std::move(step),
                                             std::move(body));
        }

        // DO n (counted)
        if (check(TokenType::INTEGER) || check(TokenType::IDENTIFIER) || check(TokenType::LPAREN)) {
            // Check if next non-newline is a statement terminator or newline
            // (i.e., it's a simple count expression)
            if (check(TokenType::INTEGER)) {
                auto cnt = parse_expression();
                auto body = parse_do_body();
                return Stmt::make_do_counted(std::move(cnt), std::move(body));
            }
        }

        // Simple DO; ...END
        auto body = parse_do_body();
        return Stmt::make_do_simple(std::move(body));
    }

    std::vector<StmtPtr> parse_do_body() {
        std::vector<StmtPtr> body;
        skip_newlines();
        while (!at_end() && !check(TokenType::END)) {
            auto s = parse_statement();
            if (s) body.push_back(std::move(s));
            skip_newlines_and_semicolons();
        }
        if (check(TokenType::END)) advance_tok();
        // Optionally consume loop var name after END
        if (check(TokenType::IDENTIFIER)) advance_tok();
        return body;
    }

    // SELECT; WHEN cond THEN stmt; ... OTHERWISE; stmts; END
    StmtPtr parse_select() {
        advance_tok(); // consume SELECT
        skip_newlines_and_semicolons();
        std::vector<WhenClause> whens;
        while (check(TokenType::WHEN)) {
            advance_tok();
            auto cond = parse_expression();
            skip_newlines();
            expect(TokenType::THEN);
            skip_newlines();
            WhenClause wc;
            wc.condition = std::move(cond);
            if (check(TokenType::DO)) {
                wc.body.push_back(parse_do());
            } else if (check(TokenType::NOP)) {
                advance_tok();
                wc.body.push_back(Stmt::make_nop());
            } else {
                wc.body.push_back(parse_statement());
            }
            whens.push_back(std::move(wc));
            skip_newlines_and_semicolons();
        }
        std::vector<StmtPtr> otherwise;
        if (match(TokenType::OTHERWISE)) {
            skip_newlines_and_semicolons();
            while (!at_end() && !check(TokenType::END)) {
                auto s = parse_statement();
                if (s) otherwise.push_back(std::move(s));
                skip_newlines_and_semicolons();
            }
        }
        if (check(TokenType::END)) advance_tok();
        return Stmt::make_select(std::move(whens), std::move(otherwise));
    }

    // CALL routine [args] | CALL ON cond | CALL OFF cond
    StmtPtr parse_call() {
        advance_tok(); // consume CALL
        if (match(TokenType::ON)) {
            auto ct = parse_condition_type();
            std::string label;
            if (check(TokenType::IDENTIFIER)) {
                label = current().value;
                advance_tok();
            }
            return Stmt::make_call_on(ct, label);
        }
        if (match(TokenType::OFF)) {
            auto ct = parse_condition_type();
            return Stmt::make_call_off(ct);
        }
        std::string name;
        if (check(TokenType::IDENTIFIER) || check(TokenType::STRING)) {
            name = current().value;
            advance_tok();
        } else {
            // Could be a keyword used as routine name
            name = current().value;
            advance_tok();
        }
        std::vector<ExprPtr> args;
        while (!is_statement_terminator()) {
            args.push_back(parse_expression());
            if (!match(TokenType::COMMA)) break;
        }
        return Stmt::make_call(name, std::move(args));
    }

    // RETURN [expr]
    StmtPtr parse_return() {
        advance_tok();
        if (is_statement_terminator()) return Stmt::make_return();
        return Stmt::make_return(parse_expression());
    }

    // EXIT [expr]
    StmtPtr parse_exit() {
        advance_tok();
        if (is_statement_terminator()) return Stmt::make_exit();
        return Stmt::make_exit(parse_expression());
    }

    // SIGNAL label | SIGNAL ON cond | SIGNAL OFF cond
    StmtPtr parse_signal() {
        advance_tok();
        if (match(TokenType::ON)) {
            auto ct = parse_condition_type();
            std::string label;
            // NAME keyword
            if (check(TokenType::IDENTIFIER) && upper_val(current()) == "NAME") {
                advance_tok();
                if (check(TokenType::IDENTIFIER)) {
                    label = current().value;
                    advance_tok();
                }
            } else if (check(TokenType::IDENTIFIER)) {
                label = current().value;
                advance_tok();
            }
            return Stmt::make_signal_on(ct, label);
        }
        if (match(TokenType::OFF)) {
            auto ct = parse_condition_type();
            return Stmt::make_signal_off(ct);
        }
        std::string label = current().value;
        advance_tok();
        return Stmt::make_signal(label);
    }

    // ADDRESS env [cmd]
    StmtPtr parse_address() {
        advance_tok();
        std::string env;
        if (check(TokenType::IDENTIFIER) || check(TokenType::STRING)) {
            env = current().value;
            advance_tok();
        }
        ExprPtr cmd = nullptr;
        if (!is_statement_terminator()) {
            cmd = parse_expression();
        }
        return Stmt::make_address(env, std::move(cmd));
    }

    // PARSE [UPPER] source template
    StmtPtr parse_parse() {
        advance_tok(); // consume PARSE
        bool do_upper = false;
        if (check(TokenType::UPPER)) {
            do_upper = true;
            advance_tok();
        }
        (void)do_upper; // recorded but not structurally critical

        ParseSource src = ParseSource::Arg;
        std::string var_name;

        if (match(TokenType::ARG)) { src = ParseSource::Arg; }
        else if (match(TokenType::PULL)) { src = ParseSource::Pull; }
        else if (check(TokenType::IDENTIFIER) && upper_val(current()) == "VAR") {
            advance_tok();
            src = ParseSource::Var;
            var_name = current().value;
            advance_tok();
        } else if (match(TokenType::VALUE)) {
            src = ParseSource::Value;
            // parse expression WITH template
        } else if (check(TokenType::IDENTIFIER) && upper_val(current()) == "SOURCE") {
            advance_tok();
            src = ParseSource::Source;
        } else if (check(TokenType::IDENTIFIER) && upper_val(current()) == "VERSION") {
            advance_tok();
            src = ParseSource::Version;
        } else if (check(TokenType::IDENTIFIER) && upper_val(current()) == "LINEIN") {
            advance_tok();
            src = ParseSource::LinIn;
        }

        auto tmpl = parse_template();
        return Stmt::make_parse(src, std::move(tmpl), var_name);
    }

    // ARG template (shorthand for PARSE UPPER ARG)
    StmtPtr parse_arg() {
        advance_tok();
        auto tmpl = parse_template();
        return Stmt::make_parse(ParseSource::Arg, std::move(tmpl));
    }

    // DROP varlist
    StmtPtr parse_drop() {
        advance_tok();
        std::vector<std::string> vars;
        while (!is_statement_terminator()) {
            vars.push_back(current().value);
            advance_tok();
        }
        return Stmt::make_drop(std::move(vars));
    }

    // ITERATE [name]
    StmtPtr parse_iterate() {
        advance_tok();
        std::string name;
        if (check(TokenType::IDENTIFIER)) {
            name = current().value;
            advance_tok();
        }
        return Stmt::make_iterate(name);
    }

    // LEAVE [name]
    StmtPtr parse_leave() {
        advance_tok();
        std::string name;
        if (check(TokenType::IDENTIFIER)) {
            name = current().value;
            advance_tok();
        }
        return Stmt::make_leave(name);
    }

    // INTERPRET expr
    StmtPtr parse_interpret() {
        advance_tok();
        return Stmt::make_interpret(parse_expression());
    }

    // NUMERIC DIGITS|FUZZ|FORM expr
    StmtPtr parse_numeric() {
        advance_tok();
        NumericSetting ns = NumericSetting::Digits;
        std::string upper = upper_val(current());
        if (upper == "DIGITS") { ns = NumericSetting::Digits; advance_tok(); }
        else if (upper == "FUZZ") { ns = NumericSetting::Fuzz; advance_tok(); }
        else if (upper == "FORM") { ns = NumericSetting::Form; advance_tok(); }
        ExprPtr val = nullptr;
        if (!is_statement_terminator()) val = parse_expression();
        return Stmt::make_numeric(ns, std::move(val));
    }

    // TRACE setting
    StmtPtr parse_trace() {
        advance_tok();
        std::string setting;
        if (!is_statement_terminator()) {
            setting = current().value;
            advance_tok();
        }
        return Stmt::make_trace(setting);
    }

    // PROCEDURE [EXPOSE varlist]
    StmtPtr parse_procedure() {
        advance_tok();
        std::vector<std::string> exposed;
        if (match(TokenType::EXPOSE)) {
            while (!is_statement_terminator()) {
                exposed.push_back(current().value);
                advance_tok();
            }
        }
        return Stmt::make_procedure(std::move(exposed));
    }

    // Assignment: identifier = expr  or  stem.tail = expr
    StmtPtr parse_assignment_or_expr() {
        // Look ahead for = (assignment)
        std::string name = current().value;

        // Check for stem assignment: ident.tail = expr
        // But first check simple assignment
        size_t save = pos_;
        advance_tok(); // skip identifier

        // stem access: name.something
        if (check(TokenType::DOT)) {
            // Could be stem assignment or stem in expression
            // Look ahead: stem.tail.tail... = expr
            std::string stem = name;
            std::vector<ExprPtr> tails;
            while (match(TokenType::DOT)) {
                if (check(TokenType::IDENTIFIER) || check(TokenType::INTEGER)) {
                    tails.push_back(Expr::make_literal(current().value));
                    advance_tok();
                } else {
                    tails.push_back(Expr::make_literal(""));
                }
            }
            if (match(TokenType::EQUAL)) {
                auto val = parse_expression();
                return Stmt::make_stem_assign(stem, std::move(tails), std::move(val));
            }
            // Not assignment — rewind and parse as expression
            pos_ = save;
            auto e = parse_expression();
            return Stmt::make_say(std::move(e)); // expression as statement
        }

        if (match(TokenType::EQUAL)) {
            auto val = parse_expression();
            return Stmt::make_assign(name, std::move(val));
        }

        // Not assignment, rewind and parse as expression
        pos_ = save;
        auto e = parse_expression();
        // Expression statement (implicitly a SAY in some contexts)
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Say;
        s->expr = std::move(e);
        return s;
    }

    // ── Template parsing (for PARSE/PULL/ARG) ──────────────────────────
    std::vector<TemplateItem> parse_template() {
        std::vector<TemplateItem> items;
        while (!is_statement_terminator()) {
            if (check(TokenType::STRING)) {
                items.push_back(TemplateItem::lit(current().value));
                advance_tok();
            } else if (check(TokenType::DOT)) {
                items.push_back(TemplateItem::dot());
                advance_tok();
            } else if (check(TokenType::EQUAL) && peek().type == TokenType::INTEGER) {
                advance_tok(); // =
                int p = std::stoi(current().value);
                advance_tok();
                items.push_back(TemplateItem::pos(p));
            } else if (check(TokenType::PLUS) && peek().type == TokenType::INTEGER) {
                advance_tok(); // +
                int p = std::stoi(current().value);
                advance_tok();
                items.push_back(TemplateItem::rel(p));
            } else if (check(TokenType::MINUS) && peek().type == TokenType::INTEGER) {
                advance_tok(); // -
                int p = -std::stoi(current().value);
                advance_tok();
                items.push_back(TemplateItem::rel(p));
            } else if (check(TokenType::IDENTIFIER)) {
                items.push_back(TemplateItem::var(current().value));
                advance_tok();
            } else if (check(TokenType::INTEGER)) {
                // Positional number (absolute column)
                int p = std::stoi(current().value);
                advance_tok();
                items.push_back(TemplateItem::pos(p));
            } else {
                break;
            }
        }
        return items;
    }

    // ── Condition type ─────────────────────────────────────────────────
    ConditionType parse_condition_type() {
        std::string name = upper_val(current());
        advance_tok();
        if (name == "ERROR")      return ConditionType::Error;
        if (name == "FAILURE")    return ConditionType::Failure;
        if (name == "HALT")       return ConditionType::Halt;
        if (name == "NOVALUE")    return ConditionType::NoValue;
        if (name == "SYNTAX")     return ConditionType::Syntax;
        if (name == "LOSTDIGITS") return ConditionType::LostDigits;
        if (name == "NOTREADY")   return ConditionType::NotReady;
        return ConditionType::Error;
    }

    // ── Expression parsing (precedence climbing) ───────────────────────
    ExprPtr parse_expression() {
        return parse_or_expr();
    }

    ExprPtr parse_or_expr() {
        auto left = parse_and_expr();
        while (check(TokenType::OR)) {
            advance_tok();
            auto right = parse_and_expr();
            left = Expr::make_binary(BinOp::LogicalOr, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_and_expr() {
        auto left = parse_comparison_expr();
        while (check(TokenType::AND)) {
            advance_tok();
            auto right = parse_comparison_expr();
            left = Expr::make_binary(BinOp::LogicalAnd, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_comparison_expr() {
        auto left = parse_concat_expr();
        BinOp op;
        bool is_cmp = true;
        switch (current().type) {
            case TokenType::EQUAL:                    op = BinOp::Eq; break;
            case TokenType::BACKSLASH_EQUAL:          op = BinOp::Ne; break;
            case TokenType::LESS:                     op = BinOp::Lt; break;
            case TokenType::GREATER:                  op = BinOp::Gt; break;
            case TokenType::LESS_EQUAL:               op = BinOp::Le; break;
            case TokenType::GREATER_EQUAL:            op = BinOp::Ge; break;
            case TokenType::STRICT_EQUAL:             op = BinOp::StrictEq; break;
            case TokenType::STRICT_BACKSLASH_EQUAL:   op = BinOp::StrictNe; break;
            case TokenType::STRICT_LESS:              op = BinOp::StrictLt; break;
            case TokenType::STRICT_GREATER:           op = BinOp::StrictGt; break;
            default: is_cmp = false; op = BinOp::Eq; break;
        }
        if (is_cmp) {
            advance_tok();
            auto right = parse_concat_expr();
            return Expr::make_comparison(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_concat_expr() {
        auto left = parse_additive_expr();
        while (check(TokenType::CONCAT)) {
            advance_tok();
            auto right = parse_additive_expr();
            left = Expr::make_binary(BinOp::Concat, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_additive_expr() {
        auto left = parse_multiplicative_expr();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            BinOp op = check(TokenType::PLUS) ? BinOp::Add : BinOp::Sub;
            advance_tok();
            auto right = parse_multiplicative_expr();
            left = Expr::make_binary(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_multiplicative_expr() {
        auto left = parse_power_expr();
        while (check(TokenType::STAR) || check(TokenType::SLASH) ||
               check(TokenType::PERCENT) || check(TokenType::DBLSLASH)) {
            BinOp op;
            if (check(TokenType::STAR)) op = BinOp::Mul;
            else if (check(TokenType::SLASH)) op = BinOp::Div;
            else if (check(TokenType::PERCENT)) op = BinOp::IntDiv;
            else op = BinOp::Mod;
            advance_tok();
            auto right = parse_power_expr();
            left = Expr::make_binary(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_power_expr() {
        auto left = parse_unary_expr();
        if (check(TokenType::POWER)) {
            advance_tok();
            auto right = parse_power_expr(); // right-associative
            left = Expr::make_binary(BinOp::Power, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_unary_expr() {
        if (check(TokenType::MINUS)) {
            advance_tok();
            auto operand = parse_primary_expr();
            return Expr::make_unary(UnOp::Negate, std::move(operand));
        }
        if (check(TokenType::NOT) || check(TokenType::BACKSLASH_EQUAL)) {
            if (check(TokenType::NOT)) {
                advance_tok();
                auto operand = parse_primary_expr();
                return Expr::make_unary(UnOp::Not, std::move(operand));
            }
        }
        if (check(TokenType::PLUS)) {
            advance_tok(); // unary plus, ignore
        }
        return parse_primary_expr();
    }

    ExprPtr parse_primary_expr() {
        // Parenthesized
        if (match(TokenType::LPAREN)) {
            auto e = parse_expression();
            expect(TokenType::RPAREN);
            return Expr::make_paren(std::move(e));
        }

        // String literal
        if (check(TokenType::STRING) || check(TokenType::HEX_STRING) ||
            check(TokenType::BINARY_STRING)) {
            std::string val = current().value;
            if (current().type == TokenType::HEX_STRING) {
                val = decode_hex_string(val);
            } else if (current().type == TokenType::BINARY_STRING) {
                val = decode_binary_string(val);
            }
            advance_tok();
            return Expr::make_literal(val);
        }

        // Number literal
        if (check(TokenType::INTEGER) || check(TokenType::FLOAT)) {
            std::string val = current().value;
            advance_tok();
            return Expr::make_literal(val);
        }

        // Identifier — could be variable, stem, or function call
        if (check(TokenType::IDENTIFIER)) {
            std::string name = current().value;
            advance_tok();

            // Function call: name(args)
            if (check(TokenType::LPAREN)) {
                advance_tok();
                std::vector<ExprPtr> args;
                if (!check(TokenType::RPAREN)) {
                    args.push_back(parse_expression());
                    while (match(TokenType::COMMA)) {
                        args.push_back(parse_expression());
                    }
                }
                expect(TokenType::RPAREN);
                return Expr::make_call(name, std::move(args));
            }

            // Stem access: name.tail
            if (check(TokenType::DOT)) {
                std::vector<ExprPtr> tails;
                while (match(TokenType::DOT)) {
                    if (check(TokenType::IDENTIFIER) || check(TokenType::INTEGER)) {
                        tails.push_back(Expr::make_variable(current().value));
                        advance_tok();
                    } else {
                        tails.push_back(Expr::make_literal(""));
                    }
                }
                return Expr::make_stem(name, std::move(tails));
            }

            return Expr::make_variable(name);
        }

        // Keyword used as identifier (e.g., VALUE, FORM, etc.)
        if (!at_end() && !is_statement_terminator()) {
            std::string val = current().value;
            advance_tok();
            return Expr::make_literal(val);
        }

        return Expr::make_literal("");
    }

    // ── Hex/Binary decode ──────────────────────────────────────────────
    static std::string decode_hex_string(const std::string& hex) {
        std::string result;
        std::string clean;
        for (char c : hex) {
            if (!std::isspace(static_cast<unsigned char>(c))) clean += c;
        }
        for (size_t i = 0; i + 1 < clean.size(); i += 2) {
            int hi = hex_digit(clean[i]);
            int lo = hex_digit(clean[i + 1]);
            if (hi >= 0 && lo >= 0) {
                result += static_cast<char>((hi << 4) | lo);
            }
        }
        return result;
    }

    static int hex_digit(char c) {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    }

    static std::string decode_binary_string(const std::string& bin) {
        std::string result;
        std::string clean;
        for (char c : bin) {
            if (c == '0' || c == '1') clean += c;
        }
        // Pad to multiple of 8
        while (clean.size() % 8 != 0) clean = "0" + clean;
        for (size_t i = 0; i < clean.size(); i += 8) {
            char byte = 0;
            for (int j = 0; j < 8; ++j) {
                byte = static_cast<char>((byte << 1) | (clean[i + static_cast<size_t>(j)] - '0'));
            }
            result += byte;
        }
        return result;
    }
};

}} // namespace lazarus::rexx

#endif // LAZARUS_REXX_PARSER_H
