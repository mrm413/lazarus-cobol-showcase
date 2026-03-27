#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

#include "lazarus/pipeline/detector.h"
#include "lazarus/pipeline/router.h"
#include "lazarus/pipeline/graph.h"
#include "lazarus/pipeline/assessment.h"
#include "lazarus/pipeline/orchestrator.h"
#include "lazarus/pipeline/exporter.h"

using namespace lazarus::pipeline;

// ============================================================================
// DETECTOR TESTS (35 tests)
// ============================================================================

// --- Extension-based detection ---

TEST_CASE("Detector: COBOL by .cbl extension") {
    auto d = detect("PAYROLL.cbl", "");
    CHECK(d.lang == SourceLang::COBOL);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: COBOL by .cob extension") {
    auto d = detect("ACCOUNTS.cob", "");
    CHECK(d.lang == SourceLang::COBOL);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: JCL by .jcl extension") {
    auto d = detect("RUNJOB.jcl", "");
    CHECK(d.lang == SourceLang::JCL);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: PLI by .pli extension") {
    auto d = detect("REPORT.pli", "");
    CHECK(d.lang == SourceLang::PLI);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: REXX by .rexx extension") {
    auto d = detect("UTIL.rexx", "");
    CHECK(d.lang == SourceLang::REXX);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: HLASM by .asm extension") {
    auto d = detect("SVCHANDL.asm", "");
    CHECK(d.lang == SourceLang::HLASM);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: Easytrieve by .ezt extension") {
    auto d = detect("DAILY.ezt", "");
    CHECK(d.lang == SourceLang::Easytrieve);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: SAS by .sas extension") {
    auto d = detect("ANALYSIS.sas", "");
    CHECK(d.lang == SourceLang::SAS);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: DB2DDL by .ddl extension") {
    auto d = detect("SCHEMA.ddl", "");
    CHECK(d.lang == SourceLang::DB2DDL);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: DB2DDL by .sql extension") {
    auto d = detect("CREATE_TABLES.sql", "");
    CHECK(d.lang == SourceLang::DB2DDL);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: SortControl by .sort extension") {
    auto d = detect("SORTJOB.sort", "");
    CHECK(d.lang == SourceLang::SortControl);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: Copybook by .cpy extension") {
    auto d = detect("CUSTOMER.cpy", "");
    CHECK(d.lang == SourceLang::Copybook);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: BMSMap by .bms extension") {
    auto d = detect("MENUMAP.bms", "");
    CHECK(d.lang == SourceLang::BMSMap);
    CHECK(d.confidence > 0.7f);
}

TEST_CASE("Detector: RACF by filename pattern") {
    auto d = detect("RACF_UNLOAD.dat", "");
    CHECK(d.lang == SourceLang::RACFUnload);
}

// --- Content-based detection ---

TEST_CASE("Detector: COBOL by content - divisions") {
    std::string content =
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. HELLO.\n"
        "       PROCEDURE DIVISION.\n"
        "           DISPLAY 'HELLO WORLD'.\n"
        "           STOP RUN.\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::COBOL);
    CHECK(d.confidence > 0.3f);
}

TEST_CASE("Detector: COBOL by content - PIC and PERFORM") {
    std::string content =
        "       01 WS-NAME PIC X(30).\n"
        "       01 WS-AGE  PIC 99.\n"
        "       PERFORM PROCESS-RECORD UNTIL EOF.\n"
        "       MOVE SPACES TO WS-NAME.\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::COBOL);
}

TEST_CASE("Detector: JCL by content - JOB/EXEC/DD") {
    std::string content =
        "//MYJOB    JOB (ACCT),'BATCH RUN',CLASS=A\n"
        "//STEP1    EXEC PGM=IEFBR14\n"
        "//SYSPRINT DD SYSOUT=*\n"
        "//INPUT    DD DSN=MY.DATA.SET,DISP=SHR\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::JCL);
    CHECK(d.confidence > 0.3f);
}

TEST_CASE("Detector: PLI by content - PROCEDURE OPTIONS") {
    std::string content =
        "HELLO: PROCEDURE OPTIONS(MAIN);\n"
        "  DCL NAME CHAR(30);\n"
        "  DCL AGE FIXED BIN(31);\n"
        "  PUT LIST('HELLO');\n"
        "END HELLO;\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::PLI);
    CHECK(d.confidence > 0.3f);
}

TEST_CASE("Detector: REXX by content - REXX comment") {
    std::string content =
        "/* REXX */\n"
        "SAY 'Hello World'\n"
        "PARSE ARG input_val\n"
        "DO i = 1 TO 10\n"
        "  SAY i\n"
        "END\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::REXX);
    CHECK(d.confidence > 0.3f);
}

TEST_CASE("Detector: Easytrieve by content") {
    std::string content =
        "FILE INFILE\n"
        "  RECORD INREC\n"
        "    FIELD NAME 1 30 A\n"
        "    FIELD AGE 31 3 N\n"
        "JOB INPUT INFILE\n"
        "  PRINT INREC\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::Easytrieve);
}

TEST_CASE("Detector: SAS by content") {
    std::string content =
        "DATA work.output;\n"
        "  SET work.input;\n"
        "  IF age > 18;\n"
        "RUN;\n"
        "PROC PRINT DATA=work.output;\n"
        "RUN;\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::SAS);
}

TEST_CASE("Detector: DB2DDL by content") {
    std::string content =
        "CREATE TABLE CUSTOMER (\n"
        "  CUST_ID INTEGER NOT NULL,\n"
        "  NAME VARCHAR(100),\n"
        "  PRIMARY KEY (CUST_ID)\n"
        ");\n"
        "CREATE INDEX IX_CUST ON CUSTOMER(NAME);\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::DB2DDL);
}

TEST_CASE("Detector: Sort control by content") {
    std::string content =
        " SORT FIELDS=(1,8,CH,A,9,5,ZD,D)\n"
        " INCLUDE COND=(15,3,CH,EQ,C'YES')\n"
        " OUTREC FIELDS=(1,20)\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::SortControl);
}

TEST_CASE("Detector: HLASM by content") {
    std::string content =
        "MYPROG   CSECT\n"
        "         USING *,15\n"
        "         BALR  12,0\n"
        "         LA    1,MSGAREA\n"
        "         DC    C'HELLO'\n"
        "         DS    CL80\n"
        "         END   MYPROG\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::HLASM);
}

TEST_CASE("Detector: RACF by content") {
    std::string content =
        "0100 USER1    USBD\n"
        "0200 GROUP1   GRBD\n"
        "0400 DATASET1 DSBD\n"
        "0100 USER2    USBD\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::RACFUnload);
}

TEST_CASE("Detector: BMS map by content") {
    std::string content =
        "MENUMAP  DFHMSD TYPE=MAP,MODE=INOUT,LANG=COBOL\n"
        "MENU01   DFHMDI SIZE=(24,80)\n"
        "TITLE    DFHMDF POS=(1,20),LENGTH=20,ATTRB=ASKIP,INITIAL='MENU'\n"
        "         DFHMSD TYPE=FINAL\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::BMSMap);
}

TEST_CASE("Detector: Copybook by content - PIC without divisions") {
    std::string content =
        "       01 CUSTOMER-RECORD.\n"
        "          05 CUST-ID    PIC 9(8).\n"
        "          05 CUST-NAME  PIC X(30).\n"
        "          05 CUST-ADDR  PIC X(50).\n";
    auto d = detect("UNKNOWN.txt", content);
    CHECK(d.lang == SourceLang::Copybook);
}

// --- Confidence scoring ---

TEST_CASE("Detector: extension + content agreement boosts confidence") {
    std::string content =
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n";
    auto with_ext = detect("TEST.cbl", content);
    auto without_ext = detect("TEST.txt", content);
    CHECK(with_ext.confidence > without_ext.confidence);
}

TEST_CASE("Detector: empty content uses extension only") {
    auto d = detect("TEST.cbl", "");
    CHECK(d.lang == SourceLang::COBOL);
    CHECK(d.confidence >= 0.8f);
}

TEST_CASE("Detector: unknown file with no content") {
    auto d = detect("RANDOM.xyz", "");
    CHECK(d.lang == SourceLang::Unknown);
    CHECK(d.confidence == 0.0f);
}

TEST_CASE("Detector: confidence range 0-1") {
    std::string content =
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. HELLO.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 WS-VAR PIC X(10).\n"
        "       PROCEDURE DIVISION.\n"
        "           DISPLAY 'HELLO'.\n"
        "           STOP RUN.\n";
    auto d = detect("HELLO.cbl", content);
    CHECK(d.confidence >= 0.0f);
    CHECK(d.confidence <= 1.0f);
}

// --- Batch detection ---

TEST_CASE("Detector: batch detection with multiple files") {
    std::vector<FileEntry> files = {
        {"PAYROLL.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. PAYROLL.\n"},
        {"RUNJOB.jcl", "//MYJOB JOB (ACCT),'RUN'\n//STEP1 EXEC PGM=PAYROLL\n"},
        {"UTILS.rexx", "/* REXX */\nSAY 'HELLO'\n"},
        {"UNKNOWN.xyz", "random garbage content\n"}
    };
    auto results = detect_batch(files);
    CHECK(results.size() == 4);
    CHECK(results[0].lang == SourceLang::COBOL);
    CHECK(results[1].lang == SourceLang::JCL);
    CHECK(results[2].lang == SourceLang::REXX);
}

TEST_CASE("Detector: batch detection empty input") {
    std::vector<FileEntry> files;
    auto results = detect_batch(files);
    CHECK(results.empty());
}

TEST_CASE("Detector: source_lang_name covers all values") {
    CHECK(std::string(source_lang_name(SourceLang::COBOL)) == "COBOL");
    CHECK(std::string(source_lang_name(SourceLang::HLASM)) == "HLASM");
    CHECK(std::string(source_lang_name(SourceLang::JCL)) == "JCL");
    CHECK(std::string(source_lang_name(SourceLang::PLI)) == "PL/I");
    CHECK(std::string(source_lang_name(SourceLang::REXX)) == "REXX");
    CHECK(std::string(source_lang_name(SourceLang::Easytrieve)) == "Easytrieve");
    CHECK(std::string(source_lang_name(SourceLang::SAS)) == "SAS");
    CHECK(std::string(source_lang_name(SourceLang::DB2DDL)) == "DB2 DDL");
    CHECK(std::string(source_lang_name(SourceLang::SortControl)) == "Sort Control");
    CHECK(std::string(source_lang_name(SourceLang::RACFUnload)) == "RACF Unload");
    CHECK(std::string(source_lang_name(SourceLang::TWSSchedule)) == "TWS Schedule");
    CHECK(std::string(source_lang_name(SourceLang::CA7Schedule)) == "CA7 Schedule");
    CHECK(std::string(source_lang_name(SourceLang::Copybook)) == "Copybook");
    CHECK(std::string(source_lang_name(SourceLang::BMSMap)) == "BMS Map");
    CHECK(std::string(source_lang_name(SourceLang::Unknown)) == "Unknown");
}

TEST_CASE("Detector: TWS schedule by filename") {
    auto d = detect("TWS_SCHED.dat", "");
    CHECK(d.lang == SourceLang::TWSSchedule);
}

TEST_CASE("Detector: CA7 schedule by filename") {
    auto d = detect("CA7_JOBS.dat", "");
    CHECK(d.lang == SourceLang::CA7Schedule);
}

// ============================================================================
// ROUTER TESTS (22 tests)
// ============================================================================

TEST_CASE("Router: default handler for unregistered language") {
    Router r;
    TranspileRequest req{SourceLang::COBOL, "test.cbl", "IDENTIFICATION DIVISION."};
    auto result = r.route(req);
    CHECK(result.success == true);
    CHECK(result.output.find("Placeholder") != std::string::npos);
    CHECK(result.lang == SourceLang::COBOL);
}

TEST_CASE("Router: register custom handler") {
    Router r;
    r.register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        return {true, "// Transpiled: " + req.filename, "", req.lang, req.filename};
    });
    CHECK(r.has_handler(SourceLang::COBOL));
    TranspileRequest req{SourceLang::COBOL, "test.cbl", "content"};
    auto result = r.route(req);
    CHECK(result.success == true);
    CHECK(result.output == "// Transpiled: test.cbl");
}

TEST_CASE("Router: unknown language returns error") {
    Router r;
    TranspileRequest req{SourceLang::Unknown, "mystery.dat", "???"};
    auto result = r.route(req);
    CHECK(result.success == false);
    CHECK(!result.error.empty());
}

TEST_CASE("Router: batch routing") {
    Router r;
    std::vector<TranspileRequest> reqs = {
        {SourceLang::COBOL, "a.cbl", "content a"},
        {SourceLang::JCL, "b.jcl", "content b"},
        {SourceLang::PLI, "c.pli", "content c"}
    };
    auto results = r.route_batch(reqs);
    CHECK(results.size() == 3);
    CHECK(results[0].lang == SourceLang::COBOL);
    CHECK(results[1].lang == SourceLang::JCL);
    CHECK(results[2].lang == SourceLang::PLI);
}

TEST_CASE("Router: batch routing preserves filenames") {
    Router r;
    std::vector<TranspileRequest> reqs = {
        {SourceLang::COBOL, "PAYROLL.cbl", ""},
        {SourceLang::JCL, "RUNJOB.jcl", ""}
    };
    auto results = r.route_batch(reqs);
    CHECK(results[0].filename == "PAYROLL.cbl");
    CHECK(results[1].filename == "RUNJOB.jcl");
}

TEST_CASE("Router: statistics tracking - total count") {
    Router r;
    r.route({SourceLang::COBOL, "a.cbl", ""});
    r.route({SourceLang::COBOL, "b.cbl", ""});
    r.route({SourceLang::JCL, "c.jcl", ""});
    CHECK(r.stats().total_routed == 3);
}

TEST_CASE("Router: statistics tracking - per language") {
    Router r;
    r.route({SourceLang::COBOL, "a.cbl", ""});
    r.route({SourceLang::COBOL, "b.cbl", ""});
    r.route({SourceLang::JCL, "c.jcl", ""});
    CHECK(r.stats().count_for(SourceLang::COBOL) == 2);
    CHECK(r.stats().count_for(SourceLang::JCL) == 1);
}

TEST_CASE("Router: statistics tracking - success rate") {
    Router r;
    r.register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        return {true, "ok", "", req.lang, req.filename};
    });
    r.route({SourceLang::COBOL, "a.cbl", ""});
    r.route({SourceLang::Unknown, "x.dat", ""}); // will fail
    CHECK(r.stats().total_success == 1);
    CHECK(r.stats().total_failure == 1);
    CHECK(r.stats().success_rate() == doctest::Approx(0.5f));
}

TEST_CASE("Router: success rate per language") {
    Router r;
    r.register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        return {true, "ok", "", req.lang, req.filename};
    });
    r.route({SourceLang::COBOL, "a.cbl", ""});
    r.route({SourceLang::COBOL, "b.cbl", ""});
    CHECK(r.stats().success_rate_for(SourceLang::COBOL) == doctest::Approx(1.0f));
}

TEST_CASE("Router: reset statistics") {
    Router r;
    r.route({SourceLang::COBOL, "a.cbl", ""});
    r.reset_stats();
    CHECK(r.stats().total_routed == 0);
}

TEST_CASE("Router: registered_languages returns correct set") {
    Router r;
    r.register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        return {true, "", "", req.lang, req.filename};
    });
    r.register_handler(SourceLang::JCL, [](const TranspileRequest& req) -> TranspileResult {
        return {true, "", "", req.lang, req.filename};
    });
    auto langs = r.registered_languages();
    CHECK(langs.size() == 2);
}

TEST_CASE("Router: has_handler false for unregistered") {
    Router r;
    CHECK(r.has_handler(SourceLang::COBOL) == false);
}

TEST_CASE("Router: handler can return failure") {
    Router r;
    r.register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        return {false, "", "Transpilation failed", req.lang, req.filename};
    });
    auto result = r.route({SourceLang::COBOL, "bad.cbl", "bad content"});
    CHECK(result.success == false);
    CHECK(result.error == "Transpilation failed");
}

TEST_CASE("Router: unknown lang error includes suggestion") {
    Router r;
    auto result = r.route({SourceLang::Unknown, "file.xyz", "data"});
    CHECK(result.error.find("Suggestion") != std::string::npos);
}

TEST_CASE("Router: batch routing with mixed success") {
    Router r;
    r.register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        return {true, "ok", "", req.lang, req.filename};
    });
    std::vector<TranspileRequest> reqs = {
        {SourceLang::COBOL, "a.cbl", ""},
        {SourceLang::Unknown, "x.dat", ""},
        {SourceLang::COBOL, "b.cbl", ""}
    };
    auto results = r.route_batch(reqs);
    CHECK(results[0].success == true);
    CHECK(results[1].success == false);
    CHECK(results[2].success == true);
}

TEST_CASE("Router: default handler output includes filename") {
    Router r;
    auto result = r.route({SourceLang::JCL, "MYJOB.jcl", "//JOB\n"});
    CHECK(result.output.find("MYJOB.jcl") != std::string::npos);
}

TEST_CASE("Router: default handler output includes language name") {
    Router r;
    auto result = r.route({SourceLang::SAS, "analysis.sas", "DATA;\nRUN;\n"});
    CHECK(result.output.find("SAS") != std::string::npos);
}

TEST_CASE("Router: overwrite handler for same language") {
    Router r;
    r.register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        return {true, "v1", "", req.lang, req.filename};
    });
    r.register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        return {true, "v2", "", req.lang, req.filename};
    });
    auto result = r.route({SourceLang::COBOL, "t.cbl", ""});
    CHECK(result.output == "v2");
}

TEST_CASE("Router: empty batch returns empty") {
    Router r;
    auto results = r.route_batch({});
    CHECK(results.empty());
}

TEST_CASE("Router: count_for untracked language returns 0") {
    Router r;
    CHECK(r.stats().count_for(SourceLang::HLASM) == 0);
}

TEST_CASE("Router: success_rate_for untracked language returns 0") {
    Router r;
    CHECK(r.stats().success_rate_for(SourceLang::PLI) == 0.0f);
}

TEST_CASE("Router: success_rate with no routes returns 0") {
    Router r;
    CHECK(r.stats().success_rate() == 0.0f);
}

// ============================================================================
// GRAPH TESTS (32 tests)
// ============================================================================

TEST_CASE("Graph: add single node") {
    DependencyGraph g;
    g.add_node("A");
    CHECK(g.node_count() == 1);
    CHECK(g.has_node("A"));
}

TEST_CASE("Graph: add multiple nodes") {
    DependencyGraph g;
    g.add_node("A");
    g.add_node("B");
    g.add_node("C");
    CHECK(g.node_count() == 3);
}

TEST_CASE("Graph: add edge creates nodes") {
    DependencyGraph g;
    g.add_edge("A", "B");
    CHECK(g.node_count() == 2);
    CHECK(g.edge_count() == 1);
}

TEST_CASE("Graph: duplicate edge not added") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("A", "B");
    CHECK(g.edge_count() == 1);
}

TEST_CASE("Graph: dependencies returns correct list") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("A", "C");
    auto deps = g.dependencies("A");
    CHECK(deps.size() == 2);
    CHECK(std::find(deps.begin(), deps.end(), "B") != deps.end());
    CHECK(std::find(deps.begin(), deps.end(), "C") != deps.end());
}

TEST_CASE("Graph: dependents returns correct list") {
    DependencyGraph g;
    g.add_edge("A", "C");
    g.add_edge("B", "C");
    auto deps = g.dependents("C");
    CHECK(deps.size() == 2);
    CHECK(std::find(deps.begin(), deps.end(), "A") != deps.end());
    CHECK(std::find(deps.begin(), deps.end(), "B") != deps.end());
}

TEST_CASE("Graph: fan_in counts incoming edges") {
    DependencyGraph g;
    g.add_edge("A", "C");
    g.add_edge("B", "C");
    g.add_edge("D", "C");
    CHECK(g.fan_in("C") == 3);
    CHECK(g.fan_in("A") == 0);
}

TEST_CASE("Graph: fan_out counts outgoing edges") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("A", "C");
    g.add_edge("A", "D");
    CHECK(g.fan_out("A") == 3);
    CHECK(g.fan_out("B") == 0);
}

TEST_CASE("Graph: topological sort - linear chain") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("B", "C");
    auto sorted = g.topological_sort();
    CHECK(sorted.size() == 3);
    // A must come before B, B before C
    auto pos_a = std::find(sorted.begin(), sorted.end(), "A") - sorted.begin();
    auto pos_b = std::find(sorted.begin(), sorted.end(), "B") - sorted.begin();
    auto pos_c = std::find(sorted.begin(), sorted.end(), "C") - sorted.begin();
    CHECK(pos_a < pos_b);
    CHECK(pos_b < pos_c);
}

TEST_CASE("Graph: topological sort - diamond") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("A", "C");
    g.add_edge("B", "D");
    g.add_edge("C", "D");
    auto sorted = g.topological_sort();
    CHECK(sorted.size() == 4);
    auto pos_a = std::find(sorted.begin(), sorted.end(), "A") - sorted.begin();
    auto pos_d = std::find(sorted.begin(), sorted.end(), "D") - sorted.begin();
    CHECK(pos_a < pos_d);
}

TEST_CASE("Graph: topological sort - empty graph") {
    DependencyGraph g;
    auto sorted = g.topological_sort();
    CHECK(sorted.empty());
}

TEST_CASE("Graph: topological sort - single node") {
    DependencyGraph g;
    g.add_node("ALONE");
    auto sorted = g.topological_sort();
    CHECK(sorted.size() == 1);
    CHECK(sorted[0] == "ALONE");
}

TEST_CASE("Graph: topological sort - disconnected nodes") {
    DependencyGraph g;
    g.add_node("X");
    g.add_node("Y");
    g.add_node("Z");
    auto sorted = g.topological_sort();
    CHECK(sorted.size() == 3);
}

TEST_CASE("Graph: cycle detection - no cycle") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("B", "C");
    auto cycle = g.detect_cycle();
    CHECK(!cycle.has_value());
}

TEST_CASE("Graph: cycle detection - self loop") {
    DependencyGraph g;
    g.add_edge("A", "A");
    auto cycle = g.detect_cycle();
    CHECK(cycle.has_value());
}

TEST_CASE("Graph: cycle detection - two-node cycle") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("B", "A");
    auto cycle = g.detect_cycle();
    CHECK(cycle.has_value());
    CHECK(cycle->size() >= 2);
}

TEST_CASE("Graph: cycle detection - three-node cycle") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("B", "C");
    g.add_edge("C", "A");
    auto cycle = g.detect_cycle();
    CHECK(cycle.has_value());
    CHECK(cycle->size() >= 3);
}

TEST_CASE("Graph: connected components - single component") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("B", "C");
    auto cc = g.connected_components();
    CHECK(cc.size() == 1);
    CHECK(cc[0].size() == 3);
}

TEST_CASE("Graph: connected components - two components") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("C", "D");
    auto cc = g.connected_components();
    CHECK(cc.size() == 2);
}

TEST_CASE("Graph: connected components - isolated node") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_node("C");
    auto cc = g.connected_components();
    CHECK(cc.size() == 2);
}

TEST_CASE("Graph: strongly connected components - DAG") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("B", "C");
    auto scc = g.strongly_connected_components();
    CHECK(scc.size() == 3); // each node is its own SCC
}

TEST_CASE("Graph: strongly connected components - cycle") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("B", "C");
    g.add_edge("C", "A");
    auto scc = g.strongly_connected_components();
    // All three form one SCC
    bool found = false;
    for (const auto& comp : scc) {
        if (comp.size() == 3) found = true;
    }
    CHECK(found);
}

TEST_CASE("Graph: strongly connected components - mixed") {
    DependencyGraph g;
    g.add_edge("A", "B");
    g.add_edge("B", "A"); // A-B cycle
    g.add_edge("B", "C"); // C is separate
    auto scc = g.strongly_connected_components();
    CHECK(scc.size() == 2);
}

TEST_CASE("Graph: has_node false for nonexistent") {
    DependencyGraph g;
    g.add_node("A");
    CHECK(!g.has_node("B"));
}

TEST_CASE("Graph: fan_in for nonexistent node returns 0") {
    DependencyGraph g;
    CHECK(g.fan_in("NONE") == 0);
}

TEST_CASE("Graph: fan_out for nonexistent node returns 0") {
    DependencyGraph g;
    CHECK(g.fan_out("NONE") == 0);
}

// --- Dependency extraction ---

TEST_CASE("Graph: extract COBOL COPY dependency") {
    auto deps = dep_extract::extract_dependencies("MAIN.cbl",
        "       COPY CUSTOMER.\n       COPY ACCOUNT.\n", SourceLang::COBOL);
    CHECK(deps.size() == 2);
    CHECK(deps[0].type == "COPY");
    CHECK(deps[0].target == "CUSTOMER");
    CHECK(deps[1].target == "ACCOUNT");
}

TEST_CASE("Graph: extract COBOL CALL dependency") {
    auto deps = dep_extract::extract_dependencies("MAIN.cbl",
        "           CALL 'SUBPROG' USING WS-DATA.\n", SourceLang::COBOL);
    CHECK(deps.size() >= 1);
    bool found_call = false;
    for (const auto& d : deps) {
        if (d.type == "CALL" && d.target == "SUBPROG") found_call = true;
    }
    CHECK(found_call);
}

TEST_CASE("Graph: extract JCL EXEC PGM dependency") {
    auto deps = dep_extract::extract_dependencies("RUN.jcl",
        "//STEP1    EXEC PGM=PAYROLL\n", SourceLang::JCL);
    CHECK(deps.size() >= 1);
    CHECK(deps[0].type == "EXEC PGM");
    CHECK(deps[0].target == "PAYROLL");
}

TEST_CASE("Graph: extract JCL PROC dependency") {
    auto deps = dep_extract::extract_dependencies("RUN.jcl",
        "//STEP1    EXEC PROC=MYPROC\n", SourceLang::JCL);
    CHECK(deps.size() >= 1);
    bool found = false;
    for (const auto& d : deps) {
        if (d.type == "PROC" && d.target == "MYPROC") found = true;
    }
    CHECK(found);
}

TEST_CASE("Graph: extract PLI INCLUDE dependency") {
    auto deps = dep_extract::extract_dependencies("MAIN.pli",
        "%INCLUDE CUSTDEF;\n%INCLUDE DATEUTIL;\n", SourceLang::PLI);
    CHECK(deps.size() == 2);
    CHECK(deps[0].target == "CUSTDEF");
    CHECK(deps[1].target == "DATEUTIL");
}

TEST_CASE("Graph: populate_graph from files") {
    DependencyGraph g;
    std::vector<std::pair<std::string, std::string>> files = {
        {"MAIN.cbl", "       COPY CUSTOMER.\n       CALL 'UTILS'.\n"},
        {"CUSTOMER.cpy", "       01 CUST-REC PIC X(80).\n"}
    };
    std::vector<SourceLang> langs = {SourceLang::COBOL, SourceLang::Copybook};
    dep_extract::populate_graph(g, files, langs);
    CHECK(g.has_node("MAIN.cbl"));
    CHECK(g.has_node("CUSTOMER.cpy"));
    CHECK(g.fan_out("MAIN.cbl") >= 1);
}

// ============================================================================
// ASSESSMENT TESTS (26 tests)
// ============================================================================

TEST_CASE("Assessment: LOC excludes blank lines") {
    std::string content = "LINE1\n\nLINE3\n\n\nLINE6\n";
    auto m = metrics::compute_metrics(content, SourceLang::Unknown);
    CHECK(m.loc == 3);
    CHECK(m.blank_lines == 3);
}

TEST_CASE("Assessment: LOC excludes COBOL comments") {
    std::string content =
        "000100 IDENTIFICATION DIVISION.\n"
        "000200*THIS IS A COMMENT\n"
        "000300 PROGRAM-ID. TEST.\n";
    auto m = metrics::compute_metrics(content, SourceLang::COBOL);
    CHECK(m.comment_lines == 1);
    CHECK(m.loc == 2);
}

TEST_CASE("Assessment: LOC excludes JCL comments") {
    std::string content =
        "//MYJOB JOB\n"
        "//* THIS IS A COMMENT\n"
        "//STEP1 EXEC PGM=IEFBR14\n";
    auto m = metrics::compute_metrics(content, SourceLang::JCL);
    CHECK(m.comment_lines == 1);
    CHECK(m.loc == 2);
}

TEST_CASE("Assessment: McCabe base complexity is 1") {
    auto m = metrics::compute_metrics("DISPLAY 'HELLO'.\n", SourceLang::COBOL);
    CHECK(m.mccabe >= 1);
}

TEST_CASE("Assessment: McCabe increments on IF") {
    std::string content =
        "       IF WS-A > 10\n"
        "           DISPLAY 'BIG'\n"
        "       END-IF.\n"
        "       IF WS-B > 20\n"
        "           DISPLAY 'BIGGER'\n"
        "       END-IF.\n";
    auto m = metrics::compute_metrics(content, SourceLang::COBOL);
    CHECK(m.mccabe >= 3); // 1 base + 2 IFs
}

TEST_CASE("Assessment: McCabe for EVALUATE/WHEN") {
    std::string content =
        "       EVALUATE WS-CODE\n"
        "           WHEN 'A'\n"
        "               DISPLAY 'ALPHA'\n"
        "           WHEN 'B'\n"
        "               DISPLAY 'BETA'\n"
        "           WHEN OTHER\n"
        "               DISPLAY 'OTHER'\n"
        "       END-EVALUATE.\n";
    auto m = metrics::compute_metrics(content, SourceLang::COBOL);
    CHECK(m.mccabe >= 4); // 1 base + 1 EVALUATE + 3 WHEN (one has 'OTHER')
}

TEST_CASE("Assessment: McCabe for JCL IF/COND") {
    std::string content =
        "//STEP1 EXEC PGM=PROG1\n"
        "// IF RC > 4 THEN\n"
        "//STEP2 EXEC PGM=PROG2,COND=(4,LT)\n"
        "// ENDIF\n";
    auto m = metrics::compute_metrics(content, SourceLang::JCL);
    CHECK(m.mccabe >= 3); // 1 + IF + COND
}

TEST_CASE("Assessment: risk score for simple program") {
    ComplexityMetrics m;
    m.loc = 50;
    m.mccabe = 3;
    float score = compute_risk_score(m, 0, 0, 0, 0, 0);
    // 50 * 0.1 + 3 * 2.0 = 5.0 + 6.0 = 11.0
    CHECK(score == doctest::Approx(11.0f));
}

TEST_CASE("Assessment: risk score with EXEC CICS") {
    ComplexityMetrics m;
    m.loc = 100;
    m.mccabe = 5;
    float score = compute_risk_score(m, 2, 0, 0, 0, 0);
    // 100*0.1 + 5*2 + 2*50 = 10 + 10 + 100 = 120
    CHECK(score == doctest::Approx(120.0f));
}

TEST_CASE("Assessment: risk score with EXEC SQL") {
    ComplexityMetrics m;
    m.loc = 100;
    m.mccabe = 5;
    float score = compute_risk_score(m, 0, 3, 0, 0, 0);
    // 100*0.1 + 5*2 + 3*30 = 10 + 10 + 90 = 110
    CHECK(score == doctest::Approx(110.0f));
}

TEST_CASE("Assessment: risk score with CALLs") {
    ComplexityMetrics m;
    m.loc = 100;
    m.mccabe = 5;
    float score = compute_risk_score(m, 0, 0, 4, 0, 0);
    // 100*0.1 + 5*2 + 4*10 = 10 + 10 + 40 = 60
    CHECK(score == doctest::Approx(60.0f));
}

TEST_CASE("Assessment: risk score with fan-in") {
    ComplexityMetrics m;
    m.loc = 100;
    m.mccabe = 5;
    float score = compute_risk_score(m, 0, 0, 0, 10, 0);
    // 100*0.1 + 5*2 + 10*5 = 10 + 10 + 50 = 70
    CHECK(score == doctest::Approx(70.0f));
}

TEST_CASE("Assessment: risk category LOW") {
    CHECK(categorize_risk(0.0f) == RiskCategory::LOW);
    CHECK(categorize_risk(24.9f) == RiskCategory::LOW);
}

TEST_CASE("Assessment: risk category MEDIUM") {
    CHECK(categorize_risk(25.0f) == RiskCategory::MEDIUM);
    CHECK(categorize_risk(74.9f) == RiskCategory::MEDIUM);
}

TEST_CASE("Assessment: risk category HIGH") {
    CHECK(categorize_risk(75.0f) == RiskCategory::HIGH);
    CHECK(categorize_risk(149.9f) == RiskCategory::HIGH);
}

TEST_CASE("Assessment: risk category CRITICAL") {
    CHECK(categorize_risk(150.0f) == RiskCategory::CRITICAL);
    CHECK(categorize_risk(999.0f) == RiskCategory::CRITICAL);
}

TEST_CASE("Assessment: risk_category_name covers all") {
    CHECK(std::string(risk_category_name(RiskCategory::LOW)) == "LOW");
    CHECK(std::string(risk_category_name(RiskCategory::MEDIUM)) == "MEDIUM");
    CHECK(std::string(risk_category_name(RiskCategory::HIGH)) == "HIGH");
    CHECK(std::string(risk_category_name(RiskCategory::CRITICAL)) == "CRITICAL");
}

TEST_CASE("Assessment: assess_file produces valid assessment") {
    DependencyGraph g;
    g.add_node("test.cbl");
    std::string content =
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. TEST.\n"
        "       PROCEDURE DIVISION.\n"
        "           IF WS-A > 0\n"
        "               DISPLAY 'POS'\n"
        "           END-IF.\n"
        "           STOP RUN.\n";
    auto a = assess_file("test.cbl", content, SourceLang::COBOL, g);
    CHECK(a.filename == "test.cbl");
    CHECK(a.lang == SourceLang::COBOL);
    CHECK(a.metrics.loc > 0);
    CHECK(a.metrics.mccabe >= 2);
    CHECK(a.risk_score > 0.0f);
}

TEST_CASE("Assessment: assess_file counts EXEC CICS") {
    DependencyGraph g;
    std::string content =
        "           EXEC CICS SEND MAP('MENU')\n"
        "           END-EXEC.\n"
        "           EXEC CICS RECEIVE MAP('MENU')\n"
        "           END-EXEC.\n";
    auto a = assess_file("online.cbl", content, SourceLang::COBOL, g);
    CHECK(a.exec_cics_count == 2);
}

TEST_CASE("Assessment: assess_file counts EXEC SQL") {
    DependencyGraph g;
    std::string content =
        "           EXEC SQL SELECT NAME INTO :WS-NAME\n"
        "               FROM CUSTOMER WHERE ID = :WS-ID\n"
        "           END-EXEC.\n";
    auto a = assess_file("dbprog.cbl", content, SourceLang::COBOL, g);
    CHECK(a.exec_sql_count == 1);
}

TEST_CASE("Assessment: wave planning produces waves") {
    DependencyGraph g;
    g.add_edge("A.cbl", "B.cpy");
    g.add_node("C.cbl");

    std::vector<FileAssessment> assessments = {
        {"A.cbl", SourceLang::COBOL, {50, 55, 3, 2, 3, 10, 5, 50.0f}, 20.0f, RiskCategory::LOW, 0, 1, 0, 0, 0, {"B.cpy"}},
        {"B.cpy", SourceLang::Copybook, {10, 12, 1, 1, 1, 2, 1, 5.0f}, 3.0f, RiskCategory::LOW, 1, 0, 0, 0, 0, {}},
        {"C.cbl", SourceLang::COBOL, {200, 220, 10, 10, 8, 30, 20, 200.0f}, 80.0f, RiskCategory::HIGH, 0, 0, 0, 0, 0, {}}
    };

    auto waves = plan_waves(assessments, g);
    CHECK(waves.size() == 2); // Two connected components
}

TEST_CASE("Assessment: portfolio summary totals") {
    DependencyGraph g;
    g.add_node("A.cbl");
    g.add_node("B.cbl");

    std::vector<FileAssessment> assessments = {
        {"A.cbl", SourceLang::COBOL, {100, 110, 5, 5, 4, 15, 10, 100.0f}, 30.0f, RiskCategory::MEDIUM, 0, 0, 0, 0, 0, {}},
        {"B.cbl", SourceLang::COBOL, {200, 220, 10, 10, 8, 30, 20, 200.0f}, 50.0f, RiskCategory::MEDIUM, 0, 0, 0, 0, 0, {}}
    };

    auto summary = summarize(assessments, g);
    CHECK(summary.total_files == 2);
    CHECK(summary.total_loc == 300);
    CHECK(summary.avg_risk == doctest::Approx(40.0f));
    CHECK(summary.max_risk == doctest::Approx(50.0f));
}

TEST_CASE("Assessment: portfolio language counts") {
    DependencyGraph g;
    std::vector<FileAssessment> assessments = {
        {"A.cbl", SourceLang::COBOL, {}, 10.0f, RiskCategory::LOW, 0, 0, 0, 0, 0, {}},
        {"B.cbl", SourceLang::COBOL, {}, 20.0f, RiskCategory::LOW, 0, 0, 0, 0, 0, {}},
        {"C.jcl", SourceLang::JCL, {}, 15.0f, RiskCategory::LOW, 0, 0, 0, 0, 0, {}}
    };
    auto summary = summarize(assessments, g);
    CHECK(summary.lang_counts[static_cast<int>(SourceLang::COBOL)] == 2);
    CHECK(summary.lang_counts[static_cast<int>(SourceLang::JCL)] == 1);
}

TEST_CASE("Assessment: Halstead metrics computed") {
    std::string content =
        "       MOVE A TO B.\n"
        "       IF C > D\n"
        "           COMPUTE E = A + B\n"
        "       END-IF.\n"
        "       DISPLAY E.\n";
    auto m = metrics::compute_metrics(content, SourceLang::COBOL);
    CHECK(m.halstead_operators > 0);
    CHECK(m.halstead_operands > 0);
    CHECK(m.halstead_volume > 0.0f);
}

TEST_CASE("Assessment: estimated effort scales with LOC and risk") {
    DependencyGraph g;
    std::vector<FileAssessment> low_risk = {
        {"a.cbl", SourceLang::COBOL, {100, 110, 5, 5, 1, 5, 3, 50.0f}, 10.0f, RiskCategory::LOW, 0, 0, 0, 0, 0, {}}
    };
    std::vector<FileAssessment> high_risk = {
        {"b.cbl", SourceLang::COBOL, {100, 110, 5, 5, 1, 5, 3, 50.0f}, 100.0f, RiskCategory::HIGH, 0, 0, 0, 0, 0, {}}
    };
    auto s1 = summarize(low_risk, g);
    auto s2 = summarize(high_risk, g);
    CHECK(s2.estimated_effort_days > s1.estimated_effort_days);
}

// ============================================================================
// ORCHESTRATOR TESTS (16 tests)
// ============================================================================

TEST_CASE("Orchestrator: runs pipeline on empty input") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    auto state = orch.run({});
    CHECK(state.completed);
    CHECK(state.results.empty());
}

TEST_CASE("Orchestrator: detects languages in pipeline") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"PAYROLL.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. PAYROLL.\n       PROCEDURE DIVISION.\n           STOP RUN.\n"},
        {"RUNJOB.jcl", "//MYJOB JOB\n//STEP1 EXEC PGM=PAYROLL\n"}
    };
    auto state = orch.run(files);
    CHECK(state.results.size() == 2);
    CHECK(state.results[0].detection.has_value());
    CHECK(state.results[0].detection->lang == SourceLang::COBOL);
    CHECK(state.results[1].detection.has_value());
    CHECK(state.results[1].detection->lang == SourceLang::JCL);
}

TEST_CASE("Orchestrator: builds dependency graph") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"MAIN.cbl", "       COPY CUSTCOPY.\n       CALL 'SUBRTN'.\n"},
        {"CUSTCOPY.cpy", "       01 CUST-REC PIC X(80).\n"}
    };
    auto state = orch.run(files);
    CHECK(state.graph.node_count() >= 2);
}

TEST_CASE("Orchestrator: produces assessments") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"TEST.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. TEST.\n       PROCEDURE DIVISION.\n           DISPLAY 'HI'.\n           STOP RUN.\n"}
    };
    auto state = orch.run(files);
    CHECK(state.results[0].assessment.has_value());
    CHECK(state.results[0].assessment->metrics.loc > 0);
}

TEST_CASE("Orchestrator: produces transpile results") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"A.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. A.\n"}
    };
    auto state = orch.run(files);
    CHECK(state.results[0].transpile_result.has_value());
    CHECK(state.results[0].transpile_result->success);
}

TEST_CASE("Orchestrator: progress callback fires") {
    int callback_count = 0;
    PipelineConfig cfg;
    cfg.on_progress = [&](int, int, const std::string&, const std::string&) {
        ++callback_count;
    };
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"A.cbl", "       IDENTIFICATION DIVISION.\n"},
        {"B.jcl", "//JOB\n"}
    };
    orch.run(files);
    CHECK(callback_count > 0);
}

TEST_CASE("Orchestrator: language filter excludes files") {
    PipelineConfig cfg;
    cfg.language_filters = {SourceLang::COBOL}; // only COBOL
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"A.cbl", "       IDENTIFICATION DIVISION.\n"},
        {"B.jcl", "//JOB\n"}
    };
    auto state = orch.run(files);
    CHECK(state.completed);
    // JCL file should not have assessment
    bool jcl_assessed = state.results[1].assessment.has_value();
    CHECK(!jcl_assessed);
}

TEST_CASE("Orchestrator: custom handler used") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    orch.router().register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        return {true, "CUSTOM OUTPUT FOR " + req.filename, "", req.lang, req.filename};
    });
    std::vector<std::pair<std::string, std::string>> files = {
        {"A.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. A.\n"}
    };
    auto state = orch.run(files);
    CHECK(state.results[0].transpile_result.has_value());
    CHECK(state.results[0].transpile_result->output.find("CUSTOM") != std::string::npos);
}

TEST_CASE("Orchestrator: pipeline state completed flag") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    auto state = orch.run({{"X.cbl", "       IDENTIFICATION DIVISION.\n"}});
    CHECK(state.completed == true);
}

TEST_CASE("Orchestrator: pipeline summary generated") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"A.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. A.\n       PROCEDURE DIVISION.\n           STOP RUN.\n"},
        {"B.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. B.\n       PROCEDURE DIVISION.\n           STOP RUN.\n"}
    };
    auto state = orch.run(files);
    CHECK(state.summary.total_files == 2);
    CHECK(state.summary.total_loc > 0);
}

TEST_CASE("Orchestrator: state serialization round-trip") {
    PipelineState ps;
    ps.stage = "detect";
    ps.files_processed = 5;
    ps.files_total = 10;
    ps.completed = false;
    FileResult fr;
    fr.filename = "test.cbl";
    fr.success = true;
    ps.results.push_back(fr);

    auto serialized = ps.serialize();
    auto restored = PipelineState::deserialize(serialized);
    CHECK(restored.stage == "detect");
    CHECK(restored.files_processed == 5);
    CHECK(restored.files_total == 10);
    CHECK(restored.completed == false);
    CHECK(restored.results.size() == 1);
    CHECK(restored.results[0].filename == "test.cbl");
}

TEST_CASE("Orchestrator: error recovery continues pipeline") {
    PipelineConfig cfg;
    cfg.stop_on_error = false;
    Orchestrator orch(cfg);
    orch.router().register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        if (req.filename == "BAD.cbl")
            return {false, "", "compile error", req.lang, req.filename};
        return {true, "ok", "", req.lang, req.filename};
    });
    std::vector<std::pair<std::string, std::string>> files = {
        {"GOOD.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. GOOD.\n"},
        {"BAD.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. BAD.\n"},
        {"ALSO_GOOD.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. ALSO.\n"}
    };
    auto state = orch.run(files);
    CHECK(state.completed);
    // BAD file failed, but pipeline continued
    int failures = 0;
    for (const auto& r : state.results) {
        if (!r.success) ++failures;
    }
    CHECK(failures == 1);
}

TEST_CASE("Orchestrator: stage_status_name covers all") {
    CHECK(std::string(stage_status_name(StageStatus::PENDING)) == "PENDING");
    CHECK(std::string(stage_status_name(StageStatus::RUNNING)) == "RUNNING");
    CHECK(std::string(stage_status_name(StageStatus::SUCCESS)) == "SUCCESS");
    CHECK(std::string(stage_status_name(StageStatus::FAILED)) == "FAILED");
    CHECK(std::string(stage_status_name(StageStatus::SKIPPED)) == "SKIPPED");
}

TEST_CASE("Orchestrator: set_config updates configuration") {
    PipelineConfig cfg1;
    cfg1.stop_on_error = false;
    Orchestrator orch(cfg1);

    PipelineConfig cfg2;
    cfg2.stop_on_error = true;
    orch.set_config(cfg2);

    // Verify by running - if stop_on_error is true, it should halt on failure
    orch.router().register_handler(SourceLang::COBOL, [](const TranspileRequest& req) -> TranspileResult {
        return {false, "", "fail", req.lang, req.filename};
    });
    auto state = orch.run({{"A.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. A.\n"}});
    // With stop_on_error, the stage should be "error"
    CHECK(state.stage == "error");
}

TEST_CASE("Orchestrator: pipeline state accessor") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    orch.run({{"A.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. A.\n"}});
    const auto& s = orch.state();
    CHECK(s.completed);
}

TEST_CASE("Orchestrator: serialization with empty state") {
    PipelineState ps;
    auto s = ps.serialize();
    auto restored = PipelineState::deserialize(s);
    CHECK(restored.stage.empty());
    CHECK(restored.files_processed == 0);
    CHECK(restored.completed == false);
}

// ============================================================================
// EXPORTER TESTS (16 tests)
// ============================================================================

static PipelineState make_test_state() {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"PAYROLL.cbl",
         "       IDENTIFICATION DIVISION.\n"
         "       PROGRAM-ID. PAYROLL.\n"
         "       DATA DIVISION.\n"
         "       WORKING-STORAGE SECTION.\n"
         "       01 WS-AMT PIC 9(7)V99.\n"
         "       PROCEDURE DIVISION.\n"
         "           EXEC SQL SELECT AMT INTO :WS-AMT FROM PAY END-EXEC.\n"
         "           IF WS-AMT > 0\n"
         "               DISPLAY WS-AMT\n"
         "           END-IF.\n"
         "           STOP RUN.\n"},
        {"RUNJOB.jcl",
         "//PAYJOB  JOB (ACCT),'PAY RUN'\n"
         "//STEP1   EXEC PGM=PAYROLL\n"
         "//INPUT   DD DSN=PAY.DATA,DISP=SHR\n"}
    };
    return orch.run(files);
}

TEST_CASE("Exporter: JSON output is valid structure") {
    auto state = make_test_state();
    auto json = Exporter::export_json(state);
    CHECK(json.find("\"summary\"") != std::string::npos);
    CHECK(json.find("\"files\"") != std::string::npos);
    CHECK(json.find("\"dependency_graph\"") != std::string::npos);
    CHECK(json.find("\"migration_waves\"") != std::string::npos);
}

TEST_CASE("Exporter: JSON contains file data") {
    auto state = make_test_state();
    auto json = Exporter::export_json(state);
    CHECK(json.find("PAYROLL.cbl") != std::string::npos);
    CHECK(json.find("RUNJOB.jcl") != std::string::npos);
}

TEST_CASE("Exporter: JSON contains language info") {
    auto state = make_test_state();
    auto json = Exporter::export_json(state);
    CHECK(json.find("COBOL") != std::string::npos);
    CHECK(json.find("JCL") != std::string::npos);
}

TEST_CASE("Exporter: JSON contains risk data") {
    auto state = make_test_state();
    auto json = Exporter::export_json(state);
    CHECK(json.find("risk_score") != std::string::npos);
    CHECK(json.find("risk_category") != std::string::npos);
}

TEST_CASE("Exporter: JSON contains summary stats") {
    auto state = make_test_state();
    auto json = Exporter::export_json(state);
    CHECK(json.find("total_files") != std::string::npos);
    CHECK(json.find("total_loc") != std::string::npos);
    CHECK(json.find("estimated_effort_days") != std::string::npos);
}

TEST_CASE("Exporter: CSV has header row") {
    auto state = make_test_state();
    auto csv = Exporter::export_csv(state);
    CHECK(csv.find("filename,language,confidence,loc,mccabe,risk_score") != std::string::npos);
}

TEST_CASE("Exporter: CSV has data rows") {
    auto state = make_test_state();
    auto csv = Exporter::export_csv(state);
    CHECK(csv.find("PAYROLL.cbl") != std::string::npos);
    CHECK(csv.find("RUNJOB.jcl") != std::string::npos);
}

TEST_CASE("Exporter: CSV row count matches file count plus header") {
    auto state = make_test_state();
    auto csv = Exporter::export_csv(state);
    int newlines = 0;
    for (char c : csv) if (c == '\n') ++newlines;
    CHECK(newlines == 3); // header + 2 data rows
}

TEST_CASE("Exporter: HTML output has doctype") {
    auto state = make_test_state();
    auto html = Exporter::export_html(state);
    CHECK(html.find("<!DOCTYPE html>") != std::string::npos);
}

TEST_CASE("Exporter: HTML has dashboard cards") {
    auto state = make_test_state();
    auto html = Exporter::export_html(state);
    CHECK(html.find("Total Files") != std::string::npos);
    CHECK(html.find("Total LOC") != std::string::npos);
    CHECK(html.find("Avg Risk") != std::string::npos);
    CHECK(html.find("Est. Effort") != std::string::npos);
}

TEST_CASE("Exporter: HTML has file inventory table") {
    auto state = make_test_state();
    auto html = Exporter::export_html(state);
    CHECK(html.find("<table>") != std::string::npos);
    CHECK(html.find("PAYROLL.cbl") != std::string::npos);
    CHECK(html.find("RUNJOB.jcl") != std::string::npos);
}

TEST_CASE("Exporter: HTML has migration waves") {
    auto state = make_test_state();
    auto html = Exporter::export_html(state);
    CHECK(html.find("Migration Waves") != std::string::npos);
    CHECK(html.find("Wave") != std::string::npos);
}

TEST_CASE("Exporter: HTML has chart data divs") {
    auto state = make_test_state();
    auto html = Exporter::export_html(state);
    CHECK(html.find("lang-data") != std::string::npos);
    CHECK(html.find("risk-data") != std::string::npos);
    CHECK(html.find("dep-data") != std::string::npos);
}

TEST_CASE("Exporter: HTML has risk styling classes") {
    auto state = make_test_state();
    auto html = Exporter::export_html(state);
    // At least one risk class should appear
    bool has_risk_class = html.find("risk-low") != std::string::npos ||
                          html.find("risk-medium") != std::string::npos ||
                          html.find("risk-high") != std::string::npos ||
                          html.find("risk-critical") != std::string::npos;
    CHECK(has_risk_class);
}

TEST_CASE("Exporter: format selection via export_report") {
    auto state = make_test_state();
    auto json = Exporter::export_report(state, ExportFormat::JSON);
    auto csv = Exporter::export_report(state, ExportFormat::CSV);
    auto html = Exporter::export_report(state, ExportFormat::HTML);
    CHECK(json.find("\"summary\"") != std::string::npos);
    CHECK(csv.find("filename,") != std::string::npos);
    CHECK(html.find("<!DOCTYPE") != std::string::npos);
}

TEST_CASE("Exporter: export_format_name covers all") {
    CHECK(std::string(export_format_name(ExportFormat::JSON)) == "JSON");
    CHECK(std::string(export_format_name(ExportFormat::CSV)) == "CSV");
    CHECK(std::string(export_format_name(ExportFormat::HTML)) == "HTML");
}

// ============================================================================
// INTEGRATION TESTS (11 tests)
// ============================================================================

TEST_CASE("Integration: full pipeline detect-assess-graph-report") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"MAIN.cbl",
         "       IDENTIFICATION DIVISION.\n"
         "       PROGRAM-ID. MAIN.\n"
         "       DATA DIVISION.\n"
         "       WORKING-STORAGE SECTION.\n"
         "       COPY CUSTCOPY.\n"
         "       PROCEDURE DIVISION.\n"
         "           CALL 'SUBRTN' USING CUST-REC.\n"
         "           STOP RUN.\n"},
        {"CUSTCOPY.cpy",
         "       01 CUST-REC.\n"
         "          05 CUST-ID PIC 9(8).\n"
         "          05 CUST-NAME PIC X(30).\n"},
        {"SUBRTN.cbl",
         "       IDENTIFICATION DIVISION.\n"
         "       PROGRAM-ID. SUBRTN.\n"
         "       PROCEDURE DIVISION.\n"
         "           DISPLAY 'SUBROUTINE'.\n"
         "           STOP RUN.\n"},
        {"RUNJOB.jcl",
         "//MAINJOB JOB (ACCT),'DAILY RUN'\n"
         "//STEP1   EXEC PGM=MAIN\n"
         "//INPUT   DD DSN=CUST.DATA,DISP=SHR\n"
         "//OUTPUT  DD DSN=CUST.REPORT,DISP=(NEW,CATLG)\n"}
    };
    auto state = orch.run(files);
    CHECK(state.completed);
    CHECK(state.results.size() == 4);
}

TEST_CASE("Integration: dependency graph reflects COPY/CALL/PGM relationships") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"MAIN.cbl", "       COPY SHARED.\n       CALL 'HELPER'.\n"},
        {"SHARED.cpy", "       01 SHARED-DATA PIC X(10).\n"},
        {"RUNJOB.jcl", "//STEP1 EXEC PGM=MAIN\n"}
    };
    auto state = orch.run(files);
    CHECK(state.graph.fan_out("MAIN.cbl") >= 1);
    CHECK(state.graph.fan_out("RUNJOB.jcl") >= 1);
}

TEST_CASE("Integration: risk scoring reflects CICS complexity") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"SIMPLE.cbl",
         "       IDENTIFICATION DIVISION.\n"
         "       PROGRAM-ID. SIMPLE.\n"
         "       PROCEDURE DIVISION.\n"
         "           DISPLAY 'HI'.\n"
         "           STOP RUN.\n"},
        {"COMPLEX.cbl",
         "       IDENTIFICATION DIVISION.\n"
         "       PROGRAM-ID. COMPLEX.\n"
         "       PROCEDURE DIVISION.\n"
         "           EXEC CICS SEND MAP('M1') END-EXEC.\n"
         "           EXEC CICS RECEIVE MAP('M1') END-EXEC.\n"
         "           EXEC SQL SELECT X INTO :Y FROM T END-EXEC.\n"
         "           IF A > B\n"
         "               PERFORM P1\n"
         "           END-IF.\n"
         "           STOP RUN.\n"}
    };
    auto state = orch.run(files);
    float simple_risk = 0, complex_risk = 0;
    for (const auto& r : state.results) {
        if (r.assessment) {
            if (r.filename == "SIMPLE.cbl") simple_risk = r.assessment->risk_score;
            if (r.filename == "COMPLEX.cbl") complex_risk = r.assessment->risk_score;
        }
    }
    CHECK(complex_risk > simple_risk);
}

TEST_CASE("Integration: JSON export from pipeline") {
    auto state = make_test_state();
    auto json = Exporter::export_json(state);
    CHECK(!json.empty());
    CHECK(json.front() == '{');
}

TEST_CASE("Integration: CSV export from pipeline") {
    auto state = make_test_state();
    auto csv = Exporter::export_csv(state);
    CHECK(!csv.empty());
    CHECK(csv.find("filename") != std::string::npos);
}

TEST_CASE("Integration: HTML export from pipeline") {
    auto state = make_test_state();
    auto html = Exporter::export_html(state);
    CHECK(!html.empty());
    CHECK(html.find("html") != std::string::npos);
}

TEST_CASE("Integration: multi-language portfolio") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"A.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. A.\n       PROCEDURE DIVISION.\n           STOP RUN.\n"},
        {"B.jcl", "//MYJOB JOB\n//STEP1 EXEC PGM=A\n"},
        {"C.pli", "MAIN: PROCEDURE OPTIONS(MAIN);\n  DCL X FIXED BIN(31);\nEND MAIN;\n"},
        {"D.rexx", "/* REXX */\nSAY 'HELLO'\n"},
        {"E.sas", "DATA work.out; SET work.in; RUN;\n"}
    };
    auto state = orch.run(files);
    CHECK(state.summary.total_files == 5);
    CHECK(state.summary.lang_counts.size() >= 3);
}

TEST_CASE("Integration: migration wave ordering low risk first") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    std::vector<std::pair<std::string, std::string>> files = {
        {"SIMPLE.cbl",
         "       IDENTIFICATION DIVISION.\n"
         "       PROGRAM-ID. SIMPLE.\n"
         "       PROCEDURE DIVISION.\n"
         "           STOP RUN.\n"},
        {"RISKY.cbl",
         "       IDENTIFICATION DIVISION.\n"
         "       PROGRAM-ID. RISKY.\n"
         "       PROCEDURE DIVISION.\n"
         "           EXEC CICS SEND MAP('M') END-EXEC.\n"
         "           EXEC CICS RECEIVE MAP('M') END-EXEC.\n"
         "           EXEC SQL SELECT A INTO :B FROM T END-EXEC.\n"
         "           EXEC SQL INSERT INTO T2 VALUES(:B) END-EXEC.\n"
         "           STOP RUN.\n"}
    };
    auto state = orch.run(files);
    if (state.summary.waves.size() >= 2) {
        CHECK(state.summary.waves[0].avg_risk <= state.summary.waves[1].avg_risk);
    }
}

TEST_CASE("Integration: connected components create separate waves") {
    PipelineConfig cfg;
    Orchestrator orch(cfg);
    // Two disconnected groups
    std::vector<std::pair<std::string, std::string>> files = {
        {"GRP1_A.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. A.\n       PROCEDURE DIVISION.\n       COPY GRP1_B.\n           STOP RUN.\n"},
        {"GRP1_B.cpy", "       01 B-REC PIC X(10).\n"},
        {"GRP2_X.cbl", "       IDENTIFICATION DIVISION.\n       PROGRAM-ID. X.\n       PROCEDURE DIVISION.\n           STOP RUN.\n"}
    };
    auto state = orch.run(files);
    auto cc = state.graph.connected_components();
    CHECK(cc.size() >= 2);
}

TEST_CASE("Integration: checkpoint save and resume") {
    PipelineState ps;
    ps.stage = "transpile";
    ps.files_processed = 3;
    ps.files_total = 5;
    ps.completed = false;
    FileResult fr1; fr1.filename = "A.cbl"; fr1.success = true;
    FileResult fr2; fr2.filename = "B.jcl"; fr2.success = false;
    ps.results = {fr1, fr2};

    auto checkpoint = ps.serialize();
    auto resumed = PipelineState::deserialize(checkpoint);

    CHECK(resumed.stage == "transpile");
    CHECK(resumed.files_processed == 3);
    CHECK(resumed.files_total == 5);
    CHECK(resumed.results.size() == 2);
    CHECK(resumed.results[0].filename == "A.cbl");
    CHECK(resumed.results[1].success == false);
}

TEST_CASE("Integration: all formats exportable from same state") {
    auto state = make_test_state();
    auto json = Exporter::export_report(state, ExportFormat::JSON);
    auto csv = Exporter::export_report(state, ExportFormat::CSV);
    auto html = Exporter::export_report(state, ExportFormat::HTML);
    CHECK(!json.empty());
    CHECK(!csv.empty());
    CHECK(!html.empty());
    // Each format has distinct markers
    CHECK(json.find('{') != std::string::npos);
    CHECK(csv.find(',') != std::string::npos);
    CHECK(html.find('<') != std::string::npos);
}
