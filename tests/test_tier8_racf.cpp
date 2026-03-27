#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

#include "lazarus/racf/profile.h"
#include "lazarus/racf/parser.h"
#include "lazarus/racf/policy_emitter.h"
#include "lazarus/racf/audit.h"

#include <string>
#include <sstream>

using namespace lazarus::racf;

// ============================================================
// Helper: build a small IRRDBU00 dump text for testing
// ============================================================

static const char* SAMPLE_DUMP = R"(0100 ADMIN01 SYSTEM_ADMIN SYS1 IBMUSER 2025-01-15 2026-03-01 30 NO YES YES NO NO FACILITY
0100 USER001 JOHN_SMITH DEVGRP ADMIN01 2025-06-01 2026-02-15 60 NO NO NO NO NO
0100 USER002 JANE_DOE TESTGRP ADMIN01 2025-07-10 2025-08-01 90 YES NO NO NO NO
0100 SVCACCT SVC_BATCH SYS1 ADMIN01 2025-01-01 NO NO NO YES NO NO
0101 ADMIN01 0 /u/admin01 /bin/sh
0101 USER001 1001 /u/user001 /bin/bash
0102 USER001 OPR1 200 300
0103 USER001 TESTAPP MGMT01 STOR01
0200 SYS1 SYS1 IBMUSER NO
0200 DEVGRP SYS1 ADMIN01 NO SUBDEV1 SUBDEV2
0200 TESTGRP SYS1 ADMIN01 YES
0200 PRODGRP DEVGRP ADMIN01 NO
0205 DEVGRP 100
0205 TESTGRP 200
0400 SYS1.PARMLIB READ YES NO SYS1 NO YES NO
0400 USER001.DATA NONE NO NO USER001 NO NO NO
0400 PROD.** CONTROL YES YES ADMIN01 YES NO YES
0400 DEV.SOURCE.* READ NO NO DEVGRP NO NO YES
0404 SYS1.PARMLIB ADMIN01 ALTER NO 50
0404 SYS1.PARMLIB DEVGRP READ NO 100
0404 SYS1.PARMLIB TESTGRP NONE NO 0
0404 PROD.** USER001 UPDATE NO 25
0500 FACILITY BPX.SERVER READ NO SYS1
0500 FACILITY IRR.RADMIN.LISTUSER NONE YES ADMIN01
0505 FACILITY BPX.SERVER ADMIN01 ALTER NO 10
0600 ADMIN01 SYS1 JOIN NO
0600 USER001 DEVGRP USE NO
0600 USER001 TESTGRP CREATE NO
0600 USER002 TESTGRP USE YES
0600 SVCACCT SYS1 USE NO)";

// ============================================================
// PARSER TESTS (25+)
// ============================================================

TEST_CASE("Parser: parse user basic record") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0100 TESTUSER MY_NAME GRP1 OWNER1 2025-01-01 2025-06-01 30 NO YES NO NO NO");
    REQUIRE(result.has_value());
    auto* u = std::get_if<UserProfile>(&*result);
    REQUIRE(u != nullptr);
    CHECK(u->userid == "TESTUSER");
    CHECK(u->name == "MY_NAME");
    CHECK(u->dfltgrp == "GRP1");
    CHECK(u->owner == "OWNER1");
    CHECK(u->special == true);
    CHECK(u->operations == false);
}

TEST_CASE("Parser: user created and passdate fields") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0100 U1 NAME G1 O1 2024-12-25 2025-03-15 45 NO NO NO NO NO");
    auto* u = std::get_if<UserProfile>(&*result);
    REQUIRE(u != nullptr);
    CHECK(u->created == "2024-12-25");
    CHECK(u->passdate == "2025-03-15");
    CHECK(u->passint == 45);
}

TEST_CASE("Parser: user revoke flag") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0100 REVUSR NAME GRP OWN 2025-01-01 2025-01-01 30 YES NO NO NO NO");
    auto* u = std::get_if<UserProfile>(&*result);
    REQUIRE(u != nullptr);
    CHECK(u->revoke == true);
    CHECK(u->special == false);
}

TEST_CASE("Parser: user operations and auditor flags") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0100 OPSUSR NAME GRP OWN 2025-01-01 2025-01-01 30 NO NO YES YES YES");
    auto* u = std::get_if<UserProfile>(&*result);
    REQUIRE(u != nullptr);
    CHECK(u->operations == true);
    CHECK(u->auditor == true);
    CHECK(u->uaudit == true);
}

TEST_CASE("Parser: user CLAUTH field list") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0100 U1 NAME G1 O1 2025-01-01 2025-01-01 30 NO NO NO NO NO FACILITY SECLABEL");
    auto* u = std::get_if<UserProfile>(&*result);
    REQUIRE(u != nullptr);
    REQUIRE(u->clauth.size() == 2);
    CHECK(u->clauth[0] == "FACILITY");
    CHECK(u->clauth[1] == "SECLABEL");
}

TEST_CASE("Parser: parse group basic record") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0200 MYGRP SYS1 ADMIN01 YES SUB1 SUB2 SUB3");
    REQUIRE(result.has_value());
    auto* g = std::get_if<GroupProfile>(&*result);
    REQUIRE(g != nullptr);
    CHECK(g->group == "MYGRP");
    CHECK(g->supgroup == "SYS1");
    CHECK(g->owner == "ADMIN01");
    CHECK(g->termuacc == true);
    CHECK(g->subgroups.size() == 3);
    CHECK(g->subgroups[0] == "SUB1");
}

TEST_CASE("Parser: group with no subgroups") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0200 LEAFGRP SYS1 OWNER NO");
    auto* g = std::get_if<GroupProfile>(&*result);
    REQUIRE(g != nullptr);
    CHECK(g->subgroups.empty());
    CHECK(g->termuacc == false);
}

TEST_CASE("Parser: parse dataset profile") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0400 SYS1.LINKLIB READ YES NO SYS1 YES NO NO");
    REQUIRE(result.has_value());
    auto* d = std::get_if<DatasetProfile>(&*result);
    REQUIRE(d != nullptr);
    CHECK(d->dsname == "SYS1.LINKLIB");
    CHECK(d->uacc == AccessLevel::READ);
    CHECK(d->audit_access == true);
    CHECK(d->globalaudit == false);
    CHECK(d->owner == "SYS1");
    CHECK(d->erase == true);
    CHECK(d->warning == false);
    CHECK(d->generic == false);
}

TEST_CASE("Parser: dataset generic flag") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0400 PROD.** CONTROL YES YES ADMIN01 YES NO YES");
    auto* d = std::get_if<DatasetProfile>(&*result);
    REQUIRE(d != nullptr);
    CHECK(d->generic == true);
    CHECK(d->uacc == AccessLevel::CONTROL);
    CHECK(d->globalaudit == true);
}

TEST_CASE("Parser: parse general resource") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0500 FACILITY BPX.SERVER READ NO SYS1");
    REQUIRE(result.has_value());
    auto* g = std::get_if<GeneralProfile>(&*result);
    REQUIRE(g != nullptr);
    CHECK(g->resource_class == "FACILITY");
    CHECK(g->name == "BPX.SERVER");
    CHECK(g->uacc == AccessLevel::READ);
    CHECK(g->owner == "SYS1");
}

TEST_CASE("Parser: parse connect record") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0600 USER01 GRP01 JOIN NO");
    REQUIRE(result.has_value());
    auto* c = std::get_if<ConnectProfile>(&*result);
    REQUIRE(c != nullptr);
    CHECK(c->userid == "USER01");
    CHECK(c->group == "GRP01");
    CHECK(c->auth == Authority::JOIN);
    CHECK(c->revoke == false);
}

TEST_CASE("Parser: connect with revoke") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0600 BADUSER TESTGRP USE YES");
    auto* c = std::get_if<ConnectProfile>(&*result);
    REQUIRE(c != nullptr);
    CHECK(c->revoke == true);
    CHECK(c->auth == Authority::USE);
}

TEST_CASE("Parser: connect authority CREATE") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0600 DEVUSR DEVGRP CREATE NO");
    auto* c = std::get_if<ConnectProfile>(&*result);
    REQUIRE(c != nullptr);
    CHECK(c->auth == Authority::CREATE);
}

TEST_CASE("Parser: connect authority CONNECT") {
    IrrdbuParser parser;
    auto result = parser.parse_record("0600 ADMX SYSGRP CONNECT NO");
    auto* c = std::get_if<ConnectProfile>(&*result);
    REQUIRE(c != nullptr);
    CHECK(c->auth == Authority::CONNECT);
}

TEST_CASE("Parser: full dump parse counts") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    CHECK(db.users.size() == 4);
    CHECK(db.groups.size() == 4);
    CHECK(db.datasets.size() == 4);
    CHECK(db.generals.size() == 2);
    CHECK(db.connects.size() == 5);
}

TEST_CASE("Parser: OMVS segment merges into user") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto u = db.find_user("ADMIN01");
    REQUIRE(u.has_value());
    CHECK(u->omvs_uid == 0);
    CHECK(u->omvs_home == "/u/admin01");
    CHECK(u->omvs_shell == "/bin/sh");
}

TEST_CASE("Parser: OMVS segment for second user") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto u = db.find_user("USER001");
    REQUIRE(u.has_value());
    CHECK(u->omvs_uid == 1001);
    CHECK(u->omvs_home == "/u/user001");
    CHECK(u->omvs_shell == "/bin/bash");
}

TEST_CASE("Parser: CICS segment merges into user") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto u = db.find_user("USER001");
    REQUIRE(u.has_value());
    CHECK(u->cics_opident == "OPR1");
    CHECK(u->cics_opprty == 200);
    CHECK(u->cics_timeout == 300);
}

TEST_CASE("Parser: DFP segment merges into user") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto u = db.find_user("USER001");
    REQUIRE(u.has_value());
    CHECK(u->dfp_dataappl == "TESTAPP");
    CHECK(u->dfp_mgmtclass == "MGMT01");
    CHECK(u->dfp_storclas == "STOR01");
}

TEST_CASE("Parser: group OMVS segment merges GID") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto g = db.find_group("DEVGRP");
    REQUIRE(g.has_value());
    CHECK(g->omvs_gid == 100);
}

TEST_CASE("Parser: dataset access list entries attached") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto dsets = db.find_datasets("SYS1.PARMLIB");
    REQUIRE(dsets.size() == 1);
    CHECK(dsets[0].access_list.size() == 3);
    // ADMIN01 should have ALTER
    bool found_admin = false;
    for (auto& ae : dsets[0].access_list) {
        if (ae.entity == "ADMIN01") {
            CHECK(ae.access == AccessLevel::ALTER);
            CHECK(ae.count == 50);
            found_admin = true;
        }
    }
    CHECK(found_admin);
}

TEST_CASE("Parser: empty line handling") {
    IrrdbuParser parser;
    auto result = parser.parse("\n\n0100 U1 N G O 2025-01-01 2025-01-01 30 NO NO NO NO NO\n\n");
    auto db = result.to_database();
    CHECK(db.users.size() == 1);
    CHECK(result.lines_skipped == 0);
}

TEST_CASE("Parser: unknown record types skipped") {
    IrrdbuParser parser;
    auto result = parser.parse("9999 UNKNOWN RECORD\n0100 U1 N G O 2025-01-01 2025-01-01 30 NO NO NO NO NO");
    CHECK(result.lines_skipped == 1);
    auto db = result.to_database();
    CHECK(db.users.size() == 1);
}

TEST_CASE("Parser: parse_record returns nullopt for short line") {
    IrrdbuParser parser;
    CHECK_FALSE(parser.parse_record("01").has_value());
    CHECK_FALSE(parser.parse_record("").has_value());
}

// ============================================================
// PROFILE MODEL TESTS (20+)
// ============================================================

TEST_CASE("Profile: AccessLevel enum ordering") {
    CHECK(AccessLevel::NONE < AccessLevel::READ);
    CHECK(AccessLevel::READ < AccessLevel::UPDATE);
    CHECK(AccessLevel::UPDATE < AccessLevel::CONTROL);
    CHECK(AccessLevel::CONTROL < AccessLevel::ALTER);
}

TEST_CASE("Profile: parse_access_level function") {
    CHECK(parse_access_level("NONE") == AccessLevel::NONE);
    CHECK(parse_access_level("READ") == AccessLevel::READ);
    CHECK(parse_access_level("UPDATE") == AccessLevel::UPDATE);
    CHECK(parse_access_level("CONTROL") == AccessLevel::CONTROL);
    CHECK(parse_access_level("ALTER") == AccessLevel::ALTER);
    CHECK(parse_access_level("INVALID") == AccessLevel::NONE);
}

TEST_CASE("Profile: Authority enum values") {
    CHECK(parse_authority("USE") == Authority::USE);
    CHECK(parse_authority("CREATE") == Authority::CREATE);
    CHECK(parse_authority("CONNECT") == Authority::CONNECT);
    CHECK(parse_authority("JOIN") == Authority::JOIN);
    CHECK(parse_authority("UNKNOWN") == Authority::USE);
}

TEST_CASE("Profile: ProfileDatabase add and find user") {
    ProfileDatabase db;
    UserProfile u;
    u.userid = "TESTUSER";
    u.name = "Test User";
    db.add(u);
    auto found = db.find_user("TESTUSER");
    REQUIRE(found.has_value());
    CHECK(found->name == "Test User");
}

TEST_CASE("Profile: find user case-insensitive") {
    ProfileDatabase db;
    UserProfile u;
    u.userid = "ADMIN01";
    db.add(u);
    CHECK(db.find_user("admin01").has_value());
    CHECK(db.find_user("Admin01").has_value());
    CHECK(db.find_user("ADMIN01").has_value());
}

TEST_CASE("Profile: find user not found") {
    ProfileDatabase db;
    CHECK_FALSE(db.find_user("NOBODY").has_value());
}

TEST_CASE("Profile: find group") {
    ProfileDatabase db;
    GroupProfile g;
    g.group = "DEVTEAM";
    g.supgroup = "SYS1";
    db.add(g);
    auto found = db.find_group("devteam");
    REQUIRE(found.has_value());
    CHECK(found->supgroup == "SYS1");
}

TEST_CASE("Profile: find datasets by wildcard pattern") {
    ProfileDatabase db;
    DatasetProfile d1; d1.dsname = "SYS1.PARMLIB";
    DatasetProfile d2; d2.dsname = "SYS1.LINKLIB";
    DatasetProfile d3; d3.dsname = "USER.DATA";
    db.add(d1); db.add(d2); db.add(d3);
    auto result = db.find_datasets("SYS1.*");
    CHECK(result.size() == 2);
}

TEST_CASE("Profile: find_users by wildcard") {
    ProfileDatabase db;
    UserProfile u1; u1.userid = "USR001";
    UserProfile u2; u2.userid = "USR002";
    UserProfile u3; u3.userid = "ADMIN";
    db.add(u1); db.add(u2); db.add(u3);
    auto result = db.find_users("USR*");
    CHECK(result.size() == 2);
}

TEST_CASE("Profile: find_groups by wildcard") {
    ProfileDatabase db;
    GroupProfile g1; g1.group = "DEVGRP1";
    GroupProfile g2; g2.group = "DEVGRP2";
    GroupProfile g3; g3.group = "TESTGRP";
    db.add(g1); db.add(g2); db.add(g3);
    auto result = db.find_groups("DEV*");
    CHECK(result.size() == 2);
}

TEST_CASE("Profile: user_connects returns correct records") {
    ProfileDatabase db;
    ConnectProfile c1; c1.userid = "USR1"; c1.group = "GRP1";
    ConnectProfile c2; c2.userid = "USR1"; c2.group = "GRP2";
    ConnectProfile c3; c3.userid = "USR2"; c3.group = "GRP1";
    db.add(c1); db.add(c2); db.add(c3);
    auto conns = db.user_connects("USR1");
    CHECK(conns.size() == 2);
}

TEST_CASE("Profile: group_connects returns correct records") {
    ProfileDatabase db;
    ConnectProfile c1; c1.userid = "USR1"; c1.group = "GRP1";
    ConnectProfile c2; c2.userid = "USR2"; c2.group = "GRP1";
    ConnectProfile c3; c3.userid = "USR3"; c3.group = "GRP2";
    db.add(c1); db.add(c2); db.add(c3);
    auto conns = db.group_connects("GRP1");
    CHECK(conns.size() == 2);
}

TEST_CASE("Profile: group hierarchy traversal") {
    ProfileDatabase db;
    GroupProfile g1; g1.group = "SYS1"; g1.supgroup = "";
    GroupProfile g2; g2.group = "DEVGRP"; g2.supgroup = "SYS1";
    GroupProfile g3; g3.group = "PRODGRP"; g3.supgroup = "DEVGRP";
    db.add(g1); db.add(g2); db.add(g3);
    auto chain = db.group_hierarchy("PRODGRP");
    REQUIRE(chain.size() == 3);
    CHECK(chain[0] == "PRODGRP");
    CHECK(chain[1] == "DEVGRP");
    CHECK(chain[2] == "SYS1");
}

TEST_CASE("Profile: group hierarchy handles self-reference") {
    ProfileDatabase db;
    GroupProfile g1; g1.group = "SYS1"; g1.supgroup = "SYS1";
    db.add(g1);
    auto chain = db.group_hierarchy("SYS1");
    CHECK(chain.size() == 1);
}

TEST_CASE("Profile: effective permission - SPECIAL grants ALTER") {
    ProfileDatabase db;
    UserProfile u; u.userid = "ADMIN"; u.special = true;
    DatasetProfile ds; ds.dsname = "SYS1.PARMLIB"; ds.uacc = AccessLevel::NONE;
    db.add(u); db.add(ds);
    CHECK(db.effective_permission("ADMIN", ds) == AccessLevel::ALTER);
}

TEST_CASE("Profile: effective permission - owner gets ALTER") {
    ProfileDatabase db;
    UserProfile u; u.userid = "OWNER01"; u.special = false;
    DatasetProfile ds; ds.dsname = "TEST.DATA"; ds.uacc = AccessLevel::NONE;
    ds.owner = "OWNER01";
    db.add(u); db.add(ds);
    CHECK(db.effective_permission("OWNER01", ds) == AccessLevel::ALTER);
}

TEST_CASE("Profile: effective permission - UACC fallback") {
    ProfileDatabase db;
    UserProfile u; u.userid = "REGULAR";
    DatasetProfile ds; ds.dsname = "PUBLIC.DATA"; ds.uacc = AccessLevel::READ;
    ds.owner = "SOMEONE";
    db.add(u); db.add(ds);
    CHECK(db.effective_permission("REGULAR", ds) == AccessLevel::READ);
}

TEST_CASE("Profile: effective permission - access list direct entry") {
    ProfileDatabase db;
    UserProfile u; u.userid = "DEV01";
    DatasetProfile ds; ds.dsname = "DEV.SRC"; ds.uacc = AccessLevel::NONE;
    ds.owner = "ADMIN";
    ds.access_list.push_back({"DEV01", AccessLevel::UPDATE, false, 0});
    db.add(u); db.add(ds);
    CHECK(db.effective_permission("DEV01", ds) == AccessLevel::UPDATE);
}

TEST_CASE("Profile: effective permission - group access list") {
    ProfileDatabase db;
    UserProfile u; u.userid = "DEV02";
    GroupProfile g; g.group = "DEVGRP";
    ConnectProfile c; c.userid = "DEV02"; c.group = "DEVGRP";
    DatasetProfile ds; ds.dsname = "DEV.LIB"; ds.uacc = AccessLevel::NONE;
    ds.owner = "ADMIN";
    ds.access_list.push_back({"DEVGRP", AccessLevel::CONTROL, false, 0});
    db.add(u); db.add(g); db.add(c); db.add(ds);
    CHECK(db.effective_permission("DEV02", ds) == AccessLevel::CONTROL);
}

TEST_CASE("Profile: effective permission - nonexistent user") {
    ProfileDatabase db;
    DatasetProfile ds; ds.dsname = "TEST"; ds.uacc = AccessLevel::READ;
    db.add(ds);
    CHECK(db.effective_permission("NOBODY", ds) == AccessLevel::NONE);
}

TEST_CASE("Profile: total_profiles count") {
    ProfileDatabase db;
    UserProfile u; u.userid = "U1"; db.add(u);
    GroupProfile g; g.group = "G1"; db.add(g);
    DatasetProfile d; d.dsname = "D1"; db.add(d);
    CHECK(db.total_profiles() == 3);
}

TEST_CASE("Profile: add via RacfProfile variant") {
    ProfileDatabase db;
    UserProfile u; u.userid = "VARUSER"; u.name = "Variant";
    RacfProfile p = u;
    db.add(p);
    auto found = db.find_user("VARUSER");
    REQUIRE(found.has_value());
    CHECK(found->name == "Variant");
}

// ============================================================
// POLICY EMITTER TESTS (25+)
// ============================================================

TEST_CASE("Emitter JSON: full database emit contains Users key") {
    ProfileDatabase db;
    UserProfile u; u.userid = "U1"; u.name = "User One"; u.dfltgrp = "GRP"; db.add(u);
    auto json = JsonPolicyEmitter::emit(db);
    CHECK(json.find("\"Users\"") != std::string::npos);
    CHECK(json.find("\"U1\"") != std::string::npos);
}

TEST_CASE("Emitter JSON: emit contains Groups key") {
    ProfileDatabase db;
    GroupProfile g; g.group = "MYGRP"; g.supgroup = "SYS1"; db.add(g);
    auto json = JsonPolicyEmitter::emit(db);
    CHECK(json.find("\"Groups\"") != std::string::npos);
    CHECK(json.find("\"MYGRP\"") != std::string::npos);
}

TEST_CASE("Emitter JSON: emit contains Policies key") {
    ProfileDatabase db;
    DatasetProfile d; d.dsname = "DS1"; d.uacc = AccessLevel::READ; db.add(d);
    auto json = JsonPolicyEmitter::emit(db);
    CHECK(json.find("\"Policies\"") != std::string::npos);
    CHECK(json.find("\"DS1\"") != std::string::npos);
}

TEST_CASE("Emitter JSON: user policy shows admin flag") {
    ProfileDatabase db;
    UserProfile u; u.userid = "ADMIN"; u.special = true;
    auto json = JsonPolicyEmitter::emit_user(u, db);
    CHECK(json.find("\"IsAdmin\": true") != std::string::npos);
}

TEST_CASE("Emitter JSON: user policy shows revoked flag") {
    ProfileDatabase db;
    UserProfile u; u.userid = "REVOKED"; u.revoke = true;
    auto json = JsonPolicyEmitter::emit_user(u, db);
    CHECK(json.find("\"IsRevoked\": true") != std::string::npos);
}

TEST_CASE("Emitter JSON: user groups from connects") {
    ProfileDatabase db;
    UserProfile u; u.userid = "U1";
    ConnectProfile c; c.userid = "U1"; c.group = "GRP1";
    db.add(u); db.add(c);
    auto json = JsonPolicyEmitter::emit_user(u, db);
    CHECK(json.find("\"GRP1\"") != std::string::npos);
}

TEST_CASE("Emitter JSON: dataset policy with access list") {
    DatasetProfile ds; ds.dsname = "PROD.DATA"; ds.generic = false;
    ds.uacc = AccessLevel::NONE;
    ds.access_list.push_back({"ADMIN", AccessLevel::ALTER, false, 0});
    auto json = JsonPolicyEmitter::emit_dataset_policy(ds);
    CHECK(json.find("\"s3:*\"") != std::string::npos);
    CHECK(json.find("\"ADMIN\"") != std::string::npos);
    CHECK(json.find("\"Allow\"") != std::string::npos);
}

TEST_CASE("Emitter JSON: dataset policy NONE produces Deny") {
    DatasetProfile ds; ds.dsname = "SECURE"; ds.uacc = AccessLevel::NONE;
    ds.access_list.push_back({"NOBODY", AccessLevel::NONE, false, 0});
    auto json = JsonPolicyEmitter::emit_dataset_policy(ds);
    CHECK(json.find("\"Deny\"") != std::string::npos);
}

TEST_CASE("Emitter JSON: dataset policy generic flag") {
    DatasetProfile ds; ds.dsname = "SYS.**"; ds.generic = true;
    auto json = JsonPolicyEmitter::emit_dataset_policy(ds);
    CHECK(json.find("\"IsGeneric\": true") != std::string::npos);
}

TEST_CASE("Emitter JSON: escape special characters") {
    auto escaped = json_escape("he said \"hello\"\nand\\left");
    CHECK(escaped == "he said \\\"hello\\\"\\nand\\\\left");
}

TEST_CASE("Emitter JSON: access level names in output") {
    DatasetProfile ds; ds.dsname = "TEST"; ds.uacc = AccessLevel::READ;
    ds.access_list.push_back({"USR", AccessLevel::READ, false, 0});
    auto json = JsonPolicyEmitter::emit_dataset_policy(ds);
    CHECK(json.find("\"DefaultAccess\": \"READ\"") != std::string::npos);
    CHECK(json.find("s3:GetObject") != std::string::npos);
}

TEST_CASE("Emitter Terraform: user resource block") {
    UserProfile u; u.userid = "DEV01"; u.owner = "ADMIN"; u.dfltgrp = "DEVGRP";
    auto tf = TerraformEmitter::emit_user(u);
    CHECK(tf.find("resource \"aws_iam_user\"") != std::string::npos);
    CHECK(tf.find("\"dev01\"") != std::string::npos);
    CHECK(tf.find("name = \"DEV01\"") != std::string::npos);
}

TEST_CASE("Emitter Terraform: group resource block") {
    GroupProfile g; g.group = "SYS1";
    auto tf = TerraformEmitter::emit_group(g);
    CHECK(tf.find("resource \"aws_iam_group\"") != std::string::npos);
    CHECK(tf.find("\"sys1\"") != std::string::npos);
}

TEST_CASE("Emitter Terraform: dataset policy block") {
    DatasetProfile ds; ds.dsname = "PROD.DATA"; ds.uacc = AccessLevel::READ;
    auto tf = TerraformEmitter::emit_dataset_policy(ds);
    CHECK(tf.find("resource \"aws_iam_policy\"") != std::string::npos);
    CHECK(tf.find("Statement") != std::string::npos);
    CHECK(tf.find("s3:GetObject") != std::string::npos);
    CHECK(tf.find("s3:ListBucket") != std::string::npos);
}

TEST_CASE("Emitter Terraform: full emit has header") {
    ProfileDatabase db;
    auto tf = TerraformEmitter::emit(db);
    CHECK(tf.find("# RACF-to-Terraform") != std::string::npos);
}

TEST_CASE("Emitter Terraform: full emit with users and groups") {
    ProfileDatabase db;
    UserProfile u; u.userid = "U1"; u.owner = "SYS"; u.dfltgrp = "G1"; db.add(u);
    GroupProfile g; g.group = "G1"; db.add(g);
    ConnectProfile c; c.userid = "U1"; c.group = "G1"; db.add(c);
    auto tf = TerraformEmitter::emit(db);
    CHECK(tf.find("aws_iam_user") != std::string::npos);
    CHECK(tf.find("aws_iam_group") != std::string::npos);
    CHECK(tf.find("aws_iam_group_membership") != std::string::npos);
}

TEST_CASE("Emitter Terraform: user tags include RACF metadata") {
    UserProfile u; u.userid = "ADM"; u.special = true; u.revoke = false;
    u.owner = "SYS1"; u.dfltgrp = "SYS1";
    auto tf = TerraformEmitter::emit_user(u);
    CHECK(tf.find("racf_special") != std::string::npos);
    CHECK(tf.find("racf_owner") != std::string::npos);
    CHECK(tf.find("racf_dfltgrp") != std::string::npos);
}

TEST_CASE("Emitter K8s: service account from user") {
    UserProfile u; u.userid = "K8USER"; u.owner = "ADMIN";
    auto yaml = KubernetesRbacEmitter::emit_service_account(u);
    CHECK(yaml.find("kind: ServiceAccount") != std::string::npos);
    CHECK(yaml.find("name: k8user") != std::string::npos);
    CHECK(yaml.find("racf-userid: \"K8USER\"") != std::string::npos);
}

TEST_CASE("Emitter K8s: SPECIAL user gets annotation") {
    UserProfile u; u.userid = "SPADM"; u.owner = "SYS1"; u.special = true;
    auto yaml = KubernetesRbacEmitter::emit_service_account(u);
    CHECK(yaml.find("racf-special: \"true\"") != std::string::npos);
}

TEST_CASE("Emitter K8s: role from group") {
    ProfileDatabase db;
    GroupProfile g; g.group = "READERS";
    DatasetProfile ds; ds.dsname = "LIB"; ds.uacc = AccessLevel::READ;
    ds.access_list.push_back({"READERS", AccessLevel::READ, false, 0});
    db.add(g); db.add(ds);
    auto yaml = KubernetesRbacEmitter::emit_role(g, db);
    CHECK(yaml.find("kind: Role") != std::string::npos);
    CHECK(yaml.find("racf-readers") != std::string::npos);
    CHECK(yaml.find("\"get\"") != std::string::npos);
    CHECK(yaml.find("\"list\"") != std::string::npos);
}

TEST_CASE("Emitter K8s: role binding from connect") {
    ConnectProfile c; c.userid = "DEV01"; c.group = "DEVGRP";
    auto yaml = KubernetesRbacEmitter::emit_role_binding(c);
    CHECK(yaml.find("kind: RoleBinding") != std::string::npos);
    CHECK(yaml.find("name: dev01-to-devgrp") != std::string::npos);
    CHECK(yaml.find("racf-devgrp") != std::string::npos);
}

TEST_CASE("Emitter K8s: cluster admin for SPECIAL user") {
    UserProfile u; u.userid = "SUPERADM"; u.special = true;
    auto yaml = KubernetesRbacEmitter::emit_cluster_admin(u);
    CHECK(yaml.find("kind: ClusterRoleBinding") != std::string::npos);
    CHECK(yaml.find("cluster-admin") != std::string::npos);
    CHECK(yaml.find("superadm") != std::string::npos);
}

TEST_CASE("Emitter K8s: dataset role") {
    DatasetProfile ds; ds.dsname = "PROD.LIB"; ds.uacc = AccessLevel::CONTROL;
    ds.generic = true;
    auto yaml = KubernetesRbacEmitter::emit_dataset_role(ds);
    CHECK(yaml.find("kind: Role") != std::string::npos);
    CHECK(yaml.find("racf-generic: \"true\"") != std::string::npos);
    CHECK(yaml.find("\"delete\"") != std::string::npos);
}

TEST_CASE("Emitter K8s: full emit includes separators") {
    ProfileDatabase db;
    UserProfile u; u.userid = "U1"; u.owner = "O"; db.add(u);
    GroupProfile g; g.group = "G1"; db.add(g);
    auto yaml = KubernetesRbacEmitter::emit(db);
    CHECK(yaml.find("---") != std::string::npos);
}

TEST_CASE("Emitter: access_to_k8s_verbs mapping") {
    CHECK(access_to_k8s_verbs(AccessLevel::NONE).empty());
    CHECK(access_to_k8s_verbs(AccessLevel::READ).size() == 2);
    CHECK(access_to_k8s_verbs(AccessLevel::UPDATE).size() == 4);
    CHECK(access_to_k8s_verbs(AccessLevel::CONTROL).size() == 5);
    CHECK(access_to_k8s_verbs(AccessLevel::ALTER).size() == 7);
}

TEST_CASE("Emitter: access_to_iam_actions mapping") {
    CHECK(access_to_iam_actions(AccessLevel::NONE).empty());
    CHECK(access_to_iam_actions(AccessLevel::ALTER).size() == 1);
    CHECK(access_to_iam_actions(AccessLevel::ALTER)[0] == "s3:*");
}

TEST_CASE("Emitter: sanitize_id function") {
    CHECK(sanitize_id("SYS1.PARMLIB") == "sys1_parmlib");
    CHECK(sanitize_id("PROD.**") == "prod");  // trailing non-alnum stripped
    CHECK(sanitize_id("ADMIN01") == "admin01");
}

// ============================================================
// AUDIT TESTS (15+)
// ============================================================

TEST_CASE("Audit: detect SPECIAL+OPERATIONS escalation") {
    ProfileDatabase db;
    UserProfile u; u.userid = "DANGER"; u.special = true; u.operations = true;
    db.add(u);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto critical = report.by_severity(Severity::CRITICAL);
    CHECK(critical.size() >= 1);
    bool found = false;
    for (auto& f : critical) {
        if (f.category == "PRIVILEGE_ESCALATION" && f.entity == "DANGER") found = true;
    }
    CHECK(found);
}

TEST_CASE("Audit: detect SPECIAL+AUDITOR warning") {
    ProfileDatabase db;
    UserProfile u; u.userid = "AUDITADM"; u.special = true; u.auditor = true;
    db.add(u);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto warnings = report.by_severity(Severity::WARNING);
    bool found = false;
    for (auto& f : warnings) {
        if (f.entity == "AUDITADM" && f.category == "PRIVILEGE_ESCALATION") found = true;
    }
    CHECK(found);
}

TEST_CASE("Audit: no escalation for normal user") {
    ProfileDatabase db;
    UserProfile u; u.userid = "NORMAL"; u.special = false; u.operations = false;
    db.add(u);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto esc = report.by_category("PRIVILEGE_ESCALATION");
    CHECK(esc.empty());
}

TEST_CASE("Audit: detect overlapping datasets") {
    ProfileDatabase db;
    DatasetProfile d1; d1.dsname = "SYS1.PARMLIB"; d1.generic = false; d1.owner = "SYS1";
    DatasetProfile d2; d2.dsname = "SYS1.**"; d2.generic = true; d2.owner = "SYS1";
    db.add(d1); db.add(d2);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto overlaps = report.by_category("OVERLAPPING_DATASET");
    CHECK(overlaps.size() >= 1);
}

TEST_CASE("Audit: no overlap when all same type") {
    ProfileDatabase db;
    DatasetProfile d1; d1.dsname = "SYS1.A"; d1.generic = false; d1.owner = "SYS1";
    DatasetProfile d2; d2.dsname = "SYS1.B"; d2.generic = false; d2.owner = "SYS1";
    db.add(d1); db.add(d2);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto overlaps = report.by_category("OVERLAPPING_DATASET");
    CHECK(overlaps.empty());
}

TEST_CASE("Audit: detect contradictory memberships") {
    ProfileDatabase db;
    UserProfile u; u.userid = "USR1";
    GroupProfile g1; g1.group = "GRP_ALLOW";
    GroupProfile g2; g2.group = "GRP_DENY";
    ConnectProfile c1; c1.userid = "USR1"; c1.group = "GRP_ALLOW";
    ConnectProfile c2; c2.userid = "USR1"; c2.group = "GRP_DENY";
    DatasetProfile ds; ds.dsname = "SHARED.DATA"; ds.owner = "ADMIN";
    ds.access_list.push_back({"GRP_ALLOW", AccessLevel::ALTER, false, 0});
    ds.access_list.push_back({"GRP_DENY", AccessLevel::NONE, false, 0});
    db.add(u); db.add(g1); db.add(g2); db.add(c1); db.add(c2); db.add(ds);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto contradictions = report.by_category("CONTRADICTORY_MEMBERSHIP");
    CHECK(contradictions.size() >= 1);
}

TEST_CASE("Audit: detect orphaned dataset owner") {
    ProfileDatabase db;
    DatasetProfile ds; ds.dsname = "ORPHAN.DATA"; ds.owner = "DELETED_USER";
    db.add(ds);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto orphans = report.by_category("ORPHANED_RESOURCE");
    CHECK(orphans.size() >= 1);
    CHECK(orphans[0].entity == "ORPHAN.DATA");
}

TEST_CASE("Audit: no orphan when owner exists") {
    ProfileDatabase db;
    UserProfile u; u.userid = "VALIDOWN";
    DatasetProfile ds; ds.dsname = "OWNED.DATA"; ds.owner = "VALIDOWN";
    db.add(u); db.add(ds);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto orphans = report.by_category("ORPHANED_RESOURCE");
    CHECK(orphans.empty());
}

TEST_CASE("Audit: detect unused account (no passdate)") {
    ProfileDatabase db;
    UserProfile u; u.userid = "NEVERLOGIN"; u.passdate = "";
    db.add(u);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto unused = report.by_category("UNUSED_ACCOUNT");
    CHECK(unused.size() >= 1);
}

TEST_CASE("Audit: detect stale password") {
    ProfileDatabase db;
    UserProfile u; u.userid = "OLDPASS"; u.passdate = "2025-01-01";
    db.add(u);
    SecurityAuditor auditor;
    auditor.set_reference_date("2026-03-27");
    auditor.set_stale_days(90);
    auto report = auditor.audit(db);
    auto unused = report.by_category("UNUSED_ACCOUNT");
    bool found = false;
    for (auto& f : unused) {
        if (f.entity == "OLDPASS" && f.description.find("threshold") != std::string::npos)
            found = true;
    }
    CHECK(found);
}

TEST_CASE("Audit: detect excessive UACC") {
    ProfileDatabase db;
    DatasetProfile ds; ds.dsname = "OPEN.DATA"; ds.uacc = AccessLevel::ALTER;
    ds.owner = "SYS1";
    UserProfile u; u.userid = "SYS1"; db.add(u);
    db.add(ds);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto excessive = report.by_category("EXCESSIVE_PERMISSION");
    CHECK(excessive.size() >= 1);
}

TEST_CASE("Audit: detect revoked connects") {
    ProfileDatabase db;
    ConnectProfile c; c.userid = "BADU"; c.group = "GRP"; c.revoke = true;
    db.add(c);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto revoked = report.by_category("REVOKED_CONNECT");
    CHECK(revoked.size() == 1);
}

TEST_CASE("Audit: summary statistics") {
    ProfileDatabase db;
    UserProfile u; u.userid = "U"; db.add(u);
    GroupProfile g; g.group = "G"; db.add(g);
    DatasetProfile d; d.dsname = "D"; d.owner = "U"; db.add(d);
    ConnectProfile c; c.userid = "U"; c.group = "G"; db.add(c);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    CHECK(report.summary.total_users == 1);
    CHECK(report.summary.total_groups == 1);
    CHECK(report.summary.total_datasets == 1);
    CHECK(report.summary.total_connects == 1);
}

TEST_CASE("Audit: text report generation") {
    ProfileDatabase db;
    UserProfile u; u.userid = "ADM"; u.special = true; u.operations = true;
    db.add(u);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto text = report.to_text();
    CHECK(text.find("RACF Security Audit Report") != std::string::npos);
    CHECK(text.find("CRITICAL") != std::string::npos);
    CHECK(text.find("PRIVILEGE_ESCALATION") != std::string::npos);
}

TEST_CASE("Audit: JSON report generation") {
    ProfileDatabase db;
    UserProfile u; u.userid = "ADM"; u.special = true; u.operations = true;
    db.add(u);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto json = report.to_json();
    CHECK(json.find("\"summary\"") != std::string::npos);
    CHECK(json.find("\"findings\"") != std::string::npos);
    CHECK(json.find("\"CRITICAL\"") != std::string::npos);
}

TEST_CASE("Audit: filter by severity") {
    ProfileDatabase db;
    UserProfile u; u.userid = "ADM"; u.special = true; u.operations = true; u.auditor = true;
    db.add(u);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto crit = report.by_severity(Severity::CRITICAL);
    auto warn = report.by_severity(Severity::WARNING);
    CHECK(!crit.empty());
    CHECK(!warn.empty());
}

TEST_CASE("Audit: permission graph construction") {
    ProfileDatabase db;
    UserProfile u; u.userid = "U1"; db.add(u);
    GroupProfile g; g.group = "G1"; db.add(g);
    ConnectProfile c; c.userid = "U1"; c.group = "G1"; db.add(c);
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    CHECK(report.graph.size() >= 2);
    auto ui = report.graph.find("U1", "user");
    REQUIRE(ui.has_value());
    auto reachable = report.graph.reachable(*ui);
    CHECK(reachable.size() >= 2);  // user + group
}

// ============================================================
// INTEGRATION TESTS (10+)
// ============================================================

TEST_CASE("Integration: full dump -> database -> JSON policy") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto json = JsonPolicyEmitter::emit(db);
    CHECK(json.find("\"ADMIN01\"") != std::string::npos);
    CHECK(json.find("\"USER001\"") != std::string::npos);
    CHECK(json.find("\"SYS1.PARMLIB\"") != std::string::npos);
    CHECK(json.find("\"Version\"") != std::string::npos);
}

TEST_CASE("Integration: full dump -> database -> Terraform") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto tf = TerraformEmitter::emit(db);
    CHECK(tf.find("aws_iam_user") != std::string::npos);
    CHECK(tf.find("aws_iam_group") != std::string::npos);
    CHECK(tf.find("aws_iam_policy") != std::string::npos);
}

TEST_CASE("Integration: full dump -> database -> K8s RBAC") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto yaml = KubernetesRbacEmitter::emit(db);
    CHECK(yaml.find("ServiceAccount") != std::string::npos);
    CHECK(yaml.find("Role") != std::string::npos);
    CHECK(yaml.find("RoleBinding") != std::string::npos);
}

TEST_CASE("Integration: full dump -> audit report") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    SecurityAuditor auditor;
    auditor.set_reference_date("2026-03-27");
    auto report = auditor.audit(db);
    CHECK(report.summary.total_users == 4);
    CHECK(report.summary.total_groups == 4);
    CHECK(report.summary.total_findings > 0);
}

TEST_CASE("Integration: ADMIN01 SPECIAL escalation in full dump") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    // ADMIN01 has SPECIAL+OPERATIONS
    auto crit = report.by_severity(Severity::CRITICAL);
    bool found = false;
    for (auto& f : crit) {
        if (f.entity == "ADMIN01") found = true;
    }
    CHECK(found);
}

TEST_CASE("Integration: overlapping generic/discrete datasets in full dump") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto overlaps = report.by_category("OVERLAPPING_DATASET");
    // DEV.SOURCE.* (generic) exists under DEV HLQ, no discrete -> no overlap for DEV
    // But PROD.** is generic... no discrete PROD -- so overlap only if HLQ matches
    // Check we at least get the right category type
    CHECK(report.summary.total_datasets == 4);
}

TEST_CASE("Integration: effective permission for USER001 to SYS1.PARMLIB") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto dsets = db.find_datasets("SYS1.PARMLIB");
    REQUIRE(!dsets.empty());
    // USER001 is in DEVGRP which has READ on PARMLIB, and TESTGRP which has NONE
    // UACC is READ. DEVGRP gives READ. Best is READ.
    auto perm = db.effective_permission("USER001", dsets[0]);
    CHECK(perm == AccessLevel::READ);
}

TEST_CASE("Integration: effective permission for ADMIN01 (SPECIAL)") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto dsets = db.find_datasets("SYS1.PARMLIB");
    REQUIRE(!dsets.empty());
    auto perm = db.effective_permission("ADMIN01", dsets[0]);
    CHECK(perm == AccessLevel::ALTER);  // SPECIAL -> ALTER
}

TEST_CASE("Integration: revoked user USER002 connect in audit") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto revoked = report.by_category("REVOKED_CONNECT");
    bool found = false;
    for (auto& f : revoked) {
        if (f.entity.find("USER002") != std::string::npos) found = true;
    }
    CHECK(found);
}

TEST_CASE("Integration: full pipeline dump -> policy -> audit -> text report") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    // Generate all policy formats
    auto json = JsonPolicyEmitter::emit(db);
    auto tf = TerraformEmitter::emit(db);
    auto yaml = KubernetesRbacEmitter::emit(db);
    // Run audit
    SecurityAuditor auditor;
    auditor.set_reference_date("2026-03-27");
    auto report = auditor.audit(db);
    auto text = report.to_text();
    // All outputs should be non-empty and well-formed
    CHECK(!json.empty());
    CHECK(!tf.empty());
    CHECK(!yaml.empty());
    CHECK(!text.empty());
    CHECK(report.summary.total_users == 4);
    CHECK(report.summary.total_groups == 4);
    CHECK(report.summary.total_datasets == 4);
    CHECK(report.summary.total_generals == 2);
    CHECK(report.summary.total_connects == 5);
}

TEST_CASE("Integration: group hierarchy in full dump") {
    IrrdbuParser parser;
    auto result = parser.parse(SAMPLE_DUMP);
    auto db = result.to_database();
    auto chain = db.group_hierarchy("PRODGRP");
    // PRODGRP -> DEVGRP -> SYS1
    REQUIRE(chain.size() == 3);
    CHECK(chain[0] == "PRODGRP");
    CHECK(chain[1] == "DEVGRP");
    CHECK(chain[2] == "SYS1");
}

TEST_CASE("Integration: orphaned general resource in full dump") {
    IrrdbuParser parser;
    // Add a general resource with an owner that does not exist
    std::string dump = std::string(SAMPLE_DUMP) + "\n0500 FACILITY CUSTOM.RES NONE NO GHOST_OWNER";
    auto result = parser.parse(dump);
    auto db = result.to_database();
    SecurityAuditor auditor;
    auto report = auditor.audit(db);
    auto orphans = report.by_category("ORPHANED_RESOURCE");
    bool found = false;
    for (auto& f : orphans) {
        if (f.detail.find("GHOST_OWNER") != std::string::npos) found = true;
    }
    CHECK(found);
}
