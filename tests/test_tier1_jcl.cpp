#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

#include "lazarus/jcl/lexer.h"
#include "lazarus/jcl/parser.h"
#include "lazarus/jcl/proc_resolver.h"
#include "lazarus/jcl/translator.h"
#include "lazarus/jcl/shell_translator.h"
#include "lazarus/jcl/utilities.h"

using namespace lazarus::jcl;

// =============================================================================
// LEXER TESTS (25+)
// =============================================================================

TEST_CASE("Lexer: JOB keyword token") {
    Lexer lex("//MYJOB   JOB (ACCT),'PROGRAMMER'");
    CHECK(lex.has_token(TokenType::JOB));
    CHECK(lex.has_token(TokenType::LABEL));
}

TEST_CASE("Lexer: EXEC keyword token") {
    Lexer lex("//STEP1   EXEC PGM=IEBGENER");
    CHECK(lex.has_token(TokenType::EXEC));
    auto labels = lex.tokens_of_type(TokenType::LABEL);
    REQUIRE(!labels.empty());
    CHECK(labels[0].value == "STEP1");
}

TEST_CASE("Lexer: DD keyword token") {
    Lexer lex("//SYSUT1  DD DSN=MY.DATA.SET,DISP=SHR");
    CHECK(lex.has_token(TokenType::DD));
    auto labels = lex.tokens_of_type(TokenType::LABEL);
    REQUIRE(!labels.empty());
    CHECK(labels[0].value == "SYSUT1");
}

TEST_CASE("Lexer: PROC and PEND keywords") {
    Lexer lex("//MYPROC  PROC VAR1=DEFAULT\n//STEP1   EXEC PGM=PROG1\n//        PEND");
    CHECK(lex.has_token(TokenType::PROC));
    CHECK(lex.has_token(TokenType::PEND));
}

TEST_CASE("Lexer: SET keyword") {
    Lexer lex("// SET MYVAR=VALUE1");
    CHECK(lex.has_token(TokenType::SET));
}

TEST_CASE("Lexer: INCLUDE keyword") {
    Lexer lex("// INCLUDE MEMBER=MYINC");
    CHECK(lex.has_token(TokenType::INCLUDE));
}

TEST_CASE("Lexer: JCLLIB keyword") {
    Lexer lex("//         JCLLIB ORDER=(MY.PROC.LIB)");
    CHECK(lex.has_token(TokenType::JCLLIB));
}

TEST_CASE("Lexer: OUTPUT keyword") {
    Lexer lex("//OUTDS   OUTPUT CLASS=A");
    CHECK(lex.has_token(TokenType::OUTPUT));
}

TEST_CASE("Lexer: IF/THEN/ELSE/ENDIF keywords") {
    std::string jcl =
        "//MYJOB  JOB ACCT\n"
        "//       IF (STEP1.RC EQ 0) THEN\n"
        "//STEP2  EXEC PGM=PROG2\n"
        "//       ELSE\n"
        "//STEP3  EXEC PGM=PROG3\n"
        "//       ENDIF\n";
    Lexer lex(jcl);
    CHECK(lex.has_token(TokenType::IF));
    CHECK(lex.has_token(TokenType::THEN));
    CHECK(lex.has_token(TokenType::ELSE));
    CHECK(lex.has_token(TokenType::ENDIF));
}

TEST_CASE("Lexer: comment detection") {
    Lexer lex("//* THIS IS A JCL COMMENT");
    CHECK(lex.has_token(TokenType::COMMENT));
    auto comments = lex.tokens_of_type(TokenType::COMMENT);
    REQUIRE(!comments.empty());
    CHECK(comments[0].value == "THIS IS A JCL COMMENT");
}

TEST_CASE("Lexer: delimiter detection") {
    Lexer lex("/*");
    CHECK(lex.has_token(TokenType::DELIMITER));
}

TEST_CASE("Lexer: parameter tokens") {
    Lexer lex("//SYSUT1  DD DSN=MY.FILE,DISP=SHR");
    CHECK(lex.has_token(TokenType::PARAM));
    CHECK(lex.has_token(TokenType::EQUALS));
    CHECK(lex.has_token(TokenType::COMMA));
}

TEST_CASE("Lexer: literal token with quotes") {
    Lexer lex("//MYJOB  JOB (ACCT),'JOHN DOE'");
    CHECK(lex.has_token(TokenType::LITERAL));
    auto literals = lex.tokens_of_type(TokenType::LITERAL);
    REQUIRE(!literals.empty());
    CHECK(literals[0].value == "'JOHN DOE'");
}

TEST_CASE("Lexer: parenthesized groups") {
    Lexer lex("//MYJOB  JOB (ACCT,DEPT),'PROG'");
    CHECK(lex.has_token(TokenType::LPAREN));
    CHECK(lex.has_token(TokenType::RPAREN));
}

TEST_CASE("Lexer: asterisk for DD *") {
    Lexer lex("//SYSIN   DD *");
    CHECK(lex.has_token(TokenType::ASTERISK));
}

TEST_CASE("Lexer: instream data after DD *") {
    std::string jcl =
        "//SYSIN   DD *\n"
        "LINE ONE\n"
        "LINE TWO\n"
        "/*\n";
    Lexer lex(jcl);
    CHECK(lex.has_token(TokenType::INSTREAM_DATA));
    auto data = lex.tokens_of_type(TokenType::INSTREAM_DATA);
    REQUIRE(data.size() >= 2);
    CHECK(data[0].value == "LINE ONE");
    CHECK(data[1].value == "LINE TWO");
}

TEST_CASE("Lexer: continuation with comma") {
    std::string jcl =
        "//SYSUT1  DD DSN=MY.FILE,\n"
        "//           DISP=SHR\n";
    Lexer lex(jcl);
    CHECK(lex.has_token(TokenType::DD));
    CHECK(lex.has_token(TokenType::PARAM));
}

TEST_CASE("Lexer: multiple statements") {
    std::string jcl =
        "//MYJOB  JOB ACCT\n"
        "//STEP1  EXEC PGM=IEFBR14\n"
        "//DD1    DD DUMMY\n";
    Lexer lex(jcl);
    CHECK(lex.has_token(TokenType::JOB));
    CHECK(lex.has_token(TokenType::EXEC));
    CHECK(lex.has_token(TokenType::DD));
}

TEST_CASE("Lexer: COND parameter") {
    Lexer lex("//MYJOB  JOB ACCT,COND=(4,LT)");
    CHECK(lex.has_token(TokenType::COND));
}

TEST_CASE("Lexer: NOTIFY parameter") {
    Lexer lex("//MYJOB  JOB ACCT,NOTIFY=&SYSUID");
    CHECK(lex.has_token(TokenType::NOTIFY));
}

TEST_CASE("Lexer: count JOB tokens") {
    std::string jcl =
        "//JOB1   JOB ACCT\n"
        "//STEP1  EXEC PGM=PROG1\n";
    Lexer lex(jcl);
    CHECK(lex.count_tokens(TokenType::JOB) == 1);
    CHECK(lex.count_tokens(TokenType::EXEC) == 1);
}

TEST_CASE("Lexer: empty input") {
    Lexer lex("");
    CHECK(lex.has_token(TokenType::END_OF_INPUT));
    CHECK(lex.count_tokens(TokenType::JOB) == 0);
}

TEST_CASE("Lexer: label extraction from multiple steps") {
    std::string jcl =
        "//STEP1  EXEC PGM=PROG1\n"
        "//STEP2  EXEC PGM=PROG2\n"
        "//STEP3  EXEC PGM=PROG3\n";
    Lexer lex(jcl);
    auto labels = lex.tokens_of_type(TokenType::LABEL);
    CHECK(labels.size() == 3);
    CHECK(labels[0].value == "STEP1");
    CHECK(labels[1].value == "STEP2");
    CHECK(labels[2].value == "STEP3");
}

TEST_CASE("Lexer: multiple comments") {
    std::string jcl =
        "//* FIRST COMMENT\n"
        "//* SECOND COMMENT\n"
        "//* THIRD COMMENT\n";
    Lexer lex(jcl);
    CHECK(lex.count_tokens(TokenType::COMMENT) == 3);
}

TEST_CASE("Lexer: token_type_name coverage") {
    CHECK(std::string(token_type_name(TokenType::JOB)) == "JOB");
    CHECK(std::string(token_type_name(TokenType::DD)) == "DD");
    CHECK(std::string(token_type_name(TokenType::EXEC)) == "EXEC");
    CHECK(std::string(token_type_name(TokenType::PROC)) == "PROC");
    CHECK(std::string(token_type_name(TokenType::COMMENT)) == "COMMENT");
    CHECK(std::string(token_type_name(TokenType::INSTREAM_DATA)) == "INSTREAM_DATA");
    CHECK(std::string(token_type_name(TokenType::SET)) == "SET");
    CHECK(std::string(token_type_name(TokenType::INCLUDE)) == "INCLUDE");
}

TEST_CASE("Lexer: classify_keyword returns OPER for unknown") {
    CHECK(classify_keyword("UNKNOWN") == TokenType::OPER);
    CHECK(classify_keyword("FOOBAR") == TokenType::OPER);
}

TEST_CASE("Lexer: classify_keyword case insensitive") {
    CHECK(classify_keyword("job") == TokenType::JOB);
    CHECK(classify_keyword("Exec") == TokenType::EXEC);
    CHECK(classify_keyword("dd") == TokenType::DD);
}

// =============================================================================
// PARSER TESTS (25+)
// =============================================================================

TEST_CASE("Parser: JOB card name") {
    std::string jcl =
        "//MYJOB   JOB (ACCT),'PROGRAMMER'\n"
        "//STEP1   EXEC PGM=IEFBR14\n";
    auto job = parse_jcl(jcl);
    CHECK(job.name == "MYJOB");
}

TEST_CASE("Parser: JOB card accounting info") {
    std::string jcl = "//MYJOB   JOB (ACCT123),'JOHN DOE'\n";
    auto job = parse_jcl(jcl);
    CHECK(job.accounting == "(ACCT123)");
}

TEST_CASE("Parser: JOB card programmer name") {
    std::string jcl = "//MYJOB   JOB ACCT,'JOHN DOE'\n";
    auto job = parse_jcl(jcl);
    CHECK(job.programmer == "'JOHN DOE'");
}

TEST_CASE("Parser: JOB card CLASS parameter") {
    std::string jcl = "//MYJOB   JOB ACCT,'PROG',CLASS=A\n";
    auto job = parse_jcl(jcl);
    CHECK(job.job_class == "A");
}

TEST_CASE("Parser: JOB card MSGCLASS parameter") {
    std::string jcl = "//MYJOB   JOB ACCT,'PROG',MSGCLASS=X\n";
    auto job = parse_jcl(jcl);
    CHECK(job.msgclass == "X");
}

TEST_CASE("Parser: JOB card NOTIFY parameter") {
    std::string jcl = "//MYJOB   JOB ACCT,'PROG',NOTIFY=&SYSUID\n";
    auto job = parse_jcl(jcl);
    CHECK(job.notify == "&SYSUID");
}

TEST_CASE("Parser: EXEC PGM step") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=IEBGENER\n";
    auto job = parse_jcl(jcl);
    REQUIRE(job.steps.size() == 1);
    CHECK(job.steps[0].name == "STEP1");
    CHECK(job.steps[0].pgm == "IEBGENER");
}

TEST_CASE("Parser: EXEC PROC step") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PROC=MYPROC\n";
    auto job = parse_jcl(jcl);
    REQUIRE(job.steps.size() == 1);
    CHECK(job.steps[0].proc == "MYPROC");
    CHECK(job.steps[0].is_proc_call);
}

TEST_CASE("Parser: bare proc call") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC MYPROC\n";
    auto job = parse_jcl(jcl);
    REQUIRE(job.steps.size() == 1);
    CHECK(job.steps[0].proc == "MYPROC");
    CHECK(job.steps[0].is_proc_call);
}

TEST_CASE("Parser: DD with DSN and DISP") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//SYSUT1  DD DSN=MY.INPUT.DATA,DISP=SHR\n";
    auto job = parse_jcl(jcl);
    REQUIRE(!job.steps.empty());
    REQUIRE(!job.steps[0].dd_statements.empty());
    auto& dd = job.steps[0].dd_statements[0];
    CHECK(dd.name == "SYSUT1");
    CHECK(dd.params.dsn == "MY.INPUT.DATA");
    CHECK(dd.params.disp_status == "SHR");
}

TEST_CASE("Parser: DD with DISP=(NEW,CATLG,DELETE)") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//SYSUT2  DD DSN=MY.OUT,DISP=(NEW,CATLG,DELETE)\n";
    auto job = parse_jcl(jcl);
    REQUIRE(!job.steps.empty());
    REQUIRE(!job.steps[0].dd_statements.empty());
    auto& dd = job.steps[0].dd_statements[0];
    CHECK(dd.params.disp_status == "NEW");
    CHECK(dd.params.disp_normal == "CATLG");
    CHECK(dd.params.disp_abnormal == "DELETE");
}

TEST_CASE("Parser: DD DUMMY") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//SYSUT1  DD DUMMY\n";
    auto job = parse_jcl(jcl);
    REQUIRE(!job.steps.empty());
    REQUIRE(!job.steps[0].dd_statements.empty());
    CHECK(job.steps[0].dd_statements[0].params.is_dummy);
}

TEST_CASE("Parser: DD * instream") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//SYSIN   DD *\n"
        "DATA LINE 1\n"
        "DATA LINE 2\n"
        "/*\n";
    auto job = parse_jcl(jcl);
    REQUIRE(!job.steps.empty());
    REQUIRE(!job.steps[0].dd_statements.empty());
    auto& dd = job.steps[0].dd_statements[0];
    CHECK(dd.params.is_instream);
    REQUIRE(dd.params.instream_data.size() >= 2);
    CHECK(dd.params.instream_data[0] == "DATA LINE 1");
    CHECK(dd.params.instream_data[1] == "DATA LINE 2");
}

TEST_CASE("Parser: DD SYSOUT") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//SYSPRINT DD SYSOUT=A\n";
    auto job = parse_jcl(jcl);
    REQUIRE(!job.steps.empty());
    REQUIRE(!job.steps[0].dd_statements.empty());
    CHECK(job.steps[0].dd_statements[0].params.sysout == "A");
}

TEST_CASE("Parser: DD SPACE parameter") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//SYSUT2  DD DSN=MY.OUT,DISP=(NEW,CATLG),SPACE=(CYL,(10,5),RLSE)\n";
    auto job = parse_jcl(jcl);
    REQUIRE(!job.steps.empty());
    REQUIRE(!job.steps[0].dd_statements.empty());
    CHECK(!job.steps[0].dd_statements[0].params.space.empty());
}

TEST_CASE("Parser: DD DCB parameter") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//SYSUT2  DD DSN=MY.OUT,DCB=(RECFM=FB,LRECL=80,BLKSIZE=800)\n";
    auto job = parse_jcl(jcl);
    REQUIRE(!job.steps.empty());
    REQUIRE(!job.steps[0].dd_statements.empty());
    CHECK(!job.steps[0].dd_statements[0].params.dcb.empty());
}

TEST_CASE("Parser: multiple steps") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//STEP2   EXEC PGM=PROG2\n"
        "//STEP3   EXEC PGM=PROG3\n";
    auto job = parse_jcl(jcl);
    CHECK(job.steps.size() == 3);
    CHECK(job.steps[0].pgm == "PROG1");
    CHECK(job.steps[1].pgm == "PROG2");
    CHECK(job.steps[2].pgm == "PROG3");
}

TEST_CASE("Parser: multiple DDs per step") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=IEBGENER\n"
        "//SYSUT1  DD DSN=MY.INPUT,DISP=SHR\n"
        "//SYSUT2  DD DSN=MY.OUTPUT,DISP=(NEW,CATLG)\n"
        "//SYSPRINT DD SYSOUT=A\n"
        "//SYSIN   DD DUMMY\n";
    auto job = parse_jcl(jcl);
    REQUIRE(job.steps.size() == 1);
    CHECK(job.steps[0].dd_statements.size() == 4);
}

TEST_CASE("Parser: EXEC with PARM") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=MYPROG,PARM='OPTION1'\n";
    auto job = parse_jcl(jcl);
    REQUIRE(job.steps.size() == 1);
    CHECK(job.steps[0].exec_params.count("PARM") > 0);
}

TEST_CASE("Parser: DD referback") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//SYSUT1  DD DSN=MY.FILE,DISP=SHR\n"
        "//STEP2   EXEC PGM=PROG2\n"
        "//SYSUT1  DD DSN=*.STEP1.SYSUT1\n";
    auto job = parse_jcl(jcl);
    REQUIRE(job.steps.size() >= 2);
    REQUIRE(!job.steps[1].dd_statements.empty());
    CHECK(job.steps[1].dd_statements[0].params.is_referback);
}

TEST_CASE("Parser: JOB COND parameter") {
    std::string jcl = "//MYJOB   JOB ACCT,COND=(4,LT,STEP1)\n";
    auto job = parse_jcl(jcl);
    REQUIRE(!job.cond.empty());
    CHECK(job.cond[0].rc == 4);
    CHECK(job.cond[0].op == "LT");
    CHECK(job.cond[0].step_name == "STEP1");
}

TEST_CASE("Parser: parse_all returns AstNode vector") {
    std::string jcl =
        "//* A COMMENT\n"
        "//MYJOB  JOB ACCT\n"
        "//STEP1  EXEC PGM=PROG1\n";
    auto nodes = parse_jcl_all(jcl);
    CHECK(nodes.size() >= 2);
}

TEST_CASE("Parser: SET statement via parse_all") {
    std::string jcl =
        "// SET MYVAR=HELLO\n";
    auto nodes = parse_jcl_all(jcl);
    bool found_set = false;
    for (auto& n : nodes) {
        if (std::holds_alternative<JclSet>(n)) {
            auto& s = std::get<JclSet>(n);
            CHECK(s.variable == "MYVAR");
            CHECK(s.value == "HELLO");
            found_set = true;
        }
    }
    CHECK(found_set);
}

TEST_CASE("Parser: INCLUDE statement via parse_all") {
    std::string jcl = "//INC1    INCLUDE MEMBER=MYINC\n";
    auto nodes = parse_jcl_all(jcl);
    bool found = false;
    for (auto& n : nodes) {
        if (std::holds_alternative<JclInclude>(n)) {
            found = true;
            CHECK(std::get<JclInclude>(n).member == "MYINC");
        }
    }
    CHECK(found);
}

TEST_CASE("Parser: PROC definition") {
    std::string jcl =
        "//MYPROC  PROC VAR1=DEFAULT1\n"
        "//PS1     EXEC PGM=&VAR1\n"
        "//DD1     DD DSN=&VAR1..DATA,DISP=SHR\n"
        "//        PEND\n";
    Lexer lex(jcl);
    Parser parser(lex.tokens());
    auto proc = parser.parse_proc();
    CHECK(proc.name == "MYPROC");
    CHECK(proc.defaults.count("VAR1") > 0);
    CHECK(proc.defaults["VAR1"] == "DEFAULT1");
    CHECK(proc.steps.size() >= 1);
}

// =============================================================================
// PROC RESOLVER TESTS (20+)
// =============================================================================

TEST_CASE("ProcResolver: simple substitution") {
    std::map<std::string, std::string> syms = {{"VAR1", "HELLO"}};
    auto result = ProcResolver::substitute("DATA.&VAR1.FILE", syms);
    CHECK(result == "DATA.HELLOFILE");
}

TEST_CASE("ProcResolver: dot-terminated substitution") {
    std::map<std::string, std::string> syms = {{"LIB", "PROD"}};
    auto result = ProcResolver::substitute("SYS1.&LIB..LOAD", syms);
    CHECK(result == "SYS1.PROD.LOAD");
}

TEST_CASE("ProcResolver: multiple substitutions") {
    std::map<std::string, std::string> syms = {{"A", "X"}, {"B", "Y"}};
    auto result = ProcResolver::substitute("&A.DATA.&B.FILE", syms);
    CHECK(result == "XDATA.YFILE");
}

TEST_CASE("ProcResolver: no matching symbol unchanged") {
    std::map<std::string, std::string> syms = {{"VAR1", "HELLO"}};
    auto result = ProcResolver::substitute("&VAR2.DATA", syms);
    CHECK(result == "&VAR2.DATA");
}

TEST_CASE("ProcResolver: empty symbol map") {
    auto result = ProcResolver::substitute("&A.&B.&C", {});
    CHECK(result == "&A.&B.&C");
}

TEST_CASE("ProcResolver: proc registration") {
    ProcResolver resolver;
    JclProc proc;
    proc.name = "TESTPROC";
    proc.defaults["P1"] = "DEFAULT";
    JclStep step;
    step.name = "S1";
    step.pgm = "&P1";
    proc.steps.push_back(step);
    resolver.add_proc("TESTPROC", proc);
    CHECK(resolver.proc_library().count("TESTPROC") > 0);
}

TEST_CASE("ProcResolver: proc expansion with defaults") {
    ProcResolver resolver;
    JclProc proc;
    proc.name = "COPYPROC";
    proc.defaults["DSN"] = "DEFAULT.DATA";
    JclStep ps;
    ps.name = "COPY";
    ps.pgm = "IEBGENER";
    JclDd dd;
    dd.name = "SYSUT1";
    dd.params.dsn = "&DSN";
    ps.dd_statements.push_back(dd);
    proc.steps.push_back(ps);
    resolver.add_proc("COPYPROC", proc);

    JclJob job;
    job.name = "TESTJOB";
    JclStep call;
    call.name = "STEP1";
    call.proc = "COPYPROC";
    call.is_proc_call = true;
    job.steps.push_back(call);

    auto resolved = resolver.resolve(job);
    REQUIRE(!resolved.steps.empty());
    REQUIRE(!resolved.steps[0].dd_statements.empty());
    CHECK(resolved.steps[0].dd_statements[0].params.dsn == "DEFAULT.DATA");
}

TEST_CASE("ProcResolver: proc expansion with overrides") {
    ProcResolver resolver;
    JclProc proc;
    proc.name = "COPYPROC";
    proc.defaults["DSN"] = "DEFAULT.DATA";
    JclStep ps;
    ps.name = "COPY";
    ps.pgm = "IEBGENER";
    JclDd dd;
    dd.name = "SYSUT1";
    dd.params.dsn = "&DSN";
    ps.dd_statements.push_back(dd);
    proc.steps.push_back(ps);
    resolver.add_proc("COPYPROC", proc);

    JclJob job;
    job.name = "TESTJOB";
    JclStep call;
    call.name = "STEP1";
    call.proc = "COPYPROC";
    call.is_proc_call = true;
    call.exec_params["DSN"] = "OVERRIDE.DATA";
    job.steps.push_back(call);

    auto resolved = resolver.resolve(job);
    REQUIRE(!resolved.steps.empty());
    REQUIRE(!resolved.steps[0].dd_statements.empty());
    CHECK(resolved.steps[0].dd_statements[0].params.dsn == "OVERRIDE.DATA");
}

TEST_CASE("ProcResolver: step name qualification") {
    ProcResolver resolver;
    JclProc proc;
    proc.name = "MYPROC";
    JclStep ps;
    ps.name = "INNER";
    ps.pgm = "PROG1";
    proc.steps.push_back(ps);
    resolver.add_proc("MYPROC", proc);

    JclJob job;
    JclStep call;
    call.name = "OUTER";
    call.proc = "MYPROC";
    call.is_proc_call = true;
    job.steps.push_back(call);

    auto resolved = resolver.resolve(job);
    REQUIRE(!resolved.steps.empty());
    CHECK(resolved.steps[0].name == "OUTER.INNER");
}

TEST_CASE("ProcResolver: DD override in proc call") {
    ProcResolver resolver;
    JclProc proc;
    proc.name = "TESTPROC";
    JclStep ps;
    ps.name = "S1";
    ps.pgm = "PROG1";
    JclDd dd;
    dd.name = "SYSUT1";
    dd.params.dsn = "ORIGINAL.DATA";
    ps.dd_statements.push_back(dd);
    proc.steps.push_back(ps);
    resolver.add_proc("TESTPROC", proc);

    JclJob job;
    JclStep call;
    call.name = "CALL1";
    call.proc = "TESTPROC";
    call.is_proc_call = true;
    JclDd ovr_dd;
    ovr_dd.name = "SYSUT1";
    ovr_dd.params.dsn = "REPLACED.DATA";
    call.dd_statements.push_back(ovr_dd);
    job.steps.push_back(call);

    auto resolved = resolver.resolve(job);
    REQUIRE(!resolved.steps.empty());
    bool found = false;
    for (auto& d : resolved.steps[0].dd_statements) {
        if (d.name == "SYSUT1") {
            CHECK(d.params.dsn == "REPLACED.DATA");
            CHECK(d.is_override);
            found = true;
        }
    }
    CHECK(found);
}

TEST_CASE("ProcResolver: add new DD in proc call") {
    ProcResolver resolver;
    JclProc proc;
    proc.name = "TESTPROC";
    JclStep ps;
    ps.name = "S1";
    ps.pgm = "PROG1";
    proc.steps.push_back(ps);
    resolver.add_proc("TESTPROC", proc);

    JclJob job;
    JclStep call;
    call.name = "CALL1";
    call.proc = "TESTPROC";
    call.is_proc_call = true;
    JclDd new_dd;
    new_dd.name = "EXTRADD";
    new_dd.params.dsn = "EXTRA.DATA";
    call.dd_statements.push_back(new_dd);
    job.steps.push_back(call);

    auto resolved = resolver.resolve(job);
    REQUIRE(!resolved.steps.empty());
    bool found = false;
    for (auto& d : resolved.steps[0].dd_statements) {
        if (d.name == "EXTRADD") {
            found = true;
            CHECK(d.params.dsn == "EXTRA.DATA");
        }
    }
    CHECK(found);
}

TEST_CASE("ProcResolver: referback resolution") {
    std::vector<JclStep> steps;
    JclStep s1;
    s1.name = "STEP1";
    JclDd dd;
    dd.name = "SYSUT1";
    dd.params.dsn = "MY.REAL.DATA";
    s1.dd_statements.push_back(dd);
    steps.push_back(s1);

    auto result = ProcResolver::resolve_referback("*.STEP1.SYSUT1", steps);
    CHECK(result == "MY.REAL.DATA");
}

TEST_CASE("ProcResolver: referback unresolved") {
    std::vector<JclStep> steps;
    auto result = ProcResolver::resolve_referback("*.NOSTEP.NODD", steps);
    CHECK(result == "*.NOSTEP.NODD");
}

TEST_CASE("ProcResolver: resolve_all_referbacks") {
    std::vector<JclStep> steps;
    JclStep s1;
    s1.name = "STEP1";
    JclDd dd1;
    dd1.name = "SYSUT1";
    dd1.params.dsn = "REAL.DATA.SET";
    s1.dd_statements.push_back(dd1);
    steps.push_back(s1);

    JclStep s2;
    s2.name = "STEP2";
    JclDd dd2;
    dd2.name = "INPUT";
    dd2.params.dsn = "*.STEP1.SYSUT1";
    dd2.params.is_referback = true;
    dd2.params.referback = "*.STEP1.SYSUT1";
    s2.dd_statements.push_back(dd2);
    steps.push_back(s2);

    ProcResolver::resolve_all_referbacks(steps);
    CHECK(steps[1].dd_statements[0].params.dsn == "REAL.DATA.SET");
    CHECK(!steps[1].dd_statements[0].params.is_referback);
}

TEST_CASE("ProcResolver: proc with multiple steps") {
    ProcResolver resolver;
    JclProc proc;
    proc.name = "MULTI";
    for (int i = 1; i <= 3; ++i) {
        JclStep ps;
        ps.name = "S" + std::to_string(i);
        ps.pgm = "PROG" + std::to_string(i);
        proc.steps.push_back(ps);
    }
    resolver.add_proc("MULTI", proc);

    JclJob job;
    JclStep call;
    call.name = "CALLER";
    call.proc = "MULTI";
    call.is_proc_call = true;
    job.steps.push_back(call);

    auto resolved = resolver.resolve(job);
    CHECK(resolved.steps.size() == 3);
}

TEST_CASE("ProcResolver: unknown proc returns call as-is") {
    ProcResolver resolver;
    JclJob job;
    JclStep call;
    call.name = "STEP1";
    call.proc = "NOEXIST";
    call.is_proc_call = true;
    job.steps.push_back(call);

    auto resolved = resolver.resolve(job);
    REQUIRE(resolved.steps.size() == 1);
    CHECK(resolved.steps[0].proc == "NOEXIST");
}

TEST_CASE("ProcResolver: non-proc steps pass through") {
    ProcResolver resolver;
    JclJob job;
    JclStep s;
    s.name = "STEP1";
    s.pgm = "MYPROG";
    JclDd dd;
    dd.name = "DD1";
    dd.params.dsn = "MY.DATA";
    s.dd_statements.push_back(dd);
    job.steps.push_back(s);

    auto resolved = resolver.resolve(job);
    REQUIRE(resolved.steps.size() == 1);
    CHECK(resolved.steps[0].pgm == "MYPROG");
    CHECK(resolved.steps[0].dd_statements[0].params.dsn == "MY.DATA");
}

TEST_CASE("ProcResolver: job-level symbol overrides") {
    ProcResolver resolver;
    JclProc proc;
    proc.name = "SYMPROC";
    proc.defaults["LIB"] = "TEST";
    JclStep ps;
    ps.name = "S1";
    ps.pgm = "PROG1";
    JclDd dd;
    dd.name = "DD1";
    dd.params.dsn = "SYS.&LIB.DATA";
    ps.dd_statements.push_back(dd);
    proc.steps.push_back(ps);
    resolver.add_proc("SYMPROC", proc);

    JclJob job;
    JclStep call;
    call.name = "C1";
    call.proc = "SYMPROC";
    call.is_proc_call = true;
    job.steps.push_back(call);

    std::map<std::string, std::string> overrides = {{"LIB", "PROD"}};
    auto resolved = resolver.resolve(job, overrides);
    REQUIRE(!resolved.steps.empty());
    // Job-level override takes effect
    CHECK(resolved.steps[0].dd_statements[0].params.dsn.find("PROD") != std::string::npos);
}

// =============================================================================
// C++17 TRANSLATOR TESTS (15+)
// =============================================================================

TEST_CASE("Translator: generates C++ output") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=IEBGENER\n"
        "//SYSUT1  DD DSN=MY.INPUT,DISP=SHR\n"
        "//SYSUT2  DD DSN=MY.OUTPUT,DISP=(NEW,CATLG)\n";
    auto job = parse_jcl(jcl);
    Translator trans(job);
    std::string code = trans.generate();
    CHECK(!code.empty());
    CHECK(code.find("class") != std::string::npos);
    CHECK(code.find("execute") != std::string::npos);
}

TEST_CASE("Translator: contains step methods") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//STEP2   EXEC PGM=PROG2\n";
    auto job = parse_jcl(jcl);
    Translator trans(job);
    std::string code = trans.generate();
    CHECK(code.find("step_step1") != std::string::npos);
    CHECK(code.find("step_step2") != std::string::npos);
}

TEST_CASE("Translator: DSN to path conversion") {
    CHECK(Translator::dsn_to_path("SYS1.LINKLIB") == "sys1/linklib");
    CHECK(Translator::dsn_to_path("MY.DATA.SET") == "my/data/set");
    CHECK(Translator::dsn_to_path("SINGLE") == "single");
}

TEST_CASE("Translator: DD map generation") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//INPUT   DD DSN=MY.INPUT.FILE,DISP=SHR\n";
    auto job = parse_jcl(jcl);
    Translator trans(job);
    std::string dd_map = trans.generate_dd_map();
    CHECK(dd_map.find("my/input/file") != std::string::npos);
}

TEST_CASE("Translator: DISP RAII for NEW,CATLG") {
    auto disp = parse_disp_action(DdParams{});
    // Default: NEW, KEEP
    CHECK(disp.creates_file());
    CHECK(disp.keeps_on_success());
}

TEST_CASE("Translator: DISP action for SHR") {
    DdParams p;
    p.disp_status = "SHR";
    auto disp = parse_disp_action(p);
    CHECK(!disp.creates_file());
    CHECK(disp.keeps_on_success());
}

TEST_CASE("Translator: DISP DELETE on success") {
    DdParams p;
    p.disp_status = "OLD";
    p.disp_normal = "DELETE";
    auto disp = parse_disp_action(p);
    CHECK(disp.deletes_on_success());
    CHECK(!disp.keeps_on_success());
}

TEST_CASE("Translator: DISP DELETE on failure") {
    DdParams p;
    p.disp_status = "NEW";
    p.disp_normal = "CATLG";
    p.disp_abnormal = "DELETE";
    auto disp = parse_disp_action(p);
    CHECK(disp.deletes_on_failure());
    CHECK(disp.keeps_on_success());
}

TEST_CASE("Translator: COND check generation") {
    std::string code = Translator::generate_cond_check("(4,LT,STEP1)", "STEP2");
    CHECK(code.find("rc_map") != std::string::npos);
    CHECK(code.find("STEP1") != std::string::npos);
    CHECK(code.find("<") != std::string::npos);
}

TEST_CASE("Translator: COND evaluation logic") {
    CondEval ce{4, "LT", "STEP1"};
    CHECK(ce.should_skip(3));      // 3 < 4 = true -> skip
    CHECK(!ce.should_skip(5));     // 5 < 4 = false -> run

    CondEval ce2{0, "EQ", "STEP1"};
    CHECK(ce2.should_skip(0));
    CHECK(!ce2.should_skip(4));
}

TEST_CASE("Translator: COND GE evaluation") {
    CondEval ce{4, "GE", "STEP1"};
    CHECK(ce.should_skip(4));
    CHECK(ce.should_skip(5));
    CHECK(!ce.should_skip(3));
}

TEST_CASE("Translator: COND NE evaluation") {
    CondEval ce{0, "NE", "STEP1"};
    CHECK(ce.should_skip(1));
    CHECK(!ce.should_skip(0));
}

TEST_CASE("Translator: parse_cond_param single") {
    auto evals = parse_cond_param("(4,LT,STEP1)");
    REQUIRE(evals.size() == 1);
    CHECK(evals[0].code == 4);
    CHECK(evals[0].op == "LT");
    CHECK(evals[0].step == "STEP1");
}

TEST_CASE("Translator: execute body contains structured bindings") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n";
    auto job = parse_jcl(jcl);
    Translator trans(job);
    std::string body = trans.generate_execute_body();
    CHECK(body.find("auto [rc, path]") != std::string::npos);
    CHECK(body.find("rc_map") != std::string::npos);
}

TEST_CASE("Translator: includes filesystem header") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n";
    auto job = parse_jcl(jcl);
    Translator trans(job);
    std::string code = trans.generate();
    CHECK(code.find("<filesystem>") != std::string::npos);
}

TEST_CASE("Translator: DISP RAII code generation") {
    DispAction disp;
    disp.status = "NEW";
    disp.normal = "CATLG";
    disp.abnormal = "DELETE";
    std::string code = Translator::generate_disp_raii("out_file", "my/output", disp);
    CHECK(code.find("DISP=(NEW,CATLG,DELETE)") != std::string::npos);
    CHECK(code.find("make_disp_guard") != std::string::npos);
}

// =============================================================================
// SHELL TRANSLATOR TESTS (15+)
// =============================================================================

TEST_CASE("ShellTranslator: generates bash shebang") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n";
    auto job = parse_jcl(jcl);
    ShellTranslator trans(job);
    std::string script = trans.generate();
    CHECK(script.find("#!/bin/bash") != std::string::npos);
}

TEST_CASE("ShellTranslator: contains step functions") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//STEP2   EXEC PGM=PROG2\n";
    auto job = parse_jcl(jcl);
    ShellTranslator trans(job);
    std::string script = trans.generate();
    CHECK(script.find("step_step1()") != std::string::npos);
    CHECK(script.find("step_step2()") != std::string::npos);
}

TEST_CASE("ShellTranslator: error handling with trap") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n";
    auto job = parse_jcl(jcl);
    ShellTranslator trans(job);
    std::string script = trans.generate();
    CHECK(script.find("trap") != std::string::npos);
    CHECK(script.find("set -euo pipefail") != std::string::npos);
}

TEST_CASE("ShellTranslator: DD to variable mapping") {
    JclDd dd;
    dd.name = "SYSUT1";
    dd.params.dsn = "MY.DATA.SET";
    std::string var = ShellTranslator::dd_to_var(dd);
    CHECK(var.find("DD_SYSUT1") != std::string::npos);
    CHECK(var.find("my/data/set") != std::string::npos);
}

TEST_CASE("ShellTranslator: DD DUMMY maps to /dev/null") {
    JclDd dd;
    dd.name = "SYSIN";
    dd.params.is_dummy = true;
    std::string var = ShellTranslator::dd_to_var(dd);
    CHECK(var.find("/dev/null") != std::string::npos);
}

TEST_CASE("ShellTranslator: DD instream maps to mktemp") {
    JclDd dd;
    dd.name = "SYSIN";
    dd.params.is_instream = true;
    std::string var = ShellTranslator::dd_to_var(dd);
    CHECK(var.find("mktemp") != std::string::npos);
}

TEST_CASE("ShellTranslator: DD SYSOUT maps to stdout") {
    JclDd dd;
    dd.name = "SYSPRINT";
    dd.params.sysout = "A";
    std::string var = ShellTranslator::dd_to_var(dd);
    CHECK(var.find("/dev/stdout") != std::string::npos);
}

TEST_CASE("ShellTranslator: PGM to command mapping") {
    std::map<std::string, std::string> params = {{"PARM", "'OPTION1'"}};
    std::string cmd = ShellTranslator::pgm_to_command("MYPROG", params);
    CHECK(cmd.find("MYPROG") != std::string::npos);
    CHECK(cmd.find("'OPTION1'") != std::string::npos);
}

TEST_CASE("ShellTranslator: COND to test expression") {
    std::string test = ShellTranslator::cond_to_test("(4,LT,STEP1)");
    CHECK(test.find("-lt") != std::string::npos);
    CHECK(test.find("RC_STEP1") != std::string::npos);
}

TEST_CASE("ShellTranslator: COND EQ test") {
    std::string test = ShellTranslator::cond_to_test("(0,EQ,STEP1)");
    CHECK(test.find("-eq") != std::string::npos);
}

TEST_CASE("ShellTranslator: COND NE test") {
    std::string test = ShellTranslator::cond_to_test("(0,NE,STEP1)");
    CHECK(test.find("-ne") != std::string::npos);
}

TEST_CASE("ShellTranslator: main function contains step calls") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//STEP2   EXEC PGM=PROG2\n";
    auto job = parse_jcl(jcl);
    ShellTranslator trans(job);
    std::string main = trans.generate_main();
    CHECK(main.find("step_step1") != std::string::npos);
    CHECK(main.find("step_step2") != std::string::npos);
    CHECK(main.find("MAX_RC") != std::string::npos);
}

TEST_CASE("ShellTranslator: job name in output") {
    std::string jcl =
        "//TESTJOB JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n";
    auto job = parse_jcl(jcl);
    ShellTranslator trans(job);
    std::string script = trans.generate();
    CHECK(script.find("TESTJOB") != std::string::npos);
}

TEST_CASE("ShellTranslator: DD variables section") {
    std::string jcl =
        "//MYJOB   JOB ACCT\n"
        "//STEP1   EXEC PGM=PROG1\n"
        "//INPUT   DD DSN=MY.FILE,DISP=SHR\n"
        "//OUTPUT  DD DSN=MY.OUT,DISP=(NEW,CATLG)\n";
    auto job = parse_jcl(jcl);
    ShellTranslator trans(job);
    std::string vars = trans.generate_dd_vars();
    CHECK(vars.find("DD_INPUT") != std::string::npos);
    CHECK(vars.find("DD_OUTPUT") != std::string::npos);
}

TEST_CASE("ShellTranslator: DISP NEW creates directory") {
    JclDd dd;
    dd.name = "OUT";
    dd.params.dsn = "MY.NEW.FILE";
    dd.params.disp_status = "NEW";
    dd.params.disp_normal = "CATLG";
    std::string cmds = ShellTranslator::disp_commands(dd, "DD_OUT", false);
    CHECK(cmds.find("mkdir -p") != std::string::npos);
}

TEST_CASE("ShellTranslator: DISP DELETE cleanup") {
    JclDd dd;
    dd.name = "TEMP";
    dd.params.dsn = "TEMP.FILE";
    dd.params.disp_status = "NEW";
    dd.params.disp_normal = "DELETE";
    std::string cmds = ShellTranslator::disp_commands(dd, "DD_TEMP", true);
    CHECK(cmds.find("rm -f") != std::string::npos);
}

// =============================================================================
// UTILITIES TESTS (20+)
// =============================================================================

TEST_CASE("IEFBR14: basic no-op") {
    auto result = IEFBR14::execute();
    CHECK(result.rc == 0);
    CHECK(result.message.find("IEFBR14") != std::string::npos);
}

TEST_CASE("IEFBR14: with DD list") {
    std::vector<std::pair<std::string, std::string>> dds = {
        {"DD1", "FILE1"}, {"DD2", "FILE2"}
    };
    auto result = IEFBR14::execute(dds);
    CHECK(result.rc == 0);
    CHECK(result.message.find("2 DD") != std::string::npos);
}

TEST_CASE("IEBGENER: copy data") {
    std::string input = "LINE1\nLINE2\nLINE3\n";
    std::string output;
    auto result = IEBGENER::execute(input, output);
    CHECK(result.rc == 0);
    CHECK(output == input);
    CHECK(result.records_copied == 3);
}

TEST_CASE("IEBGENER: empty input") {
    std::string input;
    std::string output;
    auto result = IEBGENER::execute(input, output);
    CHECK(result.rc == 0);
    CHECK(output.empty());
}

TEST_CASE("IEBGENER: config validation missing SYSUT1") {
    IEBGENER::Config config;
    config.sysut2 = "OUTPUT";
    auto result = IEBGENER::execute(config);
    CHECK(result.rc == 12);
}

TEST_CASE("IEBGENER: config validation missing SYSUT2") {
    IEBGENER::Config config;
    config.sysut1 = "INPUT";
    auto result = IEBGENER::execute(config);
    CHECK(result.rc == 12);
}

TEST_CASE("IEBGENER: record-level copy") {
    std::vector<std::string> input = {"REC1", "REC2", "REC3"};
    std::vector<std::string> output;
    auto result = IEBGENER::execute_records(input, output);
    CHECK(result.rc == 0);
    CHECK(output.size() == 3);
    CHECK(output[0] == "REC1");
}

TEST_CASE("IEBGENER: record-level copy with transform") {
    std::vector<std::string> input = {"hello", "world"};
    std::vector<std::string> output;
    auto result = IEBGENER::execute_records(input, output, [](const std::string& s) {
        std::string r = s;
        std::transform(r.begin(), r.end(), r.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return r;
    });
    CHECK(result.rc == 0);
    CHECK(output[0] == "HELLO");
    CHECK(output[1] == "WORLD");
}

TEST_CASE("IEBCOPY: copy all members") {
    IEBCOPY::PDS source = {{"MEM1", "DATA1"}, {"MEM2", "DATA2"}};
    IEBCOPY::PDS target;
    auto result = IEBCOPY::execute(source, target);
    CHECK(result.rc == 0);
    CHECK(result.members_copied == 2);
    CHECK(target.size() == 2);
    CHECK(target["MEM1"] == "DATA1");
}

TEST_CASE("IEBCOPY: merge (skip existing)") {
    IEBCOPY::PDS source = {{"MEM1", "NEW"}, {"MEM2", "NEW2"}};
    IEBCOPY::PDS target = {{"MEM1", "OLD"}};
    IEBCOPY::Config config;
    config.op = IEBCOPY::Operation::MERGE;
    auto result = IEBCOPY::execute(source, target, config);
    CHECK(target["MEM1"] == "OLD");   // Not replaced
    CHECK(target["MEM2"] == "NEW2");  // Added
    CHECK(result.members_copied == 1);
}

TEST_CASE("IEBCOPY: select specific members") {
    IEBCOPY::PDS source = {{"MEM1", "D1"}, {"MEM2", "D2"}, {"MEM3", "D3"}};
    IEBCOPY::PDS target;
    IEBCOPY::Config config;
    config.op = IEBCOPY::Operation::SELECT;
    config.select_members = {"MEM1", "MEM3"};
    auto result = IEBCOPY::execute(source, target, config);
    CHECK(result.members_copied == 2);
    CHECK(target.count("MEM2") == 0);
}

TEST_CASE("IEBCOPY: exclude specific members") {
    IEBCOPY::PDS source = {{"MEM1", "D1"}, {"MEM2", "D2"}, {"MEM3", "D3"}};
    IEBCOPY::PDS target;
    IEBCOPY::Config config;
    config.op = IEBCOPY::Operation::EXCLUDE;
    config.select_members = {"MEM2"};
    auto result = IEBCOPY::execute(source, target, config);
    CHECK(result.members_copied == 2);
    CHECK(target.count("MEM2") == 0);
    CHECK(target.count("MEM1") == 1);
}

TEST_CASE("IEBCOPY: compress PDS") {
    IEBCOPY::PDS pds = {{"M1", "D1"}};
    auto result = IEBCOPY::compress(pds);
    CHECK(result.rc == 0);
}

TEST_CASE("IDCAMS: define cluster") {
    IDCAMS idcams;
    auto result = idcams.define_cluster("MY.VSAM.FILE", IDCAMS::VsamOrg::KSDS, 100, 10, 0);
    CHECK(result.rc == 0);
    CHECK(idcams.exists("MY.VSAM.FILE"));
}

TEST_CASE("IDCAMS: define duplicate cluster fails") {
    IDCAMS idcams;
    idcams.define_cluster("DUP.FILE", IDCAMS::VsamOrg::KSDS, 80);
    auto result = idcams.define_cluster("DUP.FILE", IDCAMS::VsamOrg::KSDS, 80);
    CHECK(result.rc == 8);
}

TEST_CASE("IDCAMS: delete entry") {
    IDCAMS idcams;
    idcams.define_cluster("DEL.FILE", IDCAMS::VsamOrg::ESDS, 100);
    CHECK(idcams.exists("DEL.FILE"));
    auto result = idcams.delete_entry("DEL.FILE");
    CHECK(result.rc == 0);
    CHECK(!idcams.exists("DEL.FILE"));
}

TEST_CASE("IDCAMS: delete nonexistent fails") {
    IDCAMS idcams;
    auto result = idcams.delete_entry("NO.EXIST");
    CHECK(result.rc == 8);
}

TEST_CASE("IDCAMS: delete with purge succeeds even if missing") {
    IDCAMS idcams;
    auto result = idcams.delete_entry("NO.EXIST", true);
    CHECK(result.rc == 0);
}

TEST_CASE("IDCAMS: repro between clusters") {
    IDCAMS idcams;
    idcams.define_cluster("SRC", IDCAMS::VsamOrg::KSDS, 80);
    idcams.define_cluster("TGT", IDCAMS::VsamOrg::KSDS, 80);
    idcams.repro_from_data("SRC", {"REC1", "REC2", "REC3"});

    auto result = idcams.repro("SRC", "TGT");
    CHECK(result.rc == 0);
    CHECK(idcams.record_count("TGT") == 3);
}

TEST_CASE("IDCAMS: repro from nonexistent source fails") {
    IDCAMS idcams;
    idcams.define_cluster("TGT", IDCAMS::VsamOrg::KSDS, 80);
    auto result = idcams.repro("NOSRC", "TGT");
    CHECK(result.rc == 12);
}

TEST_CASE("IDCAMS: listcat") {
    IDCAMS idcams;
    idcams.define_cluster("FILE.A", IDCAMS::VsamOrg::KSDS, 80);
    idcams.define_cluster("FILE.B", IDCAMS::VsamOrg::ESDS, 100);
    auto result = idcams.listcat();
    CHECK(result.rc == 0);
    CHECK(result.output_lines.size() == 2);
}

TEST_CASE("IDCAMS: listcat with filter") {
    IDCAMS idcams;
    idcams.define_cluster("PROD.FILE", IDCAMS::VsamOrg::KSDS, 80);
    idcams.define_cluster("TEST.FILE", IDCAMS::VsamOrg::KSDS, 80);
    auto result = idcams.listcat("PROD");
    CHECK(result.output_lines.size() == 1);
}

TEST_CASE("IDCAMS: alter record size") {
    IDCAMS idcams;
    idcams.define_cluster("ALT.FILE", IDCAMS::VsamOrg::KSDS, 80);
    auto result = idcams.alter("ALT.FILE", {{"RECSZ", "200"}});
    CHECK(result.rc == 0);
}

TEST_CASE("IDCAMS: entry_count") {
    IDCAMS idcams;
    CHECK(idcams.entry_count() == 0);
    idcams.define_cluster("A", IDCAMS::VsamOrg::KSDS, 80);
    idcams.define_cluster("B", IDCAMS::VsamOrg::KSDS, 80);
    CHECK(idcams.entry_count() == 2);
}

TEST_CASE("SortStub: simple character sort ascending") {
    std::vector<std::string> records = {"CHARLIE", "ALPHA  ", "BRAVO  "};
    SortStub::Config config;
    config.fields.push_back({1, 7, 'C', 'A'});
    auto result = SortStub::sort_records(records, config);
    CHECK(result.rc == 0);
    CHECK(records[0] == "ALPHA  ");
    CHECK(records[1] == "BRAVO  ");
    CHECK(records[2] == "CHARLIE");
}

TEST_CASE("SortStub: descending sort") {
    std::vector<std::string> records = {"AAA", "CCC", "BBB"};
    SortStub::Config config;
    config.fields.push_back({1, 3, 'C', 'D'});
    auto result = SortStub::sort_records(records, config);
    CHECK(result.rc == 0);
    CHECK(records[0] == "CCC");
    CHECK(records[1] == "BBB");
    CHECK(records[2] == "AAA");
}

TEST_CASE("SortStub: no sort fields returns error") {
    std::vector<std::string> records = {"A", "B"};
    SortStub::Config config;
    auto result = SortStub::sort_records(records, config);
    CHECK(result.rc == 16);
}

TEST_CASE("IKJEFT01: process TSO commands") {
    std::vector<std::string> commands = {
        "ALLOC DA('MY.FILE') NEW",
        "LISTDS 'MY.FILE'",
        "FREE DA('MY.FILE')"
    };
    auto result = IKJEFT01::execute(commands);
    CHECK(result.rc == 0);
    CHECK(result.output.size() == 3);
    CHECK(result.output[0].find("ALLOCATED") != std::string::npos);
}

TEST_CASE("IKJEFT01: unknown command returns RC=4") {
    std::vector<std::string> commands = {"XYZZY UNKNOWN"};
    auto result = IKJEFT01::execute(commands);
    CHECK(result.rc == 4);
}

TEST_CASE("IEBUPDTE: add new member") {
    IEBUPDTE::PDS pds;
    std::vector<IEBUPDTE::UpdateRecord> updates;
    IEBUPDTE::UpdateRecord rec;
    rec.func = IEBUPDTE::Function::NEW;
    rec.member_name = "NEWMEM";
    rec.data_lines = {"LINE1", "LINE2", "LINE3"};
    updates.push_back(rec);
    auto result = IEBUPDTE::apply(pds, updates);
    CHECK(result.rc == 0);
    CHECK(result.members_processed == 1);
    CHECK(pds.count("NEWMEM") == 1);
    CHECK(pds["NEWMEM"].find("LINE1") != std::string::npos);
}

TEST_CASE("IEBUPDTE: replace member") {
    IEBUPDTE::PDS pds = {{"MEM1", "OLD CONTENT\n"}};
    std::vector<IEBUPDTE::UpdateRecord> updates;
    IEBUPDTE::UpdateRecord rec;
    rec.func = IEBUPDTE::Function::REPL;
    rec.member_name = "MEM1";
    rec.data_lines = {"NEW CONTENT"};
    updates.push_back(rec);
    auto result = IEBUPDTE::apply(pds, updates);
    CHECK(result.rc == 0);
    CHECK(pds["MEM1"].find("NEW CONTENT") != std::string::npos);
    CHECK(pds["MEM1"].find("OLD CONTENT") == std::string::npos);
}

TEST_CASE("IEBUPDTE: parse control statements") {
    std::string sysin =
        "./ ADD NAME=MEMBER1\n"
        "DATA LINE 1\n"
        "DATA LINE 2\n"
        "./ ENDUP\n";
    auto updates = IEBUPDTE::parse_control(sysin);
    REQUIRE(updates.size() == 1);
    CHECK(updates[0].member_name == "MEMBER1");
    CHECK(updates[0].data_lines.size() == 2);
}

TEST_CASE("IEBUPDTE: parse multiple members") {
    std::string sysin =
        "./ ADD NAME=MEM1\n"
        "LINE1\n"
        "./ ADD NAME=MEM2\n"
        "LINE2\n"
        "./ ENDUP\n";
    auto updates = IEBUPDTE::parse_control(sysin);
    CHECK(updates.size() == 2);
    CHECK(updates[0].member_name == "MEM1");
    CHECK(updates[1].member_name == "MEM2");
}

TEST_CASE("IEBUPDTE: change existing member - insert") {
    IEBUPDTE::PDS pds = {{"MEM1", "LINE1\nLINE2\nLINE3\n"}};
    std::vector<IEBUPDTE::UpdateRecord> updates;
    IEBUPDTE::UpdateRecord rec;
    rec.func = IEBUPDTE::Function::CHANGE;
    rec.member_name = "MEM1";
    rec.data_lines = {"INSERTED"};
    rec.insert_after = 1;
    updates.push_back(rec);
    auto result = IEBUPDTE::apply(pds, updates);
    CHECK(result.rc == 0);
    CHECK(pds["MEM1"].find("INSERTED") != std::string::npos);
}
