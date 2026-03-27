#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

#include "include/lazarus/pli/ast.h"
#include "include/lazarus/pli/lexer.h"
#include "include/lazarus/pli/parser.h"
#include "include/lazarus/pli/builtins.h"
#include "include/lazarus/pli/runtime.h"
#include "include/lazarus/pli/preprocessor.h"
#include "include/lazarus/pli/translator.h"

using namespace lazarus::pli;
using namespace lazarus::pli::builtins;
using namespace lazarus::pli::runtime;

// ============================================================
// SECTION 1: LEXER TESTS (40+)
// ============================================================

TEST_CASE("Lexer: empty input") {
    Lexer lex("");
    auto tokens = lex.tokenize();
    REQUIRE(tokens.size() == 1);
    CHECK(tokens[0].type == TokenType::END_OF_INPUT);
}

TEST_CASE("Lexer: PROCEDURE keyword") {
    Lexer lex("PROCEDURE");
    auto tokens = lex.tokenize();
    REQUIRE(tokens.size() >= 1);
    CHECK(tokens[0].type == TokenType::PROCEDURE);
}

TEST_CASE("Lexer: PROC abbreviation") {
    Lexer lex("PROC");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PROCEDURE);
}

TEST_CASE("Lexer: DCL keyword") {
    Lexer lex("DCL");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::DCL);
}

TEST_CASE("Lexer: DECLARE keyword") {
    Lexer lex("DECLARE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::DECLARE);
}

TEST_CASE("Lexer: BEGIN/END keywords") {
    Lexer lex("BEGIN END");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::BEGIN);
    CHECK(tokens[1].type == TokenType::END);
}

TEST_CASE("Lexer: DO/WHILE/UNTIL/TO/BY") {
    Lexer lex("DO WHILE UNTIL TO BY");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::DO);
    CHECK(tokens[1].type == TokenType::WHILE);
    CHECK(tokens[2].type == TokenType::UNTIL);
    CHECK(tokens[3].type == TokenType::TO);
    CHECK(tokens[4].type == TokenType::BY);
}

TEST_CASE("Lexer: RETURN CALL") {
    Lexer lex("RETURN CALL");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::RETURN);
    CHECK(tokens[1].type == TokenType::CALL);
}

TEST_CASE("Lexer: GOTO keyword") {
    Lexer lex("GOTO");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::GO_TO);
}

TEST_CASE("Lexer: IF THEN ELSE") {
    Lexer lex("IF THEN ELSE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::IF);
    CHECK(tokens[1].type == TokenType::THEN);
    CHECK(tokens[2].type == TokenType::ELSE);
}

TEST_CASE("Lexer: SELECT WHEN OTHERWISE") {
    Lexer lex("SELECT WHEN OTHERWISE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::SELECT);
    CHECK(tokens[1].type == TokenType::WHEN);
    CHECK(tokens[2].type == TokenType::OTHERWISE);
}

TEST_CASE("Lexer: PUT GET OPEN CLOSE") {
    Lexer lex("PUT GET OPEN CLOSE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PUT);
    CHECK(tokens[1].type == TokenType::GET);
    CHECK(tokens[2].type == TokenType::OPEN);
    CHECK(tokens[3].type == TokenType::CLOSE);
}

TEST_CASE("Lexer: READ WRITE REWRITE DELETE") {
    Lexer lex("READ WRITE REWRITE DELETE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::READ);
    CHECK(tokens[1].type == TokenType::WRITE);
    CHECK(tokens[2].type == TokenType::REWRITE);
    CHECK(tokens[3].type == TokenType::DELETE_KW);
}

TEST_CASE("Lexer: ALLOCATE FREE") {
    Lexer lex("ALLOCATE FREE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::ALLOCATE);
    CHECK(tokens[1].type == TokenType::FREE);
}

TEST_CASE("Lexer: ON SIGNAL REVERT") {
    Lexer lex("ON SIGNAL REVERT");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::ON);
    CHECK(tokens[1].type == TokenType::SIGNAL);
    CHECK(tokens[2].type == TokenType::REVERT);
}

TEST_CASE("Lexer: DISPLAY STOP LEAVE ITERATE") {
    Lexer lex("DISPLAY STOP LEAVE ITERATE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::DISPLAY);
    CHECK(tokens[1].type == TokenType::STOP);
    CHECK(tokens[2].type == TokenType::LEAVE);
    CHECK(tokens[3].type == TokenType::ITERATE);
}

TEST_CASE("Lexer: ENTRY keyword") {
    Lexer lex("ENTRY");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::ENTRY);
}

TEST_CASE("Lexer: type keywords") {
    Lexer lex("FIXED FLOAT CHAR BIT POINTER LABEL FILE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::FIXED);
    CHECK(tokens[1].type == TokenType::FLOAT);
    CHECK(tokens[2].type == TokenType::CHAR);
    CHECK(tokens[3].type == TokenType::BIT);
    CHECK(tokens[4].type == TokenType::POINTER);
    CHECK(tokens[5].type == TokenType::LABEL);
    CHECK(tokens[6].type == TokenType::FILE_KW);
}

TEST_CASE("Lexer: DEC BIN attributes") {
    Lexer lex("DEC BIN DECIMAL BINARY");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::DEC);
    CHECK(tokens[1].type == TokenType::BIN);
    CHECK(tokens[2].type == TokenType::DEC);
    CHECK(tokens[3].type == TokenType::BIN);
}

TEST_CASE("Lexer: storage class attributes") {
    Lexer lex("AUTOMATIC STATIC BASED DEFINED CONTROLLED EXTERNAL INTERNAL");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::AUTOMATIC);
    CHECK(tokens[1].type == TokenType::STATIC);
    CHECK(tokens[2].type == TokenType::BASED);
    CHECK(tokens[3].type == TokenType::DEFINED);
    CHECK(tokens[4].type == TokenType::CONTROLLED);
    CHECK(tokens[5].type == TokenType::EXTERNAL);
    CHECK(tokens[6].type == TokenType::INTERNAL);
}

TEST_CASE("Lexer: ALIGNED UNALIGNED VARYING") {
    Lexer lex("ALIGNED UNALIGNED VARYING");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::ALIGNED);
    CHECK(tokens[1].type == TokenType::UNALIGNED);
    CHECK(tokens[2].type == TokenType::VARYING);
}

TEST_CASE("Lexer: string literal") {
    Lexer lex("'hello world'");
    auto tokens = lex.tokenize();
    REQUIRE(tokens.size() >= 1);
    CHECK(tokens[0].type == TokenType::STRING_LIT);
    CHECK(tokens[0].value == "hello world");
}

TEST_CASE("Lexer: string with doubled quotes") {
    Lexer lex("'it''s'");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::STRING_LIT);
    CHECK(tokens[0].value == "it's");
}

TEST_CASE("Lexer: bit string literal") {
    Lexer lex("'1010'B");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::BIT_LIT);
    CHECK(tokens[0].value == "1010");
}

TEST_CASE("Lexer: complex literal") {
    Lexer lex("'3.14'I");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::COMPLEX_LIT);
}

TEST_CASE("Lexer: integer literal") {
    Lexer lex("42");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::INTEGER_LIT);
    CHECK(tokens[0].value == "42");
}

TEST_CASE("Lexer: float literal") {
    Lexer lex("3.14");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::FLOAT_LIT);
    CHECK(tokens[0].value == "3.14");
}

TEST_CASE("Lexer: scientific notation") {
    Lexer lex("1.5E10");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::FLOAT_LIT);
    CHECK(tokens[0].value == "1.5E10");
}

TEST_CASE("Lexer: operators + - * /") {
    Lexer lex("+ - * /");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PLUS);
    CHECK(tokens[1].type == TokenType::MINUS);
    CHECK(tokens[2].type == TokenType::STAR);
    CHECK(tokens[3].type == TokenType::SLASH);
}

TEST_CASE("Lexer: power operator **") {
    Lexer lex("**");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::POWER);
}

TEST_CASE("Lexer: concatenation ||") {
    Lexer lex("||");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::CONCAT);
}

TEST_CASE("Lexer: comparison operators") {
    Lexer lex("= ^= >= <= > <");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::EQ);
    CHECK(tokens[1].type == TokenType::NE);
    CHECK(tokens[2].type == TokenType::GE);
    CHECK(tokens[3].type == TokenType::LE);
    CHECK(tokens[4].type == TokenType::GT);
    CHECK(tokens[5].type == TokenType::LT);
}

TEST_CASE("Lexer: logical operators") {
    Lexer lex("^ & |");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::NOT);
    CHECK(tokens[1].type == TokenType::AND_OP);
    CHECK(tokens[2].type == TokenType::OR_OP);
}

TEST_CASE("Lexer: punctuation") {
    Lexer lex("( ) , ; . :");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::LPAREN);
    CHECK(tokens[1].type == TokenType::RPAREN);
    CHECK(tokens[2].type == TokenType::COMMA);
    CHECK(tokens[3].type == TokenType::SEMICOLON);
    CHECK(tokens[4].type == TokenType::DOT);
    CHECK(tokens[5].type == TokenType::COLON);
}

TEST_CASE("Lexer: comment") {
    Lexer lex("/* this is a comment */");
    auto tokens = lex.tokenize();
    REQUIRE(tokens.size() >= 1);
    CHECK(tokens[0].type == TokenType::COMMENT);
    CHECK(tokens[0].value == " this is a comment ");
}

TEST_CASE("Lexer: multiline comment") {
    Lexer lex("/* line1\nline2 */");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::COMMENT);
}

TEST_CASE("Lexer: semicolon-terminated statement") {
    Lexer lex("x = 5;");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[1].type == TokenType::EQ);
    CHECK(tokens[2].type == TokenType::INTEGER_LIT);
    CHECK(tokens[3].type == TokenType::SEMICOLON);
}

TEST_CASE("Lexer: %INCLUDE directive") {
    Lexer lex("%INCLUDE 'member';");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PCT_INCLUDE);
}

TEST_CASE("Lexer: %IF %THEN %ELSE directives") {
    Lexer lex("%IF %THEN %ELSE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PCT_IF);
    CHECK(tokens[1].type == TokenType::PCT_THEN);
    CHECK(tokens[2].type == TokenType::PCT_ELSE);
}

TEST_CASE("Lexer: %DECLARE directive") {
    Lexer lex("%DECLARE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PCT_DECLARE);
}

TEST_CASE("Lexer: %REPLACE directive") {
    Lexer lex("%REPLACE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PCT_REPLACE);
}

TEST_CASE("Lexer: %ACTIVATE %DEACTIVATE") {
    Lexer lex("%ACTIVATE %DEACTIVATE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PCT_ACTIVATE);
    CHECK(tokens[1].type == TokenType::PCT_DEACTIVATE);
}

TEST_CASE("Lexer: %DO %END") {
    Lexer lex("%DO %END");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::PCT_DO);
    CHECK(tokens[1].type == TokenType::PCT_END);
}

TEST_CASE("Lexer: case insensitivity") {
    Lexer lex("declare Declare DECLARE");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::DECLARE);
    CHECK(tokens[1].type == TokenType::DECLARE);
    CHECK(tokens[2].type == TokenType::DECLARE);
}

TEST_CASE("Lexer: identifier with special chars") {
    Lexer lex("my_var #temp @data $sys");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::IDENTIFIER);
    CHECK(tokens[0].value == "my_var");
    CHECK(tokens[1].type == TokenType::IDENTIFIER);
    CHECK(tokens[1].value == "#temp");
    CHECK(tokens[2].type == TokenType::IDENTIFIER);
    CHECK(tokens[2].value == "@data");
    CHECK(tokens[3].type == TokenType::IDENTIFIER);
    CHECK(tokens[3].value == "$sys");
}

TEST_CASE("Lexer: line and column tracking") {
    Lexer lex("x\ny");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].line == 1);
    CHECK(tokens[1].line == 2);
}

TEST_CASE("Lexer: abbreviation PTR") {
    Lexer lex("PTR");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::POINTER);
}

TEST_CASE("Lexer: abbreviation UNAL") {
    Lexer lex("UNAL");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::UNALIGNED);
}

TEST_CASE("Lexer: full declaration statement") {
    Lexer lex("DCL X FIXED BIN(31);");
    auto tokens = lex.tokenize();
    CHECK(tokens[0].type == TokenType::DCL);
    CHECK(tokens[1].type == TokenType::IDENTIFIER);
    CHECK(tokens[2].type == TokenType::FIXED);
    CHECK(tokens[3].type == TokenType::BIN);
}

// ============================================================
// SECTION 2: PARSER TESTS (40+)
// ============================================================

TEST_CASE("Parser: simple procedure") {
    Lexer lex("HELLO: PROCEDURE OPTIONS(MAIN); END HELLO;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse_program();
    CHECK(prog.name == "HELLO");
    CHECK(prog.options_main == true);
}

TEST_CASE("Parser: scalar declaration FIXED BIN") {
    Lexer lex("DCL X FIXED BIN;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->name == "X");
    CHECK(sd->attrs.base_type == BaseType::FIXED);
    CHECK(sd->attrs.scale == Scale::BINARY);
}

TEST_CASE("Parser: scalar declaration FLOAT DEC") {
    Lexer lex("DCL Y FLOAT DEC;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->name == "Y");
    CHECK(sd->attrs.base_type == BaseType::FLOAT);
    CHECK(sd->attrs.scale == Scale::DECIMAL);
}

TEST_CASE("Parser: CHAR declaration with length") {
    Lexer lex("DCL NAME CHAR(30);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->attrs.base_type == BaseType::CHAR);
    CHECK(sd->attrs.char_length == 30);
}

TEST_CASE("Parser: CHAR VARYING declaration") {
    Lexer lex("DCL MSG CHAR(100) VARYING;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->attrs.varying == true);
}

TEST_CASE("Parser: BIT declaration") {
    Lexer lex("DCL FLAGS BIT(8);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->attrs.base_type == BaseType::BIT);
    CHECK(sd->attrs.bit_length == 8);
}

TEST_CASE("Parser: POINTER declaration") {
    Lexer lex("DCL P POINTER;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->attrs.base_type == BaseType::POINTER);
}

TEST_CASE("Parser: BASED declaration") {
    Lexer lex("DCL ITEM FIXED BIN BASED(P);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->attrs.storage == StorageClass::BASED);
    CHECK(sd->attrs.based_var == "P");
}

TEST_CASE("Parser: STATIC declaration") {
    Lexer lex("DCL COUNTER FIXED BIN STATIC;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->attrs.storage == StorageClass::STATIC);
}

TEST_CASE("Parser: INITIAL declaration") {
    Lexer lex("DCL MAX_SIZE FIXED BIN INITIAL(100);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->attrs.is_initial == true);
    CHECK(sd->attrs.initial_value == "100");
}

TEST_CASE("Parser: array declaration") {
    Lexer lex("DCL NUMS(10) FIXED BIN;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* ad = std::get_if<ArrayDecl>(&decl->node);
    REQUIRE(ad != nullptr);
    CHECK(ad->name == "NUMS");
    CHECK(ad->dimensions.size() == 1);
    CHECK(ad->dimensions[0].lower == 1);
    CHECK(ad->dimensions[0].upper == 10);
}

TEST_CASE("Parser: 2D array declaration") {
    Lexer lex("DCL MATRIX(5, 5) FLOAT DEC;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* ad = std::get_if<ArrayDecl>(&decl->node);
    REQUIRE(ad != nullptr);
    CHECK(ad->dimensions.size() == 2);
}

TEST_CASE("Parser: array with lower bound") {
    Lexer lex("DCL ARR(0:9) FIXED BIN;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* ad = std::get_if<ArrayDecl>(&decl->node);
    REQUIRE(ad != nullptr);
    CHECK(ad->dimensions[0].lower == 0);
    CHECK(ad->dimensions[0].upper == 9);
}

TEST_CASE("Parser: structure declaration") {
    Lexer lex("DCL 1 EMPLOYEE, 2 NAME CHAR(30), 2 AGE FIXED BIN;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<StructureDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->name == "EMPLOYEE");
    CHECK(sd->level == 1);
    CHECK(sd->members.size() == 2);
    CHECK(sd->members[0].name == "NAME");
    CHECK(sd->members[1].name == "AGE");
}

TEST_CASE("Parser: PICTURE declaration") {
    Lexer lex("DCL AMOUNT PICTURE '999V99';");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->attrs.base_type == BaseType::PICTURE);
    CHECK(sd->attrs.picture == "999V99");
}

TEST_CASE("Parser: EXTERNAL declaration") {
    Lexer lex("DCL SHARED_VAR FIXED BIN EXTERNAL;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->attrs.scope == Scope::EXTERNAL);
}

TEST_CASE("Parser: PRECISION declaration") {
    Lexer lex("DCL PRICE FIXED DEC PRECISION(10, 2);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto decl = parser.parse_declaration();
    auto* sd = std::get_if<ScalarDecl>(&decl->node);
    REQUIRE(sd != nullptr);
    CHECK(sd->attrs.precision == 10);
    CHECK(sd->attrs.scale_factor == 2);
}

TEST_CASE("Parser: assignment statement") {
    Lexer lex("X = 42;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* assign = std::get_if<AssignmentStmt>(&stmt->node);
    REQUIRE(assign != nullptr);
}

TEST_CASE("Parser: IF/THEN/ELSE") {
    Lexer lex("IF X > 0 THEN Y = 1; ELSE Y = 0;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* ifs = std::get_if<IfStmt>(&stmt->node);
    REQUIRE(ifs != nullptr);
    CHECK(ifs->then_body.size() == 1);
    CHECK(ifs->else_body.size() == 1);
}

TEST_CASE("Parser: IF without ELSE") {
    Lexer lex("IF X = 0 THEN CALL HANDLE_ZERO;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* ifs = std::get_if<IfStmt>(&stmt->node);
    REQUIRE(ifs != nullptr);
    CHECK(ifs->then_body.size() == 1);
    CHECK(ifs->else_body.empty());
}

TEST_CASE("Parser: SELECT/WHEN/OTHERWISE") {
    Lexer lex("SELECT (X); WHEN (1) Y = 10; WHEN (2) Y = 20; OTHERWISE Y = 0; END;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* sel = std::get_if<SelectStmt>(&stmt->node);
    REQUIRE(sel != nullptr);
    CHECK(sel->whens.size() == 2);
    CHECK(sel->otherwise_body.size() == 1);
}

TEST_CASE("Parser: simple DO block") {
    Lexer lex("DO; X = 1; Y = 2; END;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* dl = std::get_if<DoLoopStmt>(&stmt->node);
    REQUIRE(dl != nullptr);
    CHECK(dl->kind == DoLoopStmt::SIMPLE);
    CHECK(dl->body.size() == 2);
}

TEST_CASE("Parser: DO WHILE loop") {
    Lexer lex("DO WHILE (X > 0); X = X - 1; END;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* dl = std::get_if<DoLoopStmt>(&stmt->node);
    REQUIRE(dl != nullptr);
    CHECK(dl->kind == DoLoopStmt::WHILE);
    CHECK(dl->while_cond != nullptr);
}

TEST_CASE("Parser: DO UNTIL loop") {
    Lexer lex("DO UNTIL (DONE = 1); X = X + 1; END;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* dl = std::get_if<DoLoopStmt>(&stmt->node);
    REQUIRE(dl != nullptr);
    CHECK(dl->kind == DoLoopStmt::UNTIL);
}

TEST_CASE("Parser: iterative DO loop") {
    Lexer lex("DO I = 1 TO 10 BY 2; X = X + I; END;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* dl = std::get_if<DoLoopStmt>(&stmt->node);
    REQUIRE(dl != nullptr);
    CHECK(dl->kind == DoLoopStmt::ITERATIVE);
    CHECK(dl->variable == "I");
    CHECK(dl->to != nullptr);
    CHECK(dl->by != nullptr);
}

TEST_CASE("Parser: CALL statement") {
    Lexer lex("CALL MYPROC(A, B, C);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* call = std::get_if<CallStmt>(&stmt->node);
    REQUIRE(call != nullptr);
    CHECK(call->target == "MYPROC");
    CHECK(call->args.size() == 3);
}

TEST_CASE("Parser: RETURN with value") {
    Lexer lex("RETURN(X + 1);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* ret = std::get_if<ReturnStmt>(&stmt->node);
    REQUIRE(ret != nullptr);
    CHECK(ret->value != nullptr);
}

TEST_CASE("Parser: RETURN without value") {
    Lexer lex("RETURN;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* ret = std::get_if<ReturnStmt>(&stmt->node);
    REQUIRE(ret != nullptr);
    CHECK(ret->value == nullptr);
}

TEST_CASE("Parser: PUT LIST") {
    Lexer lex("PUT LIST(X, Y, Z);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* put = std::get_if<PutListStmt>(&stmt->node);
    REQUIRE(put != nullptr);
    CHECK(put->items.size() == 3);
}

TEST_CASE("Parser: PUT SKIP LIST") {
    Lexer lex("PUT SKIP LIST(X);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* put = std::get_if<PutListStmt>(&stmt->node);
    REQUIRE(put != nullptr);
    CHECK(put->skip == true);
}

TEST_CASE("Parser: PUT EDIT with format") {
    Lexer lex("PUT EDIT(X, Y) (F(10,2), A(20));");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* put = std::get_if<PutEditStmt>(&stmt->node);
    REQUIRE(put != nullptr);
    CHECK(put->items.size() == 2);
    CHECK(put->formats.size() == 2);
    CHECK(put->formats[0].kind == FormatItem::F);
    CHECK(put->formats[0].w == 10);
    CHECK(put->formats[0].d == 2);
    CHECK(put->formats[1].kind == FormatItem::A);
}

TEST_CASE("Parser: GET LIST") {
    Lexer lex("GET LIST(X, Y);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* get = std::get_if<GetListStmt>(&stmt->node);
    REQUIRE(get != nullptr);
    CHECK(get->targets.size() == 2);
}

TEST_CASE("Parser: OPEN statement") {
    Lexer lex("OPEN FILE(INFILE) INPUT;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* open = std::get_if<OpenStmt>(&stmt->node);
    REQUIRE(open != nullptr);
    CHECK(open->file_name == "INFILE");
    CHECK(open->mode == OpenStmt::INPUT);
}

TEST_CASE("Parser: CLOSE statement") {
    Lexer lex("CLOSE FILE(OUTFILE);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* cls = std::get_if<CloseStmt>(&stmt->node);
    REQUIRE(cls != nullptr);
    CHECK(cls->file_name == "OUTFILE");
}

TEST_CASE("Parser: READ statement") {
    Lexer lex("READ FILE(INFILE) INTO(BUFFER);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* rd = std::get_if<ReadStmt>(&stmt->node);
    REQUIRE(rd != nullptr);
    CHECK(rd->file_name == "INFILE");
    CHECK(rd->into != nullptr);
}

TEST_CASE("Parser: WRITE statement") {
    Lexer lex("WRITE FILE(OUTFILE) FROM(RECORD);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* wr = std::get_if<WriteStmt>(&stmt->node);
    REQUIRE(wr != nullptr);
    CHECK(wr->file_name == "OUTFILE");
}

TEST_CASE("Parser: ALLOCATE statement") {
    Lexer lex("ALLOCATE ITEM SET(P);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* alloc = std::get_if<AllocateStmt>(&stmt->node);
    REQUIRE(alloc != nullptr);
    CHECK(alloc->variable == "ITEM");
}

TEST_CASE("Parser: FREE statement") {
    Lexer lex("FREE ITEM;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* fr = std::get_if<FreeStmt>(&stmt->node);
    REQUIRE(fr != nullptr);
    CHECK(fr->variable == "ITEM");
}

TEST_CASE("Parser: ON ENDFILE statement") {
    Lexer lex("ON ENDFILE(INFILE) GOTO EOF_HANDLER;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* on = std::get_if<OnStmt>(&stmt->node);
    REQUIRE(on != nullptr);
    CHECK(on->condition == "ENDFILE(INFILE)");
}

TEST_CASE("Parser: SIGNAL statement") {
    Lexer lex("SIGNAL ERROR;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* sig = std::get_if<SignalStmt>(&stmt->node);
    REQUIRE(sig != nullptr);
}

TEST_CASE("Parser: REVERT statement") {
    Lexer lex("REVERT ERROR;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* rev = std::get_if<RevertStmt>(&stmt->node);
    REQUIRE(rev != nullptr);
}

TEST_CASE("Parser: STOP statement") {
    Lexer lex("STOP;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    CHECK(std::holds_alternative<StopStmt>(stmt->node));
}

TEST_CASE("Parser: DISPLAY statement") {
    Lexer lex("DISPLAY('Hello');");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* disp = std::get_if<DisplayStmt>(&stmt->node);
    REQUIRE(disp != nullptr);
    CHECK(disp->items.size() == 1);
}

TEST_CASE("Parser: GOTO statement") {
    Lexer lex("GOTO EXIT_LABEL;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    auto* gt = std::get_if<GoToStmt>(&stmt->node);
    REQUIRE(gt != nullptr);
    CHECK(gt->label == "EXIT_LABEL");
}

TEST_CASE("Parser: LEAVE statement") {
    Lexer lex("LEAVE;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto stmt = parser.parse_statement();
    CHECK(std::holds_alternative<LeaveStmt>(stmt->node));
}

TEST_CASE("Parser: expression precedence: add/mul") {
    Lexer lex("A + B * C;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto expr = parser.parse_expression();
    // Should parse as A + (B * C)
    auto* binop = std::get_if<BinaryOpExpr>(&expr->node);
    REQUIRE(binop != nullptr);
    CHECK(binop->op == BinOp::ADD);
    auto* rhs = std::get_if<BinaryOpExpr>(&binop->right->node);
    REQUIRE(rhs != nullptr);
    CHECK(rhs->op == BinOp::MUL);
}

TEST_CASE("Parser: expression precedence: power") {
    Lexer lex("A ** B ** C;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto expr = parser.parse_expression();
    // Power is right-associative: A ** (B ** C)
    auto* binop = std::get_if<BinaryOpExpr>(&expr->node);
    REQUIRE(binop != nullptr);
    CHECK(binop->op == BinOp::POWER);
}

TEST_CASE("Parser: parenthesized expression") {
    Lexer lex("(A + B) * C;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto expr = parser.parse_expression();
    auto* binop = std::get_if<BinaryOpExpr>(&expr->node);
    REQUIRE(binop != nullptr);
    CHECK(binop->op == BinOp::MUL);
}

TEST_CASE("Parser: unary negation") {
    Lexer lex("-X;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto expr = parser.parse_expression();
    auto* unary = std::get_if<UnaryOpExpr>(&expr->node);
    REQUIRE(unary != nullptr);
    CHECK(unary->op == UnaryOp::NEGATE);
}

TEST_CASE("Parser: function call expression") {
    Lexer lex("SUBSTR(NAME, 1, 5);");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto expr = parser.parse_expression();
    auto* call = std::get_if<FunctionCallExpr>(&expr->node);
    REQUIRE(call != nullptr);
    CHECK(call->name == "SUBSTR");
    CHECK(call->args.size() == 3);
}

TEST_CASE("Parser: member access expression") {
    Lexer lex("EMPLOYEE.NAME;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto expr = parser.parse_expression();
    auto* mem = std::get_if<MemberAccessExpr>(&expr->node);
    REQUIRE(mem != nullptr);
    CHECK(mem->member == "NAME");
}

TEST_CASE("Parser: concatenation expression") {
    Lexer lex("A || B || C;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto expr = parser.parse_expression();
    auto* binop = std::get_if<BinaryOpExpr>(&expr->node);
    REQUIRE(binop != nullptr);
    CHECK(binop->op == BinOp::CONCAT);
}

TEST_CASE("Parser: comparison expression") {
    Lexer lex("X ^= Y;");
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto expr = parser.parse_expression();
    auto* binop = std::get_if<BinaryOpExpr>(&expr->node);
    REQUIRE(binop != nullptr);
    CHECK(binop->op == BinOp::NE);
}

// ============================================================
// SECTION 3: BUILTIN TESTS (45+)
// ============================================================

// -- String builtins --

TEST_CASE("Builtin: SUBSTR basic") {
    CHECK(SUBSTR("HELLO WORLD", 1, 5) == "HELLO");
}

TEST_CASE("Builtin: SUBSTR from middle") {
    CHECK(SUBSTR("HELLO WORLD", 7, 5) == "WORLD");
}

TEST_CASE("Builtin: SUBSTR to end") {
    CHECK(SUBSTR("ABCDEF", 4) == "DEF");
}

TEST_CASE("Builtin: SUBSTR out of range") {
    CHECK(SUBSTR("AB", 5) == "");
}

TEST_CASE("Builtin: INDEX found") {
    CHECK(INDEX("HELLO WORLD", "WORLD") == 7);
}

TEST_CASE("Builtin: INDEX not found") {
    CHECK(INDEX("HELLO", "WORLD") == 0);
}

TEST_CASE("Builtin: LENGTH") {
    CHECK(LENGTH("HELLO") == 5);
    CHECK(LENGTH("") == 0);
}

TEST_CASE("Builtin: TRANSLATE") {
    CHECK(TRANSLATE("HELLO", "helo", "HELO") == "hello");
}

TEST_CASE("Builtin: VERIFY all valid") {
    CHECK(VERIFY("12345", "0123456789") == 0);
}

TEST_CASE("Builtin: VERIFY invalid found") {
    CHECK(VERIFY("123A5", "0123456789") == 4);
}

TEST_CASE("Builtin: REPEAT") {
    CHECK(REPEAT("AB", 2) == "ABABAB");  // PL/I REPEAT gives n+1 copies
}

TEST_CASE("Builtin: REVERSE") {
    CHECK(REVERSE("HELLO") == "OLLEH");
}

TEST_CASE("Builtin: LEFT pad") {
    CHECK(LEFT("HI", 5) == "HI   ");
}

TEST_CASE("Builtin: LEFT truncate") {
    CHECK(LEFT("HELLO", 3) == "HEL");
}

TEST_CASE("Builtin: RIGHT pad") {
    CHECK(RIGHT("HI", 5) == "   HI");
}

TEST_CASE("Builtin: CENTER") {
    CHECK(CENTER("HI", 6) == "  HI  ");
}

TEST_CASE("Builtin: TRIM") {
    CHECK(TRIM("  HELLO  ") == "HELLO");
}

TEST_CASE("Builtin: COPY") {
    CHECK(COPY("AB", 3) == "ABABAB");
}

TEST_CASE("Builtin: HIGH") {
    auto h = HIGH(3);
    CHECK(h.size() == 3);
    CHECK(h[0] == '\xFF');
}

TEST_CASE("Builtin: LOW") {
    auto l = LOW(3);
    CHECK(l.size() == 3);
    CHECK(l[0] == '\x00');
}

TEST_CASE("Builtin: COLLATE length") {
    CHECK(COLLATE().size() == 256);
}

// -- Math builtins --

TEST_CASE("Builtin: ABS positive") {
    CHECK(ABS_fn(5.0) == 5.0);
}

TEST_CASE("Builtin: ABS negative") {
    CHECK(ABS_fn(-5.0) == 5.0);
}

TEST_CASE("Builtin: MOD") {
    CHECK(MOD(17, 5) == 2);
    CHECK(MOD(10, 3) == 1);
}

TEST_CASE("Builtin: CEIL") {
    CHECK(CEIL(3.2) == 4.0);
    CHECK(CEIL(-3.2) == -3.0);
}

TEST_CASE("Builtin: FLOOR") {
    CHECK(FLOOR(3.8) == 3.0);
    CHECK(FLOOR(-3.8) == -4.0);
}

TEST_CASE("Builtin: ROUND") {
    CHECK(ROUND(3.456, 2) == doctest::Approx(3.46));
    CHECK(ROUND(3.454, 2) == doctest::Approx(3.45));
}

TEST_CASE("Builtin: TRUNC") {
    CHECK(TRUNC(3.9) == 3.0);
    CHECK(TRUNC(-3.9) == -3.0);
}

TEST_CASE("Builtin: SIGN") {
    CHECK(SIGN(5.0) == 1);
    CHECK(SIGN(-5.0) == -1);
    CHECK(SIGN(0.0) == 0);
}

TEST_CASE("Builtin: MIN") {
    CHECK(MIN_fn(3, 1, 4, 1, 5) == 1);
}

TEST_CASE("Builtin: MAX") {
    CHECK(MAX_fn(3, 1, 4, 1, 5) == 5);
}

TEST_CASE("Builtin: SQRT") {
    CHECK(SQRT_fn(16.0) == doctest::Approx(4.0));
}

TEST_CASE("Builtin: LOG") {
    CHECK(LOG_fn(1.0) == doctest::Approx(0.0));
}

TEST_CASE("Builtin: LOG2") {
    CHECK(LOG2(8.0) == doctest::Approx(3.0));
}

TEST_CASE("Builtin: LOG10") {
    CHECK(LOG10(100.0) == doctest::Approx(2.0));
}

TEST_CASE("Builtin: EXP") {
    CHECK(EXP_fn(0.0) == doctest::Approx(1.0));
}

TEST_CASE("Builtin: SIN/COS/TAN") {
    CHECK(SIN_fn(0.0) == doctest::Approx(0.0));
    CHECK(COS_fn(0.0) == doctest::Approx(1.0));
    CHECK(TAN_fn(0.0) == doctest::Approx(0.0));
}

TEST_CASE("Builtin: ATAN") {
    CHECK(ATAN_fn(0.0) == doctest::Approx(0.0));
}

TEST_CASE("Builtin: ATAND") {
    CHECK(ATAND(1.0) == doctest::Approx(45.0));
}

TEST_CASE("Builtin: ASIN") {
    CHECK(ASIN_fn(0.0) == doctest::Approx(0.0));
}

TEST_CASE("Builtin: ACOS") {
    CHECK(ACOS_fn(1.0) == doctest::Approx(0.0));
}

// -- Conversion builtins --

TEST_CASE("Builtin: BINARY from bit string") {
    CHECK(BINARY("1010") == 10);
}

TEST_CASE("Builtin: BINARY from all ones") {
    CHECK(BINARY("11111111") == 255);
}

TEST_CASE("Builtin: FIXED from double") {
    CHECK(FIXED_fn(3.7) == 3);
}

TEST_CASE("Builtin: FLOAT from int") {
    CHECK(FLOAT_fn(42) == doctest::Approx(42.0));
}

TEST_CASE("Builtin: CHAR_conv from int") {
    CHECK(CHAR_conv(42) == "42");
}

TEST_CASE("Builtin: COMPLEX construction") {
    auto c = COMPLEX_fn(3.0, 4.0);
    CHECK(REAL(c) == doctest::Approx(3.0));
    CHECK(IMAG(c) == doctest::Approx(4.0));
}

TEST_CASE("Builtin: BOOL") {
    CHECK(BOOL_fn(1) == true);
    CHECK(BOOL_fn(0) == false);
}

TEST_CASE("Builtin: BOOL from string") {
    CHECK(BOOL_str("1") == true);
    CHECK(BOOL_str("") == false);
}

TEST_CASE("Builtin: BIT_conv") {
    auto bits = BIT_conv(5, 8);
    CHECK(bits == "00000101");
}

TEST_CASE("Builtin: UNSPEC") {
    auto bits = UNSPEC(1);
    CHECK(bits.size() == 64);
}

// -- Array builtins --

TEST_CASE("Builtin: SUM") {
    std::vector<double> arr = {1.0, 2.0, 3.0, 4.0, 5.0};
    CHECK(SUM(arr) == doctest::Approx(15.0));
}

TEST_CASE("Builtin: PROD") {
    std::vector<double> arr = {1.0, 2.0, 3.0, 4.0};
    CHECK(PROD(arr) == doctest::Approx(24.0));
}

TEST_CASE("Builtin: POLY") {
    // 1 + 2x + 3x^2 at x=2: 1 + 4 + 12 = 17
    std::vector<double> coeffs = {1.0, 2.0, 3.0};
    CHECK(POLY(coeffs, 2.0) == doctest::Approx(17.0));
}

TEST_CASE("Builtin: HBOUND") {
    std::vector<double> arr(10);
    CHECK(HBOUND(arr) == 10);
}

TEST_CASE("Builtin: DIM") {
    std::vector<double> arr(5);
    CHECK(DIM_fn(arr) == 5);
}

TEST_CASE("Builtin: ALLOCATION") {
    CHECK(ALLOCATION(true) == 1);
    CHECK(ALLOCATION(false) == 0);
}

// -- Storage builtins --

TEST_CASE("Builtin: ADDR/NULL") {
    int x = 42;
    CHECK(ADDR(&x) != 0);
    CHECK(NULL_ptr() == nullptr);
    CHECK(SYSNULL() == nullptr);
}

// -- Date/Time builtins --

TEST_CASE("Builtin: DATE format") {
    auto d = DATE();
    CHECK(d.size() == 6);
}

TEST_CASE("Builtin: TIME format") {
    auto t = TIME();
    CHECK(t.size() == 9);
}

TEST_CASE("Builtin: DATETIME format") {
    auto dt = DATETIME();
    CHECK(dt.size() >= 16);
}

TEST_CASE("Builtin: DAYS and DAYSTODATE roundtrip") {
    std::string date = "20260327";
    auto days = DAYS(date);
    CHECK(days > 0);
    auto back = DAYSTODATE(days);
    CHECK(back == date);
}

TEST_CASE("Builtin: SECS and SECSTOTIME") {
    CHECK(SECS("120000") == 43200);
    CHECK(SECSTOTIME(43200) == "120000");
}

TEST_CASE("Builtin: SECSTOTIME midnight") {
    CHECK(SECSTOTIME(0) == "000000");
}

// -- Condition query builtins --

TEST_CASE("Builtin: ONCODE default") {
    CHECK(ONCODE() == 0);
}

TEST_CASE("Builtin: is_builtin lookup") {
    CHECK(is_builtin("SUBSTR") == true);
    CHECK(is_builtin("INDEX") == true);
    CHECK(is_builtin("ABS") == true);
    CHECK(is_builtin("NOTABUILTIN") == false);
}

TEST_CASE("Builtin: lookup_builtin kinds") {
    CHECK(lookup_builtin("SUBSTR") == BuiltinKind::SUBSTR);
    CHECK(lookup_builtin("SUM") == BuiltinKind::SUM);
    CHECK(lookup_builtin("DAYS") == BuiltinKind::DAYS);
    CHECK(lookup_builtin("xyz") == BuiltinKind::UNKNOWN);
}

// ============================================================
// SECTION 4: RUNTIME TESTS (30+)
// ============================================================

TEST_CASE("Runtime: PliError condition") {
    CHECK_THROWS_AS(signal_condition("ERROR"), PliError);
}

TEST_CASE("Runtime: PliEndFile condition") {
    CHECK_THROWS_AS(signal_condition("ENDFILE"), PliEndFile);
}

TEST_CASE("Runtime: PliEndPage condition") {
    CHECK_THROWS_AS(signal_condition("ENDPAGE"), PliEndPage);
}

TEST_CASE("Runtime: PliKey condition") {
    CHECK_THROWS_AS(signal_condition("KEY"), PliKey);
}

TEST_CASE("Runtime: PliConversion condition") {
    CHECK_THROWS_AS(signal_condition("CONVERSION"), PliConversion);
}

TEST_CASE("Runtime: PliFixedOverflow condition") {
    CHECK_THROWS_AS(signal_condition("FIXEDOVERFLOW"), PliFixedOverflow);
}

TEST_CASE("Runtime: PliOverflow condition") {
    CHECK_THROWS_AS(signal_condition("OVERFLOW"), PliOverflow);
}

TEST_CASE("Runtime: PliUnderflow condition") {
    CHECK_THROWS_AS(signal_condition("UNDERFLOW"), PliUnderflow);
}

TEST_CASE("Runtime: PliZeroDivide condition") {
    CHECK_THROWS_AS(signal_condition("ZERODIVIDE"), PliZeroDivide);
}

TEST_CASE("Runtime: PliSubscriptRange condition") {
    CHECK_THROWS_AS(signal_condition("SUBSCRIPTRANGE"), PliSubscriptRange);
}

TEST_CASE("Runtime: PliStringRange condition") {
    CHECK_THROWS_AS(signal_condition("STRINGRANGE"), PliStringRange);
}

TEST_CASE("Runtime: PliSize condition") {
    CHECK_THROWS_AS(signal_condition("SIZE"), PliSize);
}

TEST_CASE("Runtime: PliArea condition") {
    CHECK_THROWS_AS(signal_condition("AREA"), PliArea);
}

TEST_CASE("Runtime: PliStorage condition") {
    CHECK_THROWS_AS(signal_condition("STORAGE"), PliStorage);
}

TEST_CASE("Runtime: PliUndefinedFile condition") {
    CHECK_THROWS_AS(signal_condition("UNDEFINEDFILE"), PliUndefinedFile);
}

TEST_CASE("Runtime: PliName condition") {
    CHECK_THROWS_AS(signal_condition("NAME"), PliName);
}

TEST_CASE("Runtime: PliRecord condition") {
    CHECK_THROWS_AS(signal_condition("RECORD"), PliRecord);
}

TEST_CASE("Runtime: handler stack push/pop") {
    handler_stack().clear();
    bool handled = false;
    push_handler("ERROR", [&](const PliCondition&) { handled = true; });
    CHECK(handler_stack().size() == 1);
    PliError err;
    dispatch_condition(err);
    CHECK(handled == true);
    pop_handler("ERROR");
    CHECK(handler_stack().empty());
}

TEST_CASE("Runtime: handler stack dispatch miss") {
    handler_stack().clear();
    PliEndFile eof;
    bool dispatched = dispatch_condition(eof);
    CHECK(dispatched == false);
}

TEST_CASE("Runtime: nested handlers") {
    handler_stack().clear();
    int which = 0;
    push_handler("ERROR", [&](const PliCondition&) { which = 1; });
    push_handler("ERROR", [&](const PliCondition&) { which = 2; });
    PliError err;
    dispatch_condition(err);
    CHECK(which == 2);  // Most recent handler wins
    pop_handler("ERROR");
    dispatch_condition(err);
    CHECK(which == 1);
    pop_handler("ERROR");
}

TEST_CASE("Runtime: format A") {
    CHECK(FormatEngine::format_A("HELLO", 10) == "HELLO     ");
    CHECK(FormatEngine::format_A("HELLO WORLD", 5) == "HELLO");
    CHECK(FormatEngine::format_A("HI", 0) == "HI");
}

TEST_CASE("Runtime: format F") {
    CHECK(FormatEngine::format_F(3.14, 8, 2) == "    3.14");
    CHECK(FormatEngine::format_F(42.0, 5, 0) == "   42");
}

TEST_CASE("Runtime: format E") {
    auto result = FormatEngine::format_E(1.23e5, 15, 2);
    CHECK(result.find('e') != std::string::npos);
}

TEST_CASE("Runtime: format B") {
    CHECK(FormatEngine::format_B(5, 8) == "00000101");
}

TEST_CASE("Runtime: format X") {
    CHECK(FormatEngine::format_X(5) == "     ");
}

TEST_CASE("Runtime: format SKIP") {
    CHECK(FormatEngine::format_SKIP(2) == "\n\n");
}

TEST_CASE("Runtime: format COLUMN") {
    CHECK(FormatEngine::format_COLUMN(10, 3) == "       ");
}

TEST_CASE("Runtime: PliFile stream output") {
    PliFile f;
    f.name = "SYSPRINT";
    f.open(FileMode::STREAM_OUTPUT);
    f.put_chars("Hello");
    f.put_chars(" World");
    CHECK(f.stream_buffer == "Hello World");
    CHECK(f.is_open == true);
    f.close();
    CHECK(f.is_open == false);
}

TEST_CASE("Runtime: PliFile stream input") {
    PliFile f;
    f.name = "SYSIN";
    f.open(FileMode::STREAM_INPUT);
    f.stream_buffer = "42 Hello 3.14";
    CHECK(f.get_list_item() == "42");
    CHECK(f.get_list_item() == "Hello");
    CHECK(f.get_list_item() == "3.14");
}

TEST_CASE("Runtime: PliFile end of file") {
    PliFile f;
    f.name = "TEST";
    f.open(FileMode::STREAM_INPUT);
    f.stream_buffer = "";
    CHECK_THROWS_AS(f.get_list_item(), PliEndFile);
}

TEST_CASE("Runtime: PliFile record write/read") {
    PliFile f;
    f.name = "RECFILE";
    f.open(FileMode::RECORD_OUTPUT);
    f.write_record("RECORD1");
    f.write_record("RECORD2");
    f.write_record("RECORD3");
    CHECK(f.records.size() == 3);

    f.open(FileMode::RECORD_INPUT);
    CHECK(f.read_record() == "RECORD1");
    CHECK(f.read_record() == "RECORD2");
    CHECK(f.read_record() == "RECORD3");
    CHECK_THROWS_AS(f.read_record(), PliEndFile);
}

TEST_CASE("Runtime: PliFile keyed write/read") {
    PliFile f;
    f.name = "KEYFILE";
    f.open(FileMode::RECORD_UPDATE);
    f.write_record_keyed("K1", "DATA1");
    f.write_record_keyed("K2", "DATA2");
    CHECK(f.read_record_keyed("K1") == "DATA1");
    CHECK(f.read_record_keyed("K2") == "DATA2");
    CHECK_THROWS_AS(f.read_record_keyed("K3"), PliKey);
}

TEST_CASE("Runtime: PliFile rewrite") {
    PliFile f;
    f.open(FileMode::RECORD_UPDATE);
    f.write_record("OLD");
    f.record_pos = 1;
    f.rewrite_record("NEW");
    f.record_pos = 0;
    CHECK(f.read_record() == "NEW");
}

TEST_CASE("Runtime: PliFile delete keyed") {
    PliFile f;
    f.open(FileMode::RECORD_UPDATE);
    f.write_record_keyed("DEL", "DATA");
    f.delete_record_keyed("DEL");
    CHECK_THROWS_AS(f.read_record_keyed("DEL"), PliKey);
}

TEST_CASE("Runtime: StringStream") {
    StringStream ss;
    ss.put("42 Hello 3.14");
    CHECK(ss.get_list_item() == "42");
    CHECK(ss.get_list_item() == "Hello");
    CHECK(ss.get_list_item() == "3.14");
    CHECK(ss.str() == "42 Hello 3.14");
}

TEST_CASE("Runtime: safe_divide double") {
    CHECK(safe_divide(10.0, 3.0) == doctest::Approx(3.333).epsilon(0.01));
    CHECK_THROWS_AS(safe_divide(1.0, 0.0), PliZeroDivide);
}

TEST_CASE("Runtime: safe_divide integer") {
    CHECK(safe_divide((int64_t)10, (int64_t)3) == 3);
    CHECK_THROWS_AS(safe_divide((int64_t)1, (int64_t)0), PliZeroDivide);
}

TEST_CASE("Runtime: check_subscript") {
    CHECK_NOTHROW(check_subscript(5, 1, 10));
    CHECK_THROWS_AS(check_subscript(0, 1, 10), PliSubscriptRange);
    CHECK_THROWS_AS(check_subscript(11, 1, 10), PliSubscriptRange);
}

TEST_CASE("Runtime: check_string_range") {
    CHECK_NOTHROW(check_string_range(1, 5, 10));
    CHECK_THROWS_AS(check_string_range(0, 1, 10), PliStringRange);
    CHECK_THROWS_AS(check_string_range(8, 5, 10), PliStringRange);
}

TEST_CASE("Runtime: global file table") {
    auto& f = get_file("TESTFILE");
    CHECK(f.name == "TESTFILE");
    auto& f2 = get_file("TESTFILE");
    CHECK(&f == &f2);  // Same file object
}

// ============================================================
// SECTION 5: TRANSLATOR TESTS (25+)
// ============================================================

TEST_CASE("Translator: type mapping FIXED BIN") {
    DataAttributes attrs;
    attrs.base_type = BaseType::FIXED;
    attrs.scale = Scale::BINARY;
    attrs.precision = 31;
    CHECK(Translator::map_type(attrs) == "int32_t");
}

TEST_CASE("Translator: type mapping FIXED BIN(15)") {
    DataAttributes attrs;
    attrs.base_type = BaseType::FIXED;
    attrs.scale = Scale::BINARY;
    attrs.precision = 15;
    CHECK(Translator::map_type(attrs) == "int16_t");
}

TEST_CASE("Translator: type mapping FIXED DEC") {
    DataAttributes attrs;
    attrs.base_type = BaseType::FIXED;
    attrs.scale = Scale::DECIMAL;
    CHECK(Translator::map_type(attrs) == "int64_t");
}

TEST_CASE("Translator: type mapping FLOAT") {
    DataAttributes attrs;
    attrs.base_type = BaseType::FLOAT;
    CHECK(Translator::map_type(attrs) == "double");
}

TEST_CASE("Translator: type mapping FLOAT single precision") {
    DataAttributes attrs;
    attrs.base_type = BaseType::FLOAT;
    attrs.precision = 6;
    CHECK(Translator::map_type(attrs) == "float");
}

TEST_CASE("Translator: type mapping CHAR VARYING") {
    DataAttributes attrs;
    attrs.base_type = BaseType::CHAR;
    attrs.varying = true;
    CHECK(Translator::map_type(attrs) == "std::string");
}

TEST_CASE("Translator: type mapping BIT") {
    DataAttributes attrs;
    attrs.base_type = BaseType::BIT;
    attrs.bit_length = 8;
    CHECK(Translator::map_type(attrs) == "std::bitset<8>");
}

TEST_CASE("Translator: type mapping POINTER") {
    DataAttributes attrs;
    attrs.base_type = BaseType::POINTER;
    CHECK(Translator::map_type(attrs) == "void*");
}

TEST_CASE("Translator: type mapping ENTRY") {
    DataAttributes attrs;
    attrs.base_type = BaseType::ENTRY;
    CHECK(Translator::map_type(attrs) == "std::function<void()>");
}

TEST_CASE("Translator: type mapping COMPLEX") {
    DataAttributes attrs;
    attrs.base_type = BaseType::COMPLEX;
    CHECK(Translator::map_type(attrs) == "std::complex<double>");
}

TEST_CASE("Translator: type mapping AREA") {
    DataAttributes attrs;
    attrs.base_type = BaseType::AREA;
    CHECK(Translator::map_type(attrs) == "std::vector<uint8_t>");
}

TEST_CASE("Translator: translate assignment") {
    Translator trans;
    auto target = make_ident("x");
    auto value = make_literal(LiteralExpr::INTEGER, "42");
    auto stmt = make_stmt(AssignmentStmt{target, value});
    auto code = trans.translate_stmt(stmt);
    CHECK(code.find("x = 42") != std::string::npos);
}

TEST_CASE("Translator: translate IF/THEN") {
    Translator trans;
    IfStmt ifs;
    ifs.condition = make_binop(BinOp::GT, make_ident("x"), make_literal(LiteralExpr::INTEGER, "0"));
    ifs.then_body.push_back(make_stmt(AssignmentStmt{make_ident("y"), make_literal(LiteralExpr::INTEGER, "1")}));
    auto code = trans.translate_stmt(make_stmt(std::move(ifs)));
    CHECK(code.find("if") != std::string::npos);
    CHECK(code.find("y = 1") != std::string::npos);
}

TEST_CASE("Translator: translate DO WHILE") {
    Translator trans;
    DoLoopStmt dl;
    dl.kind = DoLoopStmt::WHILE;
    dl.while_cond = make_binop(BinOp::GT, make_ident("n"), make_literal(LiteralExpr::INTEGER, "0"));
    dl.body.push_back(make_stmt(AssignmentStmt{make_ident("n"), make_binop(BinOp::SUB, make_ident("n"), make_literal(LiteralExpr::INTEGER, "1"))}));
    auto code = trans.translate_stmt(make_stmt(std::move(dl)));
    CHECK(code.find("while") != std::string::npos);
}

TEST_CASE("Translator: translate DO UNTIL") {
    Translator trans;
    DoLoopStmt dl;
    dl.kind = DoLoopStmt::UNTIL;
    dl.until_cond = make_ident("done");
    auto code = trans.translate_stmt(make_stmt(std::move(dl)));
    CHECK(code.find("do {") != std::string::npos);
    CHECK(code.find("while") != std::string::npos);
}

TEST_CASE("Translator: translate FOREVER loop") {
    Translator trans;
    DoLoopStmt dl;
    dl.kind = DoLoopStmt::FOREVER;
    dl.body.push_back(make_stmt(LeaveStmt{""}));
    auto code = trans.translate_stmt(make_stmt(std::move(dl)));
    CHECK(code.find("for (;;)") != std::string::npos);
    CHECK(code.find("break") != std::string::npos);
}

TEST_CASE("Translator: translate iterative DO") {
    Translator trans;
    DoLoopStmt dl;
    dl.kind = DoLoopStmt::ITERATIVE;
    dl.variable = "I";
    dl.from = make_literal(LiteralExpr::INTEGER, "1");
    dl.to = make_literal(LiteralExpr::INTEGER, "10");
    dl.by = make_literal(LiteralExpr::INTEGER, "1");
    auto code = trans.translate_stmt(make_stmt(std::move(dl)));
    CHECK(code.find("for") != std::string::npos);
    CHECK(code.find("I") != std::string::npos);
}

TEST_CASE("Translator: translate CALL") {
    Translator trans;
    CallStmt cs;
    cs.target = "MYPROC";
    cs.args.push_back(make_ident("a"));
    cs.args.push_back(make_ident("b"));
    auto code = trans.translate_stmt(make_stmt(std::move(cs)));
    CHECK(code.find("MYPROC(a, b)") != std::string::npos);
}

TEST_CASE("Translator: translate RETURN") {
    Translator trans;
    ReturnStmt rs;
    rs.value = make_literal(LiteralExpr::INTEGER, "0");
    auto code = trans.translate_stmt(make_stmt(std::move(rs)));
    CHECK(code.find("return 0") != std::string::npos);
}

TEST_CASE("Translator: translate PUT LIST") {
    Translator trans;
    PutListStmt ps;
    ps.items.push_back(make_literal(LiteralExpr::STRING, "Hello"));
    auto code = trans.translate_stmt(make_stmt(std::move(ps)));
    CHECK(code.find("std::cout") != std::string::npos);
}

TEST_CASE("Translator: translate LEAVE as break") {
    Translator trans;
    auto code = trans.translate_stmt(make_stmt(LeaveStmt{""}));
    CHECK(code.find("break") != std::string::npos);
}

TEST_CASE("Translator: translate ITERATE as continue") {
    Translator trans;
    auto code = trans.translate_stmt(make_stmt(IterateStmt{""}));
    CHECK(code.find("continue") != std::string::npos);
}

TEST_CASE("Translator: translate GOTO") {
    Translator trans;
    GoToStmt gs;
    gs.label = "EXIT";
    auto code = trans.translate_stmt(make_stmt(std::move(gs)));
    CHECK(code.find("goto EXIT") != std::string::npos);
}

TEST_CASE("Translator: translate SIGNAL as throw") {
    Translator trans;
    SignalStmt ss;
    ss.condition = "ERROR";
    auto code = trans.translate_stmt(make_stmt(std::move(ss)));
    CHECK(code.find("throw") != std::string::npos);
}

TEST_CASE("Translator: translate expression binary ops") {
    Translator trans;
    auto expr = make_binop(BinOp::ADD, make_ident("a"), make_ident("b"));
    auto code = trans.translate_expr(expr);
    CHECK(code.find("a") != std::string::npos);
    CHECK(code.find("+") != std::string::npos);
    CHECK(code.find("b") != std::string::npos);
}

TEST_CASE("Translator: translate string literal") {
    Translator trans;
    auto expr = make_literal(LiteralExpr::STRING, "Hello");
    auto code = trans.translate_expr(expr);
    CHECK(code.find("\"Hello\"") != std::string::npos);
}

TEST_CASE("Translator: translate full program") {
    Program prog;
    prog.name = "HELLO";
    prog.options_main = true;
    PutListStmt ps;
    ps.items.push_back(make_literal(LiteralExpr::STRING, "Hello World"));
    prog.statements.push_back(make_stmt(std::move(ps)));

    Translator trans;
    auto code = trans.translate(prog);
    CHECK(code.find("int main()") != std::string::npos);
    CHECK(code.find("return 0") != std::string::npos);
    CHECK(code.find("std::cout") != std::string::npos);
}

TEST_CASE("Translator: preprocessor %INCLUDE") {
    CHECK(translate_preprocessor("%INCLUDE", "member") == "#include \"member\"");
}

TEST_CASE("Translator: preprocessor %IF") {
    CHECK(translate_preprocessor("%IF", "DEBUG") == "#if DEBUG");
}

TEST_CASE("Translator: preprocessor %ELSE") {
    CHECK(translate_preprocessor("%ELSE", "") == "#else");
}

// ============================================================
// SECTION 6: PREPROCESSOR TESTS (15+)
// ============================================================

TEST_CASE("Preprocessor: define and retrieve constant") {
    Preprocessor pp;
    pp.define("VERSION", "1");
    CHECK(pp.get_constant("VERSION") == "1");
    CHECK(pp.is_defined("VERSION") == true);
    CHECK(pp.is_defined("NOTDEFINED") == false);
}

TEST_CASE("Preprocessor: case insensitive defines") {
    Preprocessor pp;
    pp.define("debug", "1");
    CHECK(pp.get_constant("DEBUG") == "1");
    CHECK(pp.get_constant("debug") == "1");
}

TEST_CASE("Preprocessor: activate/deactivate") {
    Preprocessor pp;
    pp.activate("FEATURE_X");
    CHECK(pp.is_active("FEATURE_X") == true);
    pp.deactivate("FEATURE_X");
    CHECK(pp.is_active("FEATURE_X") == false);
}

TEST_CASE("Preprocessor: evaluate condition equality") {
    Preprocessor pp;
    pp.define("MODE", "DEBUG");
    CHECK(pp.evaluate_condition("MODE = DEBUG") == true);
    CHECK(pp.evaluate_condition("MODE = RELEASE") == false);
}

TEST_CASE("Preprocessor: evaluate condition truth") {
    Preprocessor pp;
    pp.define("ENABLED", "1");
    CHECK(pp.evaluate_condition("ENABLED") == true);
    CHECK(pp.evaluate_condition("DISABLED") == false);
}

TEST_CASE("Preprocessor: %REPLACE processing") {
    Preprocessor pp;
    auto result = pp.process("%REPLACE MAX_SIZE BY 100;");
    CHECK(pp.get_constant("MAX_SIZE") == "100");
}

TEST_CASE("Preprocessor: macro substitution in output") {
    Preprocessor pp;
    pp.define("LIMIT", "256");
    auto result = pp.process("DCL X FIXED BIN INIT(LIMIT);");
    CHECK(result.find("256") != std::string::npos);
}

TEST_CASE("Preprocessor: %INCLUDE with resolver") {
    Preprocessor pp;
    pp.set_include_resolver([](const std::string& name) -> std::string {
        if (name == "DEFS") return "DCL MAX_SIZE FIXED BIN;";
        return "";
    });
    auto result = pp.process("%INCLUDE DEFS;");
    CHECK(result.find("DCL") != std::string::npos);
}

TEST_CASE("Preprocessor: %DECLARE processing") {
    Preprocessor pp;
    auto result = pp.process("%DECLARE FLAG FIXED;");
    CHECK(pp.is_defined("FLAG") == true);
}

TEST_CASE("Preprocessor: %ACTIVATE processing") {
    Preprocessor pp;
    pp.process("%ACTIVATE TESTING;");
    CHECK(pp.is_active("TESTING") == true);
}

TEST_CASE("Preprocessor: %DEACTIVATE processing") {
    Preprocessor pp;
    pp.activate("TESTING");
    pp.process("%DEACTIVATE TESTING;");
    CHECK(pp.is_active("TESTING") == false);
}

TEST_CASE("Preprocessor: %DO/%END block") {
    Preprocessor pp;
    auto result = pp.process("%DO; X = 1; %END;");
    CHECK(result.find("X") != std::string::npos);
}

TEST_CASE("Preprocessor: nested %INCLUDE") {
    Preprocessor pp;
    pp.set_include_resolver([](const std::string& name) -> std::string {
        if (name == "OUTER") return "%INCLUDE INNER;";
        if (name == "INNER") return "DCL Y FLOAT;";
        return "";
    });
    auto result = pp.process("%INCLUDE OUTER;");
    CHECK(result.find("Y") != std::string::npos);
}

TEST_CASE("Preprocessor: constants() accessor") {
    Preprocessor pp;
    pp.define("A", "1");
    pp.define("B", "2");
    CHECK(pp.constants().size() >= 2);
}

TEST_CASE("Preprocessor: multiple defines") {
    Preprocessor pp;
    pp.define("X", "10");
    pp.define("Y", "20");
    pp.define("Z", "30");
    CHECK(pp.get_constant("X") == "10");
    CHECK(pp.get_constant("Y") == "20");
    CHECK(pp.get_constant("Z") == "30");
}

// ============================================================
// SECTION 7: INTEGRATION TESTS (10+)
// ============================================================

TEST_CASE("Integration: lex + parse simple program") {
    std::string src = R"(
        HELLO: PROCEDURE OPTIONS(MAIN);
            DCL X FIXED BIN;
            X = 42;
            PUT LIST(X);
        END HELLO;
    )";
    Lexer lex(src);
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse_program();
    CHECK(prog.name == "HELLO");
    CHECK(prog.options_main == true);
    CHECK(prog.declarations.size() == 1);
    CHECK(prog.statements.size() == 2);
}

TEST_CASE("Integration: lex + parse + translate hello world") {
    std::string src = R"(
        MAIN: PROCEDURE OPTIONS(MAIN);
            PUT LIST('Hello, World!');
        END MAIN;
    )";
    Lexer lex(src);
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse_program();
    Translator trans;
    auto code = trans.translate(prog);
    CHECK(code.find("int main()") != std::string::npos);
    CHECK(code.find("Hello, World!") != std::string::npos);
}

TEST_CASE("Integration: lex + parse declarations") {
    std::string src = R"(
        TEST: PROCEDURE OPTIONS(MAIN);
            DCL NAME CHAR(20) VARYING;
            DCL AGE FIXED BIN;
            DCL SALARY FLOAT DEC;
            DCL FLAGS BIT(8);
        END TEST;
    )";
    Lexer lex(src);
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse_program();
    CHECK(prog.declarations.size() == 4);
}

TEST_CASE("Integration: lex + parse control flow") {
    std::string src = R"(
        FLOW: PROCEDURE OPTIONS(MAIN);
            DCL I FIXED BIN;
            DO I = 1 TO 10;
                IF I > 5 THEN PUT LIST(I);
            END;
        END FLOW;
    )";
    Lexer lex(src);
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse_program();
    CHECK(prog.declarations.size() == 1);
    CHECK(prog.statements.size() == 1);
}

TEST_CASE("Integration: builtin computation chain") {
    auto s = SUBSTR("HELLO WORLD", 1, 5);
    CHECK(s == "HELLO");
    auto r = REVERSE(s);
    CHECK(r == "OLLEH");
    CHECK(LENGTH(r) == 5);
    CHECK(INDEX(r, "LL") == 2);
}

TEST_CASE("Integration: runtime file operations chain") {
    PliFile f;
    f.name = "INTEG";
    f.open(FileMode::STREAM_OUTPUT);
    f.put_chars("10 20 30");
    f.close();

    f.open(FileMode::STREAM_INPUT);
    f.read_pos = 0;
    CHECK(f.get_list_item() == "10");
    CHECK(f.get_list_item() == "20");
    CHECK(f.get_list_item() == "30");
}

TEST_CASE("Integration: preprocessor + lexer pipeline") {
    Preprocessor pp;
    pp.define("LIMIT", "100");
    std::string src = "DCL X FIXED BIN INIT(LIMIT);";
    auto processed = pp.process(src);
    CHECK(processed.find("100") != std::string::npos);
}

TEST_CASE("Integration: math expression evaluation") {
    CHECK(SQRT_fn(ABS_fn(-16.0)) == doctest::Approx(4.0));
    CHECK(ROUND(LOG10(1000.0), 0) == doctest::Approx(3.0));
}

TEST_CASE("Integration: condition handling chain") {
    handler_stack().clear();
    bool caught = false;
    push_handler("ZERODIVIDE", [&](const PliCondition&) { caught = true; });
    try {
        safe_divide(1.0, 0.0);
    } catch (const PliZeroDivide& e) {
        dispatch_condition(e);
    }
    CHECK(caught == true);
    pop_handler("ZERODIVIDE");
}

TEST_CASE("Integration: full translate pipeline with structures") {
    std::string src = R"(
        STRUCTS: PROCEDURE OPTIONS(MAIN);
            DCL 1 PERSON,
                2 NAME CHAR(30),
                2 AGE FIXED BIN;
        END STRUCTS;
    )";
    Lexer lex(src);
    auto tokens = lex.tokenize();
    Parser parser(tokens);
    auto prog = parser.parse_program();
    Translator trans;
    auto code = trans.translate(prog);
    CHECK(code.find("struct") != std::string::npos);
    CHECK(code.find("NAME") != std::string::npos);
    CHECK(code.find("AGE") != std::string::npos);
}

TEST_CASE("Integration: date arithmetic") {
    auto days1 = DAYS("20260101");
    auto days2 = DAYS("20260201");
    CHECK(days2 - days1 == 31);
}

TEST_CASE("Integration: string manipulation chain") {
    auto s = LEFT("Hello", 10);
    CHECK(LENGTH(s) == 10);
    s = TRIM(s);
    CHECK(s == "Hello");
    s = CENTER(s, 11);
    CHECK(LENGTH(s) == 11);
}

TEST_CASE("Integration: array builtin chain") {
    std::vector<double> arr = {2.0, 3.0, 5.0, 7.0, 11.0};
    CHECK(SUM(arr) == doctest::Approx(28.0));
    CHECK(PROD(arr) == doctest::Approx(2310.0));
    CHECK(HBOUND(arr) == 5);
    CHECK(DIM_fn(arr) == 5);
}
