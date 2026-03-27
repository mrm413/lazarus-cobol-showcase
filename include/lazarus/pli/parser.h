#ifndef LAZARUS_PLI_PARSER_H
#define LAZARUS_PLI_PARSER_H

#include "lazarus/pli/lexer.h"
#include "lazarus/pli/ast.h"
#include <stdexcept>
#include <algorithm>
#include <sstream>

namespace lazarus { namespace pli {

class ParseError : public std::runtime_error {
public:
    int line, col;
    ParseError(const std::string& msg, int ln, int c)
        : std::runtime_error(msg), line(ln), col(c) {}
};

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens)
        : tokens_(tokens) {}

    Program parse_program() {
        Program prog;
        // Expect: name : PROCEDURE OPTIONS(MAIN) ;
        // In PL/I the program name can be a keyword (e.g., MAIN, TEST, etc.)
        if (check(TokenType::IDENTIFIER) || is_keyword_as_identifier(current().type)) {
            prog.name = current().value;
            advance();
            if (check(TokenType::COLON)) advance(); // optional colon
            expect(TokenType::PROCEDURE);
            // Parse procedure attributes
            while (!check(TokenType::SEMICOLON) && !at_end()) {
                if (check(TokenType::OPTIONS)) {
                    advance();
                    expect(TokenType::LPAREN);
                    if (check(TokenType::MAIN)) {
                        prog.options_main = true;
                        advance();
                    }
                    expect(TokenType::RPAREN);
                } else if (check(TokenType::RECURSIVE)) {
                    advance();
                } else if (check(TokenType::RETURNS)) {
                    advance();
                    if (check(TokenType::LPAREN)) {
                        advance();
                        while (!check(TokenType::RPAREN) && !at_end()) advance();
                        expect(TokenType::RPAREN);
                    }
                } else {
                    advance();
                }
            }
            expect(TokenType::SEMICOLON);
        }

        // Parse body: declarations and statements until END
        parse_body(prog.declarations, prog.statements);

        return prog;
    }

    // Parse a single declaration (for testing)
    DeclPtr parse_declaration() {
        if (check(TokenType::DCL) || check(TokenType::DECLARE)) {
            return parse_dcl();
        }
        throw ParseError("Expected DCL/DECLARE", current().line, current().column);
    }

    // Parse a single statement (for testing)
    StmtPtr parse_statement() {
        return parse_stmt();
    }

    // Parse a single expression (for testing)
    ExprPtr parse_expression() {
        return parse_expr();
    }

private:
    const std::vector<Token>& tokens_;
    size_t pos_ = 0;

    const Token& current() const {
        if (pos_ < tokens_.size()) return tokens_[pos_];
        static Token eof(TokenType::END_OF_INPUT, "", 0, 0);
        return eof;
    }

    const Token& peek_ahead(size_t offset = 1) const {
        size_t idx = pos_ + offset;
        if (idx < tokens_.size()) return tokens_[idx];
        static Token eof(TokenType::END_OF_INPUT, "", 0, 0);
        return eof;
    }

    bool at_end() const { return pos_ >= tokens_.size() || current().type == TokenType::END_OF_INPUT; }

    bool check(TokenType t) const { return !at_end() && current().type == t; }

    Token advance() {
        Token t = current();
        if (!at_end()) pos_++;
        return t;
    }

    Token expect(TokenType t) {
        if (!check(t)) {
            throw ParseError(
                std::string("Expected ") + token_type_name(t) + " but got " +
                token_type_name(current().type) + " '" + current().value + "'",
                current().line, current().column);
        }
        return advance();
    }

    void skip_comments() {
        while (check(TokenType::COMMENT)) advance();
    }

    // PL/I allows keywords as identifiers in expression context
    static bool is_keyword_as_identifier(TokenType t) {
        switch (t) {
            case TokenType::MAIN: case TokenType::RECORD:
            case TokenType::STREAM: case TokenType::SEQUENTIAL:
            case TokenType::DIRECT: case TokenType::KEYED:
            case TokenType::PRINT: case TokenType::INPUT:
            case TokenType::OUTPUT: case TokenType::UPDATE:
            case TokenType::LIST: case TokenType::EDIT:
            case TokenType::DATA: case TokenType::VARYING:
            case TokenType::FIXED: case TokenType::FLOAT:
            case TokenType::CHAR: case TokenType::BIT:
            case TokenType::POINTER: case TokenType::LABEL:
            case TokenType::FILE_KW: case TokenType::AREA:
            case TokenType::OFFSET: case TokenType::COMPLEX:
            case TokenType::DEC: case TokenType::BIN:
            case TokenType::ALIGNED: case TokenType::UNALIGNED:
            case TokenType::AUTOMATIC: case TokenType::STATIC:
            case TokenType::BASED: case TokenType::DEFINED:
            case TokenType::CONTROLLED: case TokenType::EXTERNAL:
            case TokenType::INTERNAL: case TokenType::ENTRY:
            case TokenType::STRING_KW: case TokenType::SKIP:
            case TokenType::PAGE: case TokenType::LINE_KW:
            case TokenType::COLUMN: case TokenType::TAB:
            case TokenType::INITIAL: case TokenType::LIKE:
            case TokenType::PRECISION: case TokenType::PICTURE:
            case TokenType::RECURSIVE: case TokenType::OPTIONS:
            case TokenType::RETURNS: case TokenType::NULL_KW:
            case TokenType::FREE: case TokenType::STOP:
                return true;
            default:
                return false;
        }
    }

    // ---------- Body parsing (decls + stmts) ----------

    void parse_body(std::vector<DeclPtr>& decls, std::vector<StmtPtr>& stmts) {
        while (!at_end() && !check(TokenType::END)) {
            skip_comments();
            if (at_end() || check(TokenType::END)) break;

            if (check(TokenType::DCL) || check(TokenType::DECLARE)) {
                decls.push_back(parse_dcl());
            } else {
                stmts.push_back(parse_stmt());
            }
        }
        // Consume END name ;
        if (check(TokenType::END)) {
            advance();
            // Name after END can be a keyword used as proc name
            if (check(TokenType::IDENTIFIER) || is_keyword_as_identifier(current().type)) advance();
            if (check(TokenType::SEMICOLON)) advance();
        }
    }

    // ---------- Declaration parsing ----------

    DeclPtr parse_dcl() {
        int ln = current().line;
        advance(); // consume DCL/DECLARE

        // Check for structure (level number)
        if (check(TokenType::INTEGER_LIT)) {
            return parse_structure_dcl(ln);
        }

        std::string name;
        if (check(TokenType::IDENTIFIER)) {
            name = current().value;
            advance();
        } else if (check(TokenType::LPAREN)) {
            // factored declaration: DCL (a, b, c) FIXED;
            // parse just first name for simplicity
            advance();
            name = current().value;
            advance();
            while (!check(TokenType::RPAREN) && !at_end()) advance();
            expect(TokenType::RPAREN);
        }

        // Check for array dimensions
        std::vector<ArrayDimension> dims;
        if (check(TokenType::LPAREN)) {
            dims = parse_dimensions();
        }

        // Parse attributes until semicolon
        DataAttributes attrs = parse_attributes();

        if (check(TokenType::SEMICOLON)) advance();

        // Check for PROCEDURE declaration
        if (check(TokenType::PROCEDURE) ||
            (pos_ > 0 && tokens_[pos_-2].type == TokenType::SEMICOLON &&
             check(TokenType::IDENTIFIER) && peek_ahead().type == TokenType::COLON)) {
            // Not a procedure decl from DCL -- that's a separate path
        }

        if (attrs.base_type == BaseType::ENTRY) {
            EntryDecl ed;
            ed.name = name;
            ed.returns = attrs;
            return make_decl(std::move(ed), ln);
        }

        if (attrs.base_type == BaseType::LABEL) {
            return make_decl(LabelDecl{name}, ln);
        }

        if (attrs.base_type == BaseType::FILE) {
            FileDecl fd;
            fd.name = name;
            return make_decl(std::move(fd), ln);
        }

        if (!dims.empty()) {
            ArrayDecl ad;
            ad.name = name;
            ad.dimensions = std::move(dims);
            ad.attrs = attrs;
            return make_decl(std::move(ad), ln);
        }

        ScalarDecl sd;
        sd.name = name;
        sd.attrs = attrs;
        return make_decl(std::move(sd), ln);
    }

    DeclPtr parse_structure_dcl(int ln) {
        StructureDecl sd;
        sd.level = std::stoi(current().value);
        advance(); // consume level number

        if (check(TokenType::IDENTIFIER)) {
            sd.name = current().value;
            advance();
        }

        // Parse top-level attributes
        DataAttributes top_attrs = parse_attributes();
        (void)top_attrs;

        if (check(TokenType::COMMA)) advance();

        // Parse members
        while (check(TokenType::INTEGER_LIT)) {
            StructureMember member;
            member.level = std::stoi(current().value);
            advance();
            if (check(TokenType::IDENTIFIER)) {
                member.name = current().value;
                advance();
            }
            if (check(TokenType::LPAREN)) {
                member.dimensions = parse_dimensions();
            }
            member.attrs = parse_attributes();
            sd.members.push_back(std::move(member));
            if (check(TokenType::COMMA)) advance();
        }

        if (check(TokenType::SEMICOLON)) advance();
        return make_decl(std::move(sd), ln);
    }

    std::vector<ArrayDimension> parse_dimensions() {
        std::vector<ArrayDimension> dims;
        expect(TokenType::LPAREN);
        while (!check(TokenType::RPAREN) && !at_end()) {
            ArrayDimension d;
            if (check(TokenType::INTEGER_LIT)) {
                int first = std::stoi(current().value);
                advance();
                if (check(TokenType::COLON)) {
                    advance();
                    d.lower = first;
                    d.upper = std::stoi(expect(TokenType::INTEGER_LIT).value);
                } else {
                    d.lower = 1;
                    d.upper = first;
                }
            } else if (check(TokenType::STAR)) {
                advance();
                d.lower = 1;
                d.upper = -1; // dynamic
            }
            dims.push_back(d);
            if (check(TokenType::COMMA)) advance();
            else break;
        }
        expect(TokenType::RPAREN);
        return dims;
    }

    DataAttributes parse_attributes() {
        DataAttributes attrs;
        while (!check(TokenType::SEMICOLON) && !check(TokenType::COMMA) &&
               !check(TokenType::END_OF_INPUT) && !check(TokenType::RPAREN) &&
               !check(TokenType::INTEGER_LIT)) {
            TokenType t = current().type;
            switch (t) {
                case TokenType::FIXED: attrs.base_type = BaseType::FIXED; advance(); break;
                case TokenType::FLOAT: attrs.base_type = BaseType::FLOAT; advance(); break;
                case TokenType::CHAR: {
                    attrs.base_type = BaseType::CHAR;
                    advance();
                    if (check(TokenType::LPAREN)) {
                        advance();
                        if (check(TokenType::INTEGER_LIT)) {
                            attrs.char_length = std::stoi(current().value);
                            advance();
                        } else if (check(TokenType::STAR)) {
                            attrs.char_length = -1;
                            advance();
                        }
                        expect(TokenType::RPAREN);
                    }
                    break;
                }
                case TokenType::BIT: {
                    attrs.base_type = BaseType::BIT;
                    advance();
                    if (check(TokenType::LPAREN)) {
                        advance();
                        if (check(TokenType::INTEGER_LIT)) {
                            attrs.bit_length = std::stoi(current().value);
                            advance();
                        }
                        expect(TokenType::RPAREN);
                    }
                    break;
                }
                case TokenType::POINTER: attrs.base_type = BaseType::POINTER; advance(); break;
                case TokenType::LABEL: attrs.base_type = BaseType::LABEL; advance(); break;
                case TokenType::FILE_KW: attrs.base_type = BaseType::FILE; advance(); break;
                case TokenType::AREA: attrs.base_type = BaseType::AREA; advance(); break;
                case TokenType::OFFSET: attrs.base_type = BaseType::OFFSET; advance(); break;
                case TokenType::COMPLEX: attrs.base_type = BaseType::COMPLEX; advance(); break;
                case TokenType::ENTRY: attrs.base_type = BaseType::ENTRY; advance(); break;
                case TokenType::PICTURE: {
                    attrs.base_type = BaseType::PICTURE;
                    advance();
                    if (check(TokenType::STRING_LIT)) {
                        attrs.picture = current().value;
                        advance();
                    }
                    break;
                }
                case TokenType::DEC: attrs.scale = Scale::DECIMAL; advance(); break;
                case TokenType::BIN: attrs.scale = Scale::BINARY; advance(); break;
                case TokenType::PRECISION: {
                    advance();
                    if (check(TokenType::LPAREN)) {
                        advance();
                        if (check(TokenType::INTEGER_LIT)) {
                            attrs.precision = std::stoi(current().value);
                            advance();
                        }
                        if (check(TokenType::COMMA)) {
                            advance();
                            if (check(TokenType::INTEGER_LIT)) {
                                attrs.scale_factor = std::stoi(current().value);
                                advance();
                            }
                        }
                        expect(TokenType::RPAREN);
                    }
                    break;
                }
                case TokenType::VARYING: attrs.varying = true; advance(); break;
                case TokenType::BASED: {
                    attrs.storage = StorageClass::BASED;
                    advance();
                    if (check(TokenType::LPAREN)) {
                        advance();
                        if (check(TokenType::IDENTIFIER)) {
                            attrs.based_var = current().value;
                            advance();
                        }
                        expect(TokenType::RPAREN);
                    }
                    break;
                }
                case TokenType::DEFINED: {
                    attrs.storage = StorageClass::DEFINED;
                    advance();
                    if (check(TokenType::LPAREN)) {
                        advance();
                        if (check(TokenType::IDENTIFIER)) {
                            attrs.defined_var = current().value;
                            advance();
                        }
                        expect(TokenType::RPAREN);
                    }
                    break;
                }
                case TokenType::CONTROLLED: attrs.storage = StorageClass::CONTROLLED; advance(); break;
                case TokenType::AUTOMATIC: attrs.storage = StorageClass::AUTOMATIC; advance(); break;
                case TokenType::STATIC: attrs.storage = StorageClass::STATIC; advance(); break;
                case TokenType::EXTERNAL: attrs.scope = Scope::EXTERNAL; advance(); break;
                case TokenType::INTERNAL: attrs.scope = Scope::INTERNAL; advance(); break;
                case TokenType::ALIGNED: attrs.alignment = Alignment::ALIGNED; advance(); break;
                case TokenType::UNALIGNED: attrs.alignment = Alignment::UNALIGNED; advance(); break;
                case TokenType::INITIAL: {
                    attrs.is_initial = true;
                    advance();
                    if (check(TokenType::LPAREN)) {
                        advance();
                        if (check(TokenType::STRING_LIT) || check(TokenType::INTEGER_LIT) || check(TokenType::FLOAT_LIT)) {
                            attrs.initial_value = current().value;
                            advance();
                        }
                        expect(TokenType::RPAREN);
                    }
                    break;
                }
                case TokenType::RETURNS: {
                    attrs.returns_specified = true;
                    advance();
                    if (check(TokenType::LPAREN)) {
                        advance();
                        if (check(TokenType::FIXED)) { attrs.returns_type = BaseType::FIXED; advance(); }
                        else if (check(TokenType::FLOAT)) { attrs.returns_type = BaseType::FLOAT; advance(); }
                        else if (check(TokenType::CHAR)) { attrs.returns_type = BaseType::CHAR; advance(); }
                        else if (check(TokenType::BIT)) { attrs.returns_type = BaseType::BIT; advance(); }
                        else if (check(TokenType::POINTER)) { attrs.returns_type = BaseType::POINTER; advance(); }
                        while (!check(TokenType::RPAREN) && !at_end()) advance();
                        expect(TokenType::RPAREN);
                    }
                    break;
                }
                case TokenType::RECURSIVE: attrs.recursive = true; advance(); break;
                case TokenType::OPTIONS: {
                    advance();
                    if (check(TokenType::LPAREN)) {
                        advance();
                        if (check(TokenType::MAIN)) {
                            attrs.options_main = true;
                            advance();
                        }
                        while (!check(TokenType::RPAREN) && !at_end()) advance();
                        expect(TokenType::RPAREN);
                    }
                    break;
                }
                case TokenType::LIKE: {
                    advance();
                    if (check(TokenType::IDENTIFIER)) advance();
                    break;
                }
                default:
                    return attrs;
            }
        }
        return attrs;
    }

    // ---------- Statement parsing ----------

    StmtPtr parse_stmt() {
        skip_comments();
        if (at_end()) return make_stmt(NullStmt{});

        int ln = current().line;
        TokenType t = current().type;

        switch (t) {
            case TokenType::IF: return parse_if(ln);
            case TokenType::SELECT: return parse_select(ln);
            case TokenType::DO: return parse_do(ln);
            case TokenType::CALL: return parse_call(ln);
            case TokenType::RETURN: return parse_return(ln);
            case TokenType::PUT: return parse_put(ln);
            case TokenType::GET: return parse_get(ln);
            case TokenType::OPEN: return parse_open(ln);
            case TokenType::CLOSE: return parse_close(ln);
            case TokenType::READ: return parse_read(ln);
            case TokenType::WRITE: return parse_write(ln);
            case TokenType::REWRITE: return parse_rewrite(ln);
            case TokenType::DELETE_KW: return parse_delete(ln);
            case TokenType::ALLOCATE: return parse_allocate(ln);
            case TokenType::FREE: return parse_free(ln);
            case TokenType::ON: return parse_on(ln);
            case TokenType::SIGNAL: return parse_signal(ln);
            case TokenType::REVERT: return parse_revert(ln);
            case TokenType::STOP: advance(); if (check(TokenType::SEMICOLON)) advance(); return make_stmt(StopStmt{}, ln);
            case TokenType::DISPLAY: return parse_display(ln);
            case TokenType::GO_TO: return parse_goto(ln);
            case TokenType::LEAVE: {
                advance();
                std::string lbl;
                if (check(TokenType::IDENTIFIER)) { lbl = current().value; advance(); }
                if (check(TokenType::SEMICOLON)) advance();
                return make_stmt(LeaveStmt{lbl}, ln);
            }
            case TokenType::ITERATE: {
                advance();
                std::string lbl;
                if (check(TokenType::IDENTIFIER)) { lbl = current().value; advance(); }
                if (check(TokenType::SEMICOLON)) advance();
                return make_stmt(IterateStmt{lbl}, ln);
            }
            case TokenType::ENTRY: {
                advance();
                EntryStmt es;
                if (check(TokenType::IDENTIFIER)) { es.name = current().value; advance(); }
                if (check(TokenType::LPAREN)) {
                    advance();
                    while (!check(TokenType::RPAREN) && !at_end()) {
                        if (check(TokenType::IDENTIFIER)) { es.params.push_back(current().value); advance(); }
                        if (check(TokenType::COMMA)) advance();
                    }
                    expect(TokenType::RPAREN);
                }
                if (check(TokenType::SEMICOLON)) advance();
                return make_stmt(std::move(es), ln);
            }
            default: break;
        }

        // Assignment: identifier = expr ;  or  identifier(subscript) = expr ;
        if (check(TokenType::IDENTIFIER)) {
            // Check if it's label: stmt
            if (peek_ahead().type == TokenType::COLON) {
                advance(); // name
                advance(); // :
                return parse_stmt(); // parse the actual statement
            }
            return parse_assignment(ln);
        }

        // Skip unknown tokens
        advance();
        return make_stmt(NullStmt{}, ln);
    }

    StmtPtr parse_assignment(int ln) {
        // In PL/I, = is both assignment and comparison.
        // At statement level, parse target (primary+postfix) then check for =.
        ExprPtr target = parse_postfix();
        if (check(TokenType::EQ)) {
            advance();
            ExprPtr value = parse_expr();
            if (check(TokenType::SEMICOLON)) advance();
            return make_stmt(AssignmentStmt{std::move(target), std::move(value)}, ln);
        }
        // Not an assignment -- might be a call: PROC(args);
        if (check(TokenType::SEMICOLON)) {
            advance();
            if (auto* call = std::get_if<FunctionCallExpr>(&target->node)) {
                return make_stmt(CallStmt{call->name, std::move(call->args)}, ln);
            }
        }
        return make_stmt(NullStmt{}, ln);
    }

    StmtPtr parse_if(int ln) {
        advance(); // IF
        IfStmt s;
        s.condition = parse_expr();
        expect(TokenType::THEN);

        // THEN clause - single statement or block
        if (check(TokenType::DO)) {
            auto do_stmt = parse_do(current().line);
            s.then_body.push_back(std::move(do_stmt));
        } else {
            s.then_body.push_back(parse_stmt());
        }

        // ELSE clause
        if (check(TokenType::ELSE)) {
            advance();
            if (check(TokenType::DO)) {
                auto do_stmt = parse_do(current().line);
                s.else_body.push_back(std::move(do_stmt));
            } else if (check(TokenType::IF)) {
                s.else_body.push_back(parse_if(current().line));
            } else {
                s.else_body.push_back(parse_stmt());
            }
        }

        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_select(int ln) {
        advance(); // SELECT
        SelectStmt s;
        if (!check(TokenType::SEMICOLON) && !check(TokenType::WHEN)) {
            if (check(TokenType::LPAREN)) {
                advance();
                s.expr = parse_expr();
                expect(TokenType::RPAREN);
            } else {
                s.expr = parse_expr();
            }
        }
        if (check(TokenType::SEMICOLON)) advance();

        while (check(TokenType::WHEN)) {
            advance();
            WhenClause wc;
            expect(TokenType::LPAREN);
            while (!check(TokenType::RPAREN) && !at_end()) {
                wc.values.push_back(parse_expr());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN);

            // Parse when body
            if (check(TokenType::DO)) {
                wc.body.push_back(parse_do(current().line));
            } else {
                wc.body.push_back(parse_stmt());
            }
            s.whens.push_back(std::move(wc));
        }

        if (check(TokenType::OTHERWISE)) {
            advance();
            if (check(TokenType::DO)) {
                s.otherwise_body.push_back(parse_do(current().line));
            } else {
                s.otherwise_body.push_back(parse_stmt());
            }
        }

        if (check(TokenType::END)) {
            advance();
            if (check(TokenType::SEMICOLON)) advance();
        }

        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_do(int ln) {
        advance(); // DO
        DoLoopStmt s;

        if (check(TokenType::SEMICOLON)) {
            // Simple DO block
            s.kind = DoLoopStmt::SIMPLE;
            advance();
            while (!check(TokenType::END) && !at_end()) {
                skip_comments();
                if (check(TokenType::END)) break;
                s.body.push_back(parse_stmt());
            }
            if (check(TokenType::END)) { advance(); if (check(TokenType::SEMICOLON)) advance(); }
            return make_stmt(std::move(s), ln);
        }

        if (check(TokenType::WHILE)) {
            s.kind = DoLoopStmt::WHILE;
            advance();
            expect(TokenType::LPAREN);
            s.while_cond = parse_expr();
            expect(TokenType::RPAREN);
            if (check(TokenType::SEMICOLON)) advance();
            while (!check(TokenType::END) && !at_end()) {
                skip_comments();
                if (check(TokenType::END)) break;
                s.body.push_back(parse_stmt());
            }
            if (check(TokenType::END)) { advance(); if (check(TokenType::SEMICOLON)) advance(); }
            return make_stmt(std::move(s), ln);
        }

        if (check(TokenType::UNTIL)) {
            s.kind = DoLoopStmt::UNTIL;
            advance();
            expect(TokenType::LPAREN);
            s.until_cond = parse_expr();
            expect(TokenType::RPAREN);
            if (check(TokenType::SEMICOLON)) advance();
            while (!check(TokenType::END) && !at_end()) {
                skip_comments();
                if (check(TokenType::END)) break;
                s.body.push_back(parse_stmt());
            }
            if (check(TokenType::END)) { advance(); if (check(TokenType::SEMICOLON)) advance(); }
            return make_stmt(std::move(s), ln);
        }

        // Check for FOREVER
        if (check(TokenType::IDENTIFIER)) {
            std::string upper = current().value;
            for (auto& ch : upper) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            if (upper == "FOREVER") {
                s.kind = DoLoopStmt::FOREVER;
                advance();
                if (check(TokenType::SEMICOLON)) advance();
                while (!check(TokenType::END) && !at_end()) {
                    skip_comments();
                    if (check(TokenType::END)) break;
                    s.body.push_back(parse_stmt());
                }
                if (check(TokenType::END)) { advance(); if (check(TokenType::SEMICOLON)) advance(); }
                return make_stmt(std::move(s), ln);
            }
        }

        // Iterative: DO var = from TO to BY by
        if (check(TokenType::IDENTIFIER)) {
            s.kind = DoLoopStmt::ITERATIVE;
            s.variable = current().value;
            advance();
            expect(TokenType::EQ);
            s.from = parse_expr();

            if (check(TokenType::TO)) {
                advance();
                s.to = parse_expr();
            }
            if (check(TokenType::BY)) {
                advance();
                s.by = parse_expr();
            }
            // Optional WHILE/UNTIL after iterator
            if (check(TokenType::WHILE)) {
                advance();
                expect(TokenType::LPAREN);
                s.while_cond = parse_expr();
                expect(TokenType::RPAREN);
            }
            if (check(TokenType::UNTIL)) {
                advance();
                expect(TokenType::LPAREN);
                s.until_cond = parse_expr();
                expect(TokenType::RPAREN);
            }
            if (check(TokenType::SEMICOLON)) advance();
            while (!check(TokenType::END) && !at_end()) {
                skip_comments();
                if (check(TokenType::END)) break;
                s.body.push_back(parse_stmt());
            }
            if (check(TokenType::END)) { advance(); if (check(TokenType::SEMICOLON)) advance(); }
            return make_stmt(std::move(s), ln);
        }

        // Fallthrough: simple DO
        s.kind = DoLoopStmt::SIMPLE;
        if (check(TokenType::SEMICOLON)) advance();
        while (!check(TokenType::END) && !at_end()) {
            skip_comments();
            if (check(TokenType::END)) break;
            s.body.push_back(parse_stmt());
        }
        if (check(TokenType::END)) { advance(); if (check(TokenType::SEMICOLON)) advance(); }
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_call(int ln) {
        advance(); // CALL
        CallStmt s;
        s.target = expect(TokenType::IDENTIFIER).value;
        if (check(TokenType::LPAREN)) {
            advance();
            while (!check(TokenType::RPAREN) && !at_end()) {
                s.args.push_back(parse_expr());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN);
        }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_return(int ln) {
        advance(); // RETURN
        ReturnStmt s;
        if (check(TokenType::LPAREN)) {
            advance();
            s.value = parse_expr();
            expect(TokenType::RPAREN);
        }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_put(int ln) {
        advance(); // PUT

        std::string target_string;
        if (check(TokenType::STRING_KW)) {
            advance();
            expect(TokenType::LPAREN);
            if (check(TokenType::IDENTIFIER)) {
                target_string = current().value;
                advance();
            }
            expect(TokenType::RPAREN);
        }

        if (check(TokenType::EDIT)) {
            advance();
            PutEditStmt s;
            s.target_string = target_string;
            // Items
            expect(TokenType::LPAREN);
            while (!check(TokenType::RPAREN) && !at_end()) {
                s.items.push_back(parse_expr());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN);
            // Format
            expect(TokenType::LPAREN);
            while (!check(TokenType::RPAREN) && !at_end()) {
                s.formats.push_back(parse_format_item());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN);
            if (check(TokenType::SEMICOLON)) advance();
            return make_stmt(std::move(s), ln);
        }

        if (check(TokenType::DATA)) {
            advance();
            // PUT DATA is like PUT LIST with special formatting
            PutListStmt s;
            s.target_string = target_string;
            if (check(TokenType::LPAREN)) {
                advance();
                while (!check(TokenType::RPAREN) && !at_end()) {
                    s.items.push_back(parse_expr());
                    if (check(TokenType::COMMA)) advance();
                }
                expect(TokenType::RPAREN);
            }
            if (check(TokenType::SEMICOLON)) advance();
            return make_stmt(std::move(s), ln);
        }

        // Default: PUT LIST or PUT SKIP LIST
        PutListStmt s;
        s.target_string = target_string;
        if (check(TokenType::SKIP)) { s.skip = true; advance(); }
        if (check(TokenType::LIST)) advance();
        if (check(TokenType::LPAREN)) {
            advance();
            while (!check(TokenType::RPAREN) && !at_end()) {
                s.items.push_back(parse_expr());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN);
        }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_get(int ln) {
        advance(); // GET

        std::string source_string;
        if (check(TokenType::STRING_KW)) {
            advance();
            expect(TokenType::LPAREN);
            if (check(TokenType::IDENTIFIER)) {
                source_string = current().value;
                advance();
            }
            expect(TokenType::RPAREN);
        }

        if (check(TokenType::EDIT)) {
            advance();
            GetEditStmt s;
            s.source_string = source_string;
            expect(TokenType::LPAREN);
            while (!check(TokenType::RPAREN) && !at_end()) {
                s.targets.push_back(parse_expr());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN);
            expect(TokenType::LPAREN);
            while (!check(TokenType::RPAREN) && !at_end()) {
                s.formats.push_back(parse_format_item());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN);
            if (check(TokenType::SEMICOLON)) advance();
            return make_stmt(std::move(s), ln);
        }

        // GET LIST
        GetListStmt s;
        s.source_string = source_string;
        if (check(TokenType::LIST)) advance();
        if (check(TokenType::LPAREN)) {
            advance();
            while (!check(TokenType::RPAREN) && !at_end()) {
                s.targets.push_back(parse_expr());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN);
        }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    FormatItem parse_format_item() {
        FormatItem fi;
        TokenType t = current().type;

        if (t == TokenType::SKIP) {
            fi.kind = FormatItem::SKIP;
            advance();
            if (check(TokenType::LPAREN)) {
                advance();
                if (check(TokenType::INTEGER_LIT)) { fi.w = std::stoi(current().value); advance(); }
                expect(TokenType::RPAREN);
            } else {
                fi.w = 1;
            }
            return fi;
        }
        if (t == TokenType::PAGE) { fi.kind = FormatItem::PAGE; advance(); return fi; }
        if (t == TokenType::LINE_KW) {
            fi.kind = FormatItem::LINE_FMT;
            advance();
            if (check(TokenType::LPAREN)) {
                advance();
                if (check(TokenType::INTEGER_LIT)) { fi.w = std::stoi(current().value); advance(); }
                expect(TokenType::RPAREN);
            }
            return fi;
        }
        if (t == TokenType::COLUMN || t == TokenType::TAB) {
            fi.kind = (t == TokenType::COLUMN) ? FormatItem::COLUMN : FormatItem::TAB;
            advance();
            if (check(TokenType::LPAREN)) {
                advance();
                if (check(TokenType::INTEGER_LIT)) { fi.w = std::stoi(current().value); advance(); }
                expect(TokenType::RPAREN);
            }
            return fi;
        }

        // A, F, E, B, P, X format specifiers (as identifiers)
        if (check(TokenType::IDENTIFIER)) {
            std::string name = current().value;
            char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(name[0])));
            advance();

            if (upper == 'A' && name.size() == 1) {
                fi.kind = FormatItem::A;
                if (check(TokenType::LPAREN)) {
                    advance();
                    if (check(TokenType::INTEGER_LIT)) { fi.w = std::stoi(current().value); advance(); }
                    expect(TokenType::RPAREN);
                }
                return fi;
            }
            if (upper == 'F' && name.size() == 1) {
                fi.kind = FormatItem::F;
                if (check(TokenType::LPAREN)) {
                    advance();
                    if (check(TokenType::INTEGER_LIT)) { fi.w = std::stoi(current().value); advance(); }
                    if (check(TokenType::COMMA)) {
                        advance();
                        if (check(TokenType::INTEGER_LIT)) { fi.d = std::stoi(current().value); advance(); }
                    }
                    expect(TokenType::RPAREN);
                }
                return fi;
            }
            if (upper == 'E' && name.size() == 1) {
                fi.kind = FormatItem::E;
                if (check(TokenType::LPAREN)) {
                    advance();
                    if (check(TokenType::INTEGER_LIT)) { fi.w = std::stoi(current().value); advance(); }
                    if (check(TokenType::COMMA)) {
                        advance();
                        if (check(TokenType::INTEGER_LIT)) { fi.d = std::stoi(current().value); advance(); }
                    }
                    if (check(TokenType::COMMA)) {
                        advance();
                        if (check(TokenType::INTEGER_LIT)) { fi.s = std::stoi(current().value); advance(); }
                    }
                    expect(TokenType::RPAREN);
                }
                return fi;
            }
            if (upper == 'B' && name.size() == 1) {
                fi.kind = FormatItem::B;
                if (check(TokenType::LPAREN)) {
                    advance();
                    if (check(TokenType::INTEGER_LIT)) { fi.w = std::stoi(current().value); advance(); }
                    expect(TokenType::RPAREN);
                }
                return fi;
            }
            if (upper == 'P' && name.size() == 1) {
                fi.kind = FormatItem::P;
                if (check(TokenType::STRING_LIT)) {
                    fi.picture = current().value;
                    advance();
                }
                return fi;
            }
            if (upper == 'X' && name.size() == 1) {
                fi.kind = FormatItem::X;
                if (check(TokenType::LPAREN)) {
                    advance();
                    if (check(TokenType::INTEGER_LIT)) { fi.w = std::stoi(current().value); advance(); }
                    expect(TokenType::RPAREN);
                }
                return fi;
            }
            if (upper == 'R' && name.size() == 1) {
                fi.kind = FormatItem::R;
                if (check(TokenType::LPAREN)) {
                    advance();
                    advance(); // format name
                    expect(TokenType::RPAREN);
                }
                return fi;
            }
        }

        // Unknown format item, return A as default
        fi.kind = FormatItem::A;
        return fi;
    }

    StmtPtr parse_open(int ln) {
        advance(); // OPEN
        OpenStmt s;
        if (check(TokenType::FILE_KW)) advance();
        if (check(TokenType::LPAREN)) {
            advance();
            s.file_name = expect(TokenType::IDENTIFIER).value;
            expect(TokenType::RPAREN);
        } else if (check(TokenType::IDENTIFIER)) {
            s.file_name = current().value;
            advance();
        }
        s.mode = OpenStmt::INPUT;
        while (!check(TokenType::SEMICOLON) && !at_end()) {
            if (check(TokenType::INPUT)) { s.mode = OpenStmt::INPUT; advance(); }
            else if (check(TokenType::OUTPUT)) { s.mode = OpenStmt::OUTPUT; advance(); }
            else if (check(TokenType::UPDATE)) { s.mode = OpenStmt::UPDATE; advance(); }
            else if (check(TokenType::PRINT)) { s.mode = OpenStmt::PRINT; advance(); }
            else if (check(TokenType::STREAM)) { advance(); }
            else if (check(TokenType::RECORD)) { advance(); }
            else advance();
        }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_close(int ln) {
        advance(); // CLOSE
        CloseStmt s;
        if (check(TokenType::FILE_KW)) advance();
        if (check(TokenType::LPAREN)) {
            advance();
            s.file_name = expect(TokenType::IDENTIFIER).value;
            expect(TokenType::RPAREN);
        } else if (check(TokenType::IDENTIFIER)) {
            s.file_name = current().value;
            advance();
        }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_read(int ln) {
        advance(); // READ
        ReadStmt s;
        if (check(TokenType::FILE_KW)) advance();
        if (check(TokenType::LPAREN)) {
            advance();
            s.file_name = expect(TokenType::IDENTIFIER).value;
            expect(TokenType::RPAREN);
        } else if (check(TokenType::IDENTIFIER)) {
            s.file_name = current().value;
            advance();
        }
        if (check(TokenType::INTO)) { advance(); s.into = parse_expr(); }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_write(int ln) {
        advance(); // WRITE
        WriteStmt s;
        if (check(TokenType::FILE_KW)) advance();
        if (check(TokenType::LPAREN)) {
            advance();
            s.file_name = expect(TokenType::IDENTIFIER).value;
            expect(TokenType::RPAREN);
        } else if (check(TokenType::IDENTIFIER)) {
            s.file_name = current().value;
            advance();
        }
        if (check(TokenType::FROM)) { advance(); s.from = parse_expr(); }
        if (check(TokenType::KEYFROM)) { advance(); s.keyfrom = parse_expr(); }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_rewrite(int ln) {
        advance(); // REWRITE
        RewriteStmt s;
        if (check(TokenType::FILE_KW)) advance();
        if (check(TokenType::LPAREN)) {
            advance();
            s.file_name = expect(TokenType::IDENTIFIER).value;
            expect(TokenType::RPAREN);
        } else if (check(TokenType::IDENTIFIER)) {
            s.file_name = current().value;
            advance();
        }
        if (check(TokenType::FROM)) { advance(); s.from = parse_expr(); }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_delete(int ln) {
        advance(); // DELETE
        DeleteStmt s;
        if (check(TokenType::FILE_KW)) advance();
        if (check(TokenType::LPAREN)) {
            advance();
            s.file_name = expect(TokenType::IDENTIFIER).value;
            expect(TokenType::RPAREN);
        } else if (check(TokenType::IDENTIFIER)) {
            s.file_name = current().value;
            advance();
        }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_allocate(int ln) {
        advance(); // ALLOCATE
        AllocateStmt s;
        s.variable = expect(TokenType::IDENTIFIER).value;
        if (check(TokenType::SET)) {
            advance();
            expect(TokenType::LPAREN);
            s.set_target = parse_expr();
            expect(TokenType::RPAREN);
        }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_free(int ln) {
        advance(); // FREE
        FreeStmt s;
        s.variable = expect(TokenType::IDENTIFIER).value;
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_on(int ln) {
        advance(); // ON
        OnStmt s;
        // Condition name (e.g., ENDFILE, CONVERSION, etc.)
        if (check(TokenType::IDENTIFIER)) {
            s.condition = current().value;
            advance();
        } else {
            // Might be a keyword used as condition name
            s.condition = current().value;
            advance();
        }
        // Check for file spec: ENDFILE(filename)
        if (check(TokenType::LPAREN)) {
            advance();
            s.condition += "(" + current().value + ")";
            advance();
            expect(TokenType::RPAREN);
        }

        if (check(TokenType::IDENTIFIER)) {
            std::string upper = current().value;
            for (auto& ch : upper) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            if (upper == "SYSTEM") {
                s.is_system = true;
                advance();
                if (check(TokenType::SEMICOLON)) advance();
                return make_stmt(std::move(s), ln);
            }
        }

        if (check(TokenType::BEGIN)) {
            advance();
            if (check(TokenType::SEMICOLON)) advance();
            while (!check(TokenType::END) && !at_end()) {
                skip_comments();
                if (check(TokenType::END)) break;
                s.body.push_back(parse_stmt());
            }
            if (check(TokenType::END)) { advance(); if (check(TokenType::SEMICOLON)) advance(); }
        } else {
            s.body.push_back(parse_stmt());
        }
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_signal(int ln) {
        advance(); // SIGNAL
        SignalStmt s;
        s.condition = current().value;
        advance();
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_revert(int ln) {
        advance(); // REVERT
        RevertStmt s;
        s.condition = current().value;
        advance();
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_display(int ln) {
        advance(); // DISPLAY
        DisplayStmt s;
        if (check(TokenType::LPAREN)) {
            advance();
            while (!check(TokenType::RPAREN) && !at_end()) {
                s.items.push_back(parse_expr());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN);
        } else {
            // DISPLAY expr ;
            s.items.push_back(parse_expr());
        }
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    StmtPtr parse_goto(int ln) {
        advance(); // GO or GOTO
        // Handle GO TO (two words)
        if (check(TokenType::TO)) advance();
        GoToStmt s;
        s.label = expect(TokenType::IDENTIFIER).value;
        if (check(TokenType::SEMICOLON)) advance();
        return make_stmt(std::move(s), ln);
    }

    // ---------- Expression parsing (precedence climbing) ----------

    ExprPtr parse_expr() { return parse_or(); }

    ExprPtr parse_or() {
        auto left = parse_and();
        while (check(TokenType::OR_OP)) {
            advance();
            auto right = parse_and();
            left = make_binop(BinOp::OR, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_and() {
        auto left = parse_not();
        while (check(TokenType::AND_OP)) {
            advance();
            auto right = parse_not();
            left = make_binop(BinOp::AND, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_not() {
        if (check(TokenType::NOT)) {
            advance();
            auto operand = parse_not();
            return make_unaryop(UnaryOp::NOT, std::move(operand));
        }
        return parse_comparison();
    }

    ExprPtr parse_comparison() {
        auto left = parse_concat();
        while (check(TokenType::EQ) || check(TokenType::NE) ||
               check(TokenType::LT) || check(TokenType::GT) ||
               check(TokenType::LE) || check(TokenType::GE)) {
            BinOp op;
            switch (current().type) {
                case TokenType::EQ: op = BinOp::EQ; break;
                case TokenType::NE: op = BinOp::NE; break;
                case TokenType::LT: op = BinOp::LT; break;
                case TokenType::GT: op = BinOp::GT; break;
                case TokenType::LE: op = BinOp::LE; break;
                case TokenType::GE: op = BinOp::GE; break;
                default: op = BinOp::EQ; break;
            }
            advance();
            auto right = parse_concat();
            left = make_binop(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_concat() {
        auto left = parse_additive();
        while (check(TokenType::CONCAT)) {
            advance();
            auto right = parse_additive();
            left = make_binop(BinOp::CONCAT, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_additive() {
        auto left = parse_multiplicative();
        while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
            BinOp op = check(TokenType::PLUS) ? BinOp::ADD : BinOp::SUB;
            advance();
            auto right = parse_multiplicative();
            left = make_binop(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_multiplicative() {
        auto left = parse_power();
        while (check(TokenType::STAR) || check(TokenType::SLASH)) {
            BinOp op = check(TokenType::STAR) ? BinOp::MUL : BinOp::DIV;
            advance();
            auto right = parse_power();
            left = make_binop(op, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_power() {
        auto left = parse_unary();
        if (check(TokenType::POWER)) {
            advance();
            auto right = parse_power(); // right-associative
            left = make_binop(BinOp::POWER, std::move(left), std::move(right));
        }
        return left;
    }

    ExprPtr parse_unary() {
        if (check(TokenType::MINUS)) {
            advance();
            auto operand = parse_unary();
            return make_unaryop(UnaryOp::NEGATE, std::move(operand));
        }
        if (check(TokenType::PLUS)) {
            advance();
            return parse_unary();
        }
        return parse_postfix();
    }

    ExprPtr parse_postfix() {
        auto expr = parse_primary();

        while (true) {
            if (check(TokenType::LPAREN)) {
                // Subscript or function call
                advance();
                std::vector<ExprPtr> args;
                while (!check(TokenType::RPAREN) && !at_end()) {
                    args.push_back(parse_expr());
                    if (check(TokenType::COMMA)) advance();
                }
                expect(TokenType::RPAREN);

                // If base is identifier, it's a function call
                if (auto* id = std::get_if<IdentifierExpr>(&expr->node)) {
                    expr = make_expr(FunctionCallExpr{id->name, std::move(args)});
                } else {
                    expr = make_expr(SubscriptExpr{std::move(expr), std::move(args)});
                }
            } else if (check(TokenType::DOT)) {
                advance();
                std::string member = expect(TokenType::IDENTIFIER).value;
                expr = make_expr(MemberAccessExpr{std::move(expr), member});
            } else {
                break;
            }
        }
        return expr;
    }

    ExprPtr parse_primary() {
        if (check(TokenType::INTEGER_LIT)) {
            auto tok = advance();
            return make_literal(LiteralExpr::INTEGER, tok.value, tok.line);
        }
        if (check(TokenType::FLOAT_LIT)) {
            auto tok = advance();
            return make_literal(LiteralExpr::FLOAT_LIT, tok.value, tok.line);
        }
        if (check(TokenType::STRING_LIT)) {
            auto tok = advance();
            return make_literal(LiteralExpr::STRING, tok.value, tok.line);
        }
        if (check(TokenType::BIT_LIT)) {
            auto tok = advance();
            return make_literal(LiteralExpr::BIT_STRING, tok.value, tok.line);
        }
        if (check(TokenType::COMPLEX_LIT)) {
            auto tok = advance();
            return make_literal(LiteralExpr::COMPLEX_LIT, tok.value, tok.line);
        }
        if (check(TokenType::NULL_KW)) {
            advance();
            return make_literal(LiteralExpr::INTEGER, "0");
        }
        if (check(TokenType::IDENTIFIER)) {
            auto tok = advance();
            return make_ident(tok.value, tok.line);
        }
        if (check(TokenType::LPAREN)) {
            advance();
            auto expr = parse_expr();
            expect(TokenType::RPAREN);
            return expr;
        }

        // In PL/I many keywords can appear as variable names in expression context
        if (is_keyword_as_identifier(current().type)) {
            auto tok = advance();
            return make_ident(tok.value, tok.line);
        }

        throw ParseError(
            std::string("Unexpected token in expression: ") +
            token_type_name(current().type) + " '" + current().value + "'",
            current().line, current().column);
    }
};

}} // namespace lazarus::pli

#endif // LAZARUS_PLI_PARSER_H
