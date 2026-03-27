#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

#include "lazarus/scheduler/schedule_model.h"
#include "lazarus/scheduler/tws_parser.h"
#include "lazarus/scheduler/ca7_parser.h"
#include "lazarus/scheduler/airflow_emitter.h"
#include "lazarus/scheduler/cron_emitter.h"

using namespace lazarus::scheduler;

static Dependency make_dep(const std::string& pred, const std::string& succ,
                           DependencyCondition cond = DependencyCondition::RC_ZERO) {
    Dependency d;
    d.predecessor = pred;
    d.successor = succ;
    d.condition = cond;
    return d;
}

// ============================================================
// SCHEDULE MODEL TESTS (20+)
// ============================================================

TEST_CASE("Model: ScheduledJob creation with defaults") {
    ScheduledJob job;
    job.name = "BATCH001";
    job.description = "Nightly batch";
    job.owner = "OPS";
    CHECK(job.name == "BATCH001");
    CHECK(job.priority == JobPriority::NORMAL);
    CHECK(job.priority_value() == 5);
    CHECK(job.enabled == true);
}

TEST_CASE("Model: JobPriority values") {
    ScheduledJob low, normal, high, crit;
    low.priority = JobPriority::LOW;
    normal.priority = JobPriority::NORMAL;
    high.priority = JobPriority::HIGH;
    crit.priority = JobPriority::CRITICAL;
    CHECK(low.priority_value() == 1);
    CHECK(normal.priority_value() == 5);
    CHECK(high.priority_value() == 8);
    CHECK(crit.priority_value() == 10);
}

TEST_CASE("Model: Dependency condition strings") {
    Dependency d;
    d.condition = DependencyCondition::RC_ZERO;
    CHECK(d.condition_str() == "RC=0");
    d.condition = DependencyCondition::RC_LT4;
    CHECK(d.condition_str() == "RC<4");
    d.condition = DependencyCondition::RC_LT8;
    CHECK(d.condition_str() == "RC<8");
    d.condition = DependencyCondition::ALWAYS;
    CHECK(d.condition_str() == "ALWAYS");
    d.condition = DependencyCondition::ONLY_FAIL;
    CHECK(d.condition_str() == "ONLY_FAIL");
    d.condition = DependencyCondition::CUSTOM;
    d.custom_condition = "RC=4";
    CHECK(d.condition_str() == "RC=4");
}

TEST_CASE("Model: Calendar business day detection") {
    Calendar cal;
    cal.run_days = {DayOfWeek::MONDAY, DayOfWeek::TUESDAY, DayOfWeek::WEDNESDAY,
                    DayOfWeek::THURSDAY, DayOfWeek::FRIDAY};
    CHECK(cal.is_business_day_calendar());

    cal.run_days.insert(DayOfWeek::SATURDAY);
    CHECK_FALSE(cal.is_business_day_calendar());
}

TEST_CASE("Model: Calendar holiday check") {
    Calendar cal;
    cal.holidays = {"2024-12-25", "2024-01-01"};
    cal.free_days = {"2024-07-04"};
    CHECK(cal.is_holiday("2024-12-25"));
    CHECK(cal.is_holiday("2024-07-04"));
    CHECK_FALSE(cal.is_holiday("2024-06-15"));
}

TEST_CASE("Model: TimeWindow validity") {
    TimeWindow tw;
    CHECK(tw.is_valid());  // empty is valid

    tw.earliest_start = "06:00";
    tw.latest_start = "08:00";
    tw.deadline = "12:00";
    CHECK(tw.is_valid());

    tw.latest_start = "05:00";  // before earliest
    CHECK_FALSE(tw.is_valid());
}

TEST_CASE("Model: TimeWindow conflict detection") {
    TimeWindow a, b;
    a.earliest_start = "06:00";
    a.deadline = "10:00";
    b.earliest_start = "09:00";
    b.deadline = "14:00";
    CHECK(a.has_conflict(b));

    b.earliest_start = "11:00";
    b.deadline = "14:00";
    CHECK_FALSE(a.has_conflict(b));
}

TEST_CASE("Model: SpecialResource conflict detection") {
    SpecialResource a, b;
    a.name = "TAPE1";
    a.mode = ResourceMode::EXCLUSIVE;
    a.quantity = 1;
    b.name = "TAPE1";
    b.mode = ResourceMode::SHARED;
    b.quantity = 1;
    CHECK(a.conflicts_with(b));

    b.name = "TAPE2";
    CHECK_FALSE(a.conflicts_with(b));
}

TEST_CASE("Model: SpecialResource shared capacity conflict") {
    SpecialResource a, b;
    a.name = "SLOTS";
    a.mode = ResourceMode::SHARED;
    a.quantity = 3;
    a.availability = 4;
    b.name = "SLOTS";
    b.mode = ResourceMode::SHARED;
    b.quantity = 2;
    b.availability = 4;
    CHECK(b.conflicts_with(a));  // 3+2 > 4

    a.quantity = 1;
    CHECK_FALSE(b.conflicts_with(a));  // 1+2 <= 4
}

TEST_CASE("Model: Schedule add and retrieve jobs") {
    Schedule sched;
    ScheduledJob j1, j2;
    j1.name = "JOB_A";
    j2.name = "JOB_B";
    sched.add_job(j1);
    sched.add_job(j2);
    CHECK(sched.job_count() == 2);
    auto found = sched.get_job("JOB_A");
    CHECK(found.has_value());
    CHECK(found->name == "JOB_A");
    CHECK_FALSE(sched.get_job("NONEXIST").has_value());
}

TEST_CASE("Model: Schedule dependencies and predecessor/successor queries") {
    Schedule sched;
    ScheduledJob a, b, c;
    a.name = "A"; b.name = "B"; c.name = "C";
    sched.add_job(a);
    sched.add_job(b);
    sched.add_job(c);

    Dependency d1, d2;
    d1.predecessor = "A"; d1.successor = "B";
    d2.predecessor = "A"; d2.successor = "C";
    sched.add_dependency(d1);
    sched.add_dependency(d2);

    CHECK(sched.dependency_count() == 2);
    auto preds = sched.predecessors("B");
    CHECK(preds.size() == 1);
    CHECK(preds[0] == "A");
    auto succs = sched.successors("A");
    CHECK(succs.size() == 2);
}

TEST_CASE("Model: Topological sort linear chain") {
    Schedule sched;
    ScheduledJob a, b, c;
    a.name = "A"; b.name = "B"; c.name = "C";
    sched.add_job(a);
    sched.add_job(b);
    sched.add_job(c);
    sched.add_dependency(make_dep("A", "B"));
    sched.add_dependency(make_dep("B", "C"));

    auto order = sched.topological_order();
    CHECK(order.size() == 3);
    CHECK(order[0] == "A");
    CHECK(order[1] == "B");
    CHECK(order[2] == "C");
}

TEST_CASE("Model: Topological sort diamond") {
    Schedule sched;
    ScheduledJob a, b, c, d;
    a.name = "A"; b.name = "B"; c.name = "C"; d.name = "D";
    sched.add_job(a);
    sched.add_job(b);
    sched.add_job(c);
    sched.add_job(d);
    sched.add_dependency(make_dep("A", "B"));
    sched.add_dependency(make_dep("A", "C"));
    sched.add_dependency(make_dep("B", "D"));
    sched.add_dependency(make_dep("C", "D"));

    auto order = sched.topological_order();
    CHECK(order.size() == 4);
    CHECK(order[0] == "A");
    CHECK(order[3] == "D");
}

TEST_CASE("Model: Cycle detection empty schedule") {
    Schedule sched;
    auto cycles = sched.detect_cycles();
    CHECK(cycles.empty());
}

TEST_CASE("Model: Cycle detection simple cycle") {
    Schedule sched;
    ScheduledJob a, b;
    a.name = "A"; b.name = "B";
    sched.add_job(a);
    sched.add_job(b);
    sched.add_dependency(make_dep("A", "B"));
    sched.add_dependency(make_dep("B", "A"));

    auto cycles = sched.detect_cycles();
    CHECK_FALSE(cycles.empty());
    auto order = sched.topological_order();
    CHECK(order.empty());  // cycle means no valid topo order
}

TEST_CASE("Model: Cycle detection three-node cycle") {
    Schedule sched;
    ScheduledJob a, b, c;
    a.name = "A"; b.name = "B"; c.name = "C";
    sched.add_job(a);
    sched.add_job(b);
    sched.add_job(c);
    sched.add_dependency(make_dep("A", "B"));
    sched.add_dependency(make_dep("B", "C"));
    sched.add_dependency(make_dep("C", "A"));

    auto cycles = sched.detect_cycles();
    CHECK_FALSE(cycles.empty());
}

TEST_CASE("Model: Find unreachable jobs") {
    Schedule sched;
    ScheduledJob a, b, c;
    a.name = "A"; b.name = "B"; c.name = "C";
    sched.add_job(a);
    sched.add_job(b);
    sched.add_job(c);
    // A->B->C->B (B,C in cycle, unreachable from root A via normal BFS since B has predecessor)
    sched.add_dependency(make_dep("A", "B"));
    sched.add_dependency(make_dep("B", "C"));
    sched.add_dependency(make_dep("C", "B"));

    // A is root, B and C reachable from A
    auto unreachable = sched.find_unreachable();
    CHECK(unreachable.empty());
}

TEST_CASE("Model: Validate clean schedule") {
    Schedule sched;
    ScheduledJob a, b;
    a.name = "A"; b.name = "B";
    a.time_window.earliest_start = "06:00";
    a.time_window.deadline = "10:00";
    sched.add_job(a);
    sched.add_job(b);
    sched.add_dependency(make_dep("A", "B"));

    auto result = sched.validate();
    CHECK(result.valid);
    CHECK(result.errors.empty());
}

TEST_CASE("Model: Validate detects undefined predecessor") {
    Schedule sched;
    ScheduledJob a;
    a.name = "A";
    sched.add_job(a);
    sched.add_dependency(make_dep("MISSING", "A"));

    auto result = sched.validate();
    CHECK_FALSE(result.valid);
    CHECK(result.errors.size() >= 1);
}

TEST_CASE("Model: Validate detects invalid time window") {
    Schedule sched;
    ScheduledJob a;
    a.name = "A";
    a.time_window.earliest_start = "10:00";
    a.time_window.latest_start = "08:00"; // before earliest
    sched.add_job(a);

    auto result = sched.validate();
    CHECK_FALSE(result.valid);
}

TEST_CASE("Model: Schedule merge with prefix") {
    Schedule s1, s2;
    ScheduledJob a, b;
    a.name = "JOB1"; b.name = "JOB1"; // same name in different schedules
    s1.add_job(a);
    s2.add_job(b);
    s2.add_dependency(make_dep("JOB1", "JOB1")); // self-dep for merge test

    s1.merge(s2, "APP2");
    CHECK(s1.job_count() == 2);
    CHECK(s1.get_job("APP2_JOB1").has_value());
}

TEST_CASE("Model: SourceInfo variant TWS") {
    ScheduledJob job;
    job.source_info = TWSSource{"PAYROLL", 10, "WS1"};
    auto* tws = std::get_if<TWSSource>(&(*job.source_info));
    REQUIRE(tws != nullptr);
    CHECK(tws->adid == "PAYROLL");
    CHECK(tws->operation_number == 10);
}

TEST_CASE("Model: SourceInfo variant CA7") {
    ScheduledJob job;
    job.source_info = CA7Source{"PROD", "JCLLIB", 100};
    auto* ca7 = std::get_if<CA7Source>(&(*job.source_info));
    REQUIRE(ca7 != nullptr);
    CHECK(ca7->system == "PROD");
    CHECK(ca7->schedule_id == 100);
}

// ============================================================
// TWS PARSER TESTS (20+)
// ============================================================

TEST_CASE("TWS: Parse basic application") {
    const char* input =
        "ADID=PAYROLL OWNER=OPS PRIORITY=8\n"
        "OPERATIONS\n"
        "  JOBNAME=PAY001 JOBNR=10 DURATION=30\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    CHECK(result.success);
    REQUIRE(result.applications.size() == 1);
    CHECK(result.applications[0].adid == "PAYROLL");
    CHECK(result.applications[0].owner == "OPS");
    CHECK(result.applications[0].priority == 8);
}

TEST_CASE("TWS: Parse multiple operations") {
    const char* input =
        "ADID=BATCH01 OWNER=ADMIN\n"
        "OPERATIONS\n"
        "  JOBNAME=STEP1 JOBNR=10 DURATION=15 OPINFO=Extract\n"
        "  JOBNAME=STEP2 JOBNR=20 DURATION=45 OPINFO=Transform\n"
        "  JOBNAME=STEP3 JOBNR=30 DURATION=10 OPINFO=Load\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(result.applications.size() == 1);
    CHECK(result.applications[0].operations.size() == 3);
    CHECK(result.applications[0].operations[0].jobname == "STEP1");
    CHECK(result.applications[0].operations[1].duration == 45);
    CHECK(result.applications[0].operations[2].opinfo == "Load");
}

TEST_CASE("TWS: Parse dependencies with PRED/SUCC") {
    const char* input =
        "ADID=APP01 OWNER=DEV\n"
        "OPERATIONS\n"
        "  JOBNAME=JOB1 JOBNR=10\n"
        "  JOBNAME=JOB2 JOBNR=20\n"
        "DEPENDENCIES\n"
        "  PRED(APP01.10) SUCC(APP01.20) TYPE(S)\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(result.applications.size() == 1);
    CHECK(result.applications[0].dependencies.size() == 1);
    CHECK(result.applications[0].dependencies[0].pred_adid == "APP01");
    CHECK(result.applications[0].dependencies[0].pred_opnr == 10);
    CHECK(result.applications[0].dependencies[0].succ_opnr == 20);
    CHECK(result.applications[0].dependencies[0].type == 'S');
}

TEST_CASE("TWS: Parse dependencies arrow format") {
    const char* input =
        "ADID=ETL OWNER=DATA\n"
        "OPERATIONS\n"
        "  JOBNAME=EXTRACT JOBNR=10\n"
        "  JOBNAME=LOAD JOBNR=20\n"
        "DEPENDENCIES\n"
        "  ETL.10 -> ETL.20 S\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(result.applications.size() == 1);
    REQUIRE(result.applications[0].dependencies.size() == 1);
    CHECK(result.applications[0].dependencies[0].pred_adid == "ETL");
    CHECK(result.applications[0].dependencies[0].succ_adid == "ETL");
    CHECK(result.applications[0].dependencies[0].type == 'S');
}

TEST_CASE("TWS: Parse cross-application dependency") {
    const char* input =
        "ADID=APP02 OWNER=OPS\n"
        "OPERATIONS\n"
        "  JOBNAME=JOB_X JOBNR=10\n"
        "DEPENDENCIES\n"
        "  PRED(APP01.30) SUCC(APP02.10) TYPE(E)\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(!result.applications.empty());
    auto& dep = result.applications[0].dependencies[0];
    CHECK(dep.pred_adid == "APP01");
    CHECK(dep.succ_adid == "APP02");
    CHECK(dep.type == 'E');
}

TEST_CASE("TWS: Parse special resources") {
    const char* input =
        "ADID=RES_APP OWNER=OPS\n"
        "RESOURCES\n"
        "  NAME=TAPE_DRIVE QUANTITY=2 AVAILABILITY=4\n"
        "  NAME=SPOOL QTY=1 AVAIL=1\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(result.applications.size() == 1);
    CHECK(result.applications[0].resources.size() == 2);
    CHECK(result.applications[0].resources[0].name == "TAPE_DRIVE");
    CHECK(result.applications[0].resources[0].quantity == 2);
    CHECK(result.applications[0].resources[0].availability == 4);
    CHECK(result.applications[0].resources[1].name == "SPOOL");
}

TEST_CASE("TWS: Parse calendar with RUNDAYS") {
    const char* input =
        "ADID=CAL_APP OWNER=OPS\n"
        "CALENDAR\n"
        "  RUNDAYS=MON,TUE,WED,THU,FRI\n"
        "  SHIFT=NIGHT\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(result.applications.size() == 1);
    CHECK(result.applications[0].calendar.run_days.size() == 5);
    CHECK(result.applications[0].calendar.shift == "NIGHT");
}

TEST_CASE("TWS: Parse calendar with FREEDAYS") {
    const char* input =
        "ADID=HOLAPP OWNER=OPS\n"
        "CALENDAR\n"
        "  RUNDAYS=MON,TUE,WED,THU,FRI\n"
        "  FREEDAYS=2024-12-25,2024-01-01\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(result.applications.size() == 1);
    CHECK(result.applications[0].calendar.free_days.size() == 2);
}

TEST_CASE("TWS: Parse variables") {
    const char* input =
        "ADID=VAR_APP OWNER=OPS\n"
        "VARIABLES\n"
        "  &REGION=EAST\n"
        "  &ENV=PROD\n"
        "  NAME=DB VALUE=MAINDB\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(result.applications.size() == 1);
    CHECK(result.applications[0].variables.size() == 3);
    CHECK(result.applications[0].variables[0].name == "REGION");
    CHECK(result.applications[0].variables[0].value == "EAST");
    CHECK(result.applications[0].variables[2].name == "DB");
    CHECK(result.applications[0].variables[2].value == "MAINDB");
}

TEST_CASE("TWS: Parse time restrictions") {
    const char* input =
        "ADID=TIME_APP OWNER=OPS\n"
        "OPERATIONS\n"
        "  JOBNAME=NIGHT_JOB JOBNR=10 DURATION=60\n"
        "TIMERESTRICTIONS\n"
        "  JOBNR=10 EARLIEST=22:00 LATEST=23:30 DEADLINE=06:00\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(result.applications.size() == 1);
    REQUIRE(result.applications[0].operations.size() == 1);
    CHECK(result.applications[0].operations[0].time_restriction.earliest_start == "22:00");
    CHECK(result.applications[0].operations[0].time_restriction.latest_start == "23:30");
    CHECK(result.applications[0].operations[0].time_restriction.deadline == "06:00");
}

TEST_CASE("TWS: Parse inline time on operation") {
    const char* input =
        "ADID=INTIME OWNER=OPS\n"
        "OPERATIONS\n"
        "  JOBNAME=EARLY JOBNR=10 EARLIEST=05:00 LATEST=07:00\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(result.applications.size() == 1);
    CHECK(result.applications[0].operations[0].time_restriction.earliest_start == "05:00");
}

TEST_CASE("TWS: Parse multiple applications in one input") {
    const char* input =
        "ADID=APP_A OWNER=TEAM1\n"
        "OPERATIONS\n"
        "  JOBNAME=A1 JOBNR=10\n"
        "END\n"
        "ADID=APP_B OWNER=TEAM2\n"
        "OPERATIONS\n"
        "  JOBNAME=B1 JOBNR=10\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    CHECK(result.applications.size() == 2);
    CHECK(result.applications[0].adid == "APP_A");
    CHECK(result.applications[1].adid == "APP_B");
}

TEST_CASE("TWS: Comments and blank lines ignored") {
    const char* input =
        "# This is a comment\n"
        "* Another comment style\n"
        "\n"
        "ADID=CMT_APP OWNER=OPS\n"
        "# inline comment\n"
        "OPERATIONS\n"
        "  JOBNAME=J1 JOBNR=10\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    CHECK(result.applications.size() == 1);
}

TEST_CASE("TWS: Application with AUTHORITY and CALENDAR name") {
    const char* input =
        "ADID=SEC_APP OWNER=SEC_TEAM PRIORITY=10 AUTHORITY=ADMIN CALENDAR=BIZCAL\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    REQUIRE(result.applications.size() == 1);
    CHECK(result.applications[0].authority == "ADMIN");
    CHECK(result.applications[0].calendar_name == "BIZCAL");
    CHECK(result.applications[0].priority == 10);
}

TEST_CASE("TWS: Application without END (implicit flush)") {
    const char* input =
        "ADID=NOEND OWNER=OPS\n"
        "OPERATIONS\n"
        "  JOBNAME=LAST JOBNR=10\n";

    TWSParser parser;
    auto result = parser.parse(input);
    CHECK(result.applications.size() == 1);
    CHECK(result.applications[0].adid == "NOEND");
}

TEST_CASE("TWS: Operation with workstation") {
    const char* input =
        "ADID=WS_APP OWNER=OPS\n"
        "OPERATIONS\n"
        "  JOBNAME=WSJOB JOBNR=10 WORKSTATION=CPU1 DURATION=5\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    CHECK(result.applications[0].operations[0].workstation == "CPU1");
}

TEST_CASE("TWS: to_schedule conversion") {
    const char* input =
        "ADID=CONV OWNER=BATCH\n"
        "OPERATIONS\n"
        "  JOBNAME=STEP_A JOBNR=10 DURATION=20\n"
        "  JOBNAME=STEP_B JOBNR=20 DURATION=40\n"
        "DEPENDENCIES\n"
        "  PRED(CONV.10) SUCC(CONV.20) TYPE(S)\n"
        "END\n";

    TWSParser parser;
    auto pr = parser.parse(input);
    auto sched = parser.to_schedule(pr);
    CHECK(sched.job_count() == 2);
    CHECK(sched.dependency_count() == 1);
    auto j = sched.get_job("CONV.10");
    REQUIRE(j.has_value());
    CHECK(j->jcl_member == "STEP_A");
    CHECK(j->source_type == ScheduleSource::TWS);
}

TEST_CASE("TWS: to_schedule preserves dependency conditions") {
    const char* input =
        "ADID=DEPTEST OWNER=OPS\n"
        "OPERATIONS\n"
        "  JOBNAME=J1 JOBNR=10\n"
        "  JOBNAME=J2 JOBNR=20\n"
        "DEPENDENCIES\n"
        "  PRED(DEPTEST.10) SUCC(DEPTEST.20) TYPE(E)\n"
        "END\n";

    TWSParser parser;
    auto sched = parser.to_schedule(parser.parse(input));
    REQUIRE(sched.dependency_count() == 1);
    CHECK(sched.dependencies()[0].condition == DependencyCondition::ALWAYS);
}

TEST_CASE("TWS: Application with GROUP") {
    const char* input =
        "ADID=GRPAPP OWNER=OPS GROUP=NIGHTLY\n"
        "OPERATIONS\n"
        "  JOBNAME=G1 JOBNR=10\n"
        "END\n";

    TWSParser parser;
    auto result = parser.parse(input);
    CHECK(result.applications[0].group == "NIGHTLY");
}

// ============================================================
// CA-7 PARSER TESTS (20+)
// ============================================================

TEST_CASE("CA7: Parse basic DBJOB") {
    const char* input =
        "DBJOB NAME=PAYROLL01,SYSTEM=PROD,JCLID=PAY.JCLLIB,TYPE=CPU\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    CHECK(result.success);
    REQUIRE(result.jobs.size() == 1);
    CHECK(result.jobs[0].name == "PAYROLL01");
    CHECK(result.jobs[0].system == "PROD");
    CHECK(result.jobs[0].jclid == "PAY.JCLLIB");
    CHECK(result.jobs[0].type == "CPU");
}

TEST_CASE("CA7: Parse PROSE section") {
    const char* input =
        "DBJOB NAME=DOC_JOB\n"
        "PROSE\n"
        "  This job runs the nightly payroll.\n"
        "  It processes all employee records.\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(result.jobs.size() == 1);
    CHECK(result.jobs[0].prose.lines.size() == 2);
    auto text = result.jobs[0].prose.full_text();
    CHECK(text.find("nightly payroll") != std::string::npos);
    CHECK(text.find("employee records") != std::string::npos);
}

TEST_CASE("CA7: Parse SCHD section") {
    const char* input =
        "DBJOB NAME=SCHED_JOB\n"
        "SCHD SCAL=BIZCAL,ID=100,DOTRIG=Y,STTM=0600,DLNT=1200\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(result.jobs.size() == 1);
    REQUIRE(result.jobs[0].schedules.size() == 1);
    auto& s = result.jobs[0].schedules[0];
    CHECK(s.calendar == "BIZCAL");
    CHECK(s.schedule_id == 100);
    CHECK(s.dotrig == true);
    CHECK(s.start_time == "0600");
    CHECK(s.deadline == "1200");
}

TEST_CASE("CA7: Parse SCHD with CYCLE and DAYS") {
    const char* input =
        "DBJOB NAME=WEEKLY_JOB\n"
        "SCHD SCAL=WEEKCAL,CYCLE=WEEKLY,DAYS(1,3,5)\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(!result.jobs.empty());
    auto& s = result.jobs[0].schedules[0];
    CHECK(s.cycle_type == "WEEKLY");
    CHECK(s.run_days.size() == 3);
    CHECK(s.run_days[0] == 1);
    CHECK(s.run_days[1] == 3);
    CHECK(s.run_days[2] == 5);
}

TEST_CASE("CA7: Parse SCHD MONTHLY") {
    const char* input =
        "DBJOB NAME=MONTH_JOB\n"
        "SCHD CYCLE=MONTHLY,MDAY=15,STTM=0800\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(!result.jobs.empty());
    auto& s = result.jobs[0].schedules[0];
    CHECK(s.cycle_type == "MONTHLY");
    CHECK(s.monthly_day == 15);
}

TEST_CASE("CA7: Parse TRIG section") {
    const char* input =
        "DBJOB NAME=TRIGGERED\n"
        "TRIG JOB=PREREQ01,COND=SUCC\n"
        "TRIG JOB=PREREQ02,COND=FAIL\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(result.jobs.size() == 1);
    CHECK(result.jobs[0].triggers.size() == 2);
    CHECK(result.jobs[0].triggers[0].predecessor_job == "PREREQ01");
    CHECK(result.jobs[0].triggers[0].condition == "SUCC");
    CHECK(result.jobs[0].triggers[1].predecessor_job == "PREREQ02");
    CHECK(result.jobs[0].triggers[1].condition == "FAIL");
}

TEST_CASE("CA7: Parse TRIG with SCHID") {
    const char* input =
        "DBJOB NAME=SCHIDTRIG\n"
        "TRIG JOB=BASE_JOB,COND=SUCC,SCHID=200\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(!result.jobs.empty());
    CHECK(result.jobs[0].triggers[0].schid == 200);
}

TEST_CASE("CA7: Parse RQMT dataset trigger") {
    const char* input =
        "DBJOB NAME=DSNTRIG\n"
        "RQMT DSN=PROD.DAILY.EXTRACT\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(result.jobs.size() == 1);
    REQUIRE(result.jobs[0].requirements.size() == 1);
    CHECK(result.jobs[0].requirements[0].type == CA7Requirement::Type::DATASET);
    CHECK(result.jobs[0].requirements[0].dsn == "PROD.DAILY.EXTRACT");
}

TEST_CASE("CA7: Parse RQMT time trigger") {
    const char* input =
        "DBJOB NAME=TIMETRIG\n"
        "RQMT TYPE=TIME,TIME=0600\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(!result.jobs.empty());
    CHECK(result.jobs[0].requirements[0].type == CA7Requirement::Type::TIME);
    CHECK(result.jobs[0].requirements[0].time == "0600");
}

TEST_CASE("CA7: Parse RQMT with PERM and LDTM") {
    const char* input =
        "DBJOB NAME=PERMRQMT\n"
        "RQMT DSN=DAILY.FILE,LDTM=30,PERM=Y\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(!result.jobs.empty());
    auto& r = result.jobs[0].requirements[0];
    CHECK(r.lead_time == 30);
    CHECK(r.permanent == true);
}

TEST_CASE("CA7: Parse NWK section") {
    const char* input =
        "DBJOB NAME=NET_JOB\n"
        "NWK NAME=PAYNET,JOBS(PAY01,PAY02,PAY03),DESC=Payroll_network\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(result.jobs.size() == 1);
    REQUIRE(result.jobs[0].networks.size() == 1);
    CHECK(result.jobs[0].networks[0].name == "PAYNET");
    CHECK(result.jobs[0].networks[0].job_sequence.size() == 3);
    CHECK(result.jobs[0].networks[0].description == "Payroll_network");
}

TEST_CASE("CA7: Parse #ARF section") {
    const char* input =
        "DBJOB NAME=ARF_JOB\n"
        "#ARF COND=ABEND,ACTION=RESTART,MAX=3,NOTIFY=ONCALL\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(result.jobs.size() == 1);
    REQUIRE(result.jobs[0].arf_rules.size() == 1);
    CHECK(result.jobs[0].arf_rules[0].condition == "ABEND");
    CHECK(result.jobs[0].arf_rules[0].action == "RESTART");
    CHECK(result.jobs[0].arf_rules[0].max_restarts == 3);
    CHECK(result.jobs[0].arf_rules[0].notify == "ONCALL");
}

TEST_CASE("CA7: Parse XCC section") {
    const char* input =
        "DBJOB NAME=XCC_JOB\n"
        "XCC SRC=CAL_EAST,TGT=CAL_WEST,TYPE=LINK,OFFSET=1\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(result.jobs.size() == 1);
    REQUIRE(result.jobs[0].xcc_entries.size() == 1);
    CHECK(result.jobs[0].xcc_entries[0].source_calendar == "CAL_EAST");
    CHECK(result.jobs[0].xcc_entries[0].target_calendar == "CAL_WEST");
    CHECK(result.jobs[0].xcc_entries[0].connection_type == "LINK");
    CHECK(result.jobs[0].xcc_entries[0].offset_days == 1);
}

TEST_CASE("CA7: Parse batch command format") {
    const char* input =
        "/CMD DBJOB NAME=BATCH_CMD,SYSTEM=TEST,TYPE=CPU\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(result.jobs.size() == 1);
    CHECK(result.jobs[0].name == "BATCH_CMD");
    CHECK(result.jobs[0].system == "TEST");
}

TEST_CASE("CA7: Parse /VERB format batch command") {
    const char* input =
        "/DBJOB NAME=SLASH_JOB,TYPE=XPJOB\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(result.jobs.size() == 1);
    CHECK(result.jobs[0].name == "SLASH_JOB");
    CHECK(result.jobs[0].type == "XPJOB");
}

TEST_CASE("CA7: Comments ignored") {
    const char* input =
        "//* This is a JCL comment\n"
        "** Another comment\n"
        "DBJOB NAME=CMT_JOB\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    CHECK(result.jobs.size() == 1);
}

TEST_CASE("CA7: Multiple jobs in one input") {
    const char* input =
        "DBJOB NAME=JOB_A,SYSTEM=SYS1\n"
        "ENDJOB\n"
        "DBJOB NAME=JOB_B,SYSTEM=SYS2\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    CHECK(result.jobs.size() == 2);
    CHECK(result.jobs[0].name == "JOB_A");
    CHECK(result.jobs[1].name == "JOB_B");
}

TEST_CASE("CA7: to_schedule conversion") {
    const char* input =
        "DBJOB NAME=CONV_JOB,SYSTEM=PROD,JCLID=CONV.LIB\n"
        "SCHD SCAL=DAILY,STTM=0600,CYCLE=DAILY\n"
        "TRIG JOB=PREREQ,COND=SUCC\n"
        "ENDJOB\n"
        "DBJOB NAME=PREREQ,SYSTEM=PROD\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto pr = parser.parse(input);
    auto sched = parser.to_schedule(pr);
    CHECK(sched.job_count() == 2);
    CHECK(sched.dependency_count() == 1);
    auto j = sched.get_job("CONV_JOB");
    REQUIRE(j.has_value());
    CHECK(j->source_type == ScheduleSource::CA7);
    CHECK(j->calendar.name == "DAILY");
}

TEST_CASE("CA7: to_schedule maps trigger conditions") {
    const char* input =
        "DBJOB NAME=FAIL_HANDLER\n"
        "TRIG JOB=MAIN_JOB,COND=FAIL\n"
        "ENDJOB\n"
        "DBJOB NAME=MAIN_JOB\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto sched = parser.to_schedule(parser.parse(input));
    REQUIRE(sched.dependency_count() == 1);
    CHECK(sched.dependencies()[0].condition == DependencyCondition::ONLY_FAIL);
}

TEST_CASE("CA7: Multiple schedules per job") {
    const char* input =
        "DBJOB NAME=MULTI_SCHED\n"
        "SCHD SCAL=CAL1,ID=100,CYCLE=DAILY\n"
        "SCHD SCAL=CAL2,ID=200,CYCLE=WEEKLY,DAYS=1,5\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(!result.jobs.empty());
    CHECK(result.jobs[0].schedules.size() == 2);
    CHECK(result.jobs[0].schedules[0].schedule_id == 100);
    CHECK(result.jobs[0].schedules[1].schedule_id == 200);
}

TEST_CASE("CA7: RQMT USER type") {
    const char* input =
        "DBJOB NAME=USER_RQMT\n"
        "RQMT TYPE=USER,USER=APPROVAL_FLAG\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    REQUIRE(!result.jobs.empty());
    CHECK(result.jobs[0].requirements[0].type == CA7Requirement::Type::USER);
    CHECK(result.jobs[0].requirements[0].user_field == "APPROVAL_FLAG");
}

TEST_CASE("CA7: Job without ENDJOB (implicit flush)") {
    const char* input =
        "DBJOB NAME=NOEND_CA7,SYSTEM=TEST\n"
        "PROSE\n"
        "  Implicit end test\n";

    CA7Parser parser;
    auto result = parser.parse(input);
    CHECK(result.jobs.size() == 1);
    CHECK(result.jobs[0].name == "NOEND_CA7");
}

TEST_CASE("CA7: ARF error handling to_schedule") {
    const char* input =
        "DBJOB NAME=ERR_JOB\n"
        "#ARF COND=ABEND,ACTION=RESTART,MAX=2\n"
        "#ARF COND=S0C4,ACTION=CANCEL\n"
        "ENDJOB\n";

    CA7Parser parser;
    auto sched = parser.to_schedule(parser.parse(input));
    auto j = sched.get_job("ERR_JOB");
    REQUIRE(j.has_value());
    CHECK(j->error_handling.find("ABEND") != std::string::npos);
    CHECK(j->error_handling.find("RESTART") != std::string::npos);
    CHECK(j->error_handling.find("S0C4") != std::string::npos);
}

// ============================================================
// AIRFLOW EMITTER TESTS (15+)
// ============================================================

TEST_CASE("Airflow: Basic DAG output contains imports") {
    Schedule sched;
    sched.name = "test_dag";
    ScheduledJob j;
    j.name = "JOB1";
    sched.add_job(j);

    AirflowEmitter emitter;
    auto output = emitter.emit(sched);
    CHECK(output.find("from airflow import DAG") != std::string::npos);
    CHECK(output.find("from airflow.operators.bash import BashOperator") != std::string::npos);
    CHECK(output.find("from datetime import datetime") != std::string::npos);
}

TEST_CASE("Airflow: DAG ID sanitization") {
    Schedule sched;
    sched.name = "My Complex.Schedule-1";
    ScheduledJob j;
    j.name = "JOB1";
    sched.add_job(j);

    AirflowEmitter emitter;
    auto output = emitter.emit(sched);
    CHECK(output.find("my_complex_schedule_1") != std::string::npos);
}

TEST_CASE("Airflow: Task ID from job name") {
    Schedule sched;
    ScheduledJob j;
    j.name = "PAYROLL.STEP1";
    sched.add_job(j);

    AirflowEmitter emitter;
    auto output = emitter.emit(sched);
    CHECK(output.find("payroll_step1") != std::string::npos);
    CHECK(output.find("BashOperator") != std::string::npos);
}

TEST_CASE("Airflow: Default args in output") {
    Schedule sched;
    sched.name = "test";
    ScheduledJob j;
    j.name = "J1";
    sched.add_job(j);

    AirflowEmitterConfig cfg;
    cfg.owner = "dataops";
    cfg.retries = 3;
    cfg.retry_delay_minutes = 10;
    AirflowEmitter emitter(cfg);
    auto output = emitter.emit(sched);
    CHECK(output.find("'owner': 'dataops'") != std::string::npos);
    CHECK(output.find("'retries': 3") != std::string::npos);
    CHECK(output.find("timedelta(minutes=10)") != std::string::npos);
}

TEST_CASE("Airflow: Execution timeout from duration") {
    Schedule sched;
    ScheduledJob j;
    j.name = "TIMED_JOB";
    j.time_window.duration_minutes = 120;
    sched.add_job(j);

    AirflowEmitter emitter;
    auto output = emitter.emit(sched);
    CHECK(output.find("execution_timeout=timedelta(minutes=120)") != std::string::npos);
}

TEST_CASE("Airflow: Pool from resource") {
    Schedule sched;
    ScheduledJob j;
    j.name = "RES_JOB";
    j.resources.push_back({"TAPE_POOL", ResourceMode::SHARED, 1, 4});
    sched.add_job(j);

    AirflowEmitter emitter;
    auto output = emitter.emit(sched);
    CHECK(output.find("pool='TAPE_POOL'") != std::string::npos);
}

TEST_CASE("Airflow: Priority weight from job priority") {
    Schedule sched;
    ScheduledJob j;
    j.name = "HI_JOB";
    j.priority = JobPriority::HIGH;
    sched.add_job(j);

    AirflowEmitter emitter;
    auto output = emitter.emit(sched);
    CHECK(output.find("priority_weight=8") != std::string::npos);
}

TEST_CASE("Airflow: Dependencies as >> operator") {
    Schedule sched;
    ScheduledJob a, b;
    a.name = "STEP_A";
    b.name = "STEP_B";
    sched.add_job(a);
    sched.add_job(b);
    sched.add_dependency(make_dep("STEP_A", "STEP_B"));

    AirflowEmitter emitter;
    auto output = emitter.emit(sched);
    CHECK(output.find("step_a >> step_b") != std::string::npos);
}

TEST_CASE("Airflow: trigger_rule for non-default conditions") {
    Schedule sched;
    ScheduledJob a, b;
    a.name = "MAIN";
    b.name = "CLEANUP";
    sched.add_job(a);
    sched.add_job(b);
    Dependency dep;
    dep.predecessor = "MAIN";
    dep.successor = "CLEANUP";
    dep.condition = DependencyCondition::ALWAYS;
    sched.add_dependency(dep);

    AirflowEmitter emitter;
    auto output = emitter.emit(sched);
    CHECK(output.find("trigger_rule = 'all_done'") != std::string::npos);
}

TEST_CASE("Airflow: Map trigger_rule static method") {
    CHECK(AirflowEmitter::map_trigger_rule(DependencyCondition::RC_ZERO) == "all_success");
    CHECK(AirflowEmitter::map_trigger_rule(DependencyCondition::ALWAYS) == "all_done");
    CHECK(AirflowEmitter::map_trigger_rule(DependencyCondition::ONLY_FAIL) == "all_failed");
    CHECK(AirflowEmitter::map_trigger_rule(DependencyCondition::CUSTOM) == "one_success");
}

TEST_CASE("Airflow: Map timetable daily") {
    Calendar cal;
    cal.cycle = RunCycle::DAILY;
    CHECK(AirflowEmitter::map_timetable(cal) == "@daily");
}

TEST_CASE("Airflow: Map timetable weekly") {
    Calendar cal;
    cal.cycle = RunCycle::WEEKLY;
    cal.run_days = {DayOfWeek::MONDAY, DayOfWeek::WEDNESDAY, DayOfWeek::FRIDAY};
    auto tt = AirflowEmitter::map_timetable(cal);
    CHECK(tt.find("0 0 * *") != std::string::npos);
}

TEST_CASE("Airflow: Map timetable monthly") {
    Calendar cal;
    cal.cycle = RunCycle::MONTHLY;
    cal.monthly_day = 15;
    CHECK(AirflowEmitter::map_timetable(cal) == "0 0 15 * *");
}

TEST_CASE("Airflow: Map timetable business days") {
    Calendar cal;
    cal.cycle = RunCycle::BUSINESS_DAYS;
    CHECK(AirflowEmitter::map_timetable(cal) == "0 0 * * 1-5");
}

TEST_CASE("Airflow: Cross-DAG ExternalTaskSensor") {
    Schedule sched;
    ScheduledJob a, b;
    a.name = "EXTRACT";
    a.group = "ETL_DAG";
    b.name = "REPORT";
    b.group = "RPT_DAG";
    sched.add_job(a);
    sched.add_job(b);
    sched.add_dependency(make_dep("EXTRACT", "REPORT"));

    AirflowEmitterConfig cfg;
    cfg.emit_external_sensors = true;
    AirflowEmitter emitter(cfg);
    auto output = emitter.emit(sched);
    CHECK(output.find("ExternalTaskSensor") != std::string::npos);
    CHECK(output.find("external_dag_id") != std::string::npos);
}

TEST_CASE("Airflow: Python string escaping") {
    Schedule sched;
    ScheduledJob j;
    j.name = "JOB_WITH'QUOTE";
    sched.add_job(j);

    AirflowEmitter emitter;
    auto output = emitter.emit(sched);
    CHECK(output.find("job_with_quote") != std::string::npos);
    // The sanitized ID should not have the quote
}

TEST_CASE("Airflow: Catchup and concurrency config") {
    Schedule sched;
    sched.name = "cfg_test";
    ScheduledJob j;
    j.name = "J1";
    sched.add_job(j);

    AirflowEmitterConfig cfg;
    cfg.catchup = true;
    cfg.concurrency = 32;
    AirflowEmitter emitter(cfg);
    auto output = emitter.emit(sched);
    CHECK(output.find("catchup=True") != std::string::npos);
    CHECK(output.find("concurrency=32") != std::string::npos);
}

TEST_CASE("Airflow: On failure callback") {
    Schedule sched;
    sched.name = "cb_test";
    ScheduledJob j;
    j.name = "J1";
    sched.add_job(j);

    AirflowEmitterConfig cfg;
    cfg.on_failure_callback_fn = "alert_oncall";
    AirflowEmitter emitter(cfg);
    auto output = emitter.emit(sched);
    CHECK(output.find("'on_failure_callback': alert_oncall") != std::string::npos);
}

// ============================================================
// CRON EMITTER TESTS (10+)
// ============================================================

TEST_CASE("Cron: Daily schedule at specific time") {
    ScheduledJob job;
    job.name = "DAILY_JOB";
    job.calendar.cycle = RunCycle::DAILY;
    job.time_window.earliest_start = "06:30";

    CronEmitter emitter;
    auto cron = emitter.build(job);
    CHECK(cron.minute == "30");
    CHECK(cron.hour == "6");
    CHECK(cron.day_of_month == "*");
    CHECK(cron.day_of_week == "*");
    CHECK(cron.to_string() == "30 6 * * *");
    CHECK(cron.is_valid());
}

TEST_CASE("Cron: Weekly schedule on specific days") {
    ScheduledJob job;
    job.name = "WEEKLY_JOB";
    job.calendar.cycle = RunCycle::WEEKLY;
    job.calendar.run_days = {DayOfWeek::MONDAY, DayOfWeek::WEDNESDAY, DayOfWeek::FRIDAY};
    job.time_window.earliest_start = "08:00";

    CronEmitter emitter;
    auto cron = emitter.build(job);
    CHECK(cron.minute == "0");
    CHECK(cron.hour == "8");
    CHECK(cron.day_of_week == "1,3,5");
    CHECK(cron.is_valid());
}

TEST_CASE("Cron: Monthly schedule on day 15") {
    ScheduledJob job;
    job.name = "MONTHLY_JOB";
    job.calendar.cycle = RunCycle::MONTHLY;
    job.calendar.monthly_day = 15;
    job.time_window.earliest_start = "00:00";

    CronEmitter emitter;
    auto cron = emitter.build(job);
    CHECK(cron.day_of_month == "15");
    CHECK(cron.to_string() == "0 0 15 * *");
}

TEST_CASE("Cron: Business days schedule") {
    ScheduledJob job;
    job.name = "BIZDAY";
    job.calendar.cycle = RunCycle::BUSINESS_DAYS;
    job.time_window.earliest_start = "09:00";

    CronEmitter emitter;
    auto cron = emitter.build(job);
    CHECK(cron.day_of_week == "1-5");
    CHECK(cron.to_string() == "0 9 * * 1-5");
}

TEST_CASE("Cron: Yearly schedule") {
    ScheduledJob job;
    job.name = "ANNUAL";
    job.calendar.cycle = RunCycle::YEARLY;
    job.calendar.yearly_month = 3;
    job.calendar.yearly_day = 1;
    job.time_window.earliest_start = "12:00";

    CronEmitter emitter;
    auto cron = emitter.build(job);
    CHECK(cron.to_string() == "0 12 1 3 *");
}

TEST_CASE("Cron: Validation of correct expression") {
    CHECK(CronEmitter::validate_cron_string("30 6 * * *"));
    CHECK(CronEmitter::validate_cron_string("0 0 15 * *"));
    CHECK(CronEmitter::validate_cron_string("*/5 * * * *"));
    CHECK(CronEmitter::validate_cron_string("0 9 * * 1-5"));
}

TEST_CASE("Cron: Validation rejects invalid expression") {
    CHECK_FALSE(CronEmitter::validate_cron_string("60 25 32 13 8"));
}

TEST_CASE("Cron: Human-readable description") {
    CronExpression cron;
    cron.minute = "30";
    cron.hour = "6";
    cron.day_of_month = "*";
    cron.month = "*";
    cron.day_of_week = "*";
    auto desc = CronEmitter::describe(cron);
    CHECK(desc.find("Every day") != std::string::npos);
    CHECK(desc.find("06:30") != std::string::npos);
}

TEST_CASE("Cron: Business day description") {
    CronExpression cron;
    cron.minute = "0";
    cron.hour = "9";
    cron.day_of_month = "*";
    cron.month = "*";
    cron.day_of_week = "1-5";
    auto desc = CronEmitter::describe(cron);
    CHECK(desc.find("Mon-Fri") != std::string::npos);
}

TEST_CASE("Cron: Crontab output with holiday exclusion") {
    Schedule sched;
    sched.name = "HOL_SCHED";
    ScheduledJob job;
    job.name = "HOL_JOB";
    job.calendar.cycle = RunCycle::DAILY;
    job.calendar.holidays = {"2024-12-25"};
    job.time_window.earliest_start = "08:00";
    sched.add_job(job);

    CronEmitter emitter;
    auto output = emitter.emit_crontab(sched);
    CHECK(output.find("holiday exclusion") != std::string::npos);
    CHECK(output.find("2024-12-25") != std::string::npos);
}

TEST_CASE("Cron: Multiple jobs in crontab") {
    Schedule sched;
    sched.name = "MULTI";
    ScheduledJob j1, j2;
    j1.name = "JOB_A"; j1.calendar.cycle = RunCycle::DAILY;
    j1.time_window.earliest_start = "06:00";
    j2.name = "JOB_B"; j2.calendar.cycle = RunCycle::DAILY;
    j2.time_window.earliest_start = "12:00";
    sched.add_job(j1);
    sched.add_job(j2);

    CronEmitter emitter;
    auto output = emitter.emit_crontab(sched);
    CHECK(output.find("JOB_A") != std::string::npos);
    CHECK(output.find("JOB_B") != std::string::npos);
}

TEST_CASE("Cron: Parse cron string roundtrip") {
    auto cron = CronEmitter::parse_cron_string("30 6 * * 1-5", "test");
    CHECK(cron.minute == "30");
    CHECK(cron.hour == "6");
    CHECK(cron.day_of_week == "1-5");
    CHECK(cron.comment == "test");
    CHECK(cron.to_string() == "30 6 * * 1-5");
}

// ============================================================
// INTEGRATION TESTS (7+)
// ============================================================

TEST_CASE("Integration: TWS parse to Airflow DAG") {
    const char* tws_input =
        "ADID=NIGHTLY_ETL OWNER=DATAOPS PRIORITY=8\n"
        "OPERATIONS\n"
        "  JOBNAME=EXTRACT JOBNR=10 DURATION=30\n"
        "  JOBNAME=TRANSFORM JOBNR=20 DURATION=60\n"
        "  JOBNAME=LOAD JOBNR=30 DURATION=15\n"
        "DEPENDENCIES\n"
        "  PRED(NIGHTLY_ETL.10) SUCC(NIGHTLY_ETL.20) TYPE(S)\n"
        "  PRED(NIGHTLY_ETL.20) SUCC(NIGHTLY_ETL.30) TYPE(S)\n"
        "END\n";

    TWSParser tws;
    auto pr = tws.parse(tws_input);
    auto sched = tws.to_schedule(pr);

    AirflowEmitter emitter;
    auto dag = emitter.emit(sched);

    CHECK(dag.find("from airflow import DAG") != std::string::npos);
    CHECK(dag.find("BashOperator") != std::string::npos);
    CHECK(dag.find("nightly_etl_10") != std::string::npos);
    CHECK(dag.find(">>") != std::string::npos);
}

TEST_CASE("Integration: CA-7 parse to Airflow DAG") {
    const char* ca7_input =
        "DBJOB NAME=PAYROLL,SYSTEM=PROD,TYPE=CPU\n"
        "SCHD SCAL=BIZCAL,CYCLE=DAILY,STTM=0600\n"
        "ENDJOB\n"
        "DBJOB NAME=GL_POST,SYSTEM=PROD,TYPE=CPU\n"
        "TRIG JOB=PAYROLL,COND=SUCC\n"
        "ENDJOB\n";

    CA7Parser ca7;
    auto pr = ca7.parse(ca7_input);
    auto sched = ca7.to_schedule(pr);

    AirflowEmitter emitter;
    auto dag = emitter.emit(sched);

    CHECK(dag.find("payroll") != std::string::npos);
    CHECK(dag.find("gl_post") != std::string::npos);
    CHECK(dag.find("payroll >> gl_post") != std::string::npos);
}

TEST_CASE("Integration: TWS schedule to cron") {
    const char* tws_input =
        "ADID=CRON_TEST OWNER=OPS\n"
        "OPERATIONS\n"
        "  JOBNAME=NIGHTLY JOBNR=10 DURATION=45 EARLIEST=22:00\n"
        "END\n";

    TWSParser tws;
    auto sched = tws.to_schedule(tws.parse(tws_input));

    CronEmitter cron_emitter;
    auto entries = cron_emitter.build_all(sched);
    REQUIRE(entries.size() == 1);
    CHECK(entries[0].hour == "22");
    CHECK(entries[0].minute == "0");
}

TEST_CASE("Integration: Merge TWS and CA-7 schedules") {
    const char* tws_input =
        "ADID=TWS_APP OWNER=OPS\n"
        "OPERATIONS\n"
        "  JOBNAME=T1 JOBNR=10\n"
        "END\n";

    const char* ca7_input =
        "DBJOB NAME=C1,SYSTEM=PROD\n"
        "ENDJOB\n";

    TWSParser tws;
    CA7Parser ca7;
    auto tws_sched = tws.to_schedule(tws.parse(tws_input));
    auto ca7_sched = ca7.to_schedule(ca7.parse(ca7_input));

    tws_sched.merge(ca7_sched, "CA7");
    CHECK(tws_sched.job_count() == 2);
    CHECK(tws_sched.get_job("TWS_APP.10").has_value());
    CHECK(tws_sched.get_job("CA7_C1").has_value());
}

TEST_CASE("Integration: Cross-schedule dependency after merge") {
    Schedule merged;
    ScheduledJob a, b;
    a.name = "TWS_EXTRACT";
    a.group = "TWS";
    b.name = "CA7_REPORT";
    b.group = "CA7";
    merged.add_job(a);
    merged.add_job(b);
    merged.add_dependency(make_dep("TWS_EXTRACT", "CA7_REPORT"));

    auto order = merged.topological_order();
    CHECK(order.size() == 2);
    // TWS_EXTRACT -> CA7_REPORT: TWS_EXTRACT has in_degree 0, CA7_REPORT has in_degree 1
    CHECK(order[0] == "TWS_EXTRACT");
    CHECK(order[1] == "CA7_REPORT");
}

TEST_CASE("Integration: Validate merged schedule") {
    const char* tws_input =
        "ADID=VAL_TWS OWNER=OPS\n"
        "OPERATIONS\n"
        "  JOBNAME=T1 JOBNR=10\n"
        "  JOBNAME=T2 JOBNR=20\n"
        "DEPENDENCIES\n"
        "  PRED(VAL_TWS.10) SUCC(VAL_TWS.20) TYPE(S)\n"
        "END\n";

    TWSParser tws;
    auto sched = tws.to_schedule(tws.parse(tws_input));
    auto result = sched.validate();
    CHECK(result.valid);
    CHECK(result.cycles.empty());
}

TEST_CASE("Integration: Full pipeline TWS to crontab") {
    const char* tws_input =
        "ADID=FULLPIPE OWNER=OPS\n"
        "OPERATIONS\n"
        "  JOBNAME=STEP1 JOBNR=10 DURATION=20 EARLIEST=06:00\n"
        "  JOBNAME=STEP2 JOBNR=20 DURATION=40 EARLIEST=07:00\n"
        "DEPENDENCIES\n"
        "  PRED(FULLPIPE.10) SUCC(FULLPIPE.20) TYPE(S)\n"
        "CALENDAR\n"
        "  RUNDAYS=MON,TUE,WED,THU,FRI\n"
        "END\n";

    TWSParser tws;
    auto sched = tws.to_schedule(tws.parse(tws_input));

    // Validate
    auto val = sched.validate();
    CHECK(val.valid);

    // Emit crontab
    CronEmitter cron_emitter;
    auto crontab = cron_emitter.emit_crontab(sched, "/opt/batch/run.sh {JOB}");
    CHECK(crontab.find("FULLPIPE") != std::string::npos);
    CHECK(crontab.find("/opt/batch/run.sh") != std::string::npos);

    // Emit Airflow
    AirflowEmitter airflow;
    auto dag = airflow.emit(sched);
    CHECK(dag.find("fullpipe_10") != std::string::npos);
    CHECK(dag.find("fullpipe_20") != std::string::npos);
    CHECK(dag.find(">>") != std::string::npos);
}
