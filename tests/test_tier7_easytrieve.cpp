#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

#include "include/lazarus/easytrieve/ast.h"
#include "include/lazarus/easytrieve/lexer.h"
#include "include/lazarus/easytrieve/parser.h"
#include "include/lazarus/easytrieve/translator.h"
#include "include/lazarus/easytrieve/runtime.h"

#include <sstream>
#include <fstream>
#include <cstdio>

namespace ez = lazarus::easytrieve;
namespace rt = lazarus::easytrieve::runtime;

using ez::Lexer;
using ez::Parser;
using ez::Translator;
using ez::MacroProcessor;
using ez::Token;
using TT = ez::TokenType;
using ez::Program;
using ez::FileDecl;
using ez::RecordDecl;
using ez::FieldDecl;
using ez::WorkingField;
using ez::TableDecl;
using ez::JobActivity;
using ez::SortActivity;
using ez::ReportActivity;
using ez::ProcDecl;
using ez::FieldType;
using ez::Expression;
using ez::Condition;
using ez::Statement;
using ez::IfStmt;
using ez::DoStmt;
using ez::PerformStmt;
using ez::GoToStmt;
using ez::GetStmt;
using ez::PutStmt;
using ez::PointStmt;
using ez::ReadStmt;
using ez::WriteStmt;
using ez::DisplayStmt;
using ez::MoveStmt;
using ez::SearchStmt;
using ez::StopStmt;
using ez::PrintStmt;
using ez::CompareOp;
using ez::ArithOp;
using ez::Compare;
using ez::make_field_ref;
using ez::make_literal;
using ez::make_arithmetic;
using ez::make_compare;
using ez::make_and;
using ez::make_or;
using ez::make_not;
using ez::make_range;
using ez::make_mask;
using ez::make_stmt;
using ez::field_type_from_char;
using ez::field_type_name;
using ez::compare_op_name;
using ez::compare_op_cpp;
using ez::arith_op_symbol;
using ez::ConditionAnd;
using ez::ConditionOr;
using ez::ConditionNot;
using ez::ConditionVariant;

using rt::convert::alpha_to_int;
using rt::EzRecord;
using rt::EzFile;
using rt::TableLookup;
using rt::ReportEngine;

namespace convert = rt::convert;

// Helper: lex and return clean tokens
static std::vector<Token> lex_clean(const std::string& src) {
    Lexer lexer(src);
    return lexer.tokenize_clean();
}

// Helper: lex all tokens (with comments/newlines)
static std::vector<Token> lex_all(const std::string& src) {
    Lexer lexer(src);
    return lexer.tokenize();
}

// Helper: find token of specific type
static bool has_token_type(const std::vector<Token>& tokens, TT tt) {
    for (const auto& t : tokens) {
        if (t.type == tt) return true;
    }
    return false;
}

// ============================================================
// LEXER TESTS (20+)
// ============================================================

TEST_CASE("Lexer: FILE keyword") {
    auto tokens = lex_clean("FILE EMPLOYEES");
    REQUIRE(tokens.size() >= 3);
    CHECK(tokens[0].type == TT::KW_FILE);
    CHECK(tokens[1].type == TT::IDENTIFIER);
    CHECK(tokens[1].value == "EMPLOYEES");
}

TEST_CASE("Lexer: JOB keyword") {
    auto tokens = lex_clean("JOB INPUT PAYFILE");
    CHECK(tokens[0].type == TT::KW_JOB);
}

TEST_CASE("Lexer: SORT keyword") {
    auto tokens = lex_clean("SORT INFILE");
    CHECK(tokens[0].type == TT::KW_SORT);
}

TEST_CASE("Lexer: REPORT keyword") {
    auto tokens = lex_clean("REPORT EMPRPT");
    CHECK(tokens[0].type == TT::KW_REPORT);
}

TEST_CASE("Lexer: control flow keywords") {
    auto tokens = lex_clean("IF ELSE END-IF DO END-DO PERFORM STOP");
    CHECK(tokens[0].type == TT::KW_IF);
    CHECK(tokens[1].type == TT::KW_ELSE);
    CHECK(tokens[2].type == TT::KW_END_IF);
    CHECK(tokens[3].type == TT::KW_DO);
    CHECK(tokens[4].type == TT::KW_END_DO);
    CHECK(tokens[5].type == TT::KW_PERFORM);
    CHECK(tokens[6].type == TT::KW_STOP);
}

TEST_CASE("Lexer: I/O keywords") {
    auto tokens = lex_clean("GET PUT POINT READ WRITE DISPLAY MOVE SEARCH");
    CHECK(tokens[0].type == TT::KW_GET);
    CHECK(tokens[1].type == TT::KW_PUT);
    CHECK(tokens[2].type == TT::KW_POINT);
    CHECK(tokens[3].type == TT::KW_READ);
    CHECK(tokens[4].type == TT::KW_WRITE);
    CHECK(tokens[5].type == TT::KW_DISPLAY);
    CHECK(tokens[6].type == TT::KW_MOVE);
    CHECK(tokens[7].type == TT::KW_SEARCH);
}

TEST_CASE("Lexer: report elements") {
    auto tokens = lex_clean("SEQUENCE CONTROL LINE HEADING SUM TITLE PRINT");
    CHECK(tokens[0].type == TT::KW_SEQUENCE);
    CHECK(tokens[1].type == TT::KW_CONTROL);
    CHECK(tokens[2].type == TT::KW_LINE);
    CHECK(tokens[3].type == TT::KW_HEADING);
    CHECK(tokens[4].type == TT::KW_SUM);
    CHECK(tokens[5].type == TT::KW_TITLE);
    CHECK(tokens[6].type == TT::KW_PRINT);
}

TEST_CASE("Lexer: field type characters") {
    auto tokens = lex_clean("A N P B U");
    CHECK(tokens[0].type == TT::KW_TYPE_A);
    CHECK(tokens[1].type == TT::KW_TYPE_N);
    CHECK(tokens[2].type == TT::KW_TYPE_P);
    CHECK(tokens[3].type == TT::KW_TYPE_B);
    CHECK(tokens[4].type == TT::KW_TYPE_U);
}

TEST_CASE("Lexer: comparison operators") {
    auto tokens = lex_clean("EQ NE LT LE GT GE");
    CHECK(tokens[0].type == TT::KW_EQ);
    CHECK(tokens[1].type == TT::KW_NE);
    CHECK(tokens[2].type == TT::KW_LT);
    CHECK(tokens[3].type == TT::KW_LE);
    CHECK(tokens[4].type == TT::KW_GT);
    CHECK(tokens[5].type == TT::KW_GE);
}

TEST_CASE("Lexer: logical operators") {
    auto tokens = lex_clean("AND OR NOT");
    CHECK(tokens[0].type == TT::KW_AND);
    CHECK(tokens[1].type == TT::KW_OR);
    CHECK(tokens[2].type == TT::KW_NOT);
}

TEST_CASE("Lexer: string literal") {
    auto tokens = lex_clean("'HELLO WORLD'");
    REQUIRE(tokens.size() >= 2);
    CHECK(tokens[0].type == TT::STRING_LITERAL);
    CHECK(tokens[0].value == "HELLO WORLD");
}

TEST_CASE("Lexer: string literal with escaped quote") {
    auto tokens = lex_clean("'IT''S'");
    REQUIRE(tokens.size() >= 2);
    CHECK(tokens[0].type == TT::STRING_LITERAL);
    CHECK(tokens[0].value == "IT'S");
}

TEST_CASE("Lexer: numeric literal") {
    auto tokens = lex_clean("12345 67.89");
    REQUIRE(tokens.size() >= 3);
    CHECK(tokens[0].type == TT::NUMERIC_LITERAL);
    CHECK(tokens[0].value == "12345");
    CHECK(tokens[1].type == TT::NUMERIC_LITERAL);
    CHECK(tokens[1].value == "67.89");
}

TEST_CASE("Lexer: symbols") {
    auto tokens = lex_clean("( ) . , + * / =");
    CHECK(tokens[0].type == TT::LPAREN);
    CHECK(tokens[1].type == TT::RPAREN);
    CHECK(tokens[2].type == TT::DOT);
    CHECK(tokens[3].type == TT::COMMA);
    CHECK(tokens[4].type == TT::PLUS);
    CHECK(tokens[5].type == TT::STAR);
    CHECK(tokens[6].type == TT::SLASH);
    CHECK(tokens[7].type == TT::EQUALS);
}

TEST_CASE("Lexer: comment line") {
    auto tokens = lex_all("* THIS IS A COMMENT\nFILE TEST");
    CHECK(has_token_type(tokens, TT::COMMENT));
    CHECK(has_token_type(tokens, TT::KW_FILE));
}

TEST_CASE("Lexer: continuation line") {
    auto tokens = lex_all("DISPLAY 'HELLO'\n+ ' WORLD'");
    CHECK(has_token_type(tokens, TT::CONTINUATION));
    CHECK(has_token_type(tokens, TT::KW_DISPLAY));
}

TEST_CASE("Lexer: join continuations") {
    auto result = Lexer::join_continuations("DISPLAY 'HELLO'\n+ ' WORLD'");
    CHECK(result.find("+") == std::string::npos);
}

TEST_CASE("Lexer: W keyword") {
    auto tokens = lex_clean("W");
    CHECK(tokens[0].type == TT::KW_W);
}

TEST_CASE("Lexer: field definition sequence") {
    auto tokens = lex_clean("EMPNAME 1 30 A");
    REQUIRE(tokens.size() >= 5);
    CHECK(tokens[0].type == TT::IDENTIFIER);
    CHECK(tokens[0].value == "EMPNAME");
    CHECK(tokens[1].type == TT::NUMERIC_LITERAL);
    CHECK(tokens[1].value == "1");
    CHECK(tokens[2].type == TT::NUMERIC_LITERAL);
    CHECK(tokens[2].value == "30");
    CHECK(tokens[3].type == TT::KW_TYPE_A);
}

TEST_CASE("Lexer: PROC PARM DEFINE MACRO") {
    auto tokens = lex_clean("PROC PARM DEFINE MACRO");
    CHECK(tokens[0].type == TT::KW_PROC);
    CHECK(tokens[1].type == TT::KW_PARM);
    CHECK(tokens[2].type == TT::KW_DEFINE);
    CHECK(tokens[3].type == TT::KW_MACRO);
}

TEST_CASE("Lexer: END-PROC keyword") {
    auto tokens = lex_clean("END-PROC");
    CHECK(tokens[0].type == TT::KW_END_PROC);
}

TEST_CASE("Lexer: GOTO keyword") {
    auto tokens = lex_clean("GOTO LABEL1");
    CHECK(tokens[0].type == TT::KW_GOTO);
    CHECK(tokens[1].type == TT::IDENTIFIER);
}

TEST_CASE("Lexer: LRECL VIRTUAL INDEXED KEY keywords") {
    auto tokens = lex_clean("LRECL VIRTUAL INDEXED KEY");
    CHECK(tokens[0].type == TT::KW_LRECL);
    CHECK(tokens[1].type == TT::KW_VIRTUAL);
    CHECK(tokens[2].type == TT::KW_INDEXED);
    CHECK(tokens[3].type == TT::KW_KEY);
}

TEST_CASE("Lexer: colon separator") {
    auto tokens = lex_clean("FILE1:FIELD1");
    CHECK(tokens[0].type == TT::IDENTIFIER);
    CHECK(tokens[1].type == TT::COLON);
    CHECK(tokens[2].type == TT::IDENTIFIER);
}

TEST_CASE("Lexer: token line/column tracking") {
    auto tokens = lex_all("FILE\nGET");
    // FILE should be line 1
    CHECK(tokens[0].line == 1);
    // After newline, GET should be line 2
    bool found_get = false;
    for (const auto& t : tokens) {
        if (t.type == TT::KW_GET) {
            CHECK(t.line == 2);
            found_get = true;
        }
    }
    CHECK(found_get);
}

// ============================================================
// PARSER TESTS (25+)
// ============================================================

TEST_CASE("Parser: FILE declaration basic") {
    auto tokens = lex_clean("FILE EMPFILE LRECL 80");
    Parser parser(tokens);
    auto fd = parser.parse_file_decl();
    CHECK(fd.name == "EMPFILE");
    CHECK(fd.lrecl == 80);
}

TEST_CASE("Parser: FILE with VIRTUAL") {
    auto tokens = lex_clean("FILE WORKFILE VIRTUAL");
    Parser parser(tokens);
    auto fd = parser.parse_file_decl();
    CHECK(fd.name == "WORKFILE");
    CHECK(fd.is_virtual == true);
}

TEST_CASE("Parser: FILE with INDEXED and KEY") {
    auto tokens = lex_clean("FILE MASTER INDEXED KEY (EMPNO)");
    Parser parser(tokens);
    auto fd = parser.parse_file_decl();
    CHECK(fd.name == "MASTER");
    CHECK(fd.is_indexed == true);
    CHECK(fd.key_field == "EMPNO");
}

TEST_CASE("Parser: FILE with field definitions") {
    auto tokens = lex_clean("FILE EMPFILE LRECL 80 EMPNAME 1 30 A EMPNO 31 5 N");
    Parser parser(tokens);
    auto fd = parser.parse_file_decl();
    CHECK(fd.name == "EMPFILE");
    REQUIRE(fd.fields.size() == 2);
    CHECK(fd.fields[0].name == "EMPNAME");
    CHECK(fd.fields[0].position == 1);
    CHECK(fd.fields[0].length == 30);
    CHECK(fd.fields[0].type == FieldType::Alpha);
    CHECK(fd.fields[1].name == "EMPNO");
    CHECK(fd.fields[1].position == 31);
    CHECK(fd.fields[1].length == 5);
    CHECK(fd.fields[1].type == FieldType::Numeric);
}

TEST_CASE("Parser: field with decimal places") {
    auto tokens = lex_clean("FILE F1 LRECL 20 SALARY 1 10 N 2");
    Parser parser(tokens);
    auto fd = parser.parse_file_decl();
    REQUIRE(fd.fields.size() == 1);
    CHECK(fd.fields[0].decimals == 2);
}

TEST_CASE("Parser: W block parsing") {
    auto tokens = lex_clean("W COUNTER 5 N TOTAL 10 N 2");
    Parser parser(tokens);
    auto fields = parser.parse_w_block();
    REQUIRE(fields.size() == 2);
    CHECK(fields[0].name == "COUNTER");
    CHECK(fields[0].length == 5);
    CHECK(fields[0].type == FieldType::Numeric);
    CHECK(fields[1].name == "TOTAL");
    CHECK(fields[1].length == 10);
    CHECK(fields[1].decimals == 2);
}

TEST_CASE("Parser: JOB activity") {
    auto tokens = lex_clean("JOB INPUT EMPFILE GET EMPFILE DISPLAY EMPNAME");
    Parser parser(tokens);
    auto job = parser.parse_job();
    CHECK(job.input_file == "EMPFILE");
    CHECK(job.statements.size() >= 2);
}

TEST_CASE("Parser: JOB with named activity") {
    auto tokens = lex_clean("JOB MYJOB EMPFILE GET EMPFILE");
    Parser parser(tokens);
    auto job = parser.parse_job();
    CHECK(job.name == "MYJOB");
    CHECK(job.input_file == "EMPFILE");
}

TEST_CASE("Parser: SORT activity") {
    auto tokens = lex_clean("SORT INFILE TO OUTFILE LASTNAME FIRSTNAME");
    Parser parser(tokens);
    auto sort = parser.parse_sort();
    CHECK(sort.input_file == "INFILE");
    CHECK(sort.output_file == "OUTFILE");
    CHECK(sort.sort_fields.size() == 2);
    CHECK(sort.sort_fields[0] == "LASTNAME");
    CHECK(sort.sort_fields[1] == "FIRSTNAME");
}

TEST_CASE("Parser: SORT with descending") {
    auto tokens = lex_clean("SORT INFILE SALARY D");
    Parser parser(tokens);
    auto sort = parser.parse_sort();
    REQUIRE(sort.sort_fields.size() == 1);
    CHECK(sort.sort_fields[0] == "SALARY");
    REQUIRE(sort.ascending.size() == 1);
    CHECK(sort.ascending[0] == false);
}

TEST_CASE("Parser: REPORT activity basic") {
    auto tokens = lex_clean("REPORT EMPRPT SEQUENCE EMPNO");
    Parser parser(tokens);
    auto rpt = parser.parse_report();
    CHECK(rpt.name == "EMPRPT");
    CHECK(rpt.sequence_field == "EMPNO");
}

TEST_CASE("Parser: REPORT with CONTROL") {
    auto tokens = lex_clean("REPORT RPT1 CONTROL DEPT");
    Parser parser(tokens);
    auto rpt = parser.parse_report();
    REQUIRE(rpt.control_breaks.size() == 1);
    CHECK(rpt.control_breaks[0].field_name == "DEPT");
}

TEST_CASE("Parser: REPORT with TITLE") {
    auto tokens = lex_clean("REPORT RPT1 TITLE 1 'EMPLOYEE REPORT'");
    Parser parser(tokens);
    auto rpt = parser.parse_report();
    REQUIRE(rpt.titles.size() == 1);
    CHECK(rpt.titles[0].line_number == 1);
}

TEST_CASE("Parser: IF statement") {
    auto tokens = lex_clean("JOB INPUT F1 IF SALARY GT 50000 DISPLAY 'HIGH' END-IF");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_if());
}

TEST_CASE("Parser: IF-ELSE statement") {
    auto tokens = lex_clean("JOB INPUT F1 IF STATUS EQ 'A' DISPLAY 'ACTIVE' ELSE DISPLAY 'INACTIVE' END-IF");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    auto& stmt = *job.statements[0];
    CHECK(stmt.is_if());
    auto& ifs = std::get<IfStmt>(stmt.value);
    CHECK(ifs.then_branch.size() >= 1);
    CHECK(ifs.else_branch.size() >= 1);
}

TEST_CASE("Parser: DO WHILE loop") {
    auto tokens = lex_clean("JOB INPUT F1 DO WHILE COUNTER LT 10 DISPLAY COUNTER END-DO");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_do());
    auto& ds = std::get<DoStmt>(job.statements[0]->value);
    CHECK(ds.is_while == true);
}

TEST_CASE("Parser: PERFORM statement") {
    auto tokens = lex_clean("JOB INPUT F1 PERFORM PROC1");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_perform());
    auto& ps = std::get<PerformStmt>(job.statements[0]->value);
    CHECK(ps.proc_name == "PROC1");
}

TEST_CASE("Parser: GOTO statement") {
    auto tokens = lex_clean("JOB INPUT F1 GOTO DONE");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_goto());
}

TEST_CASE("Parser: GET statement") {
    auto tokens = lex_clean("JOB INPUT F1 GET F1");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_get());
    auto& gs = std::get<GetStmt>(job.statements[0]->value);
    CHECK(gs.file_name == "F1");
}

TEST_CASE("Parser: PUT statement") {
    auto tokens = lex_clean("JOB INPUT F1 PUT OUTFILE");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_put());
}

TEST_CASE("Parser: DISPLAY statement") {
    auto tokens = lex_clean("JOB INPUT F1 DISPLAY EMPNAME ' - ' SALARY");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_display());
    auto& ds = std::get<DisplayStmt>(job.statements[0]->value);
    CHECK(ds.items.size() == 3);
}

TEST_CASE("Parser: MOVE statement") {
    auto tokens = lex_clean("JOB INPUT F1 MOVE 'X' TO STATUS");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_move());
}

TEST_CASE("Parser: DEFINE TABLE") {
    auto tokens = lex_clean("DEFINE STATETBL STATE 2 A");
    Parser parser(tokens);
    auto td = parser.parse_define_table();
    CHECK(td.name == "STATETBL");
    REQUIRE(td.key_fields.size() >= 1);
    CHECK(td.key_fields[0].name == "STATE");
}

TEST_CASE("Parser: PROC with PARM") {
    auto tokens = lex_clean("PROC CALC PARM (X , Y) DISPLAY X END-PROC");
    Parser parser(tokens);
    auto pd = parser.parse_proc();
    CHECK(pd.name == "CALC");
    REQUIRE(pd.parameters.size() == 2);
    CHECK(pd.parameters[0] == "X");
    CHECK(pd.parameters[1] == "Y");
    CHECK(pd.body.size() >= 1);
}

TEST_CASE("Parser: full program parse") {
    std::string src =
        "FILE EMPFILE LRECL 80\n"
        "  EMPNO 1 5 N\n"
        "  EMPNAME 6 30 A\n"
        "JOB INPUT EMPFILE\n"
        "  GET EMPFILE\n"
        "  DISPLAY EMPNO\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();
    CHECK(prog.files.size() == 1);
    CHECK(prog.jobs.size() == 1);
}

TEST_CASE("Parser: SEARCH statement") {
    auto tokens = lex_clean("JOB INPUT F1 SEARCH STATETBL STATE");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_search());
}

TEST_CASE("Parser: STOP statement") {
    auto tokens = lex_clean("JOB INPUT F1 STOP");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_stop());
}

TEST_CASE("Parser: PRINT statement") {
    auto tokens = lex_clean("JOB INPUT F1 PRINT RPT1");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_print());
}

// ============================================================
// AST TESTS (implicit in parser, but also direct construction)
// ============================================================

TEST_CASE("AST: FieldType mapping") {
    CHECK(field_type_from_char('A') == FieldType::Alpha);
    CHECK(field_type_from_char('N') == FieldType::Numeric);
    CHECK(field_type_from_char('P') == FieldType::Packed);
    CHECK(field_type_from_char('B') == FieldType::Binary);
    CHECK(field_type_from_char('U') == FieldType::Unsigned);
}

TEST_CASE("AST: FieldType names") {
    CHECK(std::string(field_type_name(FieldType::Alpha)) == "A");
    CHECK(std::string(field_type_name(FieldType::Numeric)) == "N");
    CHECK(std::string(field_type_name(FieldType::Packed)) == "P");
    CHECK(std::string(field_type_name(FieldType::Binary)) == "B");
    CHECK(std::string(field_type_name(FieldType::Unsigned)) == "U");
}

TEST_CASE("AST: Expression construction") {
    auto fr = make_field_ref("EMPNAME", "EMPFILE");
    CHECK(fr.is_field_ref());
    CHECK(fr.as_field_ref().file_name == "EMPFILE");
    CHECK(fr.as_field_ref().field_name == "EMPNAME");

    auto lit = make_literal("100", true);
    CHECK(lit.is_literal());
    CHECK(lit.as_literal().is_numeric == true);
}

TEST_CASE("AST: Arithmetic expression") {
    auto left = std::make_shared<Expression>(make_field_ref("SALARY"));
    auto right = std::make_shared<Expression>(make_literal("1000", true));
    auto arith = make_arithmetic(left, ArithOp::Add, right);
    CHECK(arith.is_arithmetic());
    CHECK(arith.as_arithmetic().op == ArithOp::Add);
}

TEST_CASE("AST: Condition construction") {
    auto cmp = make_compare(make_field_ref("AGE"), CompareOp::Gt, make_literal("21", true));
    CHECK(cmp.is_compare());
    auto& c = std::get<Compare>(cmp.value);
    CHECK(c.op == CompareOp::Gt);
}

TEST_CASE("AST: AND condition") {
    auto left = make_compare(make_field_ref("A"), CompareOp::Eq, make_literal("1", true));
    auto right = make_compare(make_field_ref("B"), CompareOp::Eq, make_literal("2", true));
    auto combined = make_and(left, right);
    CHECK(std::holds_alternative<std::shared_ptr<ConditionAnd>>(combined.value));
}

TEST_CASE("AST: OR condition") {
    auto left = make_compare(make_field_ref("A"), CompareOp::Eq, make_literal("1", true));
    auto right = make_compare(make_field_ref("B"), CompareOp::Eq, make_literal("2", true));
    auto combined = make_or(left, right);
    CHECK(std::holds_alternative<std::shared_ptr<ConditionOr>>(combined.value));
}

TEST_CASE("AST: NOT condition") {
    auto inner = make_compare(make_field_ref("X"), CompareOp::Eq, make_literal("0", true));
    auto negated = make_not(inner);
    CHECK(std::holds_alternative<std::shared_ptr<ConditionNot>>(negated.value));
}

TEST_CASE("AST: RANGE condition") {
    auto rng = make_range(make_field_ref("AGE"), make_literal("18", true), make_literal("65", true));
    CHECK(rng.is_range());
}

TEST_CASE("AST: MASK condition") {
    auto m = make_mask(make_field_ref("SSN"), "999-99-9999");
    CHECK(m.is_mask());
}

TEST_CASE("AST: Statement variant types") {
    auto s1 = make_stmt(GetStmt{"FILE1"});
    CHECK(s1->is_get());
    auto s2 = make_stmt(PutStmt{"FILE2"});
    CHECK(s2->is_put());
    auto s3 = make_stmt(StopStmt{});
    CHECK(s3->is_stop());
    auto s4 = make_stmt(PerformStmt{"PROC1"});
    CHECK(s4->is_perform());
}

TEST_CASE("AST: CompareOp names") {
    CHECK(std::string(compare_op_name(CompareOp::Eq)) == "EQ");
    CHECK(std::string(compare_op_name(CompareOp::Ne)) == "NE");
    CHECK(std::string(compare_op_name(CompareOp::Lt)) == "LT");
    CHECK(std::string(compare_op_cpp(CompareOp::Eq)) == "==");
    CHECK(std::string(compare_op_cpp(CompareOp::Ne)) == "!=");
    CHECK(std::string(compare_op_cpp(CompareOp::Ge)) == ">=");
}

TEST_CASE("AST: ArithOp symbols") {
    CHECK(std::string(arith_op_symbol(ArithOp::Add)) == "+");
    CHECK(std::string(arith_op_symbol(ArithOp::Sub)) == "-");
    CHECK(std::string(arith_op_symbol(ArithOp::Mul)) == "*");
    CHECK(std::string(arith_op_symbol(ArithOp::Div)) == "/");
}

// ============================================================
// TRANSLATOR TESTS (25+)
// ============================================================

TEST_CASE("Translator: FILE to struct") {
    FileDecl fd;
    fd.name = "EMPFILE";
    fd.lrecl = 80;
    fd.fields.push_back(FieldDecl("EMPNAME", 1, 30, FieldType::Alpha));
    fd.fields.push_back(FieldDecl("EMPNO", 31, 5, FieldType::Numeric));

    Translator tr;
    auto code = tr.translate_file_only(fd);
    CHECK(code.find("struct empfile_record") != std::string::npos);
    CHECK(code.find("std::string empname") != std::string::npos);
    CHECK(code.find("int32_t empno") != std::string::npos);
}

TEST_CASE("Translator: VIRTUAL file") {
    FileDecl fd;
    fd.name = "WORKFILE";
    fd.is_virtual = true;

    Translator tr;
    auto code = tr.translate_file_only(fd);
    CHECK(code.find("VIRTUAL") != std::string::npos);
    CHECK(code.find("vector") != std::string::npos);
}

TEST_CASE("Translator: field type mapping Alpha") {
    std::vector<FieldDecl> fields;
    fields.push_back(FieldDecl("NAME", 1, 30, FieldType::Alpha));
    Translator tr;
    auto code = tr.translate_fields(fields);
    CHECK(code.find("std::string") != std::string::npos);
}

TEST_CASE("Translator: field type mapping Numeric") {
    std::vector<FieldDecl> fields;
    fields.push_back(FieldDecl("COUNT", 1, 5, FieldType::Numeric));
    Translator tr;
    auto code = tr.translate_fields(fields);
    CHECK(code.find("int32_t") != std::string::npos);
}

TEST_CASE("Translator: field type mapping Numeric with decimals") {
    std::vector<FieldDecl> fields;
    fields.push_back(FieldDecl("AMOUNT", 1, 10, FieldType::Numeric, 2));
    Translator tr;
    auto code = tr.translate_fields(fields);
    CHECK(code.find("double") != std::string::npos);
}

TEST_CASE("Translator: field type mapping Packed") {
    std::vector<FieldDecl> fields;
    fields.push_back(FieldDecl("PKD", 1, 5, FieldType::Packed));
    Translator tr;
    auto code = tr.translate_fields(fields);
    CHECK(code.find("int64_t") != std::string::npos);
}

TEST_CASE("Translator: field type mapping Binary") {
    std::vector<FieldDecl> fields;
    fields.push_back(FieldDecl("FLAGS", 1, 2, FieldType::Binary));
    Translator tr;
    auto code = tr.translate_fields(fields);
    CHECK(code.find("int16_t") != std::string::npos);
}

TEST_CASE("Translator: JOB to while loop") {
    JobActivity job;
    job.input_file = "EMPFILE";
    job.statements.push_back(make_stmt(DisplayStmt{{make_field_ref("EMPNAME")}, true}));

    Translator tr;
    auto code = tr.translate_job_only(job);
    CHECK(code.find("while") != std::string::npos);
    CHECK(code.find("empfile") != std::string::npos);
    CHECK(code.find(".get()") != std::string::npos);
}

TEST_CASE("Translator: IF to if") {
    IfStmt is;
    is.condition = make_compare(make_field_ref("SALARY"), CompareOp::Gt, make_literal("50000", true));
    is.then_branch.push_back(make_stmt(DisplayStmt{{make_literal("HIGH", false)}, true}));

    Statement stmt(std::move(is));
    Translator tr;
    auto code = tr.translate_statement(stmt);
    CHECK(code.find("if") != std::string::npos);
    CHECK(code.find("salary") != std::string::npos);
    CHECK(code.find("> 50000") != std::string::npos);
}

TEST_CASE("Translator: DO WHILE to while") {
    DoStmt ds;
    ds.is_while = true;
    ds.while_condition = make_compare(make_field_ref("I"), CompareOp::Lt, make_literal("10", true));
    ds.body.push_back(make_stmt(DisplayStmt{{make_field_ref("I")}, true}));

    Statement stmt(std::move(ds));
    Translator tr;
    auto code = tr.translate_statement(stmt);
    CHECK(code.find("while") != std::string::npos);
}

TEST_CASE("Translator: PERFORM to function call") {
    Statement stmt(PerformStmt{"CALC_TOTAL"});
    Translator tr;
    auto code = tr.translate_statement(stmt);
    CHECK(code.find("calc_total()") != std::string::npos);
}

TEST_CASE("Translator: GOTO to goto") {
    Statement stmt(GoToStmt{"DONE"});
    Translator tr;
    auto code = tr.translate_statement(stmt);
    CHECK(code.find("goto done") != std::string::npos);
}

TEST_CASE("Translator: MOVE to assignment") {
    MoveStmt ms;
    ms.source = make_literal("ACTIVE", false);
    ms.target = make_field_ref("STATUS");
    Statement stmt(std::move(ms));
    Translator tr;
    auto code = tr.translate_statement(stmt);
    CHECK(code.find("status") != std::string::npos);
    CHECK(code.find("=") != std::string::npos);
}

TEST_CASE("Translator: DISPLAY to cout") {
    DisplayStmt ds;
    ds.items.push_back(make_literal("Hello", false));
    ds.items.push_back(make_field_ref("NAME"));
    ds.newline = true;
    Statement stmt(std::move(ds));
    Translator tr;
    auto code = tr.translate_statement(stmt);
    CHECK(code.find("std::cout") != std::string::npos);
    CHECK(code.find("\"Hello\"") != std::string::npos);
}

TEST_CASE("Translator: SEARCH to map lookup") {
    SearchStmt ss;
    ss.table_name = "STATETBL";
    ss.search_key = make_field_ref("STATE_CODE");
    ss.result_field = "STATE_NAME";
    Statement stmt(std::move(ss));
    Translator tr;
    auto code = tr.translate_statement(stmt);
    CHECK(code.find("statetbl_table.find") != std::string::npos);
}

TEST_CASE("Translator: expression field ref") {
    Translator tr;
    auto code = tr.translate_expr(make_field_ref("EMPNAME", "EMPFILE"));
    CHECK(code.find("empfile.current.empname") != std::string::npos);
}

TEST_CASE("Translator: expression literal string") {
    Translator tr;
    auto code = tr.translate_expr(make_literal("HELLO", false));
    CHECK(code == "\"HELLO\"");
}

TEST_CASE("Translator: expression literal numeric") {
    Translator tr;
    auto code = tr.translate_expr(make_literal("42", true));
    CHECK(code == "42");
}

TEST_CASE("Translator: expression arithmetic") {
    auto left = std::make_shared<Expression>(make_field_ref("A"));
    auto right = std::make_shared<Expression>(make_literal("10", true));
    auto expr = make_arithmetic(left, ArithOp::Mul, right);
    Translator tr;
    auto code = tr.translate_expr(expr);
    CHECK(code.find("*") != std::string::npos);
    CHECK(code.find("10") != std::string::npos);
}

TEST_CASE("Translator: condition compare") {
    auto cond = make_compare(make_field_ref("X"), CompareOp::Le, make_literal("99", true));
    Translator tr;
    auto code = tr.translate_cond(cond);
    CHECK(code.find("<=") != std::string::npos);
    CHECK(code.find("99") != std::string::npos);
}

TEST_CASE("Translator: condition AND") {
    auto left = make_compare(make_field_ref("A"), CompareOp::Gt, make_literal("0", true));
    auto right = make_compare(make_field_ref("B"), CompareOp::Lt, make_literal("100", true));
    auto cond = make_and(left, right);
    Translator tr;
    auto code = tr.translate_cond(cond);
    CHECK(code.find("&&") != std::string::npos);
}

TEST_CASE("Translator: condition OR") {
    auto left = make_compare(make_field_ref("X"), CompareOp::Eq, make_literal("1", true));
    auto right = make_compare(make_field_ref("X"), CompareOp::Eq, make_literal("2", true));
    auto cond = make_or(left, right);
    Translator tr;
    auto code = tr.translate_cond(cond);
    CHECK(code.find("||") != std::string::npos);
}

TEST_CASE("Translator: condition NOT") {
    auto inner = make_compare(make_field_ref("F"), CompareOp::Eq, make_literal("0", true));
    auto cond = make_not(inner);
    Translator tr;
    auto code = tr.translate_cond(cond);
    CHECK(code.find("!") != std::string::npos);
}

TEST_CASE("Translator: condition RANGE") {
    auto cond = make_range(make_field_ref("AGE"), make_literal("18", true), make_literal("65", true));
    Translator tr;
    auto code = tr.translate_cond(cond);
    CHECK(code.find(">=") != std::string::npos);
    CHECK(code.find("<=") != std::string::npos);
    CHECK(code.find("&&") != std::string::npos);
}

TEST_CASE("Translator: SORT to std::sort") {
    SortActivity sort;
    sort.input_file = "EMPFILE";
    sort.sort_fields.push_back("LASTNAME");
    sort.ascending.push_back(true);

    Translator tr;
    auto code = tr.translate_sort_only(sort);
    CHECK(code.find("std::sort") != std::string::npos);
    CHECK(code.find("lastname") != std::string::npos);
}

TEST_CASE("Translator: REPORT to class") {
    ReportActivity rpt;
    rpt.name = "EMPRPT";
    rpt.lines_per_page = 55;

    Translator tr;
    auto code = tr.translate_report_only(rpt);
    CHECK(code.find("class emprpt_report") != std::string::npos);
    CHECK(code.find("LINES_PER_PAGE = 55") != std::string::npos);
}

TEST_CASE("Translator: macro SET and substitution") {
    Translator tr;
    tr.macros().define("APPNAME", "PAYROLL");
    auto result = tr.macros().expand("#SET VERSION 2.0\nTITLE '%APPNAME% V%VERSION%'");
    CHECK(result.find("PAYROLL") != std::string::npos);
    CHECK(result.find("2.0") != std::string::npos);
}

TEST_CASE("Translator: macro IF/ELSE/END") {
    Translator tr;
    tr.macros().define("DEBUG", "YES");
    auto result = tr.macros().expand(
        "#IF DEBUG\n"
        "DISPLAY 'DEBUG MODE'\n"
        "#ELSE\n"
        "DISPLAY 'PROD MODE'\n"
        "#END\n"
    );
    CHECK(result.find("DEBUG MODE") != std::string::npos);
    CHECK(result.find("PROD MODE") == std::string::npos);
}

TEST_CASE("Translator: macro IF undefined") {
    Translator tr;
    auto result = tr.macros().expand(
        "#IF NOTDEFINED\n"
        "DISPLAY 'YES'\n"
        "#ELSE\n"
        "DISPLAY 'NO'\n"
        "#END\n"
    );
    CHECK(result.find("YES") == std::string::npos);
    CHECK(result.find("NO") != std::string::npos);
}

// ============================================================
// RUNTIME TESTS (25+)
// ============================================================

TEST_CASE("Runtime: alpha_to_int conversion") {
    CHECK(convert::alpha_to_int("  123  ") == 123);
    CHECK(convert::alpha_to_int("  0  ") == 0);
    CHECK(convert::alpha_to_int("     ") == 0);
    CHECK(convert::alpha_to_int("") == 0);
}

TEST_CASE("Runtime: alpha_to_int64 conversion") {
    CHECK(convert::alpha_to_int64("123456789012") == 123456789012LL);
    CHECK(convert::alpha_to_int64("") == 0);
}

TEST_CASE("Runtime: alpha_to_double conversion") {
    CHECK(convert::alpha_to_double("  123.45  ") == doctest::Approx(123.45));
    CHECK(convert::alpha_to_double("") == doctest::Approx(0.0));
}

TEST_CASE("Runtime: int_to_alpha conversion") {
    CHECK(convert::int_to_alpha(42, 5) == "00042");
    CHECK(convert::int_to_alpha(0, 3) == "000");
    CHECK(convert::int_to_alpha(12345, 3) == "345");
}

TEST_CASE("Runtime: int64_to_alpha conversion") {
    CHECK(convert::int64_to_alpha(123456789012LL, 15) == "000123456789012");
}

TEST_CASE("Runtime: double_to_alpha conversion") {
    auto result = convert::double_to_alpha(123.45, 10, 2);
    CHECK(result.find("123.45") != std::string::npos);
}

TEST_CASE("Runtime: packed decimal encode/decode") {
    char buf[4] = {};
    convert::int64_to_packed(12345, buf, 4);
    int64_t val = convert::packed_to_int64(buf, 4);
    CHECK(val == 12345);
}

TEST_CASE("Runtime: packed decimal negative") {
    char buf[4] = {};
    convert::int64_to_packed(-678, buf, 4);
    int64_t val = convert::packed_to_int64(buf, 4);
    CHECK(val == -678);
}

TEST_CASE("Runtime: binary encode/decode") {
    char buf[4] = {};
    convert::int32_to_binary(12345, buf, 4);
    int32_t val = convert::binary_to_int32(buf, 4);
    CHECK(val == 12345);
}

TEST_CASE("Runtime: binary encode/decode 2-byte") {
    char buf[2] = {};
    convert::int32_to_binary(256, buf, 2);
    int32_t val = convert::binary_to_int32(buf, 2);
    CHECK(val == 256);
}

TEST_CASE("Runtime: edit mask formatting") {
    CHECK(convert::apply_edit_mask("12345", "99,999") == "12,345");
    CHECK(convert::apply_edit_mask("00123", "ZZ,ZZ9") == "  ,123");
}

TEST_CASE("Runtime: EzRecord define and get/set alpha") {
    EzRecord rec(80);
    rec.define_field("NAME", 1, 10);
    rec.set_alpha("NAME", "SMITH");
    CHECK(rec.get_alpha("NAME") == "SMITH     ");
}

TEST_CASE("Runtime: EzRecord define and get/set numeric") {
    EzRecord rec(80);
    rec.define_field("AGE", 11, 3);
    rec.set_numeric("AGE", 25);
    CHECK(rec.get_numeric("AGE") == 25);
}

TEST_CASE("Runtime: EzRecord get decimal") {
    EzRecord rec(80);
    rec.define_field("AMOUNT", 1, 7);
    rec.set_alpha("AMOUNT", "0012345");
    CHECK(rec.get_decimal("AMOUNT", 2) == doctest::Approx(123.45));
}

TEST_CASE("Runtime: EzRecord buffer operations") {
    EzRecord rec(20);
    std::string buf = "JOHN DOE   12345    ";
    rec.set_buffer(buf);
    rec.define_field("FIRST", 1, 8);
    rec.define_field("LAST", 9, 3);
    CHECK(rec.get_alpha("FIRST") == "JOHN DOE");
}

TEST_CASE("Runtime: EzRecord has_field") {
    EzRecord rec(80);
    rec.define_field("X", 1, 5);
    CHECK(rec.has_field("X") == true);
    CHECK(rec.has_field("Y") == false);
}

TEST_CASE("Runtime: EzRecord field_names") {
    EzRecord rec(80);
    rec.define_field("A", 1, 5);
    rec.define_field("B", 6, 5);
    auto names = rec.field_names();
    CHECK(names.size() == 2);
}

TEST_CASE("Runtime: EzFile construction") {
    EzFile file(80);
    CHECK(file.lrecl() == 80);
    CHECK(file.is_open() == false);
    CHECK(file.is_eof() == false);
}

TEST_CASE("Runtime: EzFile write and read") {
    // Write test file
    const std::string path = "test_ezfile_rw.tmp";
    {
        EzFile writer(20);
        writer.define_field("NAME", 1, 10);
        writer.define_field("NUM", 11, 5);
        REQUIRE(writer.open_write(path));
        writer.record().set_alpha("NAME", "ALICE");
        writer.record().set_numeric("NUM", 100);
        writer.put();
        writer.record().set_alpha("NAME", "BOB");
        writer.record().set_numeric("NUM", 200);
        writer.put();
        writer.close();
    }
    // Read back
    {
        EzFile reader(20);
        reader.define_field("NAME", 1, 10);
        reader.define_field("NUM", 11, 5);
        REQUIRE(reader.open_read(path));
        REQUIRE(reader.get());
        CHECK(reader.record().get_alpha("NAME").substr(0, 5) == "ALICE");
        CHECK(reader.record().get_numeric("NUM") == 100);
        REQUIRE(reader.get());
        CHECK(reader.record().get_alpha("NAME").substr(0, 3) == "BOB");
        CHECK(reader.record().get_numeric("NUM") == 200);
        CHECK(reader.get() == false); // EOF
        reader.close();
    }
    std::remove(path.c_str());
}

TEST_CASE("Runtime: EzFile point seek") {
    const std::string path = "test_ezfile_point.tmp";
    {
        EzFile writer(10);
        writer.define_field("VAL", 1, 10);
        REQUIRE(writer.open_write(path));
        writer.record().set_alpha("VAL", "REC0");
        writer.put();
        writer.record().set_alpha("VAL", "REC1");
        writer.put();
        writer.record().set_alpha("VAL", "REC2");
        writer.put();
        writer.close();
    }
    {
        EzFile reader(10);
        reader.define_field("VAL", 1, 10);
        REQUIRE(reader.open_read(path));
        reader.point(2); // seek to record 2
        REQUIRE(reader.get());
        CHECK(reader.record().get_alpha("VAL").substr(0, 4) == "REC2");
        reader.close();
    }
    std::remove(path.c_str());
}

TEST_CASE("Runtime: EzFile move semantics") {
    EzFile a(80);
    a.define_field("X", 1, 5);
    EzFile b = std::move(a);
    CHECK(b.lrecl() == 80);
    CHECK(b.record().has_field("X"));
}

TEST_CASE("Runtime: TableLookup add and search") {
    TableLookup tbl;
    tbl.add_entry("NJ", "New Jersey");
    tbl.add_entry("NY", "New York");
    tbl.add_entry("PA", "Pennsylvania");

    CHECK(tbl.search("NJ") == "New Jersey");
    CHECK(tbl.search("NY") == "New York");
    CHECK(tbl.search("XX") == "");
    CHECK(tbl.search("XX", "Unknown") == "Unknown");
}

TEST_CASE("Runtime: TableLookup contains") {
    TableLookup tbl;
    tbl.add_entry("A", "Alpha");
    CHECK(tbl.contains("A") == true);
    CHECK(tbl.contains("B") == false);
}

TEST_CASE("Runtime: TableLookup multi-column") {
    TableLookup tbl;
    tbl.add_entry("NJ", {"New Jersey", "Trenton", "609"});
    CHECK(tbl.search("NJ", 0) == "New Jersey");
    CHECK(tbl.search("NJ", 1) == "Trenton");
    CHECK(tbl.search("NJ", 2) == "609");
    CHECK(tbl.search("NJ", 99) == "");
}

TEST_CASE("Runtime: TableLookup load from vectors") {
    TableLookup tbl;
    tbl.load({"A", "B", "C"}, {"Alpha", "Beta", "Charlie"});
    CHECK(tbl.size() == 3);
    CHECK(tbl.search("B") == "Beta");
}

TEST_CASE("Runtime: TableLookup keys") {
    TableLookup tbl;
    tbl.add_entry("X", "ex");
    tbl.add_entry("Y", "why");
    auto keys = tbl.keys();
    CHECK(keys.size() == 2);
}

TEST_CASE("Runtime: TableLookup clear") {
    TableLookup tbl;
    tbl.add_entry("A", "a");
    tbl.clear();
    CHECK(tbl.size() == 0);
}

TEST_CASE("Runtime: ReportEngine basic output") {
    std::ostringstream out;
    ReportEngine rpt(out, 60);
    rpt.add_title("TEST REPORT");
    rpt.add_heading("COL1    COL2    COL3");

    rpt.check_page_break();
    rpt.print_line("DATA1   DATA2   DATA3");

    std::string output = out.str();
    CHECK(output.find("TEST REPORT") != std::string::npos);
    CHECK(output.find("COL1") != std::string::npos);
    CHECK(output.find("DATA1") != std::string::npos);
}

TEST_CASE("Runtime: ReportEngine page numbering") {
    std::ostringstream out;
    ReportEngine rpt(out, 60);
    rpt.add_title("RPT");
    rpt.check_page_break();
    CHECK(rpt.current_page() == 1);
}

TEST_CASE("Runtime: ReportEngine control break accumulation") {
    std::ostringstream out;
    ReportEngine rpt(out, 60);
    rpt.add_control_break("DEPT", {"SALARY"});

    // Record 1
    rpt.check_breaks({{"DEPT", "IT"}});
    rpt.accumulate({{"SALARY", 50000.0}});

    // Record 2
    rpt.check_breaks({{"DEPT", "IT"}});
    rpt.accumulate({{"SALARY", 60000.0}});

    // Break fires
    auto fired = rpt.check_breaks({{"DEPT", "HR"}});
    CHECK(fired.size() == 1);

    double subtotal = rpt.get_subtotal(0, "SALARY");
    CHECK(subtotal == doctest::Approx(110000.0));
}

TEST_CASE("Runtime: ReportEngine grand totals") {
    std::ostringstream out;
    ReportEngine rpt(out, 60);
    rpt.add_control_break("DEPT", {"AMOUNT"});

    rpt.accumulate({{"AMOUNT", 100.0}});
    rpt.accumulate({{"AMOUNT", 200.0}});
    rpt.accumulate({{"AMOUNT", 300.0}});

    CHECK(rpt.get_grand_total("AMOUNT") == doctest::Approx(600.0));
}

TEST_CASE("Runtime: ReportEngine formatted columns") {
    std::ostringstream out;
    ReportEngine rpt(out, 60);
    rpt.add_title("RPT");
    rpt.check_page_break();
    rpt.print_formatted({{"ALICE", 15}, {"100", -10}});

    std::string output = out.str();
    CHECK(output.find("ALICE") != std::string::npos);
    CHECK(output.find("100") != std::string::npos);
}

TEST_CASE("Runtime: ReportEngine reset") {
    std::ostringstream out;
    ReportEngine rpt(out, 60);
    rpt.add_title("T");
    rpt.check_page_break();
    rpt.print_line("data");
    CHECK(rpt.current_page() == 1);
    rpt.reset();
    CHECK(rpt.current_page() == 0);
    CHECK(rpt.current_line() == 0);
}

TEST_CASE("Runtime: ReportEngine page break on overflow") {
    std::ostringstream out;
    ReportEngine rpt(out, 5); // very small page
    rpt.add_title("T");

    // First page
    rpt.check_page_break(); // triggers page 1
    for (int i = 0; i < 6; i++) {
        rpt.print_line("line");
    }
    // Now check should trigger page 2
    rpt.check_page_break();
    CHECK(rpt.current_page() == 2);
}

TEST_CASE("Runtime: date stamp format") {
    auto stamp = ReportEngine::get_date_stamp();
    // Should contain year-month-day
    CHECK(stamp.size() >= 10);
    CHECK(stamp[4] == '-');
    CHECK(stamp[7] == '-');
}

// ============================================================
// INTEGRATION TESTS (20+)
// ============================================================

TEST_CASE("Integration: lex-parse FILE with fields") {
    std::string src =
        "FILE PAYFILE LRECL 100\n"
        "  EMPNO    1  5 N\n"
        "  EMPNAME  6 25 A\n"
        "  SALARY  31 10 N 2\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.files.size() == 1);
    CHECK(prog.files[0].name == "PAYFILE");
    CHECK(prog.files[0].lrecl == 100);
    REQUIRE(prog.files[0].fields.size() == 3);
    CHECK(prog.files[0].fields[2].decimals == 2);
}

TEST_CASE("Integration: lex-parse-translate FILE") {
    std::string src = "FILE EMPFILE LRECL 80 EMPNO 1 5 N EMPNAME 6 30 A";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto fd = parser.parse_file_decl();

    Translator tr;
    auto code = tr.translate_file_only(fd);
    CHECK(code.find("struct empfile_record") != std::string::npos);
    CHECK(code.find("int32_t empno") != std::string::npos);
    CHECK(code.find("std::string empname") != std::string::npos);
    CHECK(code.find("LRECL = 80") != std::string::npos);
}

TEST_CASE("Integration: lex-parse-translate JOB with GET/DISPLAY") {
    std::string src =
        "FILE F1 LRECL 80\n"
        "  NAME 1 30 A\n"
        "JOB INPUT F1\n"
        "  GET F1\n"
        "  DISPLAY NAME\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();

    Translator tr;
    auto code = tr.translate(prog);
    CHECK(code.find("while") != std::string::npos);
    CHECK(code.find("f1.get()") != std::string::npos);
    CHECK(code.find("std::cout") != std::string::npos);
}

TEST_CASE("Integration: lex-parse-translate IF/ELSE") {
    std::string src =
        "FILE F1 LRECL 20\n"
        "  VAL 1 5 N\n"
        "JOB INPUT F1\n"
        "  IF VAL GT 100\n"
        "    DISPLAY 'BIG'\n"
        "  ELSE\n"
        "    DISPLAY 'SMALL'\n"
        "  END-IF\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();

    Translator tr;
    auto code = tr.translate(prog);
    CHECK(code.find("if") != std::string::npos);
    CHECK(code.find("else") != std::string::npos);
    CHECK(code.find("> 100") != std::string::npos);
}

TEST_CASE("Integration: lex-parse-translate SORT") {
    std::string src =
        "FILE INFILE LRECL 80\n"
        "  NAME 1 30 A\n"
        "  SALARY 31 10 N\n"
        "SORT INFILE NAME\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();

    Translator tr;
    auto code = tr.translate(prog);
    CHECK(code.find("std::sort") != std::string::npos);
}

TEST_CASE("Integration: lex-parse-translate REPORT") {
    std::string src =
        "FILE F1 LRECL 80\n"
        "  DEPT 1 10 A\n"
        "  AMOUNT 11 10 N 2\n"
        "REPORT RPT1\n"
        "  SEQUENCE DEPT\n"
        "  CONTROL DEPT\n"
        "  TITLE 1 'DEPARTMENT REPORT'\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();

    Translator tr;
    auto code = tr.translate(prog);
    CHECK(code.find("class rpt1_report") != std::string::npos);
    CHECK(code.find("DEPARTMENT REPORT") != std::string::npos);
}

TEST_CASE("Integration: lex-parse-translate W fields") {
    std::string src =
        "W\n"
        "  COUNTER 5 N\n"
        "  TOTAL 10 N 2\n"
        "  MSG 40 A\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();

    Translator tr;
    auto code = tr.translate(prog);
    CHECK(code.find("WorkingStorage") != std::string::npos);
    CHECK(code.find("int32_t counter") != std::string::npos);
    CHECK(code.find("double total") != std::string::npos);
    CHECK(code.find("std::string msg") != std::string::npos);
}

TEST_CASE("Integration: runtime file round-trip with EzFile") {
    const std::string path = "test_integration_roundtrip.tmp";
    // Write
    {
        EzFile f(30);
        f.define_field("KEY", 1, 5);
        f.define_field("DATA", 6, 25);
        REQUIRE(f.open_write(path));
        f.record().set_alpha("KEY", "001");
        f.record().set_alpha("DATA", "FIRST RECORD");
        f.put();
        f.record().set_alpha("KEY", "002");
        f.record().set_alpha("DATA", "SECOND RECORD");
        f.put();
        f.close();
    }
    // Read and verify
    {
        EzFile f(30);
        f.define_field("KEY", 1, 5);
        f.define_field("DATA", 6, 25);
        REQUIRE(f.open_read(path));

        REQUIRE(f.get());
        CHECK(f.record().get_alpha("KEY") == "001  ");
        CHECK(f.record().get_alpha("DATA").substr(0, 12) == "FIRST RECORD");

        REQUIRE(f.get());
        CHECK(f.record().get_alpha("KEY") == "002  ");

        CHECK(!f.get());
        f.close();
    }
    std::remove(path.c_str());
}

TEST_CASE("Integration: runtime table lookup workflow") {
    TableLookup dept_table;
    dept_table.add_entry("10", "ACCOUNTING");
    dept_table.add_entry("20", "RESEARCH");
    dept_table.add_entry("30", "SALES");
    dept_table.add_entry("40", "OPERATIONS");

    // Simulate Easytrieve SEARCH
    std::string dept_code = "20";
    std::string dept_name = dept_table.search(dept_code);
    CHECK(dept_name == "RESEARCH");

    dept_code = "99";
    dept_name = dept_table.search(dept_code, "UNKNOWN");
    CHECK(dept_name == "UNKNOWN");
}

TEST_CASE("Integration: runtime report with control breaks") {
    std::ostringstream out;
    ReportEngine rpt(out, 60);
    rpt.add_title("DEPARTMENT SALARY REPORT");
    rpt.add_heading("DEPT        NAME          SALARY");
    rpt.add_control_break("DEPT", {"SALARY"});

    // Simulate processing
    struct Record {
        std::string dept;
        std::string name;
        double salary;
    };
    std::vector<Record> records = {
        {"IT", "ALICE", 50000},
        {"IT", "BOB", 60000},
        {"HR", "CHARLIE", 55000},
        {"HR", "DIANA", 65000},
    };

    std::string prev_dept;
    for (size_t i = 0; i < records.size(); i++) {
        auto fired = rpt.check_breaks({{"DEPT", records[i].dept}});
        if (!fired.empty() && i > 0) {
            rpt.check_page_break();
            double sub = rpt.get_subtotal(0, "SALARY");
            rpt.print_line("  SUBTOTAL: " + std::to_string(sub));
        }
        rpt.accumulate({{"SALARY", records[i].salary}});
        rpt.check_page_break();
        rpt.print_line(records[i].dept + "  " + records[i].name + "  " +
                       std::to_string(records[i].salary));
    }

    // Final subtotal
    double final_sub = rpt.get_subtotal(0, "SALARY");
    rpt.print_line("  SUBTOTAL: " + std::to_string(final_sub));

    // Grand total
    rpt.print_grand_totals("GRAND TOTAL:");

    std::string output = out.str();
    CHECK(output.find("DEPARTMENT SALARY REPORT") != std::string::npos);
    CHECK(output.find("ALICE") != std::string::npos);
    CHECK(output.find("GRAND TOTAL:") != std::string::npos);
}

TEST_CASE("Integration: full program lex-parse-translate round trip") {
    std::string src =
        "FILE EMPFILE LRECL 80\n"
        "  EMPNO   1  5 N\n"
        "  NAME    6 20 A\n"
        "  DEPT   26  3 N\n"
        "  SALARY 29 10 N 2\n"
        "W\n"
        "  TOTAL 10 N 2\n"
        "  COUNT 5 N\n"
        "JOB INPUT EMPFILE\n"
        "  GET EMPFILE\n"
        "  IF SALARY GT 50000\n"
        "    DISPLAY NAME ' SALARY: ' SALARY\n"
        "  END-IF\n";

    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();

    CHECK(prog.files.size() == 1);
    CHECK(prog.working_fields.size() == 2);
    CHECK(prog.jobs.size() == 1);

    Translator tr;
    auto code = tr.translate(prog);
    // Verify key structural elements
    CHECK(code.find("struct empfile_record") != std::string::npos);
    CHECK(code.find("WorkingStorage") != std::string::npos);
    CHECK(code.find("int main()") != std::string::npos);
    CHECK(code.find("if") != std::string::npos);
    CHECK(code.find("> 50000") != std::string::npos);
}

TEST_CASE("Integration: PROC declaration and PERFORM") {
    std::string src =
        "FILE F1 LRECL 20\n"
        "  VAL 1 5 N\n"
        "PROC SHOWVAL\n"
        "  DISPLAY VAL\n"
        "END-PROC\n"
        "JOB INPUT F1\n"
        "  GET F1\n"
        "  PERFORM SHOWVAL\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();

    CHECK(prog.procs.size() == 1);
    CHECK(prog.procs[0].name == "SHOWVAL");
    CHECK(prog.jobs.size() == 1);

    Translator tr;
    auto code = tr.translate(prog);
    CHECK(code.find("void showval()") != std::string::npos);
    CHECK(code.find("showval()") != std::string::npos);
}

TEST_CASE("Integration: DEFINE TABLE and SEARCH") {
    std::string src =
        "DEFINE STATETBL STATE 2 A\n"
        "FILE F1 LRECL 10\n"
        "  CODE 1 2 A\n"
        "JOB INPUT F1\n"
        "  SEARCH STATETBL CODE\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();

    CHECK(prog.tables.size() == 1);
    CHECK(prog.tables[0].name == "STATETBL");

    Translator tr;
    auto code = tr.translate(prog);
    CHECK(code.find("statetbl_table") != std::string::npos);
}

TEST_CASE("Integration: macro expansion followed by lex-parse") {
    Translator tr;
    tr.macros().define("FNAME", "EMPFILE");
    tr.macros().define("RECLEN", "80");

    std::string raw_src =
        "#SET DEBUG YES\n"
        "FILE %FNAME% LRECL %RECLEN%\n"
        "  EMPNO 1 5 N\n"
        "#IF DEBUG\n"
        "W\n"
        "  DEBUGFLAG 1 A\n"
        "#END\n"
        "JOB INPUT %FNAME%\n"
        "  GET %FNAME%\n";

    auto expanded = tr.macros().expand(raw_src);
    CHECK(expanded.find("EMPFILE") != std::string::npos);
    CHECK(expanded.find("80") != std::string::npos);
    CHECK(expanded.find("DEBUGFLAG") != std::string::npos);

    auto tokens = lex_clean(expanded);
    Parser parser(tokens);
    auto prog = parser.parse();
    CHECK(prog.files.size() == 1);
    CHECK(prog.files[0].name == "EMPFILE");
}

TEST_CASE("Integration: multi-file program") {
    std::string src =
        "FILE MASTER LRECL 100\n"
        "  EMPNO 1 5 N\n"
        "  NAME 6 30 A\n"
        "FILE TRANS LRECL 50\n"
        "  EMPNO 1 5 N\n"
        "  AMOUNT 6 10 N 2\n"
        "JOB INPUT MASTER\n"
        "  GET MASTER\n"
        "  DISPLAY NAME\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();

    CHECK(prog.files.size() == 2);
    CHECK(prog.files[0].name == "MASTER");
    CHECK(prog.files[1].name == "TRANS");
}

TEST_CASE("Integration: packed decimal runtime round-trip") {
    char buf[5];
    convert::int64_to_packed(9999999, buf, 5);
    int64_t result = convert::packed_to_int64(buf, 5);
    CHECK(result == 9999999);
}

TEST_CASE("Integration: large field type Numeric int64") {
    std::vector<FieldDecl> fields;
    fields.push_back(FieldDecl("BIGNUM", 1, 15, FieldType::Numeric));
    Translator tr;
    auto code = tr.translate_fields(fields);
    CHECK(code.find("int64_t") != std::string::npos);
}

TEST_CASE("Integration: Binary field type 4-byte") {
    std::vector<FieldDecl> fields;
    fields.push_back(FieldDecl("BIN4", 1, 4, FieldType::Binary));
    Translator tr;
    auto code = tr.translate_fields(fields);
    CHECK(code.find("int32_t") != std::string::npos);
}

TEST_CASE("Integration: Binary field type 8-byte") {
    std::vector<FieldDecl> fields;
    fields.push_back(FieldDecl("BIN8", 1, 8, FieldType::Binary));
    Translator tr;
    auto code = tr.translate_fields(fields);
    CHECK(code.find("int64_t") != std::string::npos);
}

TEST_CASE("Integration: complex condition AND/OR parse and translate") {
    auto tokens = lex_clean("JOB INPUT F1 IF A GT 0 AND B LT 100 OR C EQ 'X' DISPLAY 'OK' END-IF");
    Parser parser(tokens);
    auto job = parser.parse_job();
    REQUIRE(job.statements.size() >= 1);
    CHECK(job.statements[0]->is_if());

    auto& ifs = std::get<IfStmt>(job.statements[0]->value);
    Translator tr;
    auto code = tr.translate_cond(ifs.condition);
    // Should contain logical operators
    bool has_logic = code.find("&&") != std::string::npos ||
                     code.find("||") != std::string::npos;
    CHECK(has_logic);
}

TEST_CASE("Integration: DO WHILE full cycle") {
    std::string src =
        "FILE F1 LRECL 20\n"
        "  VAL 1 5 N\n"
        "W\n"
        "  I 5 N\n"
        "JOB INPUT F1\n"
        "  DO WHILE I LT 5\n"
        "    DISPLAY I\n"
        "  END-DO\n";
    auto tokens = lex_clean(src);
    Parser parser(tokens);
    auto prog = parser.parse();
    REQUIRE(prog.jobs.size() == 1);
    REQUIRE(prog.jobs[0].statements.size() >= 1);
    CHECK(prog.jobs[0].statements[0]->is_do());

    Translator tr;
    auto code = tr.translate(prog);
    CHECK(code.find("while") != std::string::npos);
}
