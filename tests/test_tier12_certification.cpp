#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

// Tier 1: JCL
#include "lazarus/jcl/lexer.h"
#include "lazarus/jcl/parser.h"
#include "lazarus/jcl/proc_resolver.h"
#include "lazarus/jcl/translator.h"
#include "lazarus/jcl/shell_translator.h"
#include "lazarus/jcl/utilities.h"

// Tier 2: Sort
#include "lazarus/sort/parser.h"
#include "lazarus/sort/engine.h"
#include "lazarus/sort/ebcdic.h"

// Tier 3: DB2
#include "lazarus/db2/ddl_parser.h"
#include "lazarus/db2/copybook_mapper.h"
#include "lazarus/db2/postgres_translator.h"
#include "lazarus/db2/ebcdic.h"

// Tier 4: Files
#include "lazarus/file/vsam.h"
#include "lazarus/file/gdg.h"
#include "lazarus/file/pds.h"
#include "lazarus/file/sequential.h"

// Tier 8: RACF
#include "lazarus/racf/profile.h"
#include "lazarus/racf/policy_emitter.h"

// Tier 9: Scheduler
#include "lazarus/scheduler/schedule_model.h"
#include "lazarus/scheduler/tws_parser.h"
#include "lazarus/scheduler/airflow_emitter.h"
#include "lazarus/scheduler/cron_emitter.h"

// Tier 11: Pipeline
#include "lazarus/pipeline/detector.h"
#include "lazarus/pipeline/graph.h"
#include "lazarus/pipeline/assessment.h"
#include "lazarus/pipeline/orchestrator.h"
#include "lazarus/pipeline/exporter.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstdio>

namespace fs = std::filesystem;

// Temp directory helper for file-based tests
static fs::path get_temp_dir() {
    fs::path tmp = fs::temp_directory_path() / "lazarus_t12_cert";
    fs::create_directories(tmp);
    return tmp;
}

static void cleanup_temp_dir() {
    std::error_code ec;
    fs::remove_all(fs::temp_directory_path() / "lazarus_t12_cert", ec);
}

// ============================================================================
// CATEGORY 1: JCL to COBOL chain (5 tests)
// ============================================================================

TEST_CASE("Cert: JCL EXEC PGM parsed and translated to shell step") {
    std::string jcl =
        "//MYJOB  JOB (ACCT),'CERT TEST'\n"
        "//STEP1  EXEC PGM=PAYROLL\n"
        "//SYSOUT DD SYSOUT=*\n";

    // parse_jcl returns a single JclJob directly
    auto job = lazarus::jcl::parse_jcl(jcl);
    REQUIRE(job.steps.size() == 1);
    CHECK(job.steps[0].pgm == "PAYROLL");
    CHECK_FALSE(job.steps[0].is_proc_call);

    lazarus::jcl::ShellTranslator strans(job);
    std::string bash = strans.generate();
    CHECK(bash.find("PAYROLL") != std::string::npos);
}

TEST_CASE("Cert: JCL SORT step integration with SortEngine") {
    std::string jcl =
        "//SORTJOB JOB (ACCT),'SORT TEST'\n"
        "//STEP1  EXEC PGM=SORT\n"
        "//SORTIN DD DSN=INPUT.DATA,DISP=SHR\n"
        "//SORTOUT DD DSN=OUTPUT.DATA,DISP=(NEW,CATLG)\n"
        "//SYSIN  DD *\n";

    auto job = lazarus::jcl::parse_jcl(jcl);
    CHECK(job.steps[0].pgm == "SORT");

    // Sort engine processes records independently
    std::string control = "SORT FIELDS=(1,10,CH,A)";
    std::vector<std::string> records = {
        "CHERRY    DATA1",
        "APPLE     DATA2",
        "BANANA    DATA3"
    };
    auto sorted = lazarus::sort::engine::sort_records(control, records);
    REQUIRE(sorted.size() == 3);
    CHECK(sorted[0].substr(0, 10) == "APPLE     ");
    CHECK(sorted[1].substr(0, 10) == "BANANA    ");
    CHECK(sorted[2].substr(0, 10) == "CHERRY    ");
}

TEST_CASE("Cert: JCL DD DSN file routing produces valid DSN paths") {
    std::string jcl =
        "//FILEJOB JOB (ACCT),'FILE ROUTE'\n"
        "//STEP1  EXEC PGM=IEBGENER\n"
        "//SYSUT1 DD DSN=PROD.MASTER.DATA,DISP=SHR\n"
        "//SYSUT2 DD DSN=PROD.BACKUP.DATA,DISP=(NEW,CATLG)\n"
        "//SYSPRINT DD SYSOUT=*\n"
        "//SYSIN  DD DUMMY\n";

    auto job = lazarus::jcl::parse_jcl(jcl);
    auto& step = job.steps[0];
    CHECK(step.pgm == "IEBGENER");

    // Verify DD DSN values parsed correctly
    bool found_sysut1 = false, found_sysut2 = false;
    for (auto& dd : step.dd_statements) {
        if (dd.name == "SYSUT1") {
            CHECK(dd.params.dsn == "PROD.MASTER.DATA");
            CHECK(dd.params.disp_status == "SHR");
            found_sysut1 = true;
        }
        if (dd.name == "SYSUT2") {
            CHECK(dd.params.dsn == "PROD.BACKUP.DATA");
            CHECK(dd.params.disp_status == "NEW");
            found_sysut2 = true;
        }
    }
    CHECK(found_sysut1);
    CHECK(found_sysut2);

    // Translator maps DSN to path
    std::string path = lazarus::jcl::Translator::dsn_to_path("PROD.MASTER.DATA");
    CHECK_FALSE(path.empty());
}

TEST_CASE("Cert: JCL PROC expansion with symbolic substitution") {
    std::string proc_text =
        "//MYPROC PROC LIB='DEFAULT.LIB'\n"
        "//PS1    EXEC PGM=COMPRESS\n"
        "//INPUT  DD DSN=&LIB,DISP=SHR\n"
        "//       PEND\n";

    std::string jcl =
        "//PJOB   JOB (ACCT),'PROC TEST'\n"
        "//RUN    EXEC MYPROC,LIB='PROD.LOADLIB'\n";

    lazarus::jcl::ProcResolver resolver;
    resolver.add_proc_from_text("MYPROC", proc_text);

    auto job = lazarus::jcl::parse_jcl(jcl);
    REQUIRE(job.steps.size() >= 1);
    auto& step = job.steps[0];
    CHECK(step.is_proc_call);
    CHECK(step.proc == "MYPROC");

    auto resolved = resolver.resolve(job);
    CHECK(resolved.steps.size() >= 1);
}

TEST_CASE("Cert: JCL multi-step COND evaluation skips steps correctly") {
    // parse_cond_param expects (code,op,stepname) triples
    auto evals1 = lazarus::jcl::parse_cond_param("(4,LT,STEP1)");
    REQUIRE_FALSE(evals1.empty());
    CHECK(evals1[0].code == 4);
    CHECK(evals1[0].op == "LT");
    CHECK(evals1[0].step == "STEP1");
    // should_skip(rc): checks if rc <op> code
    // COND=(4,LT,STEP1): skip if step_rc < 4
    CHECK(evals1[0].should_skip(2));         // 2 < 4 => skip
    CHECK_FALSE(evals1[0].should_skip(4));   // 4 < 4 is false
    CHECK_FALSE(evals1[0].should_skip(8));   // 8 < 4 is false

    auto evals2 = lazarus::jcl::parse_cond_param("(4,GT,STEP1)");
    REQUIRE_FALSE(evals2.empty());
    // skip if step_rc > 4
    CHECK(evals2[0].should_skip(8));         // 8 > 4 => skip
    CHECK_FALSE(evals2[0].should_skip(4));   // 4 > 4 is false

    auto evals3 = lazarus::jcl::parse_cond_param("(4,EQ,STEP1)");
    REQUIRE_FALSE(evals3.empty());
    // skip if step_rc == 4
    CHECK(evals3[0].should_skip(4));         // 4 == 4 => skip
    CHECK_FALSE(evals3[0].should_skip(0));   // 0 == 4 is false
}

// ============================================================================
// CATEGORY 2: Multi-language detection (5 tests)
// ============================================================================

TEST_CASE("Cert: Detect COBOL by content signature") {
    std::string cobol_src =
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. PAYROLL.\n"
        "       DATA DIVISION.\n"
        "       PROCEDURE DIVISION.\n"
        "           DISPLAY 'HELLO'.\n"
        "           STOP RUN.\n";
    auto det = lazarus::pipeline::detect("unknown.txt", cobol_src);
    CHECK(det.lang == lazarus::pipeline::SourceLang::COBOL);
    CHECK(det.confidence >= 0.5f);
}

TEST_CASE("Cert: Detect JCL by content signature") {
    std::string jcl_src =
        "//MYJOB  JOB (ACCT),'TEST JOB',CLASS=A\n"
        "//STEP1  EXEC PGM=IEFBR14\n"
        "//DD1    DD DSN=MY.DATASET,DISP=SHR\n";
    auto det = lazarus::pipeline::detect("unknown.txt", jcl_src);
    CHECK(det.lang == lazarus::pipeline::SourceLang::JCL);
    CHECK(det.confidence >= 0.5f);
}

TEST_CASE("Cert: Detect PL/I by extension") {
    auto det = lazarus::pipeline::detect("PROGRAM.pli", "");
    CHECK(det.lang == lazarus::pipeline::SourceLang::PLI);
}

TEST_CASE("Cert: Detect REXX by content signature") {
    std::string rexx_src =
        "/* REXX */\n"
        "SAY 'Hello World'\n"
        "EXIT 0\n";
    auto det = lazarus::pipeline::detect("MYSCRIPT.rexx", rexx_src);
    CHECK(det.lang == lazarus::pipeline::SourceLang::REXX);
}

TEST_CASE("Cert: Detect Easytrieve by content signature") {
    std::string ez_src =
        "FILE EMPFILE\n"
        "  EMPLOYEE-NAME 1 30 A\n"
        "  EMPLOYEE-ID   31 5 N\n"
        "JOB INPUT EMPFILE\n"
        "  DISPLAY EMPLOYEE-NAME EMPLOYEE-ID\n";
    auto det = lazarus::pipeline::detect("REPORT.ezt", ez_src);
    CHECK(det.lang == lazarus::pipeline::SourceLang::Easytrieve);
}

// ============================================================================
// CATEGORY 3: Dependency graph spanning languages (5 tests)
// ============================================================================

TEST_CASE("Cert: COBOL COPY creates dependency edge") {
    std::string cobol =
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. MAIN.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       COPY CUSTCOPY.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n";
    auto deps = lazarus::pipeline::dep_extract::extract_dependencies(
        "MAIN.cbl", cobol, lazarus::pipeline::SourceLang::COBOL);
    bool found = false;
    for (auto& d : deps) {
        if (d.target.find("CUSTCOPY") != std::string::npos) {
            found = true;
            break;
        }
    }
    CHECK(found);
}

TEST_CASE("Cert: JCL EXEC PGM creates dependency edge") {
    std::string jcl =
        "//RUNJOB JOB (ACCT),'RUN'\n"
        "//STEP1  EXEC PGM=PAYROLL\n"
        "//STEP2  EXEC PGM=REPORTS\n";
    auto deps = lazarus::pipeline::dep_extract::extract_dependencies(
        "RUNJOB.jcl", jcl, lazarus::pipeline::SourceLang::JCL);
    bool found_payroll = false, found_reports = false;
    for (auto& d : deps) {
        if (d.target.find("PAYROLL") != std::string::npos) found_payroll = true;
        if (d.target.find("REPORTS") != std::string::npos) found_reports = true;
    }
    CHECK(found_payroll);
    CHECK(found_reports);
}

TEST_CASE("Cert: DependencyGraph topological sort produces valid order") {
    lazarus::pipeline::DependencyGraph g;
    g.add_node("A");
    g.add_node("B");
    g.add_node("C");
    g.add_edge("A", "B");
    g.add_edge("B", "C");
    auto topo = g.topological_sort();
    REQUIRE(topo.size() == 3);
    auto pos_a = std::find(topo.begin(), topo.end(), "A") - topo.begin();
    auto pos_b = std::find(topo.begin(), topo.end(), "B") - topo.begin();
    auto pos_c = std::find(topo.begin(), topo.end(), "C") - topo.begin();
    CHECK(pos_a < pos_b);
    CHECK(pos_b < pos_c);
}

TEST_CASE("Cert: DependencyGraph cycle detection") {
    lazarus::pipeline::DependencyGraph g;
    g.add_node("X");
    g.add_node("Y");
    g.add_node("Z");
    g.add_edge("X", "Y");
    g.add_edge("Y", "Z");
    g.add_edge("Z", "X");
    auto cycle = g.detect_cycle();
    CHECK(cycle.has_value());
    CHECK(cycle->size() >= 3);
}

TEST_CASE("Cert: DependencyGraph connected components") {
    lazarus::pipeline::DependencyGraph g;
    g.add_node("A");
    g.add_node("B");
    g.add_edge("A", "B");
    g.add_node("C");
    g.add_node("D");
    g.add_edge("C", "D");
    auto comps = g.connected_components();
    CHECK(comps.size() == 2);
}

// ============================================================================
// CATEGORY 4: DB2 + Copybook pipeline (3 tests)
// ============================================================================

TEST_CASE("Cert: Copybook PIC clause maps to SQL column type") {
    std::string copybook =
        "       01 CUSTOMER-REC.\n"
        "          05 CUST-ID      PIC 9(8).\n"
        "          05 CUST-NAME    PIC X(30).\n"
        "          05 CUST-BALANCE PIC S9(7)V99 COMP-3.\n";

    // Static parse returns tree: root is 01-level with children
    auto fields = lazarus::db2::CopybookParser::parse(copybook);
    REQUIRE(fields.size() >= 1);
    REQUIRE(fields[0].children.size() == 3);
    CHECK(fields[0].children[0].name == "CUST-ID");
    CHECK(fields[0].children[1].name == "CUST-NAME");
    CHECK(fields[0].children[2].name == "CUST-BALANCE");

    // Map to SQL table
    lazarus::db2::CopybookMapper mapper;
    auto table = mapper.map(fields);
    REQUIRE(table.columns.size() >= 3);

    // Verify type mappings present
    bool found_int = false, found_char = false, found_dec = false;
    for (auto& col : table.columns) {
        auto n = col.name;
        if (n.find("CUST_ID") != std::string::npos ||
            n.find("CUST-ID") != std::string::npos) found_int = true;
        if (n.find("CUST_NAME") != std::string::npos ||
            n.find("CUST-NAME") != std::string::npos) found_char = true;
        if (n.find("CUST_BALANCE") != std::string::npos ||
            n.find("CUST-BALANCE") != std::string::npos) found_dec = true;
    }
    CHECK(found_int);
    CHECK(found_char);
    CHECK(found_dec);
}

TEST_CASE("Cert: DB2 DDL parsed and translated to PostgreSQL") {
    std::string ddl =
        "CREATE TABLE SCHEMA1.ACCOUNTS (\n"
        "    ACCT_ID    INTEGER NOT NULL,\n"
        "    ACCT_NAME  VARCHAR(50),\n"
        "    BALANCE    DECIMAL(15,2),\n"
        "    CREATED    TIMESTAMP,\n"
        "    PRIMARY KEY (ACCT_ID)\n"
        ");\n";

    auto stmts = lazarus::db2::DDLParser::parse_script(ddl);
    REQUIRE(stmts.size() == 1);
    auto* tbl = std::get_if<lazarus::db2::CreateTable>(&stmts[0]);
    REQUIRE(tbl != nullptr);
    CHECK(tbl->name == "ACCOUNTS");
    CHECK(tbl->columns.size() == 4);

    lazarus::db2::PostgresTranslator pg;
    std::string pg_ddl = pg.translate(stmts[0]);
    CHECK(pg_ddl.find("accounts") != std::string::npos);
    // Verify some type keyword appears
    bool has_type = (pg_ddl.find("integer") != std::string::npos) ||
                    (pg_ddl.find("INTEGER") != std::string::npos) ||
                    (pg_ddl.find("INT") != std::string::npos);
    CHECK(has_type);
}

TEST_CASE("Cert: EBCDIC packed decimal encode/decode roundtrip") {
    using namespace lazarus::db2::ebcdic;

    // Positive value
    int64_t val1 = 12345;
    auto packed1 = packed_decimal_encode(val1, 5);
    CHECK_FALSE(packed1.empty());
    double decoded1 = packed_decimal_decode(packed1.data(), packed1.size(), 0);
    CHECK(decoded1 == doctest::Approx(12345.0));

    // Negative value
    int64_t val2 = -9876;
    auto packed2 = packed_decimal_encode(val2, 5);
    double decoded2 = packed_decimal_decode(packed2.data(), packed2.size(), 0);
    CHECK(decoded2 == doctest::Approx(-9876.0));

    // With decimal scale
    int64_t val3 = 12345; // represents 123.45 with scale=2
    auto packed3 = packed_decimal_encode(val3, 5);
    double decoded3 = packed_decimal_decode(packed3.data(), packed3.size(), 2);
    CHECK(decoded3 == doctest::Approx(123.45));
}

// ============================================================================
// CATEGORY 5: File system chain (3 tests)
// ============================================================================

TEST_CASE("Cert: GDG create and resolve generations") {
    auto tmp = get_temp_dir() / "gdg_cert";
    std::error_code ec;
    fs::remove_all(tmp, ec);

    {
        lazarus::file::GdgBase base("TEST.GDG", tmp, 5);
        CHECK(base.generation_count() == 0);

        auto g1 = base.allocate_new();
        auto g2 = base.allocate_new();
        auto g3 = base.allocate_new();
        CHECK(base.generation_count() == 3);

        // Resolve relative(0) = current = g3
        auto current = base.resolve_relative(0);
        REQUIRE(current.has_value());
        CHECK(current->gen_num == g3.gen_num);

        // Resolve relative(-1) = previous = g2
        auto prev = base.resolve_relative(-1);
        REQUIRE(prev.has_value());
        CHECK(prev->gen_num == g2.gen_num);

        // Resolve relative(-2) = g1
        auto oldest = base.resolve_relative(-2);
        REQUIRE(oldest.has_value());
        CHECK(oldest->gen_num == g1.gen_num);
    }

    cleanup_temp_dir();
}

TEST_CASE("Cert: VSAM KSDS put/get/delete cycle") {
    auto tmp = get_temp_dir() / "vsam_cert";
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp);

    lazarus::file::ClusterDef def;
    def.name = "CERT.KSDS";
    def.key_length = 8;
    def.unique = true;

    {
        lazarus::file::KsdsFile ksds(tmp / "cert.ksds", def);

        CHECK(ksds.put("KEY00001", "Record One") == lazarus::file::VsamRc::OK);
        CHECK(ksds.put("KEY00002", "Record Two") == lazarus::file::VsamRc::OK);
        CHECK(ksds.put("KEY00003", "Record Three") == lazarus::file::VsamRc::OK);
        CHECK(ksds.size() == 3);

        // Duplicate key rejected
        CHECK(ksds.put("KEY00001", "Dup") == lazarus::file::VsamRc::DUPLICATE_KEY);

        // Get
        std::string rec;
        CHECK(ksds.get("KEY00002", rec) == lazarus::file::VsamRc::OK);
        CHECK(rec == "Record Two");

        // Delete
        CHECK(ksds.erase("KEY00002") == lazarus::file::VsamRc::OK);
        CHECK(ksds.size() == 2);
        CHECK(ksds.get("KEY00002", rec) == lazarus::file::VsamRc::NOT_FOUND);

        // Browse
        ksds.startbr("KEY00001");
        std::string k, v;
        CHECK(ksds.readnext(k, v) == lazarus::file::VsamRc::OK);
        CHECK(k == "KEY00001");
    }

    cleanup_temp_dir();
}

TEST_CASE("Cert: PDS member create/list/read cycle") {
    auto tmp = get_temp_dir() / "pds_cert";
    std::error_code ec;
    fs::remove_all(tmp, ec);

    lazarus::file::Pds pds(tmp);

    pds.write_member("MEMBER1", "Content of member 1");
    pds.write_member("MEMBER2", "Content of member 2");
    pds.write_member("MEMBER3", "Content of member 3");
    CHECK(pds.member_count() == 3);

    auto members = pds.list_members();
    REQUIRE(members.size() == 3);
    CHECK(members[0] == "MEMBER1");
    CHECK(members[1] == "MEMBER2");
    CHECK(members[2] == "MEMBER3");

    CHECK(pds.read_member("MEMBER2") == "Content of member 2");

    CHECK(pds.delete_member("MEMBER2"));
    CHECK(pds.member_count() == 2);
    CHECK_FALSE(pds.member_exists("MEMBER2"));

    cleanup_temp_dir();
}

// ============================================================================
// CATEGORY 6: Sort engine integration (3 tests)
// ============================================================================

TEST_CASE("Cert: Parse sort control card and sort records") {
    std::string control = "SORT FIELDS=(1,5,CH,A)";
    lazarus::sort::parser::SortParser p;
    auto spec = p.parse(control);
    REQUIRE(spec.sort_fields.size() == 1);
    CHECK(spec.sort_fields[0].position == 1);
    CHECK(spec.sort_fields[0].length == 5);
    CHECK(spec.sort_fields[0].format == lazarus::sort::parser::FieldFormat::CH);
    CHECK(spec.sort_fields[0].order == lazarus::sort::parser::SortOrder::ASC);

    std::vector<std::string> records = {
        "DELTA record 4",
        "ALPHA record 1",
        "GAMMA record 5",
        "BRAVO record 2",
        "CHARL record 3"
    };
    lazarus::sort::engine::SortEngine eng(spec);
    auto sorted = eng.execute(records);
    REQUIRE(sorted.size() == 5);
    CHECK(sorted[0].substr(0, 5) == "ALPHA");
    CHECK(sorted[1].substr(0, 5) == "BRAVO");
    CHECK(sorted[2].substr(0, 5) == "CHARL");
    CHECK(sorted[3].substr(0, 5) == "DELTA");
    CHECK(sorted[4].substr(0, 5) == "GAMMA");
}

TEST_CASE("Cert: Sort with INCLUDE condition filters records") {
    std::string control =
        "SORT FIELDS=(1,5,CH,A)\n"
        "INCLUDE COND=((6,1,CH,EQ,'A'))";
    std::vector<std::string> records = {
        "ITEM1A data line one  ",
        "ITEM2B data line two  ",
        "ITEM3A data line three",
        "ITEM4B data line four ",
        "ITEM5A data line five "
    };
    auto sorted = lazarus::sort::engine::sort_records(control, records);
    // Only records where pos 6 length 1 == 'A' should remain
    for (auto& r : sorted) {
        CHECK(r[5] == 'A');
    }
    CHECK(sorted.size() == 3);
}

TEST_CASE("Cert: Multi-key sort with EBCDIC collation tables") {
    // Verify EBCDIC conversion roundtrip
    std::string test = "ABCXYZ";
    std::string ebcdic_str = lazarus::sort::ebcdic::ascii_to_ebcdic(test);
    std::string back = lazarus::sort::ebcdic::ebcdic_to_ascii(ebcdic_str);
    CHECK(back == test);

    // EBCDIC compare
    int cmp = lazarus::sort::ebcdic::ebcdic_compare("ABC", "DEF");
    CHECK(cmp < 0);

    // Multi-key sort: primary ascending, secondary descending
    std::string control = "SORT FIELDS=(1,3,CH,A,4,3,CH,D)";
    std::vector<std::string> records = {
        "AABZZZ",
        "AABYYY",
        "AABXXX",
        "AACZZZ",
        "AACYYY"
    };
    auto sorted = lazarus::sort::engine::sort_records(control, records);
    REQUIRE(sorted.size() == 5);
    CHECK(sorted[0].substr(0, 3) == "AAB");
    CHECK(sorted[1].substr(0, 3) == "AAB");
    CHECK(sorted[2].substr(0, 3) == "AAB");
    CHECK(sorted[3].substr(0, 3) == "AAC");
    CHECK(sorted[4].substr(0, 3) == "AAC");
    // Secondary descending within AAB: ZZZ, YYY, XXX
    CHECK(sorted[0].substr(3, 3) == "ZZZ");
    CHECK(sorted[1].substr(3, 3) == "YYY");
    CHECK(sorted[2].substr(3, 3) == "XXX");
}

// ============================================================================
// CATEGORY 7: Assessment pipeline (3 tests)
// ============================================================================

TEST_CASE("Cert: Language detect + assessment + risk scoring") {
    std::string cobol =
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. COMPLEXAPP.\n"
        "       DATA DIVISION.\n"
        "       WORKING-STORAGE SECTION.\n"
        "       01 WS-COUNTER PIC 9(5).\n"
        "       PROCEDURE DIVISION.\n"
        "           PERFORM PROCESS-LOOP VARYING WS-COUNTER\n"
        "               FROM 1 BY 1 UNTIL WS-COUNTER > 100.\n"
        "           STOP RUN.\n"
        "       PROCESS-LOOP.\n"
        "           DISPLAY WS-COUNTER.\n";

    auto det = lazarus::pipeline::detect("COMPLEX.cbl", cobol);
    CHECK(det.lang == lazarus::pipeline::SourceLang::COBOL);

    lazarus::pipeline::DependencyGraph g;
    g.add_node("COMPLEX.cbl");

    auto assessment = lazarus::pipeline::assess_file(
        "COMPLEX.cbl", cobol, det.lang, g);
    CHECK_FALSE(assessment.filename.empty());
    // risk_score should be non-negative (valid assessment was produced)
    CHECK(assessment.risk_score >= 0.0f);
}

TEST_CASE("Cert: Dependency graph + migration wave planning") {
    lazarus::pipeline::DependencyGraph g;
    g.add_node("BASE.cbl");
    g.add_node("MID.cbl");
    g.add_node("TOP.cbl");
    g.add_edge("TOP.cbl", "MID.cbl");
    g.add_edge("MID.cbl", "BASE.cbl");

    std::vector<lazarus::pipeline::FileAssessment> assessments;
    std::string dummy_src =
        "       IDENTIFICATION DIVISION.\n"
        "       PROGRAM-ID. X.\n"
        "       PROCEDURE DIVISION.\n"
        "           STOP RUN.\n";
    for (auto& name : {"BASE.cbl", "MID.cbl", "TOP.cbl"}) {
        auto a = lazarus::pipeline::assess_file(
            name, dummy_src, lazarus::pipeline::SourceLang::COBOL, g);
        assessments.push_back(a);
    }

    auto waves = lazarus::pipeline::plan_waves(assessments, g);
    CHECK(waves.size() >= 1);

    auto summary = lazarus::pipeline::summarize(assessments, g);
    CHECK(summary.total_files == 3);
}

TEST_CASE("Cert: JSON inventory export from pipeline state") {
    lazarus::pipeline::PipelineState state;
    state.stage = "complete";
    state.files_processed = 2;
    state.files_total = 2;
    state.completed = true;

    lazarus::pipeline::FileResult fr1;
    fr1.filename = "PAYROLL.cbl";
    fr1.success = true;
    lazarus::pipeline::Detection det1;
    det1.lang = lazarus::pipeline::SourceLang::COBOL;
    det1.confidence = 0.95f;
    fr1.detection = det1;

    lazarus::pipeline::FileResult fr2;
    fr2.filename = "RUNJOB.jcl";
    fr2.success = true;
    lazarus::pipeline::Detection det2;
    det2.lang = lazarus::pipeline::SourceLang::JCL;
    det2.confidence = 0.90f;
    fr2.detection = det2;

    state.results.push_back(fr1);
    state.results.push_back(fr2);

    std::string json = lazarus::pipeline::Exporter::export_json(state);
    CHECK(json.find("PAYROLL.cbl") != std::string::npos);
    CHECK(json.find("RUNJOB.jcl") != std::string::npos);
    CHECK(json.find("COBOL") != std::string::npos);
    CHECK(json.find("JCL") != std::string::npos);
}

// ============================================================================
// CATEGORY 8: Cross-tier data flow (3 tests)
// ============================================================================

TEST_CASE("Cert: JCL DD to VSAM KSDS to sequential file REPRO chain") {
    auto tmp = get_temp_dir() / "repro_cert";
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp);

    // Step 1: Parse JCL referencing VSAM and sequential datasets
    std::string jcl =
        "//REPRJOB JOB (ACCT),'REPRO'\n"
        "//STEP1  EXEC PGM=IDCAMS\n"
        "//INDD   DD DSN=VSAM.MASTER,DISP=SHR\n"
        "//OUTDD  DD DSN=SEQ.BACKUP,DISP=(NEW,CATLG)\n"
        "//SYSPRINT DD SYSOUT=*\n";

    auto job = lazarus::jcl::parse_jcl(jcl);
    CHECK(job.steps[0].pgm == "IDCAMS");

    // Step 2: Populate VSAM KSDS
    lazarus::file::ClusterDef def;
    def.name = "VSAM.MASTER";
    def.key_length = 6;
    def.unique = true;

    {
        lazarus::file::KsdsFile ksds(tmp / "master.ksds", def);
        ksds.put("EMP001", "John Smith, Engineering");
        ksds.put("EMP002", "Jane Doe, Marketing");
        ksds.put("EMP003", "Bob Wilson, Finance");
        CHECK(ksds.size() == 3);

        // Step 3: REPRO to another KSDS
        lazarus::file::ClusterDef def2 = def;
        def2.name = "VSAM.BACKUP";
        lazarus::file::KsdsFile backup(tmp / "backup.ksds", def2);
        size_t copied = lazarus::file::repro_ksds_to_ksds(ksds, backup);
        CHECK(copied == 3);
        CHECK(backup.size() == 3);

        // Step 4: REPRO KSDS to sequential stream
        std::ostringstream seq_out;
        size_t streamed = lazarus::file::repro_ksds_to_stream(ksds, seq_out);
        CHECK(streamed == 3);
        std::string output = seq_out.str();
        CHECK(output.find("EMP001") != std::string::npos);
        CHECK(output.find("John Smith") != std::string::npos);
    }

    cleanup_temp_dir();
}

TEST_CASE("Cert: RACF profile to IAM policy JSON export") {
    lazarus::racf::ProfileDatabase db;

    lazarus::racf::UserProfile user;
    user.userid = "ADMIN01";
    user.name = "System Admin";
    user.dfltgrp = "SYSADMIN";
    user.special = true;
    db.add(user);

    lazarus::racf::UserProfile user2;
    user2.userid = "DEV001";
    user2.name = "Developer";
    user2.dfltgrp = "DEVTEAM";
    db.add(user2);

    lazarus::racf::GroupProfile grp;
    grp.group = "SYSADMIN";
    grp.owner = "ADMIN01";
    db.add(grp);

    lazarus::racf::GroupProfile grp2;
    grp2.group = "DEVTEAM";
    grp2.supgroup = "SYSADMIN";
    grp2.owner = "ADMIN01";
    db.add(grp2);

    lazarus::racf::DatasetProfile ds;
    ds.dsname = "PROD.DATA.*";
    ds.uacc = lazarus::racf::AccessLevel::NONE;
    ds.generic = true;
    lazarus::racf::AccessEntry ae;
    ae.entity = "SYSADMIN";
    ae.access = lazarus::racf::AccessLevel::ALTER;
    ds.access_list.push_back(ae);
    lazarus::racf::AccessEntry ae2;
    ae2.entity = "DEV001";
    ae2.access = lazarus::racf::AccessLevel::READ;
    ds.access_list.push_back(ae2);
    db.add(ds);

    lazarus::racf::ConnectProfile conn;
    conn.userid = "ADMIN01";
    conn.group = "SYSADMIN";
    conn.auth = lazarus::racf::Authority::JOIN;
    db.add(conn);

    lazarus::racf::ConnectProfile conn2;
    conn2.userid = "DEV001";
    conn2.group = "DEVTEAM";
    conn2.auth = lazarus::racf::Authority::USE;
    db.add(conn2);

    // Emit IAM policy JSON
    std::string json = lazarus::racf::JsonPolicyEmitter::emit(db);
    CHECK(json.find("ADMIN01") != std::string::npos);
    CHECK(json.find("DEV001") != std::string::npos);
    CHECK(json.find("Version") != std::string::npos);

    // Emit Terraform HCL
    std::string tf = lazarus::racf::TerraformEmitter::emit(db);
    CHECK(tf.find("resource") != std::string::npos);

    // Emit Kubernetes RBAC (split check to avoid || in doctest)
    std::string k8s = lazarus::racf::KubernetesRbacEmitter::emit(db);
    bool k8s_valid = (k8s.find("kind") != std::string::npos) ||
                     (k8s.find("apiVersion") != std::string::npos) ||
                     (k8s.find("ServiceAccount") != std::string::npos);
    CHECK(k8s_valid);
}

TEST_CASE("Cert: TWS schedule to Airflow DAG to cron expression") {
    // TWS format: ADID=name, OPERATIONS section, DEPENDENCIES section
    std::string tws_input =
        "ADID=NIGHTBATCH OWNER=OPS PRIORITY=8\n"
        "OPERATIONS\n"
        "  JOBNAME=EXTRACT JOBNR=10 DURATION=30\n"
        "  JOBNAME=TRANSFORM JOBNR=20 DURATION=45\n"
        "  JOBNAME=LOAD JOBNR=30 DURATION=20\n"
        "DEPENDENCIES\n"
        "  PRED(NIGHTBATCH.10) SUCC(NIGHTBATCH.20) TYPE(S)\n"
        "  PRED(NIGHTBATCH.20) SUCC(NIGHTBATCH.30) TYPE(S)\n"
        "END\n";

    lazarus::scheduler::TWSParser parser;
    auto pr = parser.parse(tws_input);
    CHECK(pr.success);
    CHECK(pr.applications.size() >= 1);

    auto schedule = parser.to_schedule(pr);
    CHECK(schedule.job_count() >= 3);

    auto topo = schedule.topological_order();
    CHECK(topo.size() >= 3);

    // Emit Airflow DAG
    lazarus::scheduler::AirflowEmitter airflow;
    std::string dag = airflow.emit(schedule);
    CHECK(dag.find("from airflow") != std::string::npos);
    CHECK(dag.find("DAG") != std::string::npos);

    // Emit cron expressions
    lazarus::scheduler::CronEmitter cron;
    std::string crontab = cron.emit_crontab(schedule);
    CHECK_FALSE(crontab.empty());
    bool crontab_valid = (crontab.find("SHELL") != std::string::npos) ||
                         (crontab.find("#") != std::string::npos);
    CHECK(crontab_valid);
}
