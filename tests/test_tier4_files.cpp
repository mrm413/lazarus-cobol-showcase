#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"
// ============================================================================
// Lazarus C++17 Platform — Tier 4: GDG + Flat File Migration Tests
// Torsova LLC — lazarus-systems.com
// ============================================================================

#include "include/lazarus/file/dsn.h"
#include "include/lazarus/file/gdg.h"
#include "include/lazarus/file/sequential.h"
#include "include/lazarus/file/vsam.h"
#include "include/lazarus/file/pds.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace lazarus::file;

// ── Temporary directory helper ─────────────────────────────────────────
struct TempDir {
    fs::path path;
    TempDir(const std::string& label) {
        path = fs::temp_directory_path() / ("laz_t4_" + label + "_" +
            std::to_string(std::hash<std::string>{}(label) ^
                static_cast<size_t>(std::chrono::steady_clock::now()
                    .time_since_epoch().count())));
        fs::create_directories(path);
    }
    ~TempDir() {
        std::error_code ec;
        fs::remove_all(path, ec);
    }
    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;
};

// ============================================================================
// DSN TESTS (15+)
// ============================================================================

TEST_CASE("DSN: parse simple two-qualifier name") {
    auto c = parse_dsn("SYS1.MACLIB");
    CHECK(c.hlq == "SYS1");
    CHECK(c.qualifiers.size() == 1);
    CHECK(c.qualifiers[0] == "MACLIB");
    CHECK(c.member.empty());
    CHECK_FALSE(c.is_gdg);
    CHECK_FALSE(c.is_temporary);
}

TEST_CASE("DSN: parse three-qualifier name") {
    auto c = parse_dsn("PROD.PAYROLL.DATA");
    CHECK(c.hlq == "PROD");
    CHECK(c.qualifiers.size() == 2);
    CHECK(c.qualifiers[0] == "PAYROLL");
    CHECK(c.qualifiers[1] == "DATA");
}

TEST_CASE("DSN: parse with member") {
    auto c = parse_dsn("SYS1.MACLIB(IEFBR14)");
    CHECK(c.hlq == "SYS1");
    CHECK(c.qualifiers[0] == "MACLIB");
    CHECK(c.member == "IEFBR14");
    CHECK_FALSE(c.is_gdg);
}

TEST_CASE("DSN: parse GDG relative +1") {
    auto c = parse_dsn("PROD.DAILY(+1)");
    CHECK(c.is_gdg);
    CHECK(c.gdg_relative == 1);
    CHECK(c.hlq == "PROD");
}

TEST_CASE("DSN: parse GDG relative -1") {
    auto c = parse_dsn("PROD.DAILY(-1)");
    CHECK(c.is_gdg);
    CHECK(c.gdg_relative == -1);
}

TEST_CASE("DSN: parse GDG relative 0") {
    auto c = parse_dsn("PROD.DAILY(0)");
    CHECK(c.is_gdg);
    CHECK(c.gdg_relative == 0);
}

TEST_CASE("DSN: parse temporary dataset") {
    auto c = parse_dsn("&&SORTWORK");
    CHECK(c.is_temporary);
    CHECK(c.hlq == "SORTWORK");
}

TEST_CASE("DSN: validate correct qualifiers") {
    CHECK(is_valid_qualifier("SYS1"));
    CHECK(is_valid_qualifier("A"));
    CHECK(is_valid_qualifier("ABCDEFGH"));
    CHECK(is_valid_qualifier("#NATL"));
    CHECK(is_valid_qualifier("@SPEC"));
    CHECK(is_valid_qualifier("$DOLLAR"));
}

TEST_CASE("DSN: reject invalid qualifiers") {
    CHECK_FALSE(is_valid_qualifier(""));
    CHECK_FALSE(is_valid_qualifier("TOOLONGNAM"));
    CHECK_FALSE(is_valid_qualifier("1STARTS"));
    CHECK_FALSE(is_valid_qualifier("HAS SPACE"));
    CHECK_FALSE(is_valid_qualifier("HAS-DASH"));
}

TEST_CASE("DSN: validate full DSN") {
    CHECK(validate_dsn("SYS1.MACLIB"));
    CHECK(validate_dsn("PROD.PAYROLL.DATA"));
    CHECK(validate_dsn("SYS1.MACLIB(MEMBER1)"));
    CHECK(validate_dsn("#HLQ.QUAL"));
    CHECK_FALSE(validate_dsn("TOOLONGHLQ.QUAL"));
    CHECK_FALSE(validate_dsn("1BAD.QUAL"));
}

TEST_CASE("DSN: dsn_to_string round-trip") {
    auto c = parse_dsn("PROD.PAYROLL.DATA");
    CHECK(dsn_to_string(c) == "PROD.PAYROLL.DATA");

    auto c2 = parse_dsn("SYS1.MACLIB(IEFBR14)");
    CHECK(dsn_to_string(c2) == "SYS1.MACLIB(IEFBR14)");
}

TEST_CASE("DSN: resolver maps to filesystem path") {
    TempDir td("dsn_resolve");
    DsnResolver resolver(td.path);
    auto p = resolver.resolve("PROD.PAYROLL.DATA");
    CHECK(p == td.path / "PROD" / "PAYROLL" / "DATA");
}

TEST_CASE("DSN: resolver with member appends filename") {
    TempDir td("dsn_member");
    DsnResolver resolver(td.path);
    auto p = resolver.resolve("SYS1.MACLIB(MYMEMBER)");
    CHECK(p == td.path / "SYS1" / "MACLIB" / "MYMEMBER");
}

TEST_CASE("DSN: resolve_base strips member") {
    TempDir td("dsn_base");
    DsnResolver resolver(td.path);
    auto p = resolver.resolve_base("SYS1.MACLIB(MEMBER)");
    CHECK(p == td.path / "SYS1" / "MACLIB");
}

TEST_CASE("DSN: concatenation reads across files") {
    TempDir td("dsn_concat");
    fs::path f1 = td.path / "part1.dat";
    fs::path f2 = td.path / "part2.dat";
    {
        std::ofstream(f1) << "LINE1\nLINE2\n";
        std::ofstream(f2) << "LINE3\nLINE4\n";
    }
    DsnConcatenation concat;
    concat.add(f1);
    concat.add(f2);
    CHECK(concat.count() == 2);
    auto lines = concat.read_all_lines();
    CHECK(lines.size() == 4);
    CHECK(lines[0] == "LINE1");
    CHECK(lines[2] == "LINE3");
}

TEST_CASE("DSN: concatenation skips missing files") {
    TempDir td("dsn_concat_miss");
    fs::path f1 = td.path / "exists.dat";
    fs::path f2 = td.path / "nope.dat";
    std::ofstream(f1) << "DATA\n";
    DsnConcatenation concat;
    concat.add(f1);
    concat.add(f2);
    auto lines = concat.read_all_lines();
    CHECK(lines.size() == 1);
    CHECK(lines[0] == "DATA");
}

TEST_CASE("DSN: temporary dataset write and read") {
    TempDataset tmp("SORTWORK");
    tmp.write("TEMPORARY DATA CONTENT");
    CHECK(tmp.read() == "TEMPORARY DATA CONTENT");
    CHECK(fs::exists(tmp.path()));
}

TEST_CASE("DSN: temporary dataset auto-cleanup") {
    fs::path saved;
    {
        TempDataset tmp("AUTOCLEAN");
        tmp.write("GONE");
        saved = tmp.path();
        CHECK(fs::exists(saved));
    }
    CHECK_FALSE(fs::exists(saved));
}

TEST_CASE("DSN: parse missing closing paren throws") {
    CHECK_THROWS_AS(parse_dsn("SYS1.MACLIB(BAD"), std::invalid_argument);
}

// ============================================================================
// GDG TESTS (25+)
// ============================================================================

TEST_CASE("GDG: create base with no generations") {
    TempDir td("gdg_empty");
    GdgBase base("PROD.DAILY", td.path, 5);
    CHECK(base.generation_count() == 0);
    CHECK(base.base_name() == "PROD.DAILY");
    CHECK(base.limit() == 5);
}

TEST_CASE("GDG: allocate single generation") {
    TempDir td("gdg_alloc1");
    GdgBase base("TEST.GDG", td.path, 10);
    auto e = base.allocate_new();
    CHECK(base.generation_count() == 1);
    CHECK(e.gen_num == 1);
    CHECK(e.version == 0);
    CHECK(e.active);
}

TEST_CASE("GDG: generation naming format") {
    auto name = format_generation_name("PROD.DAILY", 1, 0);
    CHECK(name == "PROD.DAILY.G0001V00");

    auto name2 = format_generation_name("PROD.DAILY", 123, 5);
    CHECK(name2 == "PROD.DAILY.G0123V05");
}

TEST_CASE("GDG: parse generation suffix") {
    uint32_t gen = 0, ver = 0;
    CHECK(parse_generation_suffix("PROD.DAILY.G0042V03", gen, ver));
    CHECK(gen == 42);
    CHECK(ver == 3);
}

TEST_CASE("GDG: parse bad suffix fails") {
    uint32_t gen = 0, ver = 0;
    CHECK_FALSE(parse_generation_suffix("PROD.DAILY.XBAD", gen, ver));
    CHECK_FALSE(parse_generation_suffix("PROD.DAILY.G", gen, ver));
}

TEST_CASE("GDG: allocate multiple generations") {
    TempDir td("gdg_multi");
    GdgBase base("MULTI.GDG", td.path, 10);
    base.allocate_new();
    base.allocate_new();
    base.allocate_new();
    CHECK(base.generation_count() == 3);
}

TEST_CASE("GDG: resolve relative 0 returns newest") {
    TempDir td("gdg_rel0");
    GdgBase base("REL.GDG", td.path, 10);
    base.allocate_new();
    base.allocate_new();
    auto e = base.resolve_relative(0);
    REQUIRE(e.has_value());
    CHECK(e->gen_num == 2);
}

TEST_CASE("GDG: resolve relative -1 returns previous") {
    TempDir td("gdg_relm1");
    GdgBase base("REL.GDG", td.path, 10);
    base.allocate_new();
    base.allocate_new();
    base.allocate_new();
    auto e = base.resolve_relative(-1);
    REQUIRE(e.has_value());
    CHECK(e->gen_num == 2);
}

TEST_CASE("GDG: resolve relative -2 returns two back") {
    TempDir td("gdg_relm2");
    GdgBase base("REL.GDG", td.path, 10);
    base.allocate_new();
    base.allocate_new();
    base.allocate_new();
    auto e = base.resolve_relative(-2);
    REQUIRE(e.has_value());
    CHECK(e->gen_num == 1);
}

TEST_CASE("GDG: resolve relative out of range returns nullopt") {
    TempDir td("gdg_oor");
    GdgBase base("REL.GDG", td.path, 10);
    base.allocate_new();
    CHECK_FALSE(base.resolve_relative(-1).has_value());
    CHECK_FALSE(base.resolve_relative(-5).has_value());
    CHECK_FALSE(base.resolve_relative(+1).has_value());
}

TEST_CASE("GDG: resolve absolute generation number") {
    TempDir td("gdg_abs");
    GdgBase base("ABS.GDG", td.path, 10);
    base.allocate_new(); // gen 1
    base.allocate_new(); // gen 2
    base.allocate_new(); // gen 3
    auto e = base.resolve_absolute(2);
    REQUIRE(e.has_value());
    CHECK(e->gen_num == 2);
    CHECK_FALSE(base.resolve_absolute(99).has_value());
}

TEST_CASE("GDG: LIMIT with SCRATCH rollover") {
    TempDir td("gdg_limit_scratch");
    GdgBase base("LIM.GDG", td.path, 3, GdgScratch::SCRATCH);
    base.allocate_new(); // 1
    base.allocate_new(); // 2
    base.allocate_new(); // 3
    CHECK(base.generation_count() == 3);

    // Adding 4th should roll off gen 1
    base.allocate_new(); // 4
    CHECK(base.generation_count() == 3);
    CHECK_FALSE(base.resolve_absolute(1).has_value());
    CHECK(base.resolve_absolute(4).has_value());
}

TEST_CASE("GDG: LIMIT with NOSCRATCH keeps inactive entries") {
    TempDir td("gdg_noscratch");
    GdgBase base("LIM.GDG", td.path, 2, GdgScratch::NOSCRATCH);
    auto e1 = base.allocate_new();
    // Write data to gen 1 so file exists
    { std::ofstream out(e1.data_path); out << "DATA1"; }

    base.allocate_new();
    base.allocate_new(); // should deactivate gen 1

    CHECK(base.generation_count() == 2);
    // NOSCRATCH: file may still exist on disk (not deleted)
    // The entry is inactive but data was preserved
    CHECK_FALSE(base.resolve_absolute(1).has_value());
}

TEST_CASE("GDG: SCRATCH deletes rolled-off data") {
    TempDir td("gdg_scratch_del");
    GdgBase base("DEL.GDG", td.path, 2, GdgScratch::SCRATCH);
    auto e1 = base.allocate_new();
    { std::ofstream out(e1.data_path); out << "OLD DATA"; }
    CHECK(fs::exists(e1.data_path));

    base.allocate_new();
    base.allocate_new(); // rolls off gen 1 with SCRATCH

    CHECK_FALSE(fs::exists(e1.data_path));
}

TEST_CASE("GDG: rollback_latest deactivates newest") {
    TempDir td("gdg_rollback");
    GdgBase base("RB.GDG", td.path, 10);
    base.allocate_new();
    base.allocate_new();
    CHECK(base.generation_count() == 2);
    base.rollback_latest();
    CHECK(base.generation_count() == 1);
}

TEST_CASE("GDG: rollback on empty returns false") {
    TempDir td("gdg_rb_empty");
    GdgBase base("RB.GDG", td.path, 10);
    CHECK_FALSE(base.rollback_latest());
}

TEST_CASE("GDG: GdgGeneration RAII commit") {
    TempDir td("gdg_raii_commit");
    GdgBase base("RAII.GDG", td.path, 10);
    {
        GdgGeneration gen(base);
        gen.write("COMMITTED DATA");
        gen.commit();
    }
    CHECK(base.generation_count() == 1);
    auto e = base.resolve_relative(0);
    REQUIRE(e.has_value());
}

TEST_CASE("GDG: GdgGeneration RAII rollback on scope exit") {
    TempDir td("gdg_raii_rb");
    GdgBase base("RAII.GDG", td.path, 10);
    {
        GdgGeneration gen(base);
        gen.write("WILL BE ROLLED BACK");
        // no commit — destructor should rollback
    }
    CHECK(base.generation_count() == 0);
}

TEST_CASE("GDG: GdgGeneration write and read") {
    TempDir td("gdg_wr");
    GdgBase base("WR.GDG", td.path, 10);
    GdgGeneration gen(base);
    gen.write("PAYLOAD 12345");
    CHECK(gen.read() == "PAYLOAD 12345");
    gen.commit();
}

TEST_CASE("GDG: generation name from entry") {
    TempDir td("gdg_name");
    GdgBase base("NM.GDG", td.path, 10);
    GdgGeneration gen(base);
    CHECK(gen.name() == "NM.GDG.G0001V00");
    gen.commit();
}

TEST_CASE("GDG: active_entries returns oldest first") {
    TempDir td("gdg_order");
    GdgBase base("ORD.GDG", td.path, 10);
    base.allocate_new();
    base.allocate_new();
    base.allocate_new();
    auto entries = base.active_entries();
    CHECK(entries.size() == 3);
    CHECK(entries[0].gen_num == 1);
    CHECK(entries[1].gen_num == 2);
    CHECK(entries[2].gen_num == 3);
}

TEST_CASE("GDG: catalog persists across instances") {
    TempDir td("gdg_persist");
    {
        GdgBase base("PER.GDG", td.path, 10);
        base.allocate_new();
        base.allocate_new();
    }
    {
        GdgBase base2("PER.GDG", td.path, 10);
        CHECK(base2.generation_count() == 2);
        auto e = base2.resolve_relative(0);
        REQUIRE(e.has_value());
        CHECK(e->gen_num == 2);
    }
}

TEST_CASE("GDG: stress — allocate beyond LIMIT multiple times") {
    TempDir td("gdg_stress");
    GdgBase base("STR.GDG", td.path, 3);
    for (int i = 0; i < 10; ++i)
        base.allocate_new();
    CHECK(base.generation_count() == 3);
    auto e = base.resolve_relative(0);
    REQUIRE(e.has_value());
    CHECK(e->gen_num == 10);
}

TEST_CASE("GDG: current_path returns newest path") {
    TempDir td("gdg_curpath");
    GdgBase base("CP.GDG", td.path, 10);
    base.allocate_new();
    auto p = base.current_path();
    CHECK(p.string().find("G0001V00") != std::string::npos);
}

TEST_CASE("GDG: current_path on empty throws") {
    TempDir td("gdg_curpath_empty");
    GdgBase base("CP.GDG", td.path, 10);
    CHECK_THROWS(base.current_path());
}

// ============================================================================
// SEQUENTIAL FILE TESTS (15+)
// ============================================================================

TEST_CASE("Sequential: write and read fixed-length records") {
    TempDir td("seq_fixed");
    SeqDcb dcb;
    dcb.recfm = RecFm::FB;
    dcb.lrecl = 20;
    SequentialFile sf(td.path / "test.dat", dcb);
    sf.write_records({"RECORD ONE", "RECORD TWO", "REC THREE"});
    auto recs = sf.read_records();
    CHECK(recs.size() == 3);
    CHECK(recs[0].size() == 20); // padded to LRECL
    CHECK(recs[0].substr(0, 10) == "RECORD ONE");
}

TEST_CASE("Sequential: fixed records pad with spaces") {
    TempDir td("seq_pad");
    SeqDcb dcb;
    dcb.recfm = RecFm::F;
    dcb.lrecl = 10;
    SequentialFile sf(td.path / "pad.dat", dcb);
    sf.write_records({"HI"});
    auto recs = sf.read_records();
    REQUIRE(recs.size() == 1);
    CHECK(recs[0] == "HI        ");
}

TEST_CASE("Sequential: fixed records truncate long data") {
    TempDir td("seq_trunc");
    SeqDcb dcb;
    dcb.recfm = RecFm::FB;
    dcb.lrecl = 5;
    SequentialFile sf(td.path / "trunc.dat", dcb);
    sf.write_records({"ABCDEFGHIJ"});
    auto recs = sf.read_records();
    CHECK(recs[0] == "ABCDE");
}

TEST_CASE("Sequential: variable-length records with RDW") {
    TempDir td("seq_var");
    SeqDcb dcb;
    dcb.recfm = RecFm::V;
    dcb.lrecl = 100;
    SequentialFile sf(td.path / "var.dat", dcb);
    sf.write_records({"SHORT", "A LONGER RECORD HERE", "X"});
    auto recs = sf.read_records();
    CHECK(recs.size() == 3);
    CHECK(recs[0] == "SHORT");
    CHECK(recs[1] == "A LONGER RECORD HERE");
    CHECK(recs[2] == "X");
}

TEST_CASE("Sequential: VB records preserve variable lengths") {
    TempDir td("seq_vb");
    SeqDcb dcb;
    dcb.recfm = RecFm::VB;
    dcb.lrecl = 200;
    SequentialFile sf(td.path / "vb.dat", dcb);
    std::vector<std::string> recs;
    for (int i = 0; i < 20; ++i)
        recs.push_back(std::string(static_cast<size_t>(i + 1), 'A' + (i % 26)));
    sf.write_records(recs);
    auto out = sf.read_records();
    CHECK(out.size() == 20);
    CHECK(out[0].size() == 1);
    CHECK(out[19].size() == 20);
}

TEST_CASE("Sequential: V record exceeding LRECL throws") {
    TempDir td("seq_v_over");
    SeqDcb dcb;
    dcb.recfm = RecFm::V;
    dcb.lrecl = 5;
    SequentialFile sf(td.path / "over.dat", dcb);
    CHECK_THROWS(sf.write_records({"TOOLONG"}));
}

TEST_CASE("Sequential: append_record adds to end") {
    TempDir td("seq_append");
    SeqDcb dcb;
    dcb.recfm = RecFm::FB;
    dcb.lrecl = 10;
    SequentialFile sf(td.path / "app.dat", dcb);
    sf.write_records({"LINE1"});
    sf.append_record("LINE2");
    auto recs = sf.read_records();
    CHECK(recs.size() == 2);
}

TEST_CASE("Sequential: record_count") {
    TempDir td("seq_count");
    SeqDcb dcb;
    dcb.recfm = RecFm::FB;
    dcb.lrecl = 80;
    SequentialFile sf(td.path / "cnt.dat", dcb);
    sf.write_records({"A", "B", "C", "D", "E"});
    CHECK(sf.record_count() == 5);
}

TEST_CASE("Sequential: range-based for loop") {
    TempDir td("seq_iter");
    SeqDcb dcb;
    dcb.recfm = RecFm::V;
    dcb.lrecl = 100;
    SequentialFile sf(td.path / "iter.dat", dcb);
    sf.write_records({"ALPHA", "BETA", "GAMMA"});
    std::vector<std::string> collected;
    for (auto& rec : sf) {
        collected.push_back(rec);
    }
    CHECK(collected.size() == 3);
    CHECK(collected[0] == "ALPHA");
    CHECK(collected[2] == "GAMMA");
}

TEST_CASE("Sequential: block processing with FB") {
    TempDir td("seq_blk");
    SeqDcb dcb;
    dcb.recfm = RecFm::FB;
    dcb.lrecl = 10;
    dcb.blksize = 30; // 3 records per block
    SequentialFile sf(td.path / "blk.dat", dcb);
    sf.write_records({"R1", "R2", "R3", "R4", "R5", "R6", "R7"});
    auto blocks = sf.read_blocks();
    CHECK(blocks.size() == 3); // 3+3+1
    CHECK(blocks[0].size() == 3);
    CHECK(blocks[2].size() == 1);
}

TEST_CASE("Sequential: DCB records_per_block calculation") {
    SeqDcb dcb;
    dcb.recfm = RecFm::FB;
    dcb.lrecl = 80;
    dcb.blksize = 800;
    CHECK(dcb.records_per_block() == 10);
}

TEST_CASE("Sequential: recfm_name strings") {
    CHECK(std::string(recfm_name(RecFm::F)) == "F");
    CHECK(std::string(recfm_name(RecFm::FB)) == "FB");
    CHECK(std::string(recfm_name(RecFm::V)) == "V");
    CHECK(std::string(recfm_name(RecFm::VB)) == "VB");
}

TEST_CASE("Sequential: empty file yields no records") {
    TempDir td("seq_empty");
    SeqDcb dcb;
    dcb.recfm = RecFm::FB;
    dcb.lrecl = 80;
    SequentialFile sf(td.path / "empty.dat", dcb);
    sf.write_records({});
    auto recs = sf.read_records();
    CHECK(recs.empty());
}

TEST_CASE("Sequential: RECFM=F single record per effective block") {
    SeqDcb dcb;
    dcb.recfm = RecFm::F;
    dcb.lrecl = 80;
    CHECK(dcb.records_per_block() == 1);
}

TEST_CASE("Sequential: default blksize auto-calc for FB") {
    SeqDcb dcb;
    dcb.recfm = RecFm::FB;
    dcb.lrecl = 80;
    dcb.blksize = 0;
    CHECK(dcb.effective_blksize() == 800);
}

// ============================================================================
// VSAM TESTS (20+)
// ============================================================================

TEST_CASE("VSAM KSDS: put and get") {
    TempDir td("vsam_ksds_pg");
    ClusterDef def;
    def.name = "CUST.KSDS";
    def.key_length = 6;
    KsdsFile ksds(td.path / "cust.ksds", def);
    CHECK(ksds.put("000001", "JOHN DOE, 123 MAIN ST") == VsamRc::OK);
    std::string rec;
    CHECK(ksds.get("000001", rec) == VsamRc::OK);
    CHECK(rec == "JOHN DOE, 123 MAIN ST");
}

TEST_CASE("VSAM KSDS: duplicate key rejected") {
    TempDir td("vsam_ksds_dup");
    ClusterDef def;
    def.name = "DUP.KSDS";
    def.unique = true;
    KsdsFile ksds(td.path / "dup.ksds", def);
    ksds.put("K1", "FIRST");
    CHECK(ksds.put("K1", "SECOND") == VsamRc::DUPLICATE_KEY);
}

TEST_CASE("VSAM KSDS: update existing record") {
    TempDir td("vsam_ksds_upd");
    ClusterDef def;
    def.name = "UPD.KSDS";
    KsdsFile ksds(td.path / "upd.ksds", def);
    ksds.put("K1", "OLD VALUE");
    CHECK(ksds.update("K1", "NEW VALUE") == VsamRc::OK);
    std::string rec;
    ksds.get("K1", rec);
    CHECK(rec == "NEW VALUE");
}

TEST_CASE("VSAM KSDS: update non-existent fails") {
    TempDir td("vsam_ksds_upd2");
    ClusterDef def;
    KsdsFile ksds(td.path / "upd2.ksds", def);
    CHECK(ksds.update("NOPE", "DATA") == VsamRc::NOT_FOUND);
}

TEST_CASE("VSAM KSDS: delete record") {
    TempDir td("vsam_ksds_del");
    ClusterDef def;
    KsdsFile ksds(td.path / "del.ksds", def);
    ksds.put("K1", "DATA");
    CHECK(ksds.erase("K1") == VsamRc::OK);
    CHECK(ksds.size() == 0);
    std::string rec;
    CHECK(ksds.get("K1", rec) == VsamRc::NOT_FOUND);
}

TEST_CASE("VSAM KSDS: delete non-existent") {
    TempDir td("vsam_ksds_del2");
    ClusterDef def;
    KsdsFile ksds(td.path / "del2.ksds", def);
    CHECK(ksds.erase("NOPE") == VsamRc::NOT_FOUND);
}

TEST_CASE("VSAM KSDS: browse forward with STARTBR/READNEXT") {
    TempDir td("vsam_ksds_br");
    ClusterDef def;
    KsdsFile ksds(td.path / "br.ksds", def);
    ksds.put("A", "REC_A");
    ksds.put("B", "REC_B");
    ksds.put("C", "REC_C");

    ksds.startbr("A");
    std::string key, rec;
    CHECK(ksds.readnext(key, rec) == VsamRc::OK);
    CHECK(key == "A");
    CHECK(ksds.readnext(key, rec) == VsamRc::OK);
    CHECK(key == "B");
    CHECK(ksds.readnext(key, rec) == VsamRc::OK);
    CHECK(key == "C");
    CHECK(ksds.readnext(key, rec) == VsamRc::END_OF_FILE);
    ksds.endbr();
}

TEST_CASE("VSAM KSDS: browse backward with READPREV") {
    TempDir td("vsam_ksds_brp");
    ClusterDef def;
    KsdsFile ksds(td.path / "brp.ksds", def);
    ksds.put("A", "1");
    ksds.put("B", "2");
    ksds.put("C", "3");

    ksds.startbr("C");
    std::string key, rec;
    CHECK(ksds.readprev(key, rec) == VsamRc::OK);
    CHECK(key == "C");
    CHECK(ksds.readprev(key, rec) == VsamRc::OK);
    CHECK(key == "B");
    ksds.endbr();
}

TEST_CASE("VSAM KSDS: point positions browse") {
    TempDir td("vsam_ksds_pt");
    ClusterDef def;
    KsdsFile ksds(td.path / "pt.ksds", def);
    ksds.put("A", "1");
    ksds.put("B", "2");
    ksds.put("C", "3");

    ksds.point("B");
    std::string key, rec;
    CHECK(ksds.readnext(key, rec) == VsamRc::OK);
    CHECK(key == "B");
    ksds.endbr();
}

TEST_CASE("VSAM KSDS: alternate index lookup") {
    TempDir td("vsam_ksds_alt");
    ClusterDef def;
    KsdsFile ksds(td.path / "alt.ksds", def);
    ksds.put("001", "SMITH:JOHN:NYC");
    ksds.put("002", "DOE:JANE:LA");
    ksds.put("003", "JONES:BOB:CHI");

    // Define alt index on last name (first field before colon)
    ksds.define_alt_index("LNAME", [](const std::string& r) -> std::string {
        auto pos = r.find(':');
        return pos != std::string::npos ? r.substr(0, pos) : "";
    });

    std::string key, rec;
    CHECK(ksds.get_by_alt("LNAME", "DOE", key, rec) == VsamRc::OK);
    CHECK(key == "002");
    CHECK(rec == "DOE:JANE:LA");
}

TEST_CASE("VSAM KSDS: alt index not found") {
    TempDir td("vsam_ksds_alt2");
    ClusterDef def;
    KsdsFile ksds(td.path / "alt2.ksds", def);
    ksds.put("001", "SMITH:JOHN");
    ksds.define_alt_index("LNAME", [](const std::string& r) {
        return r.substr(0, r.find(':'));
    });
    std::string key, rec;
    CHECK(ksds.get_by_alt("LNAME", "NOPE", key, rec) == VsamRc::NOT_FOUND);
}

TEST_CASE("VSAM KSDS: alt index on invalid name") {
    TempDir td("vsam_ksds_alt3");
    ClusterDef def;
    KsdsFile ksds(td.path / "alt3.ksds", def);
    std::string key, rec;
    CHECK(ksds.get_by_alt("NOINDEX", "X", key, rec) == VsamRc::INVALID_REQUEST);
}

TEST_CASE("VSAM KSDS: persistence across instances") {
    TempDir td("vsam_ksds_per");
    fs::path p = td.path / "per.ksds";
    {
        ClusterDef def;
        KsdsFile ksds(p, def);
        ksds.put("K1", "VAL1");
        ksds.put("K2", "VAL2");
    }
    {
        ClusterDef def;
        KsdsFile ksds(p, def);
        CHECK(ksds.size() == 2);
        std::string rec;
        CHECK(ksds.get("K1", rec) == VsamRc::OK);
        CHECK(rec == "VAL1");
    }
}

TEST_CASE("VSAM KSDS: size and empty") {
    TempDir td("vsam_ksds_sz");
    ClusterDef def;
    KsdsFile ksds(td.path / "sz.ksds", def);
    CHECK(ksds.empty());
    CHECK(ksds.size() == 0);
    ksds.put("A", "1");
    CHECK_FALSE(ksds.empty());
    CHECK(ksds.size() == 1);
}

TEST_CASE("VSAM ESDS: append and retrieve by RBA") {
    TempDir td("vsam_esds");
    ClusterDef def;
    def.name = "LOG.ESDS";
    EsdsFile esds(td.path / "log.esds", def);
    auto [rc1, rba1] = esds.put("FIRST ENTRY");
    auto [rc2, rba2] = esds.put("SECOND ENTRY");
    CHECK(rc1 == VsamRc::OK);
    CHECK(rc2 == VsamRc::OK);
    CHECK(rba1 != rba2);

    std::string rec;
    CHECK(esds.get(rba1, rec) == VsamRc::OK);
    CHECK(rec == "FIRST ENTRY");
    CHECK(esds.get(rba2, rec) == VsamRc::OK);
    CHECK(rec == "SECOND ENTRY");
}

TEST_CASE("VSAM ESDS: sequential browse") {
    TempDir td("vsam_esds_br");
    ClusterDef def;
    EsdsFile esds(td.path / "esbr.esds", def);
    esds.put("REC1");
    esds.put("REC2");
    esds.put("REC3");

    esds.startbr();
    uint64_t rba;
    std::string rec;
    CHECK(esds.readnext(rba, rec) == VsamRc::OK);
    CHECK(rec == "REC1");
    CHECK(esds.readnext(rba, rec) == VsamRc::OK);
    CHECK(rec == "REC2");
    CHECK(esds.readnext(rba, rec) == VsamRc::OK);
    CHECK(rec == "REC3");
    CHECK(esds.readnext(rba, rec) == VsamRc::END_OF_FILE);
    esds.endbr();
}

TEST_CASE("VSAM ESDS: invalid RBA returns NOT_FOUND") {
    TempDir td("vsam_esds_nf");
    ClusterDef def;
    EsdsFile esds(td.path / "nf.esds", def);
    esds.put("DATA");
    std::string rec;
    CHECK(esds.get(9999, rec) == VsamRc::NOT_FOUND);
}

TEST_CASE("VSAM RRDS: put and get by slot") {
    TempDir td("vsam_rrds");
    ClusterDef def;
    def.record_size = 20;
    RrdsFile rrds(td.path / "test.rrds", def);
    CHECK(rrds.put(1, "SLOT ONE") == VsamRc::OK);
    CHECK(rrds.put(5, "SLOT FIVE") == VsamRc::OK);

    std::string rec;
    CHECK(rrds.get(1, rec) == VsamRc::OK);
    CHECK(rec.substr(0, 8) == "SLOT ONE");
    CHECK(rrds.get(5, rec) == VsamRc::OK);
    CHECK(rec.substr(0, 9) == "SLOT FIVE");
}

TEST_CASE("VSAM RRDS: duplicate slot rejected") {
    TempDir td("vsam_rrds_dup");
    ClusterDef def;
    def.record_size = 10;
    RrdsFile rrds(td.path / "dup.rrds", def);
    rrds.put(1, "FIRST");
    CHECK(rrds.put(1, "AGAIN") == VsamRc::DUPLICATE_KEY);
}

TEST_CASE("VSAM RRDS: update slot") {
    TempDir td("vsam_rrds_upd");
    ClusterDef def;
    def.record_size = 20;
    RrdsFile rrds(td.path / "upd.rrds", def);
    rrds.put(3, "ORIGINAL");
    CHECK(rrds.update(3, "UPDATED") == VsamRc::OK);
    std::string rec;
    rrds.get(3, rec);
    CHECK(rec.substr(0, 7) == "UPDATED");
}

TEST_CASE("VSAM RRDS: delete slot") {
    TempDir td("vsam_rrds_del");
    ClusterDef def;
    def.record_size = 10;
    RrdsFile rrds(td.path / "del.rrds", def);
    rrds.put(1, "DATA");
    CHECK(rrds.erase(1) == VsamRc::OK);
    std::string rec;
    CHECK(rrds.get(1, rec) == VsamRc::NOT_FOUND);
    CHECK(rrds.size() == 0);
}

TEST_CASE("VSAM RRDS: slot 0 rejected") {
    TempDir td("vsam_rrds_s0");
    ClusterDef def;
    def.record_size = 10;
    RrdsFile rrds(td.path / "s0.rrds", def);
    CHECK(rrds.put(0, "BAD") == VsamRc::INVALID_REQUEST);
}

TEST_CASE("VSAM RRDS: browse skips empty slots") {
    TempDir td("vsam_rrds_br");
    ClusterDef def;
    def.record_size = 10;
    RrdsFile rrds(td.path / "br.rrds", def);
    rrds.put(1, "A");
    rrds.put(5, "E");
    rrds.put(10, "J");

    rrds.startbr();
    uint32_t slot;
    std::string rec;
    CHECK(rrds.readnext(slot, rec) == VsamRc::OK);
    CHECK(slot == 1);
    CHECK(rrds.readnext(slot, rec) == VsamRc::OK);
    CHECK(slot == 5);
    CHECK(rrds.readnext(slot, rec) == VsamRc::OK);
    CHECK(slot == 10);
    CHECK(rrds.readnext(slot, rec) == VsamRc::END_OF_FILE);
    rrds.endbr();
}

TEST_CASE("VSAM REPRO: KSDS to KSDS copy") {
    TempDir td("vsam_repro");
    ClusterDef def;
    KsdsFile src(td.path / "src.ksds", def);
    KsdsFile dst(td.path / "dst.ksds", def);
    src.put("A", "1");
    src.put("B", "2");
    src.put("C", "3");
    size_t copied = repro_ksds_to_ksds(src, dst);
    CHECK(copied == 3);
    CHECK(dst.size() == 3);
}

TEST_CASE("VSAM REPRO: KSDS to KSDS with filter") {
    TempDir td("vsam_repro_f");
    ClusterDef def;
    KsdsFile src(td.path / "src.ksds", def);
    KsdsFile dst(td.path / "dst.ksds", def);
    src.put("A", "1");
    src.put("B", "2");
    src.put("C", "3");
    size_t copied = repro_ksds_to_ksds(src, dst,
        [](const std::string& key, const std::string&) { return key >= "B"; });
    CHECK(copied == 2);
}

// ============================================================================
// PDS TESTS (15+)
// ============================================================================

TEST_CASE("PDS: create and write member") {
    TempDir td("pds_write");
    Pds pds(td.path / "MY.PDS");
    pds.write_member("MEMBER1", "CONTENT OF MEMBER 1");
    CHECK(pds.member_exists("MEMBER1"));
}

TEST_CASE("PDS: read member") {
    TempDir td("pds_read");
    Pds pds(td.path / "MY.PDS");
    pds.write_member("DATA", "HELLO WORLD");
    CHECK(pds.read_member("DATA") == "HELLO WORLD");
}

TEST_CASE("PDS: read non-existent member throws") {
    TempDir td("pds_read_nf");
    Pds pds(td.path / "MY.PDS");
    CHECK_THROWS(pds.read_member("NOPE"));
}

TEST_CASE("PDS: delete member") {
    TempDir td("pds_del");
    Pds pds(td.path / "MY.PDS");
    pds.write_member("TEMP", "DATA");
    CHECK(pds.delete_member("TEMP"));
    CHECK_FALSE(pds.member_exists("TEMP"));
}

TEST_CASE("PDS: delete non-existent returns false") {
    TempDir td("pds_del_nf");
    Pds pds(td.path / "MY.PDS");
    CHECK_FALSE(pds.delete_member("NOPE"));
}

TEST_CASE("PDS: rename member") {
    TempDir td("pds_rename");
    Pds pds(td.path / "MY.PDS");
    pds.write_member("OLD", "DATA");
    CHECK(pds.rename_member("OLD", "NEW"));
    CHECK_FALSE(pds.member_exists("OLD"));
    CHECK(pds.member_exists("NEW"));
    CHECK(pds.read_member("NEW") == "DATA");
}

TEST_CASE("PDS: rename to existing name fails") {
    TempDir td("pds_rename2");
    Pds pds(td.path / "MY.PDS");
    pds.write_member("A", "1");
    pds.write_member("B", "2");
    CHECK_FALSE(pds.rename_member("A", "B"));
}

TEST_CASE("PDS: list members sorted") {
    TempDir td("pds_list");
    Pds pds(td.path / "MY.PDS");
    pds.write_member("ZEBRA", "Z");
    pds.write_member("ALPHA", "A");
    pds.write_member("MANGO", "M");
    auto list = pds.list_members();
    CHECK(list.size() == 3);
    CHECK(list[0] == "ALPHA");
    CHECK(list[1] == "MANGO");
    CHECK(list[2] == "ZEBRA");
}

TEST_CASE("PDS: member count") {
    TempDir td("pds_count");
    Pds pds(td.path / "MY.PDS");
    CHECK(pds.member_count() == 0);
    pds.write_member("A", "1");
    pds.write_member("B", "2");
    CHECK(pds.member_count() == 2);
}

TEST_CASE("PDS: list with statistics") {
    TempDir td("pds_stats");
    Pds pds(td.path / "MY.PDS");
    pds.write_member("PROG1", "01234567890123456789");
    auto stats = pds.list_members_with_stats();
    REQUIRE(stats.size() == 1);
    CHECK(stats[0].name == "PROG1");
    CHECK(stats[0].size_bytes == 20);
}

TEST_CASE("PDS: pattern matching with wildcard *") {
    TempDir td("pds_pat_star");
    Pds pds(td.path / "MY.PDS");
    pds.write_member("COBRPT01", "1");
    pds.write_member("COBRPT02", "2");
    pds.write_member("PLIRPT01", "3");
    auto selected = pds.select_members("COB*");
    CHECK(selected.size() == 2);
}

TEST_CASE("PDS: pattern matching with ?") {
    TempDir td("pds_pat_q");
    Pds pds(td.path / "MY.PDS");
    pds.write_member("RPTA", "1");
    pds.write_member("RPTB", "2");
    pds.write_member("RPTCC", "3");
    auto selected = pds.select_members("RPT?");
    CHECK(selected.size() == 2);
    CHECK(selected[0] == "RPTA");
    CHECK(selected[1] == "RPTB");
}

TEST_CASE("PDS: merge from another PDS") {
    TempDir td("pds_merge");
    Pds src(td.path / "SRC.PDS");
    Pds dst(td.path / "DST.PDS");
    src.write_member("M1", "DATA1");
    src.write_member("M2", "DATA2");
    src.write_member("M3", "DATA3");

    auto result = dst.merge_from(src);
    CHECK(result.copied == 3);
    CHECK(result.skipped == 0);
    CHECK(dst.member_count() == 3);
    CHECK(dst.read_member("M2") == "DATA2");
}

TEST_CASE("PDS: merge skip existing") {
    TempDir td("pds_merge_skip");
    Pds src(td.path / "SRC.PDS");
    Pds dst(td.path / "DST.PDS");
    src.write_member("M1", "SRC_DATA");
    dst.write_member("M1", "DST_DATA");

    auto result = dst.merge_from(src, false);
    CHECK(result.skipped == 1);
    CHECK(result.copied == 0);
    CHECK(dst.read_member("M1") == "DST_DATA"); // not overwritten
}

TEST_CASE("PDS: merge replace existing") {
    TempDir td("pds_merge_repl");
    Pds src(td.path / "SRC.PDS");
    Pds dst(td.path / "DST.PDS");
    src.write_member("M1", "NEW DATA");
    dst.write_member("M1", "OLD DATA");

    auto result = dst.merge_from(src, true);
    CHECK(result.replaced == 1);
    CHECK(dst.read_member("M1") == "NEW DATA");
}

TEST_CASE("PDS: merge with pattern filter") {
    TempDir td("pds_merge_pat");
    Pds src(td.path / "SRC.PDS");
    Pds dst(td.path / "DST.PDS");
    src.write_member("COBPGM1", "1");
    src.write_member("COBPGM2", "2");
    src.write_member("PLIPGM1", "3");

    auto result = dst.merge_from(src, "COB*", false);
    CHECK(result.copied == 2);
    CHECK(dst.member_count() == 2);
}

TEST_CASE("PDS: invalid member name rejected") {
    TempDir td("pds_badname");
    Pds pds(td.path / "MY.PDS");
    CHECK_THROWS(pds.write_member("TOOLONGNAME", "DATA"));
    CHECK_THROWS(pds.write_member("", "DATA"));
    CHECK_THROWS(pds.write_member("BAD-NAM", "DATA"));
}

TEST_CASE("PDS: overwrite existing member") {
    TempDir td("pds_overwrite");
    Pds pds(td.path / "MY.PDS");
    pds.write_member("M1", "ORIGINAL");
    pds.write_member("M1", "UPDATED");
    CHECK(pds.read_member("M1") == "UPDATED");
    CHECK(pds.member_count() == 1);
}
