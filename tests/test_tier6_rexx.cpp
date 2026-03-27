#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

#include "include/lazarus/rexx/ast.h"
#include "include/lazarus/rexx/lexer.h"
#include "include/lazarus/rexx/parser.h"
#include "include/lazarus/rexx/builtins.h"
#include "include/lazarus/rexx/runtime.h"
#include "include/lazarus/rexx/python_translator.h"

using namespace lazarus::rexx;
using namespace lazarus::rexx::builtins;

// ============================================================================
// LEXER TESTS (35 tests)
// ============================================================================

TEST_CASE("Lexer: SAY keyword") {
    Lexer lex("SAY");
    auto tokens = lex.tokenize();
    REQUIRE(tokens.size() >= 2);
    CHECK(tokens[0].type == TokenType::SAY);
}

TEST_CASE("Lexer: DO keyword") {
    Lexer lex("DO");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::DO);
}

TEST_CASE("Lexer: END keyword") {
    Lexer lex("END");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::END);
}

TEST_CASE("Lexer: IF THEN ELSE") {
    Lexer lex("IF x THEN SAY ELSE NOP");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::IF);
    CHECK(tokens[2].type == TokenType::THEN);
    CHECK(tokens[4].type == TokenType::ELSE);
    CHECK(tokens[5].type == TokenType::NOP);
}

TEST_CASE("Lexer: SELECT WHEN OTHERWISE") {
    Lexer lex("SELECT WHEN OTHERWISE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::SELECT);
    CHECK(tokens[1].type == TokenType::WHEN);
    CHECK(tokens[2].type == TokenType::OTHERWISE);
}

TEST_CASE("Lexer: CALL RETURN EXIT") {
    Lexer lex("CALL RETURN EXIT");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::CALL);
    CHECK(tokens[1].type == TokenType::RETURN);
    CHECK(tokens[2].type == TokenType::EXIT);
}

TEST_CASE("Lexer: PULL PUSH QUEUE ARG") {
    Lexer lex("PULL PUSH QUEUE ARG");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PULL);
    CHECK(tokens[1].type == TokenType::PUSH);
    CHECK(tokens[2].type == TokenType::QUEUE);
    CHECK(tokens[3].type == TokenType::ARG);
}

TEST_CASE("Lexer: PARSE ADDRESS SIGNAL") {
    Lexer lex("PARSE ADDRESS SIGNAL");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PARSE);
    CHECK(tokens[1].type == TokenType::ADDRESS);
    CHECK(tokens[2].type == TokenType::SIGNAL);
}

TEST_CASE("Lexer: DROP ITERATE LEAVE NOP TRACE") {
    Lexer lex("DROP ITERATE LEAVE NOP TRACE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::DROP);
    CHECK(tokens[1].type == TokenType::ITERATE);
    CHECK(tokens[2].type == TokenType::LEAVE);
    CHECK(tokens[3].type == TokenType::NOP);
    CHECK(tokens[4].type == TokenType::TRACE);
}

TEST_CASE("Lexer: UPPER PROCEDURE EXPOSE INTERPRET") {
    Lexer lex("UPPER PROCEDURE EXPOSE INTERPRET");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::UPPER);
    CHECK(tokens[1].type == TokenType::PROCEDURE);
    CHECK(tokens[2].type == TokenType::EXPOSE);
    CHECK(tokens[3].type == TokenType::INTERPRET);
}

TEST_CASE("Lexer: NUMERIC OPTIONS VALUE WITH") {
    Lexer lex("NUMERIC OPTIONS VALUE WITH");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::NUMERIC);
    CHECK(tokens[1].type == TokenType::OPTIONS);
    CHECK(tokens[2].type == TokenType::VALUE);
    CHECK(tokens[3].type == TokenType::WITH);
}

TEST_CASE("Lexer: TO BY FOR WHILE UNTIL FOREVER") {
    Lexer lex("TO BY FOR WHILE UNTIL FOREVER");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::TO);
    CHECK(tokens[1].type == TokenType::BY);
    CHECK(tokens[2].type == TokenType::FOR);
    CHECK(tokens[3].type == TokenType::WHILE);
    CHECK(tokens[4].type == TokenType::UNTIL);
    CHECK(tokens[5].type == TokenType::FOREVER);
}

TEST_CASE("Lexer: integer literal") {
    Lexer lex("42");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::INTEGER);
    CHECK(tokens[0].value == "42");
}

TEST_CASE("Lexer: float literal") {
    Lexer lex("3.14");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::FLOAT);
    CHECK(tokens[0].value == "3.14");
}

TEST_CASE("Lexer: single-quoted string") {
    Lexer lex("'hello world'");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "hello world");
}

TEST_CASE("Lexer: double-quoted string") {
    Lexer lex("\"hello\"");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "hello");
}

TEST_CASE("Lexer: escaped quotes in string") {
    Lexer lex("'it''s'");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::STRING);
    CHECK(tokens[0].value == "it's");
}

TEST_CASE("Lexer: hex string") {
    Lexer lex("'48454C4C4F'x");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::HEX_STRING);
    CHECK(tokens[0].value == "48454C4C4F");
}

TEST_CASE("Lexer: binary string") {
    Lexer lex("'01000001'b");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::BINARY_STRING);
    CHECK(tokens[0].value == "01000001");
}

TEST_CASE("Lexer: arithmetic operators") {
    Lexer lex("+ - * / % // **");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PLUS);
    CHECK(tokens[1].type == TokenType::MINUS);
    CHECK(tokens[2].type == TokenType::STAR);
    CHECK(tokens[3].type == TokenType::SLASH);
    CHECK(tokens[4].type == TokenType::PERCENT);
    CHECK(tokens[5].type == TokenType::DBLSLASH);
    CHECK(tokens[6].type == TokenType::POWER);
}

TEST_CASE("Lexer: comparison operators") {
    Lexer lex("= \\= < > <= >=");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::EQUAL);
    CHECK(tokens[1].type == TokenType::BACKSLASH_EQUAL);
    CHECK(tokens[2].type == TokenType::LESS);
    CHECK(tokens[3].type == TokenType::GREATER);
    CHECK(tokens[4].type == TokenType::LESS_EQUAL);
    CHECK(tokens[5].type == TokenType::GREATER_EQUAL);
}

TEST_CASE("Lexer: strict comparison operators") {
    Lexer lex("== \\== << >>");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::STRICT_EQUAL);
    CHECK(tokens[1].type == TokenType::STRICT_BACKSLASH_EQUAL);
    CHECK(tokens[2].type == TokenType::STRICT_LESS);
    CHECK(tokens[3].type == TokenType::STRICT_GREATER);
}

TEST_CASE("Lexer: concatenation operator") {
    Lexer lex("||");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::CONCAT);
}

TEST_CASE("Lexer: logical operators") {
    Lexer lex("& | && \\");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::AND);
    CHECK(tokens[1].type == TokenType::OR);
    CHECK(tokens[2].type == TokenType::XOR);
    CHECK(tokens[3].type == TokenType::NOT);
}

TEST_CASE("Lexer: parentheses and punctuation") {
    Lexer lex("( ) , ; .");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::LPAREN);
    CHECK(tokens[1].type == TokenType::RPAREN);
    CHECK(tokens[2].type == TokenType::COMMA);
    CHECK(tokens[3].type == TokenType::SEMICOLON);
    CHECK(tokens[4].type == TokenType::DOT);
}

TEST_CASE("Lexer: identifier") {
    Lexer lex("myVar");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[0].value == "myVar");
}

TEST_CASE("Lexer: label (word colon)") {
    Lexer lex("myLabel:");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::LABEL);
}

TEST_CASE("Lexer: case insensitive keywords") {
    Lexer lex("say Say SAY");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::SAY);
    CHECK(tokens[1].type == TokenType::SAY);
    CHECK(tokens[2].type == TokenType::SAY);
}

TEST_CASE("Lexer: block comment") {
    Lexer lex("SAY /* this is a comment */ 'hello'");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::SAY);
    CHECK(tokens[1].type == TokenType::STRING);
    CHECK(tokens[1].value == "hello");
}

TEST_CASE("Lexer: nested block comments") {
    Lexer lex("SAY /* outer /* inner */ still comment */ 'ok'");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::SAY);
    CHECK(tokens[1].type == TokenType::STRING);
    CHECK(tokens[1].value == "ok");
}

TEST_CASE("Lexer: line continuation with comma") {
    Lexer lex("SAY 'hello',\n'world'");
    auto tokens = lex.tokenize();
    // comma at end of line is continuation, not emitted
    CHECK(tokens[0].type == TokenType::SAY);
    CHECK(tokens[1].type == TokenType::STRING);
    CHECK(tokens[1].value == "hello");
    CHECK(tokens[2].type == TokenType::STRING);
    CHECK(tokens[2].value == "world");
}

TEST_CASE("Lexer: newline tokens") {
    Lexer lex("SAY\nPULL");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::SAY);
    CHECK(tokens[1].type == TokenType::NEWLINE);
    CHECK(tokens[2].type == TokenType::PULL);
}

TEST_CASE("Lexer: ON and OFF keywords") {
    Lexer lex("ON OFF");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::ON);
    CHECK(tokens[1].type == TokenType::OFF);
}

TEST_CASE("Lexer: multiple tokens in expression") {
    Lexer lex("x = 10 + 20");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[1].type == TokenType::EQUAL);
    CHECK(tokens[2].type == TokenType::INTEGER);
    CHECK(tokens[3].type == TokenType::PLUS);
    CHECK(tokens[4].type == TokenType::INTEGER);
}

TEST_CASE("Lexer: caret as NOT") {
    Lexer lex("^=");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::BACKSLASH_EQUAL);
}

// ============================================================================
// PARSER TESTS (35 tests)
// ============================================================================

TEST_CASE("Parser: SAY string literal") {
    Lexer lex("SAY 'hello'");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Say);
}

TEST_CASE("Parser: SAY expression") {
    Lexer lex("SAY 1 + 2");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Say);
    CHECK(prog.statements[0]->expr->type == ExprType::BinaryOp);
}

TEST_CASE("Parser: assignment") {
    Lexer lex("x = 42");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Assign);
    CHECK(prog.statements[0]->assign_target == "x");
}

TEST_CASE("Parser: PULL template") {
    Lexer lex("PULL name age");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Pull);
    CHECK(prog.statements[0]->tmpl.size() == 2);
}

TEST_CASE("Parser: PUSH expression") {
    Lexer lex("PUSH 'hello'");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Push);
}

TEST_CASE("Parser: QUEUE expression") {
    Lexer lex("QUEUE 'data'");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Queue);
}

TEST_CASE("Parser: IF THEN ELSE") {
    Lexer lex("IF x = 1 THEN SAY 'yes' ELSE SAY 'no'");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::If);
    CHECK(prog.statements[0]->then_branch != nullptr);
    CHECK(prog.statements[0]->else_branch != nullptr);
}

TEST_CASE("Parser: IF THEN NOP") {
    Lexer lex("IF x = 1 THEN NOP");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->then_branch->type == StmtType::Nop);
}

TEST_CASE("Parser: DO simple") {
    Lexer lex("DO\nSAY 'x'\nEND");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Do);
    CHECK(prog.statements[0]->do_variant == DoVariant::Simple);
}

TEST_CASE("Parser: DO counted") {
    Lexer lex("DO 5\nSAY 'hi'\nEND");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->do_variant == DoVariant::Counted);
}

TEST_CASE("Parser: DO controlled") {
    Lexer lex("DO i = 1 TO 10\nSAY i\nEND");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->do_variant == DoVariant::Controlled);
    CHECK(prog.statements[0]->do_var == "i");
}

TEST_CASE("Parser: DO controlled with BY") {
    Lexer lex("DO i = 1 TO 10 BY 2\nSAY i\nEND");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->do_step != nullptr);
}

TEST_CASE("Parser: DO WHILE") {
    Lexer lex("DO WHILE x < 10\nSAY x\nEND");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->do_variant == DoVariant::While);
}

TEST_CASE("Parser: DO UNTIL") {
    Lexer lex("DO UNTIL x = 10\nSAY x\nEND");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->do_variant == DoVariant::Until);
}

TEST_CASE("Parser: DO FOREVER") {
    Lexer lex("DO FOREVER\nSAY 'loop'\nEND");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->do_variant == DoVariant::Forever);
}

TEST_CASE("Parser: SELECT WHEN OTHERWISE") {
    Lexer lex("SELECT\nWHEN x = 1 THEN SAY 'one'\nWHEN x = 2 THEN SAY 'two'\nOTHERWISE\nSAY 'other'\nEND");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Select);
    CHECK(prog.statements[0]->when_clauses.size() == 2);
    CHECK(prog.statements[0]->otherwise_body.size() == 1);
}

TEST_CASE("Parser: CALL routine") {
    Lexer lex("CALL myFunc 1, 2");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Call);
    CHECK(prog.statements[0]->target_name == "myFunc");
}

TEST_CASE("Parser: CALL ON condition") {
    Lexer lex("CALL ON ERROR");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->is_on == true);
    CHECK(prog.statements[0]->cond_type == ConditionType::Error);
}

TEST_CASE("Parser: CALL OFF condition") {
    Lexer lex("CALL OFF HALT");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->is_off == true);
}

TEST_CASE("Parser: SIGNAL label") {
    Lexer lex("SIGNAL done");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Signal);
    CHECK(prog.statements[0]->target_name == "done");
}

TEST_CASE("Parser: SIGNAL ON SYNTAX") {
    Lexer lex("SIGNAL ON SYNTAX");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->is_on == true);
    CHECK(prog.statements[0]->cond_type == ConditionType::Syntax);
}

TEST_CASE("Parser: ADDRESS environment") {
    Lexer lex("ADDRESS TSO 'LISTCAT'");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Address);
    CHECK(prog.statements[0]->environment == "TSO");
}

TEST_CASE("Parser: PARSE ARG template") {
    Lexer lex("PARSE ARG first second");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Parse);
    CHECK(prog.statements[0]->parse_source == ParseSource::Arg);
    CHECK(prog.statements[0]->tmpl.size() == 2);
}

TEST_CASE("Parser: PARSE VAR") {
    Lexer lex("PARSE VAR myline first rest");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->parse_source == ParseSource::Var);
    CHECK(prog.statements[0]->parse_var_name == "myline");
}

TEST_CASE("Parser: PARSE with literal delimiter") {
    Lexer lex("PARSE VAR line name ',' addr");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->tmpl.size() == 3);
    CHECK(prog.statements[0]->tmpl[1].type == TemplateItemType::LiteralDelim);
    CHECK(prog.statements[0]->tmpl[1].value == ",");
}

TEST_CASE("Parser: PROCEDURE EXPOSE") {
    Lexer lex("PROCEDURE EXPOSE x y z");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Procedure);
    CHECK(prog.statements[0]->expose_vars.size() == 3);
}

TEST_CASE("Parser: INTERPRET expression") {
    Lexer lex("INTERPRET 'SAY 42'");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Interpret);
}

TEST_CASE("Parser: NUMERIC DIGITS") {
    Lexer lex("NUMERIC DIGITS 15");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Numeric);
    CHECK(prog.statements[0]->numeric_setting == NumericSetting::Digits);
}

TEST_CASE("Parser: DROP variables") {
    Lexer lex("DROP a b c");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Drop);
    CHECK(prog.statements[0]->drop_vars.size() == 3);
}

TEST_CASE("Parser: TRACE setting") {
    Lexer lex("TRACE R");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Trace);
}

TEST_CASE("Parser: RETURN with expression") {
    Lexer lex("RETURN x + 1");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Return);
    CHECK(prog.statements[0]->expr != nullptr);
}

TEST_CASE("Parser: EXIT with expression") {
    Lexer lex("EXIT 0");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Exit);
}

TEST_CASE("Parser: function call in expression") {
    Lexer lex("SAY LENGTH('hello')");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    auto& expr = prog.statements[0]->expr;
    CHECK(expr->type == ExprType::FunctionCall);
    CHECK(expr->func_name == "LENGTH");
}

TEST_CASE("Parser: operator precedence mul over add") {
    Lexer lex("x = 2 + 3 * 4");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    auto& val = prog.statements[0]->assign_value;
    CHECK(val->type == ExprType::BinaryOp);
    CHECK(val->bin_op == BinOp::Add);
    CHECK(val->right->type == ExprType::BinaryOp);
    CHECK(val->right->bin_op == BinOp::Mul);
}

TEST_CASE("Parser: ARG template shorthand") {
    Lexer lex("ARG first second");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.statements.size() == 1);
    CHECK(prog.statements[0]->type == StmtType::Parse);
    CHECK(prog.statements[0]->parse_source == ParseSource::Arg);
}

// ============================================================================
// BUILTIN STRING FUNCTION TESTS (25 tests)
// ============================================================================

TEST_CASE("Builtin: ABBREV positive") {
    auto r = fn_abbrev({RexxValue("INFORMATION"), RexxValue("INFO")});
    CHECK(r.to_integer() == 1);
}

TEST_CASE("Builtin: ABBREV negative") {
    auto r = fn_abbrev({RexxValue("INFORMATION"), RexxValue("XYZA")});
    CHECK(r.to_integer() == 0);
}

TEST_CASE("Builtin: CENTER string") {
    auto r = fn_center({RexxValue("AB"), RexxValue(int64_t(6))});
    CHECK(r.to_string() == "  AB  ");
}

TEST_CASE("Builtin: CENTER with pad char") {
    auto r = fn_center({RexxValue("AB"), RexxValue(int64_t(6)), RexxValue("*")});
    CHECK(r.to_string() == "**AB**");
}

TEST_CASE("Builtin: CHANGESTR") {
    auto r = fn_changestr({RexxValue("o"), RexxValue("hello world"), RexxValue("0")});
    CHECK(r.to_string() == "hell0 w0rld");
}

TEST_CASE("Builtin: COMPARE equal") {
    auto r = fn_compare({RexxValue("abc"), RexxValue("abc")});
    CHECK(r.to_integer() == 0);
}

TEST_CASE("Builtin: COMPARE different") {
    auto r = fn_compare({RexxValue("abc"), RexxValue("axc")});
    CHECK(r.to_integer() == 2);
}

TEST_CASE("Builtin: COPIES") {
    auto r = fn_copies({RexxValue("abc"), RexxValue(int64_t(3))});
    CHECK(r.to_string() == "abcabcabc");
}

TEST_CASE("Builtin: COUNTSTR") {
    auto r = fn_countstr({RexxValue("ll"), RexxValue("hello llama")});
    CHECK(r.to_integer() == 2);
}

TEST_CASE("Builtin: DELSTR") {
    auto r = fn_delstr({RexxValue("abcdef"), RexxValue(int64_t(3)), RexxValue(int64_t(2))});
    CHECK(r.to_string() == "abef");
}

TEST_CASE("Builtin: INSERT") {
    auto r = fn_insert({RexxValue("xyz"), RexxValue("abcdef"), RexxValue(int64_t(3))});
    CHECK(r.to_string() == "abcxyzdef");
}

TEST_CASE("Builtin: LASTPOS") {
    auto r = fn_lastpos({RexxValue("l"), RexxValue("hello world")});
    CHECK(r.to_integer() == 10);
}

TEST_CASE("Builtin: LEFT") {
    auto r = fn_left({RexxValue("hello"), RexxValue(int64_t(3))});
    CHECK(r.to_string() == "hel");
}

TEST_CASE("Builtin: LEFT with padding") {
    auto r = fn_left({RexxValue("hi"), RexxValue(int64_t(5))});
    CHECK(r.to_string() == "hi   ");
}

TEST_CASE("Builtin: LENGTH") {
    auto r = fn_length({RexxValue("hello")});
    CHECK(r.to_integer() == 5);
}

TEST_CASE("Builtin: OVERLAY") {
    auto r = fn_overlay({RexxValue("XX"), RexxValue("abcdef"), RexxValue(int64_t(3))});
    CHECK(r.to_string() == "abXXef");
}

TEST_CASE("Builtin: POS found") {
    auto r = fn_pos({RexxValue("lo"), RexxValue("hello")});
    CHECK(r.to_integer() == 4);
}

TEST_CASE("Builtin: POS not found") {
    auto r = fn_pos({RexxValue("xyz"), RexxValue("hello")});
    CHECK(r.to_integer() == 0);
}

TEST_CASE("Builtin: REVERSE") {
    auto r = fn_reverse({RexxValue("hello")});
    CHECK(r.to_string() == "olleh");
}

TEST_CASE("Builtin: RIGHT") {
    auto r = fn_right({RexxValue("hello"), RexxValue(int64_t(3))});
    CHECK(r.to_string() == "llo");
}

TEST_CASE("Builtin: SPACE") {
    auto r = fn_space({RexxValue("  hello   world  "), RexxValue(int64_t(1))});
    CHECK(r.to_string() == "hello world");
}

TEST_CASE("Builtin: STRIP both") {
    auto r = fn_strip({RexxValue("  hello  ")});
    CHECK(r.to_string() == "hello");
}

TEST_CASE("Builtin: STRIP leading") {
    auto r = fn_strip({RexxValue("  hello  "), RexxValue("L")});
    CHECK(r.to_string() == "hello  ");
}

TEST_CASE("Builtin: SUBSTR") {
    auto r = fn_substr({RexxValue("hello world"), RexxValue(int64_t(7))});
    CHECK(r.to_string() == "world");
}

TEST_CASE("Builtin: SUBSTR with length") {
    auto r = fn_substr({RexxValue("hello world"), RexxValue(int64_t(1)), RexxValue(int64_t(5))});
    CHECK(r.to_string() == "hello");
}

// ============================================================================
// BUILTIN STRING WORD FUNCTIONS (8 tests)
// ============================================================================

TEST_CASE("Builtin: SUBWORD") {
    auto r = fn_subword({RexxValue("one two three four"), RexxValue(int64_t(2)), RexxValue(int64_t(2))});
    CHECK(r.to_string() == "two three");
}

TEST_CASE("Builtin: TRANSLATE no args") {
    auto r = fn_translate({RexxValue("hello")});
    CHECK(r.to_string() == "HELLO");
}

TEST_CASE("Builtin: TRANSLATE with tables") {
    auto r = fn_translate({RexxValue("abcdef"), RexxValue("FEDCBA"), RexxValue("abcdef")});
    CHECK(r.to_string() == "FEDCBA");
}

TEST_CASE("Builtin: VERIFY nomatch") {
    auto r = fn_verify({RexxValue("abc123"), RexxValue("abcdefghij")});
    CHECK(r.to_integer() == 4); // first non-alpha
}

TEST_CASE("Builtin: WORD") {
    auto r = fn_word({RexxValue("one two three"), RexxValue(int64_t(2))});
    CHECK(r.to_string() == "two");
}

TEST_CASE("Builtin: WORDINDEX") {
    auto r = fn_wordindex({RexxValue("  hello world"), RexxValue(int64_t(2))});
    CHECK(r.to_integer() == 9);
}

TEST_CASE("Builtin: WORDPOS") {
    auto r = fn_wordpos({RexxValue("two"), RexxValue("one two three")});
    CHECK(r.to_integer() == 2);
}

TEST_CASE("Builtin: WORDS") {
    auto r = fn_words({RexxValue("one two three")});
    CHECK(r.to_integer() == 3);
}

// ============================================================================
// BUILTIN CONVERSION FUNCTION TESTS (12 tests)
// ============================================================================

TEST_CASE("Builtin: B2X") {
    auto r = fn_b2x({RexxValue("11000001")});
    CHECK(r.to_string() == "C1");
}

TEST_CASE("Builtin: C2D") {
    auto r = fn_c2d({RexxValue("A")});
    CHECK(r.to_integer() == 65);
}

TEST_CASE("Builtin: C2X") {
    auto r = fn_c2x({RexxValue("A")});
    CHECK(r.to_string() == "41");
}

TEST_CASE("Builtin: D2C") {
    auto r = fn_d2c({RexxValue(int64_t(65))});
    CHECK(r.to_string() == "A");
}

TEST_CASE("Builtin: D2X") {
    auto r = fn_d2x({RexxValue(int64_t(255))});
    CHECK(r.to_string() == "FF");
}

TEST_CASE("Builtin: D2X with length") {
    auto r = fn_d2x({RexxValue(int64_t(10)), RexxValue(int64_t(4))});
    CHECK(r.to_string() == "000A");
}

TEST_CASE("Builtin: X2B") {
    auto r = fn_x2b({RexxValue("C1")});
    CHECK(r.to_string() == "11000001");
}

TEST_CASE("Builtin: X2C") {
    auto r = fn_x2c({RexxValue("41")});
    CHECK(r.to_string() == "A");
}

TEST_CASE("Builtin: X2D") {
    auto r = fn_x2d({RexxValue("FF")});
    CHECK(r.to_integer() == 255);
}

TEST_CASE("Builtin: FORMAT integer") {
    auto r = fn_format({RexxValue(int64_t(42))});
    CHECK(r.to_string().find("42") != std::string::npos);
}

TEST_CASE("Builtin: FORMAT with decimals") {
    auto r = fn_format({RexxValue(3.14159), RexxValue(int64_t(0)), RexxValue(int64_t(2))});
    CHECK(r.to_string() == "3.14");
}

TEST_CASE("Builtin: XRANGE") {
    auto r = fn_xrange({RexxValue(int64_t(65)), RexxValue(int64_t(67))});
    CHECK(r.to_string() == "ABC");
}

// ============================================================================
// BUILTIN MATH FUNCTION TESTS (10 tests)
// ============================================================================

TEST_CASE("Builtin: ABS positive") {
    auto r = fn_abs({RexxValue(int64_t(42))});
    CHECK(r.to_integer() == 42);
}

TEST_CASE("Builtin: ABS negative") {
    auto r = fn_abs({RexxValue(int64_t(-42))});
    CHECK(r.to_integer() == 42);
}

TEST_CASE("Builtin: MAX") {
    auto r = fn_max({RexxValue(int64_t(1)), RexxValue(int64_t(5)), RexxValue(int64_t(3))});
    CHECK(r.to_integer() == 5);
}

TEST_CASE("Builtin: MIN") {
    auto r = fn_min({RexxValue(int64_t(1)), RexxValue(int64_t(5)), RexxValue(int64_t(3))});
    CHECK(r.to_integer() == 1);
}

TEST_CASE("Builtin: SIGN positive") {
    auto r = fn_sign({RexxValue(int64_t(42))});
    CHECK(r.to_integer() == 1);
}

TEST_CASE("Builtin: SIGN negative") {
    auto r = fn_sign({RexxValue(int64_t(-42))});
    CHECK(r.to_integer() == -1);
}

TEST_CASE("Builtin: SIGN zero") {
    auto r = fn_sign({RexxValue(int64_t(0))});
    CHECK(r.to_integer() == 0);
}

TEST_CASE("Builtin: TRUNC integer") {
    auto r = fn_trunc({RexxValue(3.7)});
    CHECK(r.to_integer() == 3);
}

TEST_CASE("Builtin: TRUNC with decimals") {
    auto r = fn_trunc({RexxValue(3.14159), RexxValue(int64_t(2))});
    CHECK(r.to_string() == "3.14");
}

TEST_CASE("Builtin: RANDOM in range") {
    auto r = fn_random({RexxValue(int64_t(1)), RexxValue(int64_t(10)), RexxValue(int64_t(42))});
    int v = static_cast<int>(r.to_integer());
    CHECK(v >= 1);
    CHECK(v <= 10);
}

// ============================================================================
// BUILTIN MISC FUNCTION TESTS (10 tests)
// ============================================================================

TEST_CASE("Builtin: DATATYPE NUM") {
    auto r = fn_datatype({RexxValue("123")});
    CHECK(r.to_string() == "NUM");
}

TEST_CASE("Builtin: DATATYPE CHAR") {
    auto r = fn_datatype({RexxValue("hello")});
    CHECK(r.to_string() == "CHAR");
}

TEST_CASE("Builtin: DATATYPE type A") {
    auto r = fn_datatype({RexxValue("abc123"), RexxValue("A")});
    CHECK(r.to_integer() == 1);
}

TEST_CASE("Builtin: DATATYPE type N") {
    auto r = fn_datatype({RexxValue("42"), RexxValue("N")});
    CHECK(r.to_integer() == 1);
}

TEST_CASE("Builtin: SYMBOL VAR") {
    auto r = fn_symbol({RexxValue("myVar")});
    CHECK(r.to_string() == "VAR");
}

TEST_CASE("Builtin: SYMBOL LIT") {
    auto r = fn_symbol({RexxValue("123")});
    CHECK(r.to_string() == "LIT");
}

TEST_CASE("Builtin: ERRORTEXT") {
    auto r = fn_errortext({RexxValue(int64_t(16))});
    CHECK(r.to_string() == "Label not found");
}

TEST_CASE("Builtin: DIGITS default") {
    auto r = fn_digits({});
    CHECK(r.to_integer() == 9);
}

TEST_CASE("Builtin: FORM default") {
    auto r = fn_form({});
    CHECK(r.to_string() == "SCIENTIFIC");
}

TEST_CASE("Builtin: call_builtin dispatch") {
    auto r = call_builtin("LENGTH", {RexxValue("test")});
    CHECK(r.to_integer() == 4);
}

// ============================================================================
// PYTHON TRANSLATOR TESTS (38 tests)
// ============================================================================

TEST_CASE("Translator: SAY generates print") {
    PythonTranslator tr;
    auto result = tr.translate_source("SAY 'hello'");
    CHECK(result.find("print(") != std::string::npos);
}

TEST_CASE("Translator: assignment") {
    PythonTranslator tr;
    auto result = tr.translate_source("x = 42");
    CHECK(result.find("x = 42") != std::string::npos);
}

TEST_CASE("Translator: PULL generates input") {
    PythonTranslator tr;
    auto result = tr.translate_source("PULL name");
    CHECK(result.find("input()") != std::string::npos);
    CHECK(result.find(".upper()") != std::string::npos);
}

TEST_CASE("Translator: PUSH generates insert") {
    PythonTranslator tr;
    auto result = tr.translate_source("PUSH 'data'");
    CHECK(result.find("rexx_queue.insert(0,") != std::string::npos);
}

TEST_CASE("Translator: QUEUE generates append") {
    PythonTranslator tr;
    auto result = tr.translate_source("QUEUE 'data'");
    CHECK(result.find("rexx_queue.append(") != std::string::npos);
}

TEST_CASE("Translator: IF generates if") {
    PythonTranslator tr;
    auto result = tr.translate_source("IF x = 1 THEN SAY 'yes'");
    CHECK(result.find("if ") != std::string::npos);
    CHECK(result.find("print(") != std::string::npos);
}

TEST_CASE("Translator: IF ELSE generates else") {
    PythonTranslator tr;
    auto result = tr.translate_source("IF x = 1 THEN SAY 'yes' ELSE SAY 'no'");
    CHECK(result.find("else:") != std::string::npos);
}

TEST_CASE("Translator: DO controlled generates for range") {
    PythonTranslator tr;
    auto result = tr.translate_source("DO i = 1 TO 10\nSAY i\nEND");
    CHECK(result.find("for i in range(1, 10 + 1, 1)") != std::string::npos);
}

TEST_CASE("Translator: DO WHILE generates while") {
    PythonTranslator tr;
    auto result = tr.translate_source("DO WHILE x < 10\nSAY x\nEND");
    CHECK(result.find("while ") != std::string::npos);
}

TEST_CASE("Translator: DO FOREVER generates while True") {
    PythonTranslator tr;
    auto result = tr.translate_source("DO FOREVER\nSAY 'loop'\nEND");
    CHECK(result.find("while True:") != std::string::npos);
}

TEST_CASE("Translator: DO UNTIL generates while True with break") {
    PythonTranslator tr;
    auto result = tr.translate_source("DO UNTIL x = 10\nSAY x\nEND");
    CHECK(result.find("while True:") != std::string::npos);
    CHECK(result.find("break") != std::string::npos);
}

TEST_CASE("Translator: DO counted generates for range") {
    PythonTranslator tr;
    auto result = tr.translate_source("DO 5\nSAY 'hi'\nEND");
    CHECK(result.find("for _rexx_i in range(5)") != std::string::npos);
}

TEST_CASE("Translator: SELECT generates if/elif/else") {
    PythonTranslator tr;
    auto result = tr.translate_source("SELECT\nWHEN x = 1 THEN SAY 'a'\nWHEN x = 2 THEN SAY 'b'\nOTHERWISE\nSAY 'c'\nEND");
    CHECK(result.find("if ") != std::string::npos);
    CHECK(result.find("elif ") != std::string::npos);
    CHECK(result.find("else:") != std::string::npos);
}

TEST_CASE("Translator: CALL generates function call") {
    PythonTranslator tr;
    auto result = tr.translate_source("CALL myFunc 1, 2");
    CHECK(result.find("myfunc(") != std::string::npos);
}

TEST_CASE("Translator: RETURN generates return") {
    PythonTranslator tr;
    auto result = tr.translate_source("RETURN 42");
    CHECK(result.find("return 42") != std::string::npos);
}

TEST_CASE("Translator: EXIT generates sys.exit") {
    PythonTranslator tr;
    auto result = tr.translate_source("EXIT 0");
    CHECK(result.find("sys.exit(") != std::string::npos);
}

TEST_CASE("Translator: ADDRESS with command generates subprocess") {
    PythonTranslator tr;
    auto result = tr.translate_source("ADDRESS TSO 'LISTCAT'");
    CHECK(result.find("subprocess.run(") != std::string::npos);
}

TEST_CASE("Translator: SIGNAL generates raise") {
    PythonTranslator tr;
    auto result = tr.translate_source("SIGNAL done");
    CHECK(result.find("raise Exception") != std::string::npos);
}

TEST_CASE("Translator: NOP generates pass") {
    PythonTranslator tr;
    auto result = tr.translate_source("IF x = 1 THEN NOP");
    CHECK(result.find("pass") != std::string::npos);
}

TEST_CASE("Translator: ITERATE generates continue") {
    PythonTranslator tr;
    auto result = tr.translate_source("DO FOREVER\nITERATE\nEND");
    CHECK(result.find("continue") != std::string::npos);
}

TEST_CASE("Translator: LEAVE generates break") {
    PythonTranslator tr;
    auto result = tr.translate_source("DO FOREVER\nLEAVE\nEND");
    CHECK(result.find("break") != std::string::npos);
}

TEST_CASE("Translator: INTERPRET generates exec") {
    PythonTranslator tr;
    auto result = tr.translate_source("INTERPRET 'x = 1'");
    CHECK(result.find("exec(") != std::string::npos);
}

TEST_CASE("Translator: DROP generates del") {
    PythonTranslator tr;
    auto result = tr.translate_source("DROP x");
    CHECK(result.find("del ") != std::string::npos);
}

TEST_CASE("Translator: NUMERIC generates comment") {
    PythonTranslator tr;
    auto result = tr.translate_source("NUMERIC DIGITS 15");
    CHECK(result.find("# NUMERIC DIGITS") != std::string::npos);
}

TEST_CASE("Translator: TRACE generates comment") {
    PythonTranslator tr;
    auto result = tr.translate_source("TRACE R");
    CHECK(result.find("# TRACE") != std::string::npos);
}

TEST_CASE("Translator: PARSE VAR with word split") {
    PythonTranslator tr;
    auto result = tr.translate_source("PARSE VAR line first second rest");
    CHECK(result.find(".split()") != std::string::npos);
    CHECK(result.find("first") != std::string::npos);
}

TEST_CASE("Translator: PARSE VAR with literal delimiter") {
    PythonTranslator tr;
    auto result = tr.translate_source("PARSE VAR line name ',' addr");
    CHECK(result.find(".find(") != std::string::npos);
}

TEST_CASE("Translator: PROCEDURE EXPOSE generates nonlocal") {
    PythonTranslator tr;
    auto result = tr.translate_source("PROCEDURE EXPOSE x y");
    CHECK(result.find("nonlocal ") != std::string::npos);
}

TEST_CASE("Translator: function call LENGTH maps to len") {
    PythonTranslator tr;
    auto result = tr.translate_source("SAY LENGTH('hello')");
    CHECK(result.find("len(") != std::string::npos);
}

TEST_CASE("Translator: function call SUBSTR maps") {
    PythonTranslator tr;
    auto result = tr.translate_source("SAY SUBSTR('hello', 2, 3)");
    CHECK(result.find("rexx_substr(") != std::string::npos);
}

TEST_CASE("Translator: comparison operator == to Python ==") {
    PythonTranslator tr;
    auto result = tr.translate_source("IF x = 1 THEN NOP");
    CHECK(result.find("==") != std::string::npos);
}

TEST_CASE("Translator: strict comparison") {
    PythonTranslator tr;
    auto result = tr.translate_source("IF x == y THEN NOP");
    CHECK(result.find("str(") != std::string::npos);
}

TEST_CASE("Translator: concatenation operator") {
    PythonTranslator tr;
    auto result = tr.translate_source("x = a || b");
    CHECK(result.find("str(") != std::string::npos);
}

TEST_CASE("Translator: power operator") {
    PythonTranslator tr;
    auto result = tr.translate_source("x = 2 ** 3");
    CHECK(result.find("**") != std::string::npos);
}

TEST_CASE("Translator: integer division") {
    PythonTranslator tr;
    auto result = tr.translate_source("x = 10 % 3");
    CHECK(result.find("//") != std::string::npos);
}

TEST_CASE("Translator: Python keyword avoidance") {
    // variable named 'class' should become 'rexx_class'
    auto result = PythonTranslator::py_varname("class");
    CHECK(result == "rexx_class");
}

TEST_CASE("Translator: header includes imports") {
    PythonTranslator tr;
    auto result = tr.translate_source("SAY 'hello'");
    CHECK(result.find("import subprocess") != std::string::npos);
    CHECK(result.find("import sys") != std::string::npos);
}

TEST_CASE("Translator: label generates def") {
    PythonTranslator tr;
    Lexer lex("myFunc:");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse();
    auto result = tr.translate(prog);
    CHECK(result.find("def myfunc():") != std::string::npos);
}

// ============================================================================
// RUNTIME TESTS (30 tests)
// ============================================================================

TEST_CASE("Runtime: set and get variable") {
    RexxRuntime rt;
    rt.set_var("X", RexxValue(int64_t(42)));
    CHECK(rt.get_var("X").to_integer() == 42);
}

TEST_CASE("Runtime: variable case insensitive") {
    RexxRuntime rt;
    rt.set_var("myVar", RexxValue("hello"));
    CHECK(rt.get_var("MYVAR").to_string() == "hello");
}

TEST_CASE("Runtime: unset variable returns name") {
    RexxRuntime rt;
    CHECK(rt.get_var("UNDEFINED").to_string() == "UNDEFINED");
}

TEST_CASE("Runtime: drop variable") {
    RexxRuntime rt;
    rt.set_var("X", RexxValue(int64_t(10)));
    rt.drop_var("X");
    CHECK(!rt.var_exists("X"));
}

TEST_CASE("Runtime: stem set and get") {
    RexxRuntime rt;
    rt.set_stem_value("ARR", {"1"}, RexxValue("first"));
    CHECK(rt.get_stem_value("ARR", {"1"}).to_string() == "first");
}

TEST_CASE("Runtime: stem compound key") {
    RexxRuntime rt;
    rt.set_stem_value("DATA", {"A", "B"}, RexxValue("compound"));
    CHECK(rt.get_stem_value("DATA", {"A", "B"}).to_string() == "compound");
}

TEST_CASE("Runtime: stem default value") {
    RexxRuntime rt;
    rt.get_stem("ARR").default_value = RexxValue("default");
    rt.set_stem_value("ARR", {"1"}, RexxValue("set"));
    CHECK(rt.get_stem_value("ARR", {"1"}).to_string() == "set");
    CHECK(rt.get_stem_value("ARR", {"99"}).to_string() == "default");
}

TEST_CASE("Runtime: stem size") {
    RexxRuntime rt;
    rt.set_stem_value("S", {"A"}, RexxValue("1"));
    rt.set_stem_value("S", {"B"}, RexxValue("2"));
    rt.set_stem_value("S", {"C"}, RexxValue("3"));
    CHECK(rt.get_stem("S").size() == 3);
}

TEST_CASE("Runtime: stem drop key") {
    RexxRuntime rt;
    rt.set_stem_value("S", {"X"}, RexxValue("val"));
    rt.get_stem("S").drop("X");
    CHECK(!rt.get_stem("S").exists("X"));
}

TEST_CASE("Runtime: stem build_key") {
    auto key = RexxStem::build_key({"a", "b", "c"});
    CHECK(key == "A.B.C");
}

TEST_CASE("Runtime: queue PUSH (LIFO)") {
    RexxRuntime rt;
    rt.get_queue().push(RexxValue("first"));
    rt.get_queue().push(RexxValue("second"));
    CHECK(rt.get_queue().pull().to_string() == "second");
    CHECK(rt.get_queue().pull().to_string() == "first");
}

TEST_CASE("Runtime: queue QUEUE (FIFO)") {
    RexxRuntime rt;
    rt.get_queue().queue(RexxValue("first"));
    rt.get_queue().queue(RexxValue("second"));
    CHECK(rt.get_queue().pull().to_string() == "first");
    CHECK(rt.get_queue().pull().to_string() == "second");
}

TEST_CASE("Runtime: queue QUEUED count") {
    RexxRuntime rt;
    rt.get_queue().push(RexxValue("a"));
    rt.get_queue().push(RexxValue("b"));
    CHECK(rt.get_queue().queued() == 2);
    rt.get_queue().pull();
    CHECK(rt.get_queue().queued() == 1);
}

TEST_CASE("Runtime: queue empty pull") {
    RexxRuntime rt;
    CHECK(rt.get_queue().pull().to_string() == "");
}

TEST_CASE("Runtime: ADDRESS register and execute") {
    RexxRuntime rt;
    int last_rc = -1;
    rt.register_address("TEST", [&last_rc](const std::string& cmd) -> int {
        last_rc = (cmd == "HELLO") ? 0 : 4;
        return last_rc;
    });
    int rc = rt.execute_address("TEST", "HELLO");
    CHECK(rc == 0);
    CHECK(last_rc == 0);
}

TEST_CASE("Runtime: ADDRESS unregistered returns -1") {
    RexxRuntime rt;
    CHECK(rt.execute_address("NOSUCH", "cmd") == -1);
}

TEST_CASE("Runtime: current address default") {
    RexxRuntime rt;
    CHECK(rt.current_address() == "SYSTEM");
}

TEST_CASE("Runtime: set current address") {
    RexxRuntime rt;
    rt.set_current_address("TSO");
    CHECK(rt.current_address() == "TSO");
}

TEST_CASE("Runtime: TRACE level") {
    RexxRuntime rt;
    rt.set_trace("R");
    CHECK(rt.trace_level() == TraceLevel::Results);
}

TEST_CASE("Runtime: TRACE all levels") {
    CHECK(parse_trace_level("A") == TraceLevel::All);
    CHECK(parse_trace_level("C") == TraceLevel::Commands);
    CHECK(parse_trace_level("E") == TraceLevel::Error);
    CHECK(parse_trace_level("F") == TraceLevel::Failure);
    CHECK(parse_trace_level("I") == TraceLevel::Intermediates);
    CHECK(parse_trace_level("O") == TraceLevel::Off);
    CHECK(parse_trace_level("S") == TraceLevel::Scan);
}

TEST_CASE("Runtime: TRACE level name roundtrip") {
    CHECK(trace_level_name(TraceLevel::Results) == "R");
    CHECK(trace_level_name(TraceLevel::Normal) == "N");
}

TEST_CASE("Runtime: NUMERIC DIGITS") {
    RexxRuntime rt;
    CHECK(rt.numeric_digits() == 9);
    rt.set_numeric_digits(15);
    CHECK(rt.numeric_digits() == 15);
}

TEST_CASE("Runtime: NUMERIC FUZZ") {
    RexxRuntime rt;
    CHECK(rt.numeric_fuzz() == 0);
    rt.set_numeric_fuzz(3);
    CHECK(rt.numeric_fuzz() == 3);
}

TEST_CASE("Runtime: NUMERIC FORM") {
    RexxRuntime rt;
    CHECK(rt.numeric_form() == "SCIENTIFIC");
    rt.set_numeric_form("ENGINEERING");
    CHECK(rt.numeric_form() == "ENGINEERING");
}

TEST_CASE("Runtime: SIGNAL ON condition trap") {
    RexxRuntime rt;
    rt.set_condition_trap(ConditionType::Syntax, true, "syntax_handler");
    CHECK(rt.has_condition_trap(ConditionType::Syntax));
    auto& trap = rt.get_condition_trap(ConditionType::Syntax);
    CHECK(trap.is_signal == true);
    CHECK(trap.label == "syntax_handler");
}

TEST_CASE("Runtime: CALL ON condition trap") {
    RexxRuntime rt;
    rt.set_condition_trap(ConditionType::Error, false, "error_handler");
    CHECK(rt.has_condition_trap(ConditionType::Error));
    auto& trap = rt.get_condition_trap(ConditionType::Error);
    CHECK(trap.is_signal == false);
}

TEST_CASE("Runtime: clear condition trap") {
    RexxRuntime rt;
    rt.set_condition_trap(ConditionType::Halt, true, "halt_handler");
    rt.clear_condition_trap(ConditionType::Halt);
    CHECK(!rt.has_condition_trap(ConditionType::Halt));
}

TEST_CASE("Runtime: SAY output capture") {
    RexxRuntime rt;
    rt.say("line 1");
    rt.say("line 2");
    CHECK(rt.output().size() == 2);
    CHECK(rt.output()[0] == "line 1");
}

TEST_CASE("Runtime: PARSE template word split") {
    RexxRuntime rt;
    std::vector<TemplateItem> tmpl = {
        TemplateItem::var("first"),
        TemplateItem::var("second"),
        TemplateItem::var("rest")
    };
    rt.apply_template("hello world how are you", tmpl);
    CHECK(rt.get_var("FIRST").to_string() == "hello");
    CHECK(rt.get_var("SECOND").to_string() == "world");
    CHECK(rt.get_var("REST").to_string() == "how are you");
}

TEST_CASE("Runtime: PARSE template literal delimiter") {
    RexxRuntime rt;
    std::vector<TemplateItem> tmpl = {
        TemplateItem::var("name"),
        TemplateItem::lit(","),
        TemplateItem::var("city")
    };
    rt.apply_template("John,New York", tmpl);
    CHECK(rt.get_var("NAME").to_string() == "John");
    CHECK(rt.get_var("CITY").to_string() == "New York");
}

// ============================================================================
// REXX VALUE TESTS (8 tests)
// ============================================================================

TEST_CASE("RexxValue: integer coercion") {
    RexxValue v(int64_t(42));
    CHECK(v.to_integer() == 42);
    CHECK(v.to_double() == 42.0);
    CHECK(v.to_string() == "42");
}

TEST_CASE("RexxValue: double coercion") {
    RexxValue v(3.14);
    CHECK(v.to_integer() == 3);
    CHECK(v.to_string().find("3.14") != std::string::npos);
}

TEST_CASE("RexxValue: string numeric coercion") {
    RexxValue v("123");
    CHECK(v.is_numeric());
    CHECK(v.to_integer() == 123);
}

TEST_CASE("RexxValue: string non-numeric") {
    RexxValue v("hello");
    CHECK(!v.is_numeric());
}

TEST_CASE("RexxValue: arithmetic add") {
    auto r = RexxValue::add(RexxValue(int64_t(10)), RexxValue(int64_t(20)));
    CHECK(r.to_integer() == 30);
}

TEST_CASE("RexxValue: arithmetic sub") {
    auto r = RexxValue::sub(RexxValue(int64_t(10)), RexxValue(int64_t(3)));
    CHECK(r.to_integer() == 7);
}

TEST_CASE("RexxValue: arithmetic mul") {
    auto r = RexxValue::mul(RexxValue(int64_t(6)), RexxValue(int64_t(7)));
    CHECK(r.to_integer() == 42);
}

TEST_CASE("RexxValue: concat") {
    auto r = RexxValue::concat(RexxValue("hello"), RexxValue(" world"));
    CHECK(r.to_string() == "hello world");
}

// ============================================================================
// INTEGRATION TESTS (12 tests)
// ============================================================================

TEST_CASE("Integration: simple SAY program") {
    PythonTranslator tr;
    std::string src = "SAY 'Hello, World!'";
    auto py = tr.translate_source(src);
    CHECK(py.find("print('Hello, World!')") != std::string::npos);
}

TEST_CASE("Integration: assignment and SAY") {
    PythonTranslator tr;
    std::string src = "x = 42\nSAY x";
    auto py = tr.translate_source(src);
    CHECK(py.find("x = 42") != std::string::npos);
    CHECK(py.find("print(x)") != std::string::npos);
}

TEST_CASE("Integration: DO loop program") {
    PythonTranslator tr;
    std::string src = "DO i = 1 TO 5\n  SAY i\nEND";
    auto py = tr.translate_source(src);
    CHECK(py.find("for i in range(") != std::string::npos);
    CHECK(py.find("print(i)") != std::string::npos);
}

TEST_CASE("Integration: IF ELSE program") {
    PythonTranslator tr;
    std::string src = "x = 10\nIF x > 5 THEN SAY 'big' ELSE SAY 'small'";
    auto py = tr.translate_source(src);
    CHECK(py.find("if ") != std::string::npos);
    CHECK(py.find("else:") != std::string::npos);
}

TEST_CASE("Integration: SELECT program") {
    PythonTranslator tr;
    std::string src = "x = 2\nSELECT\nWHEN x = 1 THEN SAY 'one'\nWHEN x = 2 THEN SAY 'two'\nOTHERWISE\nSAY 'other'\nEND";
    auto py = tr.translate_source(src);
    CHECK(py.find("if ") != std::string::npos);
    CHECK(py.find("elif ") != std::string::npos);
}

TEST_CASE("Integration: function call in expression") {
    PythonTranslator tr;
    std::string src = "SAY LENGTH('hello')";
    auto py = tr.translate_source(src);
    CHECK(py.find("print(len(") != std::string::npos);
}

TEST_CASE("Integration: DO WHILE with LEAVE") {
    PythonTranslator tr;
    std::string src = "x = 0\nDO WHILE x < 100\nx = x + 1\nIF x = 10 THEN LEAVE\nEND";
    auto py = tr.translate_source(src);
    CHECK(py.find("while ") != std::string::npos);
    CHECK(py.find("break") != std::string::npos);
}

TEST_CASE("Integration: PARSE template") {
    PythonTranslator tr;
    std::string src = "line = 'John,NYC'\nPARSE VAR line name ',' city\nSAY name\nSAY city";
    auto py = tr.translate_source(src);
    CHECK(py.find(".find(") != std::string::npos);
    CHECK(py.find("print(name)") != std::string::npos);
}

TEST_CASE("Integration: runtime stem operations") {
    RexxRuntime rt;
    // Simulate: arr.0 = 3; arr.1 = 'a'; arr.2 = 'b'; arr.3 = 'c'
    rt.set_stem_value("ARR", {"0"}, RexxValue(int64_t(3)));
    rt.set_stem_value("ARR", {"1"}, RexxValue("a"));
    rt.set_stem_value("ARR", {"2"}, RexxValue("b"));
    rt.set_stem_value("ARR", {"3"}, RexxValue("c"));
    int count = static_cast<int>(rt.get_stem_value("ARR", {"0"}).to_integer());
    CHECK(count == 3);
    CHECK(rt.get_stem_value("ARR", {"2"}).to_string() == "b");
}

TEST_CASE("Integration: runtime queue round trip") {
    RexxRuntime rt;
    rt.get_queue().queue(RexxValue("A"));
    rt.get_queue().queue(RexxValue("B"));
    rt.get_queue().push(RexxValue("Z"));
    // PUSH adds to front, so order is: Z, A, B
    CHECK(rt.get_queue().pull().to_string() == "Z");
    CHECK(rt.get_queue().pull().to_string() == "A");
    CHECK(rt.get_queue().pull().to_string() == "B");
}

TEST_CASE("Integration: condition name roundtrip") {
    CHECK(condition_name(ConditionType::Error) == "ERROR");
    CHECK(condition_name(ConditionType::Failure) == "FAILURE");
    CHECK(condition_name(ConditionType::Halt) == "HALT");
    CHECK(condition_name(ConditionType::NoValue) == "NOVALUE");
    CHECK(condition_name(ConditionType::Syntax) == "SYNTAX");
    CHECK(condition_name(ConditionType::LostDigits) == "LOSTDIGITS");
    CHECK(condition_name(ConditionType::NotReady) == "NOTREADY");
}

TEST_CASE("Integration: is_builtin check") {
    CHECK(is_builtin("LENGTH"));
    CHECK(is_builtin("length"));
    CHECK(is_builtin("SUBSTR"));
    CHECK(is_builtin("ABS"));
    CHECK(!is_builtin("NOTABUILTIN"));
}
