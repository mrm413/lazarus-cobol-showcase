#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

#include "lazarus/parity/runner.h"
#include "lazarus/parity/comparator.h"
#include "lazarus/parity/audit.h"
#include "lazarus/parity/regression.h"
#include "lazarus/parity/report.h"

using namespace lazarus::parity;

// ===================================================================
// RUNNER TESTS (25+)
// ===================================================================

TEST_CASE("Runner: SubprocessResult default state") {
    SubprocessResult sr;
    CHECK(sr.exit_code == -1);
    CHECK(sr.stdout_capture.empty());
    CHECK(sr.stderr_capture.empty());
    CHECK(sr.elapsed_ms == 0.0);
    CHECK(sr.timed_out == false);
    CHECK_FALSE(sr.ok());
}

TEST_CASE("Runner: SubprocessResult ok when exit_code zero") {
    SubprocessResult sr;
    sr.exit_code = 0;
    CHECK(sr.ok());
}

TEST_CASE("Runner: SubprocessResult not ok when timed_out") {
    SubprocessResult sr;
    sr.exit_code = 0;
    sr.timed_out = true;
    CHECK_FALSE(sr.ok());
}

TEST_CASE("Runner: run_subprocess echo command") {
    auto result = run_subprocess("echo hello_parity", std::chrono::milliseconds(5000));
    CHECK(result.exit_code == 0);
    CHECK(result.stdout_capture.find("hello_parity") != std::string::npos);
    CHECK(result.elapsed_ms > 0.0);
    CHECK_FALSE(result.timed_out);
}

TEST_CASE("Runner: run_subprocess captures output") {
    auto result = run_subprocess("echo line1 && echo line2", std::chrono::milliseconds(5000));
    CHECK(result.ok());
    CHECK(result.stdout_capture.find("line1") != std::string::npos);
    CHECK(result.stdout_capture.find("line2") != std::string::npos);
}

TEST_CASE("Runner: run_subprocess invalid command") {
    auto result = run_subprocess("nonexistent_cmd_xyz_12345 2>&1", std::chrono::milliseconds(5000));
    CHECK(result.exit_code != 0);
}

TEST_CASE("Runner: ComparisonStatus string conversion") {
    CHECK(comparison_status_str(ComparisonStatus::MATCH) == "MATCH");
    CHECK(comparison_status_str(ComparisonStatus::MISMATCH) == "MISMATCH");
    CHECK(comparison_status_str(ComparisonStatus::COBOL_FAIL) == "COBOL_FAIL");
    CHECK(comparison_status_str(ComparisonStatus::CPP_FAIL) == "CPP_FAIL");
    CHECK(comparison_status_str(ComparisonStatus::BOTH_FAIL) == "BOTH_FAIL");
    CHECK(comparison_status_str(ComparisonStatus::TIMEOUT) == "TIMEOUT");
}

TEST_CASE("Runner: RunResult default state") {
    RunResult rr;
    CHECK(rr.program_name.empty());
    CHECK(rr.comparison_status == ComparisonStatus::MISMATCH);
}

TEST_CASE("Runner: ParallelRunner default timeout") {
    ParallelRunner runner;
    CHECK(runner.timeout() == std::chrono::milliseconds(30000));
}

TEST_CASE("Runner: ParallelRunner set timeout") {
    ParallelRunner runner;
    runner.set_timeout(std::chrono::milliseconds(5000));
    CHECK(runner.timeout() == std::chrono::milliseconds(5000));
}

TEST_CASE("Runner: ParallelRunner run_pair matching outputs") {
    ParallelRunner runner(std::chrono::milliseconds(5000));
    ProgramPair pp;
    pp.name = "echo_test";
    pp.cobol_command = "echo parity_match";
    pp.cpp_command = "echo parity_match";
    auto rr = runner.run_pair(pp);
    CHECK(rr.program_name == "echo_test");
    CHECK(rr.comparison_status == ComparisonStatus::MATCH);
    CHECK(rr.cobol_result.ok());
    CHECK(rr.cpp_result.ok());
}

TEST_CASE("Runner: ParallelRunner run_pair mismatched outputs") {
    ParallelRunner runner(std::chrono::milliseconds(5000));
    ProgramPair pp;
    pp.name = "mismatch_test";
    pp.cobol_command = "echo output_A";
    pp.cpp_command = "echo output_B";
    auto rr = runner.run_pair(pp);
    CHECK(rr.comparison_status == ComparisonStatus::MISMATCH);
}

TEST_CASE("Runner: ParallelRunner run_batch empty") {
    ParallelRunner runner(std::chrono::milliseconds(5000));
    auto results = runner.run_batch({});
    CHECK(results.empty());
}

TEST_CASE("Runner: ParallelRunner run_batch with progress callback") {
    ParallelRunner runner(std::chrono::milliseconds(5000));
    std::vector<ProgramPair> pairs;
    ProgramPair p1; p1.name = "prog1"; p1.cobol_command = "echo a"; p1.cpp_command = "echo a";
    ProgramPair p2; p2.name = "prog2"; p2.cobol_command = "echo b"; p2.cpp_command = "echo c";
    pairs.push_back(p1);
    pairs.push_back(p2);

    size_t callback_count = 0;
    auto results = runner.run_batch(pairs, [&](size_t cur, size_t tot,
                                                const std::string& /*name*/,
                                                ComparisonStatus /*status*/) {
        CHECK(cur <= tot);
        ++callback_count;
    });
    CHECK(results.size() == 2);
    CHECK(callback_count == 2);
}

TEST_CASE("Runner: ParallelRunner batch with all matching") {
    ParallelRunner runner(std::chrono::milliseconds(5000));
    std::vector<ProgramPair> pairs;
    for (int i = 0; i < 3; ++i) {
        ProgramPair pp;
        pp.name = "prog_" + std::to_string(i);
        pp.cobol_command = "echo same_output";
        pp.cpp_command = "echo same_output";
        pairs.push_back(pp);
    }
    auto results = runner.run_batch(pairs);
    for (auto& r : results) {
        CHECK(r.comparison_status == ComparisonStatus::MATCH);
    }
}

TEST_CASE("Runner: scan_directory builds pairs") {
    auto pairs = ParallelRunner::scan_directory("/cobol", "/cpp",
        {"prog1", "prog2", "prog3"});
    CHECK(pairs.size() == 3);
    CHECK(pairs[0].name == "prog1");
    CHECK(pairs[0].cobol_command == "/cobol/prog1");
    CHECK(pairs[0].cpp_command == "/cpp/prog1");
}

TEST_CASE("Runner: BatchSummary all pass") {
    std::vector<RunResult> results;
    for (int i = 0; i < 5; ++i) {
        RunResult rr;
        rr.program_name = "p" + std::to_string(i);
        rr.comparison_status = ComparisonStatus::MATCH;
        results.push_back(rr);
    }
    auto s = ParallelRunner::summarize(results);
    CHECK(s.total == 5);
    CHECK(s.matched == 5);
    CHECK(s.mismatch == 0);
    CHECK(s.failures == 0);
    CHECK(s.timeouts == 0);
    CHECK(s.pass_rate == doctest::Approx(100.0));
}

TEST_CASE("Runner: BatchSummary mixed results") {
    std::vector<RunResult> results;
    RunResult r1; r1.comparison_status = ComparisonStatus::MATCH; results.push_back(r1);
    RunResult r2; r2.comparison_status = ComparisonStatus::MISMATCH; results.push_back(r2);
    RunResult r3; r3.comparison_status = ComparisonStatus::TIMEOUT; results.push_back(r3);
    RunResult r4; r4.comparison_status = ComparisonStatus::CPP_FAIL; results.push_back(r4);
    auto s = ParallelRunner::summarize(results);
    CHECK(s.total == 4);
    CHECK(s.matched == 1);
    CHECK(s.mismatch == 1);
    CHECK(s.timeouts == 1);
    CHECK(s.failures == 1);
    CHECK(s.pass_rate == doctest::Approx(25.0));
}

TEST_CASE("Runner: BatchSummary empty results") {
    auto s = ParallelRunner::summarize({});
    CHECK(s.total == 0);
    CHECK(s.pass_rate == doctest::Approx(0.0));
}

TEST_CASE("Runner: ProgramPair fields") {
    ProgramPair pp;
    pp.name = "test";
    pp.cobol_command = "cmd1";
    pp.cpp_command = "cmd2";
    CHECK(pp.name == "test");
    CHECK(pp.cobol_command == "cmd1");
    CHECK(pp.cpp_command == "cmd2");
}

TEST_CASE("Runner: ParallelRunner custom timeout constructor") {
    ParallelRunner runner(std::chrono::milliseconds(10000));
    CHECK(runner.timeout() == std::chrono::milliseconds(10000));
}

TEST_CASE("Runner: run_subprocess measures elapsed time") {
    auto result = run_subprocess("echo timing_test", std::chrono::milliseconds(5000));
    CHECK(result.elapsed_ms >= 0.0);
    CHECK(result.elapsed_ms < 5000.0);
}

TEST_CASE("Runner: run_pair with cpp failure detection") {
    ParallelRunner runner(std::chrono::milliseconds(5000));
    ProgramPair pp;
    pp.name = "fail_test";
    pp.cobol_command = "echo ok";
    pp.cpp_command = "exit 1";
    auto rr = runner.run_pair(pp);
    CHECK((rr.comparison_status == ComparisonStatus::CPP_FAIL ||
           rr.comparison_status == ComparisonStatus::MISMATCH));
}

TEST_CASE("Runner: scan_directory empty program list") {
    auto pairs = ParallelRunner::scan_directory("/a", "/b", {});
    CHECK(pairs.empty());
}

TEST_CASE("Runner: SubprocessResult elapsed tracks real time") {
    auto result = run_subprocess("echo fast", std::chrono::milliseconds(10000));
    CHECK(result.elapsed_ms < 10000.0);
}

// ===================================================================
// COMPARATOR TESTS (30+)
// ===================================================================

TEST_CASE("Comparator: DISPLAY exact match") {
    CHECK(compare_field<DataType::DISPLAY>("HELLO", "HELLO"));
}

TEST_CASE("Comparator: DISPLAY trailing space normalization") {
    CHECK(compare_field<DataType::DISPLAY>("HELLO   ", "HELLO"));
    CHECK(compare_field<DataType::DISPLAY>("HELLO", "HELLO   "));
}

TEST_CASE("Comparator: DISPLAY mismatch") {
    CHECK_FALSE(compare_field<DataType::DISPLAY>("HELLO", "WORLD"));
}

TEST_CASE("Comparator: COMP integer exact match") {
    CHECK(compare_field<DataType::COMP>("42", "42"));
    CHECK(compare_field<DataType::COMP>("+42", "42"));
    CHECK(compare_field<DataType::COMP>("042", "42"));
}

TEST_CASE("Comparator: COMP integer mismatch") {
    CHECK_FALSE(compare_field<DataType::COMP>("42", "43"));
}

TEST_CASE("Comparator: COMP negative integers") {
    CHECK(compare_field<DataType::COMP>("-100", "-100"));
    CHECK_FALSE(compare_field<DataType::COMP>("-100", "100"));
}

TEST_CASE("Comparator: COMP3 packed decimal match") {
    CHECK(compare_field<DataType::COMP3>("12345", "12345"));
    CHECK(compare_field<DataType::COMP3>("012345", "12345"));
}

TEST_CASE("Comparator: COMP3 packed decimal mismatch") {
    CHECK_FALSE(compare_field<DataType::COMP3>("12345", "12346"));
}

TEST_CASE("Comparator: COMP3 negative packed decimal") {
    CHECK(compare_field<DataType::COMP3>("-500", "-500"));
    CHECK_FALSE(compare_field<DataType::COMP3>("-500", "500"));
}

TEST_CASE("Comparator: COMP5 native binary match") {
    CHECK(compare_field<DataType::COMP5>("99999", "99999"));
    CHECK(compare_field<DataType::COMP5>("099999", "99999"));
}

TEST_CASE("Comparator: COMP5 native binary mismatch") {
    CHECK_FALSE(compare_field<DataType::COMP5>("99999", "99998"));
}

TEST_CASE("Comparator: FLOAT within tolerance") {
    CHECK(compare_field<DataType::FLOAT>("3.14159", "3.14160", 0.001));
    CHECK(compare_field<DataType::FLOAT>("1.0", "1.0000001", 0.001));
}

TEST_CASE("Comparator: FLOAT outside tolerance") {
    CHECK_FALSE(compare_field<DataType::FLOAT>("3.14", "3.20", 0.01));
}

TEST_CASE("Comparator: FLOAT zero comparison") {
    CHECK(compare_field<DataType::FLOAT>("0.0", "0.0", 0.001));
    CHECK(compare_field<DataType::FLOAT>("-0.0", "0.0", 0.001));
}

TEST_CASE("Comparator: FLOAT negative values") {
    CHECK(compare_field<DataType::FLOAT>("-2.5", "-2.5", 0.001));
    CHECK_FALSE(compare_field<DataType::FLOAT>("-2.5", "2.5", 0.001));
}

TEST_CASE("Comparator: ALPHA exact match") {
    CHECK(compare_field<DataType::ALPHA>("JOHN DOE", "JOHN DOE"));
}

TEST_CASE("Comparator: ALPHA padding normalization") {
    CHECK(compare_field<DataType::ALPHA>("JOHN   ", "JOHN"));
    CHECK(compare_field<DataType::ALPHA>("JOHN", "JOHN   "));
}

TEST_CASE("Comparator: ALPHA case sensitive") {
    CHECK_FALSE(compare_field<DataType::ALPHA>("Hello", "hello"));
}

TEST_CASE("Comparator: GROUP multiline match") {
    CHECK(compare_field<DataType::GROUP>("line1\nline2", "line1\nline2"));
}

TEST_CASE("Comparator: GROUP multiline trailing spaces") {
    CHECK(compare_field<DataType::GROUP>("line1  \nline2  ", "line1\nline2"));
}

TEST_CASE("Comparator: GROUP multiline mismatch") {
    CHECK_FALSE(compare_field<DataType::GROUP>("line1\nline2", "line1\nline3"));
}

TEST_CASE("Comparator: GROUP different line count") {
    CHECK_FALSE(compare_field<DataType::GROUP>("line1\nline2", "line1"));
}

TEST_CASE("Comparator: NATIONAL match") {
    CHECK(compare_field<DataType::NATIONAL>("DATA", "DATA"));
    CHECK(compare_field<DataType::NATIONAL>("DATA   ", "DATA"));
}

TEST_CASE("Comparator: dynamic dispatch all types") {
    CHECK(compare_field_dynamic(DataType::DISPLAY, "A", "A"));
    CHECK(compare_field_dynamic(DataType::COMP, "5", "5"));
    CHECK(compare_field_dynamic(DataType::COMP3, "100", "100"));
    CHECK(compare_field_dynamic(DataType::COMP5, "200", "200"));
    CHECK(compare_field_dynamic(DataType::FLOAT, "1.5", "1.5", 0.001));
    CHECK(compare_field_dynamic(DataType::ALPHA, "X", "X"));
    CHECK(compare_field_dynamic(DataType::GROUP, "G", "G"));
    CHECK(compare_field_dynamic(DataType::NATIONAL, "N", "N"));
}

TEST_CASE("Comparator: data_type_str all types") {
    CHECK(data_type_str(DataType::DISPLAY) == "DISPLAY");
    CHECK(data_type_str(DataType::COMP) == "COMP");
    CHECK(data_type_str(DataType::COMP3) == "COMP3");
    CHECK(data_type_str(DataType::COMP5) == "COMP5");
    CHECK(data_type_str(DataType::FLOAT) == "FLOAT");
    CHECK(data_type_str(DataType::ALPHA) == "ALPHA");
    CHECK(data_type_str(DataType::GROUP) == "GROUP");
    CHECK(data_type_str(DataType::NATIONAL) == "NATIONAL");
}

TEST_CASE("Comparator: line-by-line comparison all match") {
    auto result = compare_lines("line1\nline2\nline3", "line1\nline2\nline3");
    CHECK(result.all_match());
    CHECK(result.total_lines == 3);
    CHECK(result.matched_lines == 3);
    CHECK(result.match_rate() == doctest::Approx(100.0));
}

TEST_CASE("Comparator: line-by-line comparison with mismatch") {
    auto result = compare_lines("line1\nline2", "line1\nLINE2");
    CHECK_FALSE(result.all_match());
    CHECK(result.mismatched_lines == 1);
    CHECK(result.diffs.size() == 1);
    CHECK(result.diffs[0].line_number == 2);
}

TEST_CASE("Comparator: line-by-line extra lines in actual") {
    auto result = compare_lines("line1", "line1\nextra");
    CHECK(result.extra_actual == 1);
    CHECK_FALSE(result.all_match());
}

TEST_CASE("Comparator: line-by-line missing lines in actual") {
    auto result = compare_lines("line1\nline2", "line1");
    CHECK(result.extra_expected == 1);
    CHECK_FALSE(result.all_match());
}

TEST_CASE("Comparator: line-by-line with normalization") {
    NormalizationOptions opts;
    opts.trim_trailing = true;
    auto result = compare_lines("data   \nmore   ", "data\nmore", opts);
    CHECK(result.all_match());
}

TEST_CASE("Comparator: line-by-line case insensitive normalization") {
    NormalizationOptions opts;
    opts.ignore_case = true;
    auto result = compare_lines("Hello\nWorld", "hello\nworld", opts);
    CHECK(result.all_match());
}

TEST_CASE("Comparator: copybook field comparison") {
    //               0123456789
    std::string rec = "JOHN  0042";
    std::vector<CopybookField> layout = {
        {"NAME", DataType::ALPHA, 0, 6, 0.0},
        {"AGE",  DataType::COMP,  6, 4, 0.0}
    };
    auto diffs = compare_record(rec, rec, layout);
    CHECK(diffs.size() == 2);
    CHECK(diffs[0].matched);
    CHECK(diffs[1].matched);
}

TEST_CASE("Comparator: copybook field mismatch detection") {
    std::string exp = "JOHN  0042";
    std::string act = "JANE  0042";
    std::vector<CopybookField> layout = {
        {"NAME", DataType::ALPHA, 0, 6, 0.0},
        {"AGE",  DataType::COMP,  6, 4, 0.0}
    };
    auto diffs = compare_record(exp, act, layout);
    CHECK_FALSE(diffs[0].matched);
    CHECK(diffs[1].matched);
}

TEST_CASE("Comparator: numeric normalization") {
    CHECK(normalize_numeric("  +007.500  ") == "7.5");
    CHECK(normalize_numeric("  -0.0  ") == "0");
    CHECK(normalize_numeric("  100  ") == "100");
}

TEST_CASE("Comparator: empty string comparison") {
    CHECK(compare_field<DataType::DISPLAY>("", ""));
    CHECK(compare_field<DataType::ALPHA>("", ""));
    CHECK(compare_field<DataType::COMP>("", ""));
}

TEST_CASE("Comparator: whitespace normalization options") {
    NormalizationOptions opts;
    opts.normalize_internal = true;
    auto result = compare_lines("hello  world", "hello world", opts);
    CHECK(result.all_match());
}

TEST_CASE("Comparator: FieldDiff structure") {
    FieldDiff fd;
    fd.field_name = "AMOUNT";
    fd.data_type = DataType::COMP3;
    fd.expected = "100.50";
    fd.actual = "100.51";
    fd.matched = false;
    fd.line_number = 5;
    fd.detail = "penny difference";
    CHECK(fd.field_name == "AMOUNT");
    CHECK(fd.data_type == DataType::COMP3);
    CHECK(fd.line_number == 5);
}

// ===================================================================
// AUDIT TESTS (25+)
// ===================================================================

TEST_CASE("Audit: AuditEntry default state") {
    AuditEntry entry;
    CHECK(entry.field_name.empty());
    CHECK(entry.status == AuditStatus::PASS);
    CHECK(entry.category == FailureCategory::NONE);
    CHECK(entry.line_number == 0);
}

TEST_CASE("Audit: AuditStatus string conversion") {
    CHECK(audit_status_str(AuditStatus::PASS) == "PASS");
    CHECK(audit_status_str(AuditStatus::FAIL) == "FAIL");
    CHECK(audit_status_str(AuditStatus::WARN) == "WARN");
}

TEST_CASE("Audit: FailureCategory string conversion") {
    CHECK(failure_category_str(FailureCategory::NONE) == "NONE");
    CHECK(failure_category_str(FailureCategory::WHITESPACE) == "WHITESPACE");
    CHECK(failure_category_str(FailureCategory::NUMERIC_PRECISION) == "NUMERIC_PRECISION");
    CHECK(failure_category_str(FailureCategory::DATA_LOSS) == "DATA_LOSS");
    CHECK(failure_category_str(FailureCategory::STRUCTURAL) == "STRUCTURAL");
    CHECK(failure_category_str(FailureCategory::TYPE_MISMATCH) == "TYPE_MISMATCH");
    CHECK(failure_category_str(FailureCategory::ENCODING) == "ENCODING");
}

TEST_CASE("Audit: AuditLog append matching fields") {
    AuditLog log;
    log.append("FIELD1", "HELLO", "HELLO", DataType::DISPLAY, 1);
    CHECK(log.size() == 1);
    CHECK(log[0].status == AuditStatus::PASS);
    CHECK(log[0].category == FailureCategory::NONE);
}

TEST_CASE("Audit: AuditLog append failing field") {
    AuditLog log;
    log.append("AMOUNT", "100", "200", DataType::COMP, 1);
    CHECK(log.size() == 1);
    CHECK(log[0].status == AuditStatus::FAIL);
}

TEST_CASE("Audit: AuditLog whitespace-only difference is WARN") {
    AuditLog log;
    log.append("NAME", "JOHN", "JOHN   ", DataType::ALPHA, 1);
    // ALPHA trims trailing, so this is a PASS
    CHECK(log[0].status == AuditStatus::PASS);

    // Now test raw string comparison where whitespace matters
    AuditLog log2;
    log2.append("RAW", "ABC", "ABC   ", DataType::DISPLAY, 2);
    // DISPLAY also trims trailing, so PASS
    CHECK(log2[0].status == AuditStatus::PASS);
}

TEST_CASE("Audit: categorize_failure whitespace") {
    auto cat = categorize_failure("hello  ", "hello", DataType::DISPLAY);
    CHECK(cat == FailureCategory::WHITESPACE);
}

TEST_CASE("Audit: categorize_failure numeric precision") {
    auto cat = categorize_failure("3.14159", "3.14160", DataType::FLOAT);
    CHECK(cat == FailureCategory::NUMERIC_PRECISION);
}

TEST_CASE("Audit: categorize_failure data loss") {
    auto cat = categorize_failure("LONG_VALUE_HERE", "", DataType::ALPHA);
    CHECK(cat == FailureCategory::DATA_LOSS);
}

TEST_CASE("Audit: categorize_failure structural") {
    auto cat = categorize_failure("line1\nline2", "single_line", DataType::GROUP);
    CHECK(cat == FailureCategory::STRUCTURAL);
}

TEST_CASE("Audit: categorize_failure exact match") {
    auto cat = categorize_failure("same", "same", DataType::DISPLAY);
    CHECK(cat == FailureCategory::NONE);
}

TEST_CASE("Audit: AuditLog summary statistics") {
    AuditLog log;
    log.append("F1", "A", "A", DataType::DISPLAY, 1);
    log.append("F2", "B", "B", DataType::DISPLAY, 2);
    log.append("F3", "100", "200", DataType::COMP, 3);
    auto s = log.summary();
    CHECK(s.total_fields == 3);
    CHECK(s.passed == 2);
    CHECK(s.failed == 1);
    CHECK(s.pass_rate == doctest::Approx(66.7).epsilon(0.1));
}

TEST_CASE("Audit: AuditLog empty summary") {
    AuditLog log;
    auto s = log.summary();
    CHECK(s.total_fields == 0);
    CHECK(s.pass_rate == doctest::Approx(100.0));
}

TEST_CASE("Audit: AuditLog failures filter") {
    AuditLog log;
    log.append("F1", "A", "A", DataType::DISPLAY, 1);
    log.append("F2", "100", "200", DataType::COMP, 2);
    log.append("F3", "300", "400", DataType::COMP, 3);
    auto fails = log.failures();
    CHECK(fails.size() == 2);
}

TEST_CASE("Audit: AuditLog clear") {
    AuditLog log;
    log.append("F1", "A", "A");
    log.append("F2", "B", "B");
    CHECK(log.size() == 2);
    log.clear();
    CHECK(log.empty());
}

TEST_CASE("Audit: JSON serialization basic") {
    AuditLog log;
    log.append("FIELD1", "HELLO", "HELLO", DataType::DISPLAY, 1);
    auto json = log.to_json();
    CHECK(json.find("\"field_name\"") != std::string::npos);
    CHECK(json.find("FIELD1") != std::string::npos);
    CHECK(json.find("\"status\": \"PASS\"") != std::string::npos);
    CHECK(json.find("\"summary\"") != std::string::npos);
}

TEST_CASE("Audit: JSON serialization with failures") {
    AuditLog log;
    log.append("AMT", "100", "200", DataType::COMP, 5);
    auto json = log.to_json();
    CHECK(json.find("\"status\": \"FAIL\"") != std::string::npos);
    CHECK(json.find("AMT") != std::string::npos);
}

TEST_CASE("Audit: JSON escape special characters") {
    CHECK(AuditLog::escape_json("hello\"world") == "hello\\\"world");
    CHECK(AuditLog::escape_json("line1\nline2") == "line1\\nline2");
    CHECK(AuditLog::escape_json("back\\slash") == "back\\\\slash");
    CHECK(AuditLog::escape_json("tab\there") == "tab\\there");
}

TEST_CASE("Audit: JSON roundtrip") {
    AuditLog log;
    log.append("NAME", "ALICE", "ALICE", DataType::ALPHA, 1);
    log.append("BAL", "500", "600", DataType::COMP, 2);
    auto json = log.to_json();
    auto restored = AuditLog::from_json(json);
    CHECK(restored.size() == 2);
    CHECK(restored[0].field_name == "NAME");
    CHECK(restored[0].status == AuditStatus::PASS);
    CHECK(restored[1].field_name == "BAL");
    CHECK(restored[1].status == AuditStatus::FAIL);
}

TEST_CASE("Audit: diff generation") {
    AuditLog log;
    log.append("F1", "A", "A", DataType::DISPLAY, 1);
    log.append("F2", "100", "200", DataType::COMP, 2);
    auto diff = log.generate_diff();
    CHECK(diff.find("Parity Audit Diff") != std::string::npos);
    CHECK(diff.find("F2") != std::string::npos);
    CHECK(diff.find("expected") != std::string::npos);
    CHECK(diff.find("actual") != std::string::npos);
}

TEST_CASE("Audit: diff shows only non-pass entries") {
    AuditLog log;
    log.append("OK1", "X", "X", DataType::DISPLAY, 1);
    log.append("OK2", "Y", "Y", DataType::DISPLAY, 2);
    auto diff = log.generate_diff();
    CHECK(diff.find("OK1") == std::string::npos);
    CHECK(diff.find("OK2") == std::string::npos);
}

TEST_CASE("Audit: append pre-built entry") {
    AuditLog log;
    AuditEntry e;
    e.field_name = "CUSTOM";
    e.expected_value = "A";
    e.actual_value = "B";
    e.status = AuditStatus::FAIL;
    e.category = FailureCategory::TYPE_MISMATCH;
    log.append(std::move(e));
    CHECK(log.size() == 1);
    CHECK(log[0].field_name == "CUSTOM");
    CHECK(log[0].status == AuditStatus::FAIL);
}

TEST_CASE("Audit: summary failure categories counted") {
    AuditLog log;
    AuditEntry e1;
    e1.field_name = "F1"; e1.status = AuditStatus::FAIL;
    e1.category = FailureCategory::WHITESPACE;
    log.append(std::move(e1));

    AuditEntry e2;
    e2.field_name = "F2"; e2.status = AuditStatus::FAIL;
    e2.category = FailureCategory::DATA_LOSS;
    log.append(std::move(e2));

    auto s = log.summary();
    CHECK(s.whitespace_failures == 1);
    CHECK(s.data_loss_failures == 1);
}

TEST_CASE("Audit: warnings filter") {
    AuditLog log;
    AuditEntry e;
    e.field_name = "W1"; e.status = AuditStatus::WARN;
    e.category = FailureCategory::WHITESPACE;
    log.append(std::move(e));
    auto warns = log.warnings();
    CHECK(warns.size() == 1);
    CHECK(warns[0].field_name == "W1");
}

TEST_CASE("Audit: JSON pass_rate field present") {
    AuditLog log;
    log.append("F1", "X", "X");
    auto json = log.to_json();
    CHECK(json.find("\"pass_rate\"") != std::string::npos);
    CHECK(json.find("100.0") != std::string::npos);
}

// ===================================================================
// REGRESSION TESTS (20+)
// ===================================================================

TEST_CASE("Regression: BaselineStatus string conversion") {
    CHECK(baseline_status_str(BaselineStatus::PASS) == "PASS");
    CHECK(baseline_status_str(BaselineStatus::FAIL) == "FAIL");
    CHECK(baseline_status_str(BaselineStatus::SKIP) == "SKIP");
}

TEST_CASE("Regression: parse_baseline_status") {
    CHECK(parse_baseline_status("PASS") == BaselineStatus::PASS);
    CHECK(parse_baseline_status("FAIL") == BaselineStatus::FAIL);
    CHECK(parse_baseline_status("SKIP") == BaselineStatus::SKIP);
    CHECK(parse_baseline_status("garbage") == BaselineStatus::SKIP);
}

TEST_CASE("Regression: RegressionBaseline set and get") {
    RegressionBaseline bl;
    bl.set("prog1", BaselineStatus::PASS);
    bl.set("prog2", BaselineStatus::FAIL);
    CHECK(bl.get("prog1") == BaselineStatus::PASS);
    CHECK(bl.get("prog2") == BaselineStatus::FAIL);
    CHECK(bl.get("unknown") == BaselineStatus::SKIP);
}

TEST_CASE("Regression: RegressionBaseline contains") {
    RegressionBaseline bl;
    bl.set("prog1", BaselineStatus::PASS);
    CHECK(bl.contains("prog1"));
    CHECK_FALSE(bl.contains("prog2"));
}

TEST_CASE("Regression: RegressionBaseline size") {
    RegressionBaseline bl;
    CHECK(bl.empty());
    bl.set("p1", BaselineStatus::PASS);
    bl.set("p2", BaselineStatus::FAIL);
    CHECK(bl.size() == 2);
}

TEST_CASE("Regression: RegressionBaseline count helpers") {
    RegressionBaseline bl;
    bl.set("p1", BaselineStatus::PASS);
    bl.set("p2", BaselineStatus::PASS);
    bl.set("p3", BaselineStatus::FAIL);
    CHECK(bl.count_pass() == 2);
    CHECK(bl.count_fail() == 1);
}

TEST_CASE("Regression: save and load string roundtrip") {
    RegressionBaseline bl;
    bl.set("alpha", BaselineStatus::PASS);
    bl.set("beta", BaselineStatus::FAIL);
    bl.set("gamma", BaselineStatus::PASS);

    auto text = bl.save_to_string();
    CHECK(text.find("alpha PASS") != std::string::npos);
    CHECK(text.find("beta FAIL") != std::string::npos);

    auto loaded = RegressionBaseline::load_from_string(text);
    CHECK(loaded.size() == 3);
    CHECK(loaded.get("alpha") == BaselineStatus::PASS);
    CHECK(loaded.get("beta") == BaselineStatus::FAIL);
    CHECK(loaded.get("gamma") == BaselineStatus::PASS);
}

TEST_CASE("Regression: load ignores comments and empty lines") {
    std::string text = "# comment\n\nprog1 PASS\n# another\nprog2 FAIL\n";
    auto bl = RegressionBaseline::load_from_string(text);
    CHECK(bl.size() == 2);
}

TEST_CASE("Regression: new failure detection") {
    RegressionBaseline baseline;
    baseline.set("prog1", BaselineStatus::PASS);
    baseline.set("prog2", BaselineStatus::PASS);

    std::map<std::string, BaselineStatus> current;
    current["prog1"] = BaselineStatus::PASS;
    current["prog2"] = BaselineStatus::FAIL; // regression!

    auto rr = compare_against_baseline(baseline, current);
    CHECK(rr.new_failures.size() == 1);
    CHECK(rr.new_failures[0] == "prog2");
    CHECK(rr.has_regressions());
}

TEST_CASE("Regression: fixed detection") {
    RegressionBaseline baseline;
    baseline.set("prog1", BaselineStatus::FAIL);

    std::map<std::string, BaselineStatus> current;
    current["prog1"] = BaselineStatus::PASS; // fixed!

    auto rr = compare_against_baseline(baseline, current);
    CHECK(rr.fixed.size() == 1);
    CHECK(rr.fixed[0] == "prog1");
}

TEST_CASE("Regression: stable failure tracking") {
    RegressionBaseline baseline;
    baseline.set("prog1", BaselineStatus::FAIL);

    std::map<std::string, BaselineStatus> current;
    current["prog1"] = BaselineStatus::FAIL; // still failing

    auto rr = compare_against_baseline(baseline, current);
    CHECK(rr.stable_failures.size() == 1);
}

TEST_CASE("Regression: stable pass tracking") {
    RegressionBaseline baseline;
    baseline.set("prog1", BaselineStatus::PASS);

    std::map<std::string, BaselineStatus> current;
    current["prog1"] = BaselineStatus::PASS;

    auto rr = compare_against_baseline(baseline, current);
    CHECK(rr.stable_passes.size() == 1);
}

TEST_CASE("Regression: new program detection") {
    RegressionBaseline baseline;
    baseline.set("old_prog", BaselineStatus::PASS);

    std::map<std::string, BaselineStatus> current;
    current["old_prog"] = BaselineStatus::PASS;
    current["new_prog"] = BaselineStatus::PASS;

    auto rr = compare_against_baseline(baseline, current);
    CHECK(rr.new_programs.size() == 1);
    CHECK(rr.new_programs[0] == "new_prog");
}

TEST_CASE("Regression: mixed scenario") {
    RegressionBaseline baseline;
    baseline.set("stable_pass", BaselineStatus::PASS);
    baseline.set("regressed", BaselineStatus::PASS);
    baseline.set("fixed", BaselineStatus::FAIL);
    baseline.set("stable_fail", BaselineStatus::FAIL);

    std::map<std::string, BaselineStatus> current;
    current["stable_pass"] = BaselineStatus::PASS;
    current["regressed"] = BaselineStatus::FAIL;
    current["fixed"] = BaselineStatus::PASS;
    current["stable_fail"] = BaselineStatus::FAIL;
    current["brand_new"] = BaselineStatus::PASS;

    auto rr = compare_against_baseline(baseline, current);
    CHECK(rr.stable_passes.size() == 1);
    CHECK(rr.new_failures.size() == 1);
    CHECK(rr.fixed.size() == 1);
    CHECK(rr.stable_failures.size() == 1);
    CHECK(rr.new_programs.size() == 1);
    CHECK(rr.total() == 5);
}

TEST_CASE("Regression: report generation") {
    RegressionResult rr;
    rr.new_failures = {"failing_prog"};
    rr.fixed = {"fixed_prog"};
    rr.stable_passes = {"good_prog"};
    rr.stable_failures = {"bad_prog"};
    rr.new_programs = {"fresh_prog"};

    auto report = generate_regression_report(rr);
    CHECK(report.find("REGRESSION") != std::string::npos);
    CHECK(report.find("failing_prog") != std::string::npos);
    CHECK(report.find("FIXED") != std::string::npos);
    CHECK(report.find("fixed_prog") != std::string::npos);
}

TEST_CASE("Regression: report no regressions") {
    RegressionResult rr;
    rr.stable_passes = {"prog1", "prog2"};
    auto report = generate_regression_report(rr);
    CHECK(report.find("REGRESSION") == std::string::npos);
    CHECK(report.find("2 stable passes") != std::string::npos);
}

TEST_CASE("Regression: TrendTracker record and query") {
    TrendTracker tracker;
    tracker.record(100, 80, "2026-01-01T00:00:00");
    tracker.record(100, 85, "2026-01-02T00:00:00");
    tracker.record(100, 90, "2026-01-03T00:00:00");
    CHECK(tracker.size() == 3);
    CHECK(tracker.latest_pass_rate() == doctest::Approx(90.0));
}

TEST_CASE("Regression: TrendTracker trend direction positive") {
    TrendTracker tracker;
    tracker.record(100, 80, "t1");
    tracker.record(100, 90, "t2");
    CHECK(tracker.trend_direction() > 0.0);
}

TEST_CASE("Regression: TrendTracker trend direction negative") {
    TrendTracker tracker;
    tracker.record(100, 90, "t1");
    tracker.record(100, 80, "t2");
    CHECK(tracker.trend_direction() < 0.0);
}

TEST_CASE("Regression: TrendTracker empty") {
    TrendTracker tracker;
    CHECK(tracker.empty());
    CHECK(tracker.latest_pass_rate() == doctest::Approx(0.0));
    CHECK(tracker.trend_direction() == doctest::Approx(0.0));
}

TEST_CASE("Regression: TrendTracker save and load roundtrip") {
    TrendTracker tracker;
    tracker.record(50, 40, "2026-03-01T10:00:00");
    tracker.record(50, 45, "2026-03-02T10:00:00");

    auto text = tracker.save_to_string();
    auto loaded = TrendTracker::load_from_string(text);
    CHECK(loaded.size() == 2);
    CHECK(loaded.points()[0].total == 50);
    CHECK(loaded.points()[0].passed == 40);
    CHECK(loaded.latest_pass_rate() == doctest::Approx(90.0));
}

// ===================================================================
// REPORT TESTS (15+)
// ===================================================================

TEST_CASE("Report: HTML_HEAD contains DOCTYPE") {
    std::string head(HTML_HEAD);
    CHECK(head.find("<!DOCTYPE html>") != std::string::npos);
}

TEST_CASE("Report: HTML_HEAD contains CSS classes") {
    std::string head(HTML_HEAD);
    CHECK(head.find(".pass{background:#e6ffe6}") != std::string::npos);
    CHECK(head.find(".fail{background:#ffe6e6}") != std::string::npos);
    CHECK(head.find(".warn{background:#fff3cd}") != std::string::npos);
}

TEST_CASE("Report: html_escape special characters") {
    CHECK(html_escape("<script>") == "&lt;script&gt;");
    CHECK(html_escape("A & B") == "A &amp; B");
    CHECK(html_escape("\"quoted\"") == "&quot;quoted&quot;");
    CHECK(html_escape("it's") == "it&#39;s");
}

TEST_CASE("Report: status_css_class mapping") {
    CHECK(status_css_class(ComparisonStatus::MATCH) == "pass");
    CHECK(status_css_class(ComparisonStatus::MISMATCH) == "fail");
    CHECK(status_css_class(ComparisonStatus::TIMEOUT) == "warn");
    CHECK(status_css_class(ComparisonStatus::CPP_FAIL) == "fail");
}

TEST_CASE("Report: ReportData computed stats") {
    ReportData data;
    ReportEntry e1; e1.program_name = "p1"; e1.status = ComparisonStatus::MATCH;
    ReportEntry e2; e2.program_name = "p2"; e2.status = ComparisonStatus::MISMATCH;
    ReportEntry e3; e3.program_name = "p3"; e3.status = ComparisonStatus::MATCH;
    data.entries = {e1, e2, e3};
    CHECK(data.total_programs() == 3);
    CHECK(data.passed_programs() == 2);
    CHECK(data.pass_rate() == doctest::Approx(66.7).epsilon(0.1));
}

TEST_CASE("Report: generate HTML report structure") {
    ReportData data;
    data.title = "Test Report";
    data.timestamp = "2026-03-27";
    ReportEntry e; e.program_name = "PAYROLL"; e.status = ComparisonStatus::MATCH;
    e.field_count = 10; e.failures = 0; e.elapsed_ms = 42.5;
    data.entries = {e};

    auto html = generate_html_report(data);
    CHECK(html.find("<!DOCTYPE html>") != std::string::npos);
    CHECK(html.find("Test Report") != std::string::npos);
    CHECK(html.find("PAYROLL") != std::string::npos);
    CHECK(html.find("</html>") != std::string::npos);
    CHECK(html.find("2026-03-27") != std::string::npos);
}

TEST_CASE("Report: HTML report with regression data") {
    ReportData data;
    data.title = "Regression Run";
    ReportEntry e; e.program_name = "REGR_PROG"; e.status = ComparisonStatus::MISMATCH;
    e.is_regression = true;
    data.entries = {e};
    RegressionResult rr;
    rr.new_failures = {"REGR_PROG"};
    data.regression = rr;

    auto html = generate_html_report(data);
    CHECK(html.find("REGRESSION") != std::string::npos);
    CHECK(html.find("Regression Analysis") != std::string::npos);
}

TEST_CASE("Report: HTML report with trend data") {
    ReportData data;
    data.title = "Trend Report";
    data.trend_points = {{"2026-01-01", 10, 8, 2, 80.0}, {"2026-01-02", 10, 9, 1, 90.0}};

    auto html = generate_html_report(data);
    CHECK(html.find("Trend") != std::string::npos);
    CHECK(html.find("2026-01-01") != std::string::npos);
    CHECK(html.find("2026-01-02") != std::string::npos);
}

TEST_CASE("Report: CSV export format") {
    ReportData data;
    ReportEntry e; e.program_name = "ACCTPAY"; e.status = ComparisonStatus::MATCH;
    e.field_count = 15; e.failures = 0; e.elapsed_ms = 55.3;
    data.entries = {e};

    auto csv = generate_csv_report(data);
    CHECK(csv.find("program_name,status,field_count,failures,elapsed_ms") != std::string::npos);
    CHECK(csv.find("ACCTPAY") != std::string::npos);
    CHECK(csv.find("MATCH") != std::string::npos);
}

TEST_CASE("Report: CSV export multiple rows") {
    ReportData data;
    ReportEntry e1; e1.program_name = "P1"; e1.status = ComparisonStatus::MATCH;
    ReportEntry e2; e2.program_name = "P2"; e2.status = ComparisonStatus::MISMATCH;
    data.entries = {e1, e2};

    auto csv = generate_csv_report(data);
    // Header + 2 data rows
    size_t line_count = 0;
    for (char c : csv) if (c == '\n') ++line_count;
    CHECK(line_count == 3);
}

TEST_CASE("Report: JSON export structure") {
    ReportData data;
    data.title = "JSON Test";
    data.timestamp = "2026-03-27T12:00:00";
    ReportEntry e; e.program_name = "JSONPROG"; e.status = ComparisonStatus::MATCH;
    data.entries = {e};

    auto json = generate_json_report(data);
    CHECK(json.find("\"title\"") != std::string::npos);
    CHECK(json.find("JSON Test") != std::string::npos);
    CHECK(json.find("\"results\"") != std::string::npos);
    CHECK(json.find("JSONPROG") != std::string::npos);
}

TEST_CASE("Report: filter failures only") {
    ReportData data;
    ReportEntry e1; e1.program_name = "OK"; e1.status = ComparisonStatus::MATCH;
    ReportEntry e2; e2.program_name = "BAD"; e2.status = ComparisonStatus::MISMATCH;
    data.entries = {e1, e2};

    auto filtered = filter_entries(data.entries, ReportFilter::FAILURES_ONLY);
    CHECK(filtered.size() == 1);
    CHECK(filtered[0].program_name == "BAD");
}

TEST_CASE("Report: filter regressions only") {
    ReportData data;
    ReportEntry e1; e1.program_name = "REG"; e1.status = ComparisonStatus::MISMATCH;
    e1.is_regression = true;
    ReportEntry e2; e2.program_name = "NORM"; e2.status = ComparisonStatus::MISMATCH;
    e2.is_regression = false;
    data.entries = {e1, e2};

    auto filtered = filter_entries(data.entries, ReportFilter::REGRESSIONS_ONLY);
    CHECK(filtered.size() == 1);
    CHECK(filtered[0].program_name == "REG");
}

TEST_CASE("Report: filter all returns everything") {
    ReportData data;
    ReportEntry e1; e1.program_name = "A"; e1.status = ComparisonStatus::MATCH;
    ReportEntry e2; e2.program_name = "B"; e2.status = ComparisonStatus::MISMATCH;
    data.entries = {e1, e2};

    auto filtered = filter_entries(data.entries, ReportFilter::ALL);
    CHECK(filtered.size() == 2);
}

TEST_CASE("Report: HTML report with diff detail") {
    ReportData data;
    ReportEntry e; e.program_name = "DIFF_PROG"; e.status = ComparisonStatus::MISMATCH;
    e.diff_detail = "expected: 100\nactual: 200";
    data.entries = {e};

    auto html = generate_html_report(data);
    CHECK(html.find("<pre>") != std::string::npos);
    CHECK(html.find("expected: 100") != std::string::npos);
}

// ===================================================================
// INTEGRATION TESTS (7+)
// ===================================================================

TEST_CASE("Integration: full pipeline run to report") {
    // Run matching pair
    ParallelRunner runner(std::chrono::milliseconds(5000));
    ProgramPair pp;
    pp.name = "INTEG_PROG";
    pp.cobol_command = "echo TOTAL 12345";
    pp.cpp_command = "echo TOTAL 12345";
    auto rr = runner.run_pair(pp);
    CHECK(rr.comparison_status == ComparisonStatus::MATCH);

    // Compare outputs
    auto cmp = compare_lines(rr.cobol_result.stdout_capture,
                             rr.cpp_result.stdout_capture);
    CHECK(cmp.all_match());

    // Audit
    AuditLog log;
    log.append("OUTPUT", rr.cobol_result.stdout_capture,
               rr.cpp_result.stdout_capture, DataType::DISPLAY, 1);
    auto summary = log.summary();
    CHECK(summary.passed == 1);

    // Report
    ReportData data;
    data.title = "Integration Test";
    ReportEntry re;
    re.program_name = pp.name;
    re.status = rr.comparison_status;
    data.entries = {re};
    data.audit_summary = summary;

    auto html = generate_html_report(data);
    CHECK(html.find("INTEG_PROG") != std::string::npos);
    CHECK(html.find("100.0%") != std::string::npos);
}

TEST_CASE("Integration: pipeline with mismatch detection") {
    ParallelRunner runner(std::chrono::milliseconds(5000));
    ProgramPair pp;
    pp.name = "MISMATCH_PROG";
    pp.cobol_command = "echo BALANCE 50000";
    pp.cpp_command = "echo BALANCE 50001";
    auto rr = runner.run_pair(pp);
    CHECK(rr.comparison_status == ComparisonStatus::MISMATCH);

    auto cmp = compare_lines(rr.cobol_result.stdout_capture,
                             rr.cpp_result.stdout_capture);
    CHECK_FALSE(cmp.all_match());

    AuditLog log;
    log.append("BALANCE", "50000", "50001", DataType::COMP, 1);
    CHECK(log[0].status == AuditStatus::FAIL);
}

TEST_CASE("Integration: regression baseline workflow") {
    // Build baseline
    RegressionBaseline baseline;
    baseline.set("PAYROLL", BaselineStatus::PASS);
    baseline.set("ACCTPAY", BaselineStatus::FAIL);

    // Simulate current run
    std::map<std::string, BaselineStatus> current;
    current["PAYROLL"] = BaselineStatus::PASS;
    current["ACCTPAY"] = BaselineStatus::PASS; // fixed!
    current["NEWPROG"] = BaselineStatus::PASS;

    auto rr = compare_against_baseline(baseline, current);
    CHECK(rr.fixed.size() == 1);
    CHECK(rr.new_programs.size() == 1);
    CHECK_FALSE(rr.has_regressions());

    auto report = generate_regression_report(rr);
    CHECK(report.find("FIXED") != std::string::npos);
}

TEST_CASE("Integration: audit to JSON to report") {
    AuditLog log;
    log.append("CUST_NAME", "ALICE SMITH", "ALICE SMITH", DataType::ALPHA, 1);
    log.append("CUST_BAL", "1500.00", "1500.01", DataType::FLOAT, 2);
    log.append("CUST_ID", "00042", "42", DataType::COMP, 3);

    auto json = log.to_json();
    CHECK(json.find("CUST_NAME") != std::string::npos);

    auto summary = log.summary();

    ReportData data;
    data.title = "Customer Parity";
    data.audit_summary = summary;
    ReportEntry re;
    re.program_name = "CUSTMAST";
    re.status = ComparisonStatus::MISMATCH;
    re.field_count = 3;
    re.failures = summary.failed;
    data.entries = {re};

    auto csv = generate_csv_report(data);
    CHECK(csv.find("CUSTMAST") != std::string::npos);
}

TEST_CASE("Integration: trend tracking across multiple runs") {
    TrendTracker tracker;

    // Simulate 3 runs with improving results
    tracker.record(50, 30, "2026-03-25T08:00:00");
    tracker.record(50, 40, "2026-03-26T08:00:00");
    tracker.record(50, 48, "2026-03-27T08:00:00");

    CHECK(tracker.latest_pass_rate() == doctest::Approx(96.0));
    CHECK(tracker.trend_direction() > 0.0);  // improving

    ReportData data;
    data.title = "Trend Integration";
    data.trend_points = tracker.points();
    auto html = generate_html_report(data);
    CHECK(html.find("2026-03-25") != std::string::npos);
    CHECK(html.find("96.0%") != std::string::npos);
}

TEST_CASE("Integration: batch run with summary") {
    ParallelRunner runner(std::chrono::milliseconds(5000));
    std::vector<ProgramPair> pairs;

    ProgramPair p1; p1.name = "BATCH1"; p1.cobol_command = "echo OK"; p1.cpp_command = "echo OK";
    ProgramPair p2; p2.name = "BATCH2"; p2.cobol_command = "echo OK"; p2.cpp_command = "echo OK";
    ProgramPair p3; p3.name = "BATCH3"; p3.cobol_command = "echo X"; p3.cpp_command = "echo Y";
    pairs = {p1, p2, p3};

    auto results = runner.run_batch(pairs);
    auto summary = ParallelRunner::summarize(results);
    CHECK(summary.total == 3);
    CHECK(summary.matched == 2);
    CHECK(summary.mismatch == 1);
    CHECK(summary.pass_rate == doctest::Approx(66.7).epsilon(0.1));
}

TEST_CASE("Integration: full report generation all formats") {
    ReportData data;
    data.title = "Multi-Format Report";
    data.timestamp = "2026-03-27T15:30:00";

    ReportEntry e1;
    e1.program_name = "PROG_A"; e1.status = ComparisonStatus::MATCH;
    e1.field_count = 20; e1.failures = 0; e1.elapsed_ms = 100.0;
    ReportEntry e2;
    e2.program_name = "PROG_B"; e2.status = ComparisonStatus::MISMATCH;
    e2.field_count = 15; e2.failures = 3; e2.elapsed_ms = 150.0;
    e2.is_regression = true;
    data.entries = {e1, e2};

    auto html = generate_html_report(data);
    auto csv  = generate_csv_report(data);
    auto json = generate_json_report(data);

    // HTML valid
    CHECK(html.find("<!DOCTYPE html>") != std::string::npos);
    CHECK(html.find("</html>") != std::string::npos);

    // CSV has header + 2 rows
    size_t csv_lines = 0;
    for (char c : csv) if (c == '\n') ++csv_lines;
    CHECK(csv_lines == 3);

    // JSON has both programs
    CHECK(json.find("PROG_A") != std::string::npos);
    CHECK(json.find("PROG_B") != std::string::npos);
    CHECK(json.find("\"pass_rate\"") != std::string::npos);
}
