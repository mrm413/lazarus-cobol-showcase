#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

#include "include/lazarus/sort/ebcdic.h"
#include "include/lazarus/sort/parser.h"
#include "include/lazarus/sort/engine.h"
#include "include/lazarus/sort/icetool.h"
#include "include/lazarus/sort/translator.h"

#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdint>

using namespace lazarus::sort;

// Helper: build a fixed-length record from a string, padded with spaces
static std::string make_rec(const std::string& s, size_t len = 80) {
    std::string r = s;
    if (r.size() < len) r.resize(len, ' ');
    return r;
}

// Helper: encode a 4-byte big-endian unsigned integer
static std::string encode_bi32(uint32_t val) {
    std::string r(4, '\0');
    r[0] = static_cast<char>((val >> 24) & 0xFF);
    r[1] = static_cast<char>((val >> 16) & 0xFF);
    r[2] = static_cast<char>((val >> 8) & 0xFF);
    r[3] = static_cast<char>(val & 0xFF);
    return r;
}

// Helper: encode a 4-byte big-endian signed integer
static std::string encode_fi32(int32_t val) {
    uint32_t uv = static_cast<uint32_t>(val);
    return encode_bi32(uv);
}

// Helper: encode a packed decimal value into N bytes
static std::string encode_pd(int64_t val, int bytes) {
    bool neg = val < 0;
    if (neg) val = -val;
    int total_digits = bytes * 2 - 1;
    std::vector<int> digits(static_cast<size_t>(total_digits), 0);
    for (int i = total_digits - 1; i >= 0 && val > 0; --i) {
        digits[static_cast<size_t>(i)] = static_cast<int>(val % 10);
        val /= 10;
    }
    std::string r(static_cast<size_t>(bytes), '\0');
    size_t di = 0;
    for (int i = 0; i < bytes - 1; ++i) {
        r[static_cast<size_t>(i)] = static_cast<char>((digits[di] << 4) | digits[di + 1]);
        di += 2;
    }
    uint8_t sign = neg ? 0x0D : 0x0C;
    r[static_cast<size_t>(bytes - 1)] = static_cast<char>((digits[di] << 4) | sign);
    return r;
}

// Helper: encode a zoned decimal value into N bytes
static std::string encode_zd(int64_t val, int bytes) {
    bool neg = val < 0;
    if (neg) val = -val;
    std::string r(static_cast<size_t>(bytes), '\0');
    for (int i = bytes - 1; i >= 0; --i) {
        int digit = static_cast<int>(val % 10);
        val /= 10;
        uint8_t zone = 0xF0;
        if (i == bytes - 1) zone = neg ? 0xD0 : 0xC0;
        r[static_cast<size_t>(i)] = static_cast<char>(zone | static_cast<uint8_t>(digit));
    }
    return r;
}

// Helper: encode big-endian float
static std::string encode_fl32(float val) {
    uint32_t bits;
    std::memcpy(&bits, &val, sizeof(bits));
    return encode_bi32(bits);
}

// ================================================================
// PARSER TESTS (30+)
// ================================================================

TEST_CASE("Parser: parse SORT FIELDS single key CH ascending") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,10,CH,A)");
    CHECK(spec.sort_fields.size() == 1);
    CHECK(spec.sort_fields[0].position == 1);
    CHECK(spec.sort_fields[0].length == 10);
    CHECK(spec.sort_fields[0].format == parser::FieldFormat::CH);
    CHECK(spec.sort_fields[0].order == parser::SortOrder::ASC);
}

TEST_CASE("Parser: parse SORT FIELDS single key CH descending") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,10,CH,D)");
    CHECK(spec.sort_fields[0].order == parser::SortOrder::DESC);
}

TEST_CASE("Parser: parse SORT FIELDS multiple keys") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A,10,3,ZD,D)");
    CHECK(spec.sort_fields.size() == 2);
    CHECK(spec.sort_fields[0].format == parser::FieldFormat::CH);
    CHECK(spec.sort_fields[1].format == parser::FieldFormat::ZD);
    CHECK(spec.sort_fields[1].position == 10);
    CHECK(spec.sort_fields[1].length == 3);
    CHECK(spec.sort_fields[1].order == parser::SortOrder::DESC);
}

TEST_CASE("Parser: parse PD format sort field") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(5,4,PD,A)");
    CHECK(spec.sort_fields[0].format == parser::FieldFormat::PD);
}

TEST_CASE("Parser: parse BI format sort field") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,4,BI,A)");
    CHECK(spec.sort_fields[0].format == parser::FieldFormat::BI);
}

TEST_CASE("Parser: parse FI format sort field") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,4,FI,D)");
    CHECK(spec.sort_fields[0].format == parser::FieldFormat::FI);
    CHECK(spec.sort_fields[0].order == parser::SortOrder::DESC);
}

TEST_CASE("Parser: parse FL format sort field") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,4,FL,A)");
    CHECK(spec.sort_fields[0].format == parser::FieldFormat::FL);
}

TEST_CASE("Parser: parse AC format sort field") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,8,AC,A)");
    CHECK(spec.sort_fields[0].format == parser::FieldFormat::AC);
}

TEST_CASE("Parser: parse CSF format sort field") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,6,CSF,A)");
    CHECK(spec.sort_fields[0].format == parser::FieldFormat::CSF);
}

TEST_CASE("Parser: parse CSL format sort field") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,6,CSL,A)");
    CHECK(spec.sort_fields[0].format == parser::FieldFormat::CSL);
}

TEST_CASE("Parser: parse CST format sort field") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,6,CST,A)");
    CHECK(spec.sort_fields[0].format == parser::FieldFormat::CST);
}

TEST_CASE("Parser: parse MERGE FIELDS") {
    parser::SortParser p;
    auto spec = p.parse("MERGE FIELDS=(1,10,CH,A)");
    CHECK(spec.operation == parser::SortSpec::Operation::MERGE);
    CHECK(spec.sort_fields.size() == 1);
}

TEST_CASE("Parser: parse INCLUDE COND simple EQ") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,10,CH,A)\nINCLUDE COND=((5,3,CH,EQ,'ABC'))");
    CHECK(spec.include_cond.has_value());
    auto& sc = std::get<parser::SimpleCondition>(*spec.include_cond);
    CHECK(sc.position == 5);
    CHECK(sc.length == 3);
    CHECK(sc.op == parser::CondOp::EQ);
    CHECK(sc.value == "ABC");
}

TEST_CASE("Parser: parse INCLUDE COND NE operator") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A)\nINCLUDE COND=((1,3,CH,NE,'XYZ'))");
    auto& sc = std::get<parser::SimpleCondition>(*spec.include_cond);
    CHECK(sc.op == parser::CondOp::NE);
}

TEST_CASE("Parser: parse INCLUDE COND GT operator") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A)\nINCLUDE COND=((1,3,CH,GT,'AAA'))");
    auto& sc = std::get<parser::SimpleCondition>(*spec.include_cond);
    CHECK(sc.op == parser::CondOp::GT);
}

TEST_CASE("Parser: parse INCLUDE COND GE operator") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A)\nINCLUDE COND=((1,3,CH,GE,'BBB'))");
    auto& sc = std::get<parser::SimpleCondition>(*spec.include_cond);
    CHECK(sc.op == parser::CondOp::GE);
}

TEST_CASE("Parser: parse INCLUDE COND LT operator") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A)\nINCLUDE COND=((1,3,CH,LT,'ZZZ'))");
    auto& sc = std::get<parser::SimpleCondition>(*spec.include_cond);
    CHECK(sc.op == parser::CondOp::LT);
}

TEST_CASE("Parser: parse INCLUDE COND LE operator") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A)\nINCLUDE COND=((1,3,CH,LE,'MMM'))");
    auto& sc = std::get<parser::SimpleCondition>(*spec.include_cond);
    CHECK(sc.op == parser::CondOp::LE);
}

TEST_CASE("Parser: parse INCLUDE COND compound AND") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,10,CH,A)\nINCLUDE COND=((1,3,CH,EQ,'ABC') AND (5,2,CH,EQ,'XY'))");
    auto& cc = std::get<parser::CompoundCondition>(*spec.include_cond);
    CHECK(cc.logic == parser::LogicOp::AND);
    CHECK(cc.children.size() == 2);
}

TEST_CASE("Parser: parse INCLUDE COND compound OR") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,10,CH,A)\nINCLUDE COND=((1,3,CH,EQ,'AAA') OR (1,3,CH,EQ,'BBB'))");
    auto& cc = std::get<parser::CompoundCondition>(*spec.include_cond);
    CHECK(cc.logic == parser::LogicOp::OR);
    CHECK(cc.children.size() == 2);
}

TEST_CASE("Parser: parse OMIT COND") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,10,CH,A)\nOMIT COND=((3,2,CH,EQ,'XX'))");
    CHECK(spec.omit_cond.has_value());
}

TEST_CASE("Parser: parse INREC BUILD") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A)\nINREC BUILD=(1,5,10,3)");
    CHECK(spec.inrec.has_value());
    CHECK(spec.inrec->type == parser::RecordReformat::Type::BUILD);
    CHECK(spec.inrec->fields.size() == 2);
}

TEST_CASE("Parser: parse OUTREC BUILD with literal") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A)\nOUTREC BUILD=(1,5,'|',10,3)");
    CHECK(spec.outrec.has_value());
    CHECK(spec.outrec->fields.size() == 3);
    CHECK(spec.outrec->fields[1].literal == "|");
}

TEST_CASE("Parser: parse OUTREC OVERLAY") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A)\nOUTREC OVERLAY=(1,5)");
    CHECK(spec.outrec.has_value());
    CHECK(spec.outrec->type == parser::RecordReformat::Type::OVERLAY);
}

TEST_CASE("Parser: parse SUM FIELDS") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A)\nSUM FIELDS=(10,4,BI)");
    CHECK(spec.sum_fields.size() == 1);
    CHECK(spec.sum_fields[0].position == 10);
    CHECK(spec.sum_fields[0].length == 4);
    CHECK(spec.sum_fields[0].format == parser::FieldFormat::BI);
}

TEST_CASE("Parser: parse OPTION COPY") {
    parser::SortParser p;
    auto spec = p.parse("OPTION COPY");
    CHECK(spec.options.copy == true);
}

TEST_CASE("Parser: parse OPTION SKIPREC") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,10,CH,A)\nOPTION SKIPREC=5");
    CHECK(spec.options.skip_rec == 5);
}

TEST_CASE("Parser: parse OPTION STOPAFT") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,10,CH,A)\nOPTION STOPAFT=100");
    CHECK(spec.options.stop_aft == 100);
}

TEST_CASE("Parser: parse OPTION EQUALS") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,10,CH,A)\nOPTION EQUALS");
    CHECK(spec.options.equals == true);
}

TEST_CASE("Parser: parse OPTION NOEQUALS") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,10,CH,A)\nOPTION NOEQUALS");
    CHECK(spec.options.equals == false);
}

TEST_CASE("Parser: parse SORT FIELDS=COPY") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=COPY");
    CHECK(spec.options.copy == true);
}

TEST_CASE("Parser: parse three sort keys") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A,6,4,PD,D,15,2,BI,A)");
    CHECK(spec.sort_fields.size() == 3);
    CHECK(spec.sort_fields[2].format == parser::FieldFormat::BI);
}

TEST_CASE("Parser: format_to_string roundtrip all formats") {
    CHECK(parser::format_to_string(parser::FieldFormat::CH) == "CH");
    CHECK(parser::format_to_string(parser::FieldFormat::ZD) == "ZD");
    CHECK(parser::format_to_string(parser::FieldFormat::PD) == "PD");
    CHECK(parser::format_to_string(parser::FieldFormat::BI) == "BI");
    CHECK(parser::format_to_string(parser::FieldFormat::FI) == "FI");
    CHECK(parser::format_to_string(parser::FieldFormat::FL) == "FL");
    CHECK(parser::format_to_string(parser::FieldFormat::AC) == "AC");
    CHECK(parser::format_to_string(parser::FieldFormat::CSF) == "CSF");
    CHECK(parser::format_to_string(parser::FieldFormat::CSL) == "CSL");
    CHECK(parser::format_to_string(parser::FieldFormat::CST) == "CST");
}

TEST_CASE("Parser: SUM FIELDS=NONE (dedup only)") {
    parser::SortParser p;
    auto spec = p.parse("SORT FIELDS=(1,5,CH,A)\nSUM FIELDS=NONE");
    CHECK(spec.sum_fields.empty());
}

// ================================================================
// ENGINE TESTS (35+)
// ================================================================

TEST_CASE("Engine: sort CH ascending") {
    std::vector<engine::Record> recs = {
        make_rec("Charlie"), make_rec("Alpha"), make_rec("Bravo")
    };
    auto result = engine::sort_records("SORT FIELDS=(1,7,CH,A)", recs);
    CHECK(result.size() == 3);
    CHECK(result[0].substr(0, 5) == "Alpha");
    CHECK(result[1].substr(0, 5) == "Bravo");
    CHECK(result[2].substr(0, 7) == "Charlie");
}

TEST_CASE("Engine: sort CH descending") {
    std::vector<engine::Record> recs = {
        make_rec("Alpha"), make_rec("Charlie"), make_rec("Bravo")
    };
    auto result = engine::sort_records("SORT FIELDS=(1,7,CH,D)", recs);
    CHECK(result[0].substr(0, 7) == "Charlie");
    CHECK(result[2].substr(0, 5) == "Alpha");
}

TEST_CASE("Engine: sort BI ascending") {
    std::string r1 = encode_bi32(300) + std::string(76, ' ');
    std::string r2 = encode_bi32(100) + std::string(76, ' ');
    std::string r3 = encode_bi32(200) + std::string(76, ' ');
    std::vector<engine::Record> recs = {r1, r2, r3};
    auto result = engine::sort_records("SORT FIELDS=(1,4,BI,A)", recs);
    CHECK(engine::decode_bi(std::string_view(result[0]).substr(0, 4)) == 100);
    CHECK(engine::decode_bi(std::string_view(result[1]).substr(0, 4)) == 200);
    CHECK(engine::decode_bi(std::string_view(result[2]).substr(0, 4)) == 300);
}

TEST_CASE("Engine: sort FI signed ascending") {
    std::string r1 = encode_fi32(50) + std::string(76, ' ');
    std::string r2 = encode_fi32(-10) + std::string(76, ' ');
    std::string r3 = encode_fi32(20) + std::string(76, ' ');
    std::vector<engine::Record> recs = {r1, r2, r3};
    auto result = engine::sort_records("SORT FIELDS=(1,4,FI,A)", recs);
    CHECK(engine::decode_fi(std::string_view(result[0]).substr(0, 4)) == -10);
    CHECK(engine::decode_fi(std::string_view(result[1]).substr(0, 4)) == 20);
    CHECK(engine::decode_fi(std::string_view(result[2]).substr(0, 4)) == 50);
}

TEST_CASE("Engine: decode_zd positive") {
    auto zd = encode_zd(12345, 5);
    CHECK(engine::decode_zd(zd) == 12345);
}

TEST_CASE("Engine: decode_zd negative") {
    auto zd = encode_zd(-9876, 5);
    CHECK(engine::decode_zd(zd) == -9876);
}

TEST_CASE("Engine: sort ZD ascending") {
    auto r1 = encode_zd(300, 5) + std::string(75, ' ');
    auto r2 = encode_zd(100, 5) + std::string(75, ' ');
    auto r3 = encode_zd(200, 5) + std::string(75, ' ');
    std::vector<engine::Record> recs = {r1, r2, r3};
    auto result = engine::sort_records("SORT FIELDS=(1,5,ZD,A)", recs);
    CHECK(engine::decode_zd(std::string_view(result[0]).substr(0, 5)) == 100);
    CHECK(engine::decode_zd(std::string_view(result[2]).substr(0, 5)) == 300);
}

TEST_CASE("Engine: decode_pd positive") {
    auto pd = encode_pd(12345, 3);
    CHECK(engine::decode_pd(pd) == 12345);
}

TEST_CASE("Engine: decode_pd negative") {
    auto pd = encode_pd(-9876, 3);
    CHECK(engine::decode_pd(pd) == -9876);
}

TEST_CASE("Engine: sort PD ascending") {
    auto r1 = encode_pd(500, 3) + std::string(77, ' ');
    auto r2 = encode_pd(100, 3) + std::string(77, ' ');
    auto r3 = encode_pd(300, 3) + std::string(77, ' ');
    std::vector<engine::Record> recs = {r1, r2, r3};
    auto result = engine::sort_records("SORT FIELDS=(1,3,PD,A)", recs);
    CHECK(engine::decode_pd(std::string_view(result[0]).substr(0, 3)) == 100);
    CHECK(engine::decode_pd(std::string_view(result[2]).substr(0, 3)) == 500);
}

TEST_CASE("Engine: decode_bi") {
    CHECK(engine::decode_bi(encode_bi32(0)) == 0);
    CHECK(engine::decode_bi(encode_bi32(1)) == 1);
    CHECK(engine::decode_bi(encode_bi32(0xFFFFFFFF)) == 0xFFFFFFFF);
}

TEST_CASE("Engine: decode_fi signed") {
    CHECK(engine::decode_fi(encode_fi32(0)) == 0);
    CHECK(engine::decode_fi(encode_fi32(127)) == 127);
    CHECK(engine::decode_fi(encode_fi32(-1)) == -1);
    CHECK(engine::decode_fi(encode_fi32(-128)) == -128);
}

TEST_CASE("Engine: decode_fl float32") {
    auto r = encode_fl32(3.14f);
    auto val = engine::decode_fl(r);
    CHECK(val == doctest::Approx(3.14).epsilon(0.01));
}

TEST_CASE("Engine: decode_csf positive") {
    CHECK(engine::decode_csf("+12345") == 12345);
}

TEST_CASE("Engine: decode_csf negative") {
    CHECK(engine::decode_csf("-500") == -500);
}

TEST_CASE("Engine: decode_cst trailing sign") {
    CHECK(engine::decode_cst("500-") == -500);
    CHECK(engine::decode_cst("123+") == 123);
}

TEST_CASE("Engine: multi-key sort CH+BI") {
    auto r1 = std::string("AAA") + encode_bi32(200) + std::string(73, ' ');
    auto r2 = std::string("AAA") + encode_bi32(100) + std::string(73, ' ');
    auto r3 = std::string("BBB") + encode_bi32(50) + std::string(73, ' ');
    std::vector<engine::Record> recs = {r1, r2, r3};
    auto result = engine::sort_records("SORT FIELDS=(1,3,CH,A,4,4,BI,A)", recs);
    CHECK(result[0].substr(0, 3) == "AAA");
    CHECK(engine::decode_bi(std::string_view(result[0]).substr(3, 4)) == 100);
    CHECK(result[1].substr(0, 3) == "AAA");
    CHECK(engine::decode_bi(std::string_view(result[1]).substr(3, 4)) == 200);
    CHECK(result[2].substr(0, 3) == "BBB");
}

TEST_CASE("Engine: INCLUDE filter EQ") {
    std::vector<engine::Record> recs = {
        make_rec("AAAHELLO"), make_rec("BBBWORLD"), make_rec("AAAGOODBYE")
    };
    auto result = engine::sort_records(
        "SORT FIELDS=(1,10,CH,A)\nINCLUDE COND=((1,3,CH,EQ,'AAA'))", recs);
    CHECK(result.size() == 2);
    CHECK(result[0].substr(3, 7) == "GOODBYE");
    CHECK(result[1].substr(3, 5) == "HELLO");
}

TEST_CASE("Engine: OMIT filter") {
    std::vector<engine::Record> recs = {
        make_rec("AAAHELLO"), make_rec("BBBWORLD"), make_rec("CCCFOO")
    };
    auto result = engine::sort_records(
        "SORT FIELDS=(1,10,CH,A)\nOMIT COND=((1,3,CH,EQ,'BBB'))", recs);
    CHECK(result.size() == 2);
}

TEST_CASE("Engine: INCLUDE compound AND filter") {
    std::vector<engine::Record> recs = {
        make_rec("AAXYDATA1"), make_rec("AAXXDATA2"), make_rec("BBXYDATA3")
    };
    auto result = engine::sort_records(
        "SORT FIELDS=(1,9,CH,A)\nINCLUDE COND=((1,2,CH,EQ,'AA') AND (3,2,CH,EQ,'XY'))", recs);
    CHECK(result.size() == 1);
    CHECK(result[0].substr(0, 9) == "AAXYDATA1");
}

TEST_CASE("Engine: INCLUDE compound OR filter") {
    std::vector<engine::Record> recs = {
        make_rec("AAADATA"), make_rec("BBBDATA"), make_rec("CCCDATA")
    };
    auto result = engine::sort_records(
        "SORT FIELDS=(1,7,CH,A)\nINCLUDE COND=((1,3,CH,EQ,'AAA') OR (1,3,CH,EQ,'CCC'))", recs);
    CHECK(result.size() == 2);
}

TEST_CASE("Engine: INREC BUILD reformat") {
    std::vector<engine::Record> recs = {
        make_rec("ABCDEfghij")
    };
    auto result = engine::sort_records(
        "SORT FIELDS=(1,5,CH,A)\nINREC BUILD=(1,5,6,5)", recs);
    CHECK(result.size() == 1);
    CHECK(result[0].substr(0, 5) == "ABCDE");
    CHECK(result[0].substr(5, 5) == "fghij");
}

TEST_CASE("Engine: OUTREC BUILD with literal insertion") {
    std::vector<engine::Record> recs = {
        make_rec("HELLO WORLD")
    };
    auto result = engine::sort_records(
        "SORT FIELDS=(1,11,CH,A)\nOUTREC BUILD=(1,5,'|',7,5)", recs);
    CHECK(result.size() == 1);
    CHECK(result[0] == "HELLO|WORLD");
}

TEST_CASE("Engine: SKIPREC option") {
    std::vector<engine::Record> recs = {
        make_rec("A"), make_rec("B"), make_rec("C"), make_rec("D"), make_rec("E")
    };
    auto result = engine::sort_records(
        "SORT FIELDS=(1,1,CH,A)\nOPTION SKIPREC=2", recs);
    CHECK(result.size() == 3);
}

TEST_CASE("Engine: STOPAFT option") {
    std::vector<engine::Record> recs = {
        make_rec("A"), make_rec("B"), make_rec("C"), make_rec("D"), make_rec("E")
    };
    auto result = engine::sort_records(
        "SORT FIELDS=(1,1,CH,A)\nOPTION STOPAFT=3", recs);
    CHECK(result.size() == 3);
}

TEST_CASE("Engine: OPTION COPY passthrough") {
    std::vector<engine::Record> recs = {
        make_rec("Charlie"), make_rec("Alpha"), make_rec("Bravo")
    };
    auto result = engine::sort_records("SORT FIELDS=COPY", recs);
    CHECK(result.size() == 3);
    CHECK(result[0].substr(0, 7) == "Charlie");
    CHECK(result[2].substr(0, 5) == "Bravo");
}

TEST_CASE("Engine: SUM FIELDS binary aggregation") {
    // Records: key=AAA, value at pos 4 (4-byte BI)
    auto r1 = std::string("AAA") + encode_bi32(10) + std::string(73, ' ');
    auto r2 = std::string("AAA") + encode_bi32(20) + std::string(73, ' ');
    auto r3 = std::string("BBB") + encode_bi32(5) + std::string(73, ' ');
    std::vector<engine::Record> recs = {r1, r2, r3};
    auto result = engine::sort_records(
        "SORT FIELDS=(1,3,CH,A)\nSUM FIELDS=(4,4,BI)", recs);
    CHECK(result.size() == 2);
    CHECK(engine::decode_bi(std::string_view(result[0]).substr(3, 4)) == 30);
    CHECK(engine::decode_bi(std::string_view(result[1]).substr(3, 4)) == 5);
}

TEST_CASE("Engine: MERGE two sorted inputs") {
    parser::SortParser p;
    auto spec = p.parse("MERGE FIELDS=(1,3,CH,A)");
    engine::SortEngine eng(spec);
    std::vector<std::vector<engine::Record>> inputs = {
        {make_rec("AAA"), make_rec("CCC"), make_rec("EEE")},
        {make_rec("BBB"), make_rec("DDD"), make_rec("FFF")}
    };
    auto result = eng.execute_merge(inputs);
    CHECK(result.size() == 6);
    CHECK(result[0].substr(0, 3) == "AAA");
    CHECK(result[1].substr(0, 3) == "BBB");
    CHECK(result[5].substr(0, 3) == "FFF");
}

TEST_CASE("Engine: MERGE three sorted inputs") {
    parser::SortParser p;
    auto spec = p.parse("MERGE FIELDS=(1,1,CH,A)");
    engine::SortEngine eng(spec);
    std::vector<std::vector<engine::Record>> inputs = {
        {make_rec("A"), make_rec("D"), make_rec("G")},
        {make_rec("B"), make_rec("E"), make_rec("H")},
        {make_rec("C"), make_rec("F"), make_rec("I")}
    };
    auto result = eng.execute_merge(inputs);
    CHECK(result.size() == 9);
    for (int i = 0; i < 9; ++i) {
        CHECK(result[static_cast<size_t>(i)][0] == 'A' + i);
    }
}

TEST_CASE("Engine: extract_field boundary check") {
    auto rec = make_rec("ABCDE", 5);
    CHECK(engine::extract_field(rec, 1, 3) == "ABC");
    CHECK(engine::extract_field(rec, 4, 5) == "DE");  // Truncated
    CHECK(engine::extract_field(rec, 6, 2) == "");     // Past end
}

TEST_CASE("Engine: compare_ch equal strings") {
    CHECK(engine::compare_ch("ABC", "ABC") == 0);
}

TEST_CASE("Engine: compare_ch ordering") {
    CHECK(engine::compare_ch("ABC", "ABD") < 0);
    CHECK(engine::compare_ch("XYZ", "ABC") > 0);
}

TEST_CASE("Engine: RecordComparator equal records") {
    parser::SortField f{1, 5, parser::FieldFormat::CH, parser::SortOrder::ASC};
    engine::RecordComparator cmp({f});
    auto r1 = make_rec("HELLO");
    auto r2 = make_rec("HELLO");
    CHECK(cmp.compare(r1, r2) == 0);
}

TEST_CASE("Engine: empty record sort") {
    std::vector<engine::Record> recs;
    auto result = engine::sort_records("SORT FIELDS=(1,5,CH,A)", recs);
    CHECK(result.empty());
}

TEST_CASE("Engine: single record sort") {
    std::vector<engine::Record> recs = {make_rec("ONLY")};
    auto result = engine::sort_records("SORT FIELDS=(1,4,CH,A)", recs);
    CHECK(result.size() == 1);
}

TEST_CASE("Engine: sort AC format ascending") {
    std::vector<engine::Record> recs = {
        make_rec("Zebra"), make_rec("Apple"), make_rec("Mango")
    };
    auto result = engine::sort_records("SORT FIELDS=(1,5,AC,A)", recs);
    CHECK(result[0].substr(0, 5) == "Apple");
    CHECK(result[2].substr(0, 5) == "Zebra");
}

TEST_CASE("Engine: SUM with ZD fields") {
    auto r1 = std::string("AAA") + encode_zd(100, 5) + std::string(72, ' ');
    auto r2 = std::string("AAA") + encode_zd(250, 5) + std::string(72, ' ');
    auto r3 = std::string("BBB") + encode_zd(50, 5) + std::string(72, ' ');
    std::vector<engine::Record> recs = {r1, r2, r3};
    auto result = engine::sort_records(
        "SORT FIELDS=(1,3,CH,A)\nSUM FIELDS=(4,5,ZD)", recs);
    CHECK(result.size() == 2);
    CHECK(engine::decode_zd(std::string_view(result[0]).substr(3, 5)) == 350);
}

// ================================================================
// ICETOOL TESTS (35+)
// ================================================================

TEST_CASE("Icetool: COPY operation") {
    icetool::IcetoolExecutor exec;
    icetool::Operation op = icetool::Copy{};
    std::vector<engine::Record> recs = {make_rec("A"), make_rec("B")};
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 2);
}

TEST_CASE("Icetool: COUNT operation") {
    icetool::IcetoolExecutor exec;
    icetool::Count count_op;
    icetool::Operation op = count_op;
    std::vector<engine::Record> recs = {make_rec("A"), make_rec("B"), make_rec("C")};
    auto result = exec.execute(op, recs);
    CHECK(std::get<icetool::Count>(op).result == 3);
}

TEST_CASE("Icetool: DISPLAY min/max") {
    icetool::IcetoolExecutor exec;
    icetool::Display disp;
    disp.position = 1;
    disp.length = 3;
    disp.format = parser::FieldFormat::CH;
    icetool::Operation op = disp;
    std::vector<engine::Record> recs = {make_rec("BBB"), make_rec("AAA"), make_rec("CCC")};
    auto result = exec.execute(op, recs);
    auto& d = std::get<icetool::Display>(op);
    CHECK(d.result.min_val == "AAA");
    CHECK(d.result.max_val == "CCC");
    CHECK(d.result.count == 3);
}

TEST_CASE("Icetool: MODE operation") {
    icetool::IcetoolExecutor exec;
    icetool::Mode mode_op;
    mode_op.position = 1;
    mode_op.length = 3;
    mode_op.format = parser::FieldFormat::CH;
    icetool::Operation op = mode_op;
    std::vector<engine::Record> recs = {
        make_rec("AAA"), make_rec("BBB"), make_rec("AAA"),
        make_rec("CCC"), make_rec("AAA")
    };
    auto result = exec.execute(op, recs);
    auto& m = std::get<icetool::Mode>(op);
    CHECK(m.result.mode_val == "AAA");
    CHECK(m.result.frequency == 3);
}

TEST_CASE("Icetool: OCCUR operation") {
    icetool::IcetoolExecutor exec;
    icetool::Occur occur_op;
    occur_op.position = 1;
    occur_op.length = 1;
    occur_op.format = parser::FieldFormat::CH;
    icetool::Operation op = occur_op;
    std::vector<engine::Record> recs = {
        make_rec("A"), make_rec("B"), make_rec("A"),
        make_rec("C"), make_rec("B"), make_rec("A")
    };
    auto result = exec.execute(op, recs);
    auto& o = std::get<icetool::Occur>(op);
    CHECK(o.result.size() == 3);
    bool found_a = false;
    for (const auto& e : o.result) {
        if (e.value[0] == 'A') { CHECK(e.count == 3); found_a = true; }
    }
    CHECK(found_a);
}

TEST_CASE("Icetool: RANGE operation") {
    icetool::IcetoolExecutor exec;
    icetool::Range range_op;
    range_op.position = 1;
    range_op.length = 3;
    range_op.format = parser::FieldFormat::CH;
    range_op.low = "BBB";
    range_op.high = "DDD";
    icetool::Operation op = range_op;
    std::vector<engine::Record> recs = {
        make_rec("AAA"), make_rec("BBB"), make_rec("CCC"),
        make_rec("DDD"), make_rec("EEE")
    };
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 3);
}

TEST_CASE("Icetool: SELECT EQ operation") {
    icetool::IcetoolExecutor exec;
    icetool::Select sel;
    sel.position = 1;
    sel.length = 3;
    sel.format = parser::FieldFormat::CH;
    sel.op = parser::CondOp::EQ;
    sel.value = "AAA";
    icetool::Operation op = sel;
    std::vector<engine::Record> recs = {
        make_rec("AAA"), make_rec("BBB"), make_rec("AAA"), make_rec("CCC")
    };
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 2);
}

TEST_CASE("Icetool: SELECT GT operation") {
    icetool::IcetoolExecutor exec;
    icetool::Select sel;
    sel.position = 1;
    sel.length = 3;
    sel.format = parser::FieldFormat::CH;
    sel.op = parser::CondOp::GT;
    sel.value = "BBB";
    icetool::Operation op = sel;
    std::vector<engine::Record> recs = {
        make_rec("AAA"), make_rec("BBB"), make_rec("CCC"), make_rec("DDD")
    };
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 2);
}

TEST_CASE("Icetool: SELECT LT operation") {
    icetool::IcetoolExecutor exec;
    icetool::Select sel;
    sel.position = 1;
    sel.length = 3;
    sel.format = parser::FieldFormat::CH;
    sel.op = parser::CondOp::LT;
    sel.value = "CCC";
    icetool::Operation op = sel;
    std::vector<engine::Record> recs = {
        make_rec("AAA"), make_rec("BBB"), make_rec("CCC"), make_rec("DDD")
    };
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 2);
}

TEST_CASE("Icetool: SELECT NE operation") {
    icetool::IcetoolExecutor exec;
    icetool::Select sel;
    sel.position = 1;
    sel.length = 3;
    sel.format = parser::FieldFormat::CH;
    sel.op = parser::CondOp::NE;
    sel.value = "BBB";
    icetool::Operation op = sel;
    std::vector<engine::Record> recs = {
        make_rec("AAA"), make_rec("BBB"), make_rec("CCC")
    };
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 2);
}

TEST_CASE("Icetool: SELECT SS substring operation") {
    icetool::IcetoolExecutor exec;
    icetool::Select sel;
    sel.position = 1;
    sel.length = 10;
    sel.format = parser::FieldFormat::CH;
    sel.op = parser::CondOp::SS;
    sel.value = "ELL";
    icetool::Operation op = sel;
    std::vector<engine::Record> recs = {
        make_rec("HELLO WRLD"), make_rec("GOODBYE   "), make_rec("JELLY BEAN")
    };
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 2);
}

TEST_CASE("Icetool: SORT operation") {
    icetool::IcetoolExecutor exec;
    icetool::Sort sort_op;
    sort_op.fields = {{1, 3, parser::FieldFormat::CH, parser::SortOrder::ASC}};
    icetool::Operation op = sort_op;
    std::vector<engine::Record> recs = {make_rec("CCC"), make_rec("AAA"), make_rec("BBB")};
    auto result = exec.execute(op, recs);
    CHECK(result[0].substr(0, 3) == "AAA");
    CHECK(result[2].substr(0, 3) == "CCC");
}

TEST_CASE("Icetool: STATS operation") {
    icetool::IcetoolExecutor exec;
    icetool::Stats stats_op;
    stats_op.position = 1;
    stats_op.length = 4;
    stats_op.format = parser::FieldFormat::BI;
    icetool::Operation op = stats_op;
    std::vector<engine::Record> recs;
    for (int v : {10, 20, 30, 40, 50}) {
        recs.push_back(encode_bi32(static_cast<uint32_t>(v)) + std::string(76, ' '));
    }
    auto result = exec.execute(op, recs);
    auto& s = std::get<icetool::Stats>(op);
    CHECK(s.result.count == 5);
    CHECK(s.result.sum == 150);
    CHECK(s.result.mean == doctest::Approx(30.0));
    CHECK(s.result.min_val == 10);
    CHECK(s.result.max_val == 50);
    CHECK(s.result.std_dev > 0.0);
}

TEST_CASE("Icetool: UNIQUE operation") {
    icetool::IcetoolExecutor exec;
    icetool::Unique uniq;
    uniq.position = 1;
    uniq.length = 3;
    uniq.format = parser::FieldFormat::CH;
    icetool::Operation op = uniq;
    std::vector<engine::Record> recs = {
        make_rec("AAA"), make_rec("BBB"), make_rec("AAA"),
        make_rec("CCC"), make_rec("BBB")
    };
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 3);
}

TEST_CASE("Icetool: VERIFY valid ZD") {
    icetool::IcetoolExecutor exec;
    icetool::Verify ver;
    ver.position = 1;
    ver.length = 3;
    ver.format = parser::FieldFormat::ZD;
    icetool::Operation op = ver;
    std::vector<engine::Record> recs = {
        encode_zd(123, 3) + std::string(77, ' '),
        encode_zd(456, 3) + std::string(77, ' ')
    };
    auto result = exec.execute(op, recs);
    CHECK(std::get<icetool::Verify>(op).result.valid);
    CHECK(std::get<icetool::Verify>(op).result.error_count == 0);
}

TEST_CASE("Icetool: VERIFY invalid ZD") {
    icetool::IcetoolExecutor exec;
    icetool::Verify ver;
    ver.position = 1;
    ver.length = 3;
    ver.format = parser::FieldFormat::ZD;
    icetool::Operation op = ver;
    // 'ABC' is not valid zoned decimal
    std::vector<engine::Record> recs = {make_rec("ABC")};
    auto result = exec.execute(op, recs);
    CHECK_FALSE(std::get<icetool::Verify>(op).result.valid);
    CHECK(std::get<icetool::Verify>(op).result.error_count == 1);
}

TEST_CASE("Icetool: VERIFY valid PD") {
    icetool::IcetoolExecutor exec;
    icetool::Verify ver;
    ver.position = 1;
    ver.length = 3;
    ver.format = parser::FieldFormat::PD;
    icetool::Operation op = ver;
    std::vector<engine::Record> recs = {
        encode_pd(12345, 3) + std::string(77, ' ')
    };
    auto result = exec.execute(op, recs);
    CHECK(std::get<icetool::Verify>(op).result.valid);
}

TEST_CASE("Icetool: OUTFIL conditional routing") {
    icetool::Outfil outfil;
    parser::SimpleCondition cond_a;
    cond_a.position = 1; cond_a.length = 1; cond_a.format = parser::FieldFormat::CH;
    cond_a.op = parser::CondOp::EQ; cond_a.value = "A";
    outfil.routes.push_back({cond_a, "OUTPUT_A"});

    parser::SimpleCondition cond_b;
    cond_b.position = 1; cond_b.length = 1; cond_b.format = parser::FieldFormat::CH;
    cond_b.op = parser::CondOp::EQ; cond_b.value = "B";
    outfil.routes.push_back({cond_b, "OUTPUT_B"});

    std::vector<engine::Record> recs = {
        make_rec("AONE"), make_rec("BTWO"), make_rec("ATHREE"), make_rec("BFOUR")
    };
    auto routed = outfil.route(recs);
    CHECK(routed["OUTPUT_A"].size() == 2);
    CHECK(routed["OUTPUT_B"].size() == 2);
}

TEST_CASE("Icetool: OUTFIL routing with no match") {
    icetool::Outfil outfil;
    parser::SimpleCondition cond;
    cond.position = 1; cond.length = 1; cond.format = parser::FieldFormat::CH;
    cond.op = parser::CondOp::EQ; cond.value = "X";
    outfil.routes.push_back({cond, "OUTPUT_X"});

    std::vector<engine::Record> recs = {make_rec("ADATA"), make_rec("BDATA")};
    auto routed = outfil.route(recs);
    CHECK(routed["OUTPUT_X"].empty());
}

TEST_CASE("Icetool: SYMNAMES define and lookup") {
    icetool::SymNames sym;
    sym.define("CUSTID", 1, 8, parser::FieldFormat::CH);
    sym.define("BALANCE", 10, 4, parser::FieldFormat::BI);

    CHECK(sym.has("CUSTID"));
    CHECK(sym.has("BALANCE"));
    CHECK_FALSE(sym.has("FOOBAR"));

    auto& cust = sym.get("CUSTID");
    CHECK(cust.position == 1);
    CHECK(cust.length == 8);
}

TEST_CASE("Icetool: SYMNAMES extract field") {
    icetool::SymNames sym;
    sym.define("NAME", 1, 5, parser::FieldFormat::CH);

    auto rec = make_rec("HELLO WORLD");
    auto val = sym.extract("NAME", rec);
    CHECK(val == "HELLO");
}

TEST_CASE("Icetool: SYMNAMES to_sort_field") {
    icetool::SymNames sym;
    sym.define("DEPT", 5, 3, parser::FieldFormat::CH);
    auto sf = sym.to_sort_field("DEPT", parser::SortOrder::DESC);
    CHECK(sf.position == 5);
    CHECK(sf.length == 3);
    CHECK(sf.order == parser::SortOrder::DESC);
}

TEST_CASE("Icetool: SYMNAMES size and names") {
    icetool::SymNames sym;
    sym.define("A", 1, 1);
    sym.define("B", 2, 1);
    sym.define("C", 3, 1);
    CHECK(sym.size() == 3);
    auto names = sym.names();
    CHECK(names.size() == 3);
}

TEST_CASE("Icetool: IFTHEN conditional reformat") {
    icetool::IfthenProcessor proc;

    parser::SimpleCondition cond_a;
    cond_a.position = 1; cond_a.length = 1; cond_a.format = parser::FieldFormat::CH;
    cond_a.op = parser::CondOp::EQ; cond_a.value = "A";

    icetool::IfthenClause clause;
    clause.type = icetool::IfthenClause::Type::BUILD;
    clause.when_cond = cond_a;
    clause.fields = {{1, 5, ""}, {0, 0, "-TYPE-A"}};
    proc.add_clause(clause);

    auto r1 = make_rec("AHELLO");
    auto result = proc.apply(r1);
    CHECK(result == "AHELL-TYPE-A");
}

TEST_CASE("Icetool: IFTHEN no matching clause returns original") {
    icetool::IfthenProcessor proc;

    parser::SimpleCondition cond;
    cond.position = 1; cond.length = 1; cond.format = parser::FieldFormat::CH;
    cond.op = parser::CondOp::EQ; cond.value = "X";

    icetool::IfthenClause clause;
    clause.type = icetool::IfthenClause::Type::BUILD;
    clause.when_cond = cond;
    clause.fields = {{1, 3, ""}};
    proc.add_clause(clause);

    auto rec = make_rec("AHELLO");
    auto result = proc.apply(rec);
    CHECK(result == rec);
}

TEST_CASE("Icetool: IFTHEN multiple clauses first match wins") {
    icetool::IfthenProcessor proc;

    parser::SimpleCondition cond1;
    cond1.position = 1; cond1.length = 1; cond1.format = parser::FieldFormat::CH;
    cond1.op = parser::CondOp::EQ; cond1.value = "A";
    icetool::IfthenClause c1;
    c1.type = icetool::IfthenClause::Type::BUILD;
    c1.when_cond = cond1;
    c1.fields = {{0, 0, "FIRST"}};
    proc.add_clause(c1);

    parser::SimpleCondition cond2;
    cond2.position = 1; cond2.length = 1; cond2.format = parser::FieldFormat::CH;
    cond2.op = parser::CondOp::EQ; cond2.value = "A";
    icetool::IfthenClause c2;
    c2.type = icetool::IfthenClause::Type::BUILD;
    c2.when_cond = cond2;
    c2.fields = {{0, 0, "SECOND"}};
    proc.add_clause(c2);

    auto rec = make_rec("AHELLO");
    auto result = proc.apply(rec);
    CHECK(result == "FIRST");
}

TEST_CASE("Icetool: empty records handle gracefully") {
    icetool::IcetoolExecutor exec;
    icetool::Count count_op;
    icetool::Operation op = count_op;
    std::vector<engine::Record> recs;
    auto result = exec.execute(op, recs);
    CHECK(std::get<icetool::Count>(op).result == 0);
}

TEST_CASE("Icetool: STATS standard deviation calculation") {
    icetool::IcetoolExecutor exec;
    icetool::Stats stats_op;
    stats_op.position = 1;
    stats_op.length = 4;
    stats_op.format = parser::FieldFormat::BI;
    icetool::Operation op = stats_op;
    // All same value: std_dev should be 0
    std::vector<engine::Record> recs;
    for (int i = 0; i < 5; ++i) {
        recs.push_back(encode_bi32(100) + std::string(76, ' '));
    }
    auto result = exec.execute(op, recs);
    auto& s = std::get<icetool::Stats>(op);
    CHECK(s.result.std_dev == doctest::Approx(0.0));
}

TEST_CASE("Icetool: SELECT GE operation") {
    icetool::IcetoolExecutor exec;
    icetool::Select sel;
    sel.position = 1; sel.length = 3;
    sel.format = parser::FieldFormat::CH;
    sel.op = parser::CondOp::GE; sel.value = "CCC";
    icetool::Operation op = sel;
    std::vector<engine::Record> recs = {
        make_rec("AAA"), make_rec("BBB"), make_rec("CCC"), make_rec("DDD")
    };
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 2);
}

TEST_CASE("Icetool: SELECT LE operation") {
    icetool::IcetoolExecutor exec;
    icetool::Select sel;
    sel.position = 1; sel.length = 3;
    sel.format = parser::FieldFormat::CH;
    sel.op = parser::CondOp::LE; sel.value = "BBB";
    icetool::Operation op = sel;
    std::vector<engine::Record> recs = {
        make_rec("AAA"), make_rec("BBB"), make_rec("CCC")
    };
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 2);
}

TEST_CASE("Icetool: RANGE empty result") {
    icetool::IcetoolExecutor exec;
    icetool::Range range_op;
    range_op.position = 1; range_op.length = 3;
    range_op.format = parser::FieldFormat::CH;
    range_op.low = "XXX"; range_op.high = "ZZZ";
    icetool::Operation op = range_op;
    std::vector<engine::Record> recs = {make_rec("AAA"), make_rec("BBB")};
    auto result = exec.execute(op, recs);
    CHECK(result.empty());
}

TEST_CASE("Icetool: UNIQUE on empty input") {
    icetool::IcetoolExecutor exec;
    icetool::Unique uniq;
    uniq.position = 1; uniq.length = 3; uniq.format = parser::FieldFormat::CH;
    icetool::Operation op = uniq;
    std::vector<engine::Record> recs;
    auto result = exec.execute(op, recs);
    CHECK(result.empty());
}

TEST_CASE("Icetool: UNIQUE all duplicates") {
    icetool::IcetoolExecutor exec;
    icetool::Unique uniq;
    uniq.position = 1; uniq.length = 3; uniq.format = parser::FieldFormat::CH;
    icetool::Operation op = uniq;
    std::vector<engine::Record> recs = {
        make_rec("AAA"), make_rec("AAA"), make_rec("AAA")
    };
    auto result = exec.execute(op, recs);
    CHECK(result.size() == 1);
}

// ================================================================
// EBCDIC TESTS (15+)
// ================================================================

TEST_CASE("EBCDIC: to_ascii space") {
    CHECK(ebcdic::to_ascii(0x40) == 0x20);
}

TEST_CASE("EBCDIC: to_ascii letter A") {
    CHECK(ebcdic::to_ascii(0xC1) == 'A');
}

TEST_CASE("EBCDIC: to_ascii letter Z") {
    CHECK(ebcdic::to_ascii(0xE9) == 'Z');
}

TEST_CASE("EBCDIC: to_ascii digit 0") {
    CHECK(ebcdic::to_ascii(0xF0) == '0');
}

TEST_CASE("EBCDIC: to_ascii digit 9") {
    CHECK(ebcdic::to_ascii(0xF9) == '9');
}

TEST_CASE("EBCDIC: to_ebcdic letter A") {
    CHECK(ebcdic::to_ebcdic('A') == 0xC1);
}

TEST_CASE("EBCDIC: to_ebcdic digit 0") {
    CHECK(ebcdic::to_ebcdic('0') == 0xF0);
}

TEST_CASE("EBCDIC: string roundtrip ASCII->EBCDIC->ASCII") {
    std::string original = "HELLO WORLD 123";
    auto ebc = ebcdic::ascii_to_ebcdic(original);
    auto back = ebcdic::ebcdic_to_ascii(ebc);
    CHECK(back == original);
}

TEST_CASE("EBCDIC: string roundtrip uppercase letters") {
    std::string original = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    auto ebc = ebcdic::ascii_to_ebcdic(original);
    auto back = ebcdic::ebcdic_to_ascii(ebc);
    CHECK(back == original);
}

TEST_CASE("EBCDIC: string roundtrip digits") {
    std::string original = "0123456789";
    auto ebc = ebcdic::ascii_to_ebcdic(original);
    auto back = ebcdic::ebcdic_to_ascii(ebc);
    CHECK(back == original);
}

TEST_CASE("EBCDIC: string roundtrip lowercase letters") {
    std::string original = "abcdefghijklmnopqrstuvwxyz";
    auto ebc = ebcdic::ascii_to_ebcdic(original);
    auto back = ebcdic::ebcdic_to_ascii(ebc);
    CHECK(back == original);
}

TEST_CASE("EBCDIC: ebcdic_compare equal") {
    auto a = ebcdic::ascii_to_ebcdic("HELLO");
    auto b = ebcdic::ascii_to_ebcdic("HELLO");
    CHECK(ebcdic::ebcdic_compare(a, b) == 0);
}

TEST_CASE("EBCDIC: ebcdic_compare ordering") {
    auto a = ebcdic::ascii_to_ebcdic("AAAA");
    auto b = ebcdic::ascii_to_ebcdic("BBBB");
    CHECK(ebcdic::ebcdic_compare(a, b) < 0);
    CHECK(ebcdic::ebcdic_compare(b, a) > 0);
}

TEST_CASE("EBCDIC: alternate collation construction") {
    auto alt = ebcdic::make_danish_collation();
    // The custom weight should be set
    CHECK(alt.weights[0x9C] == 0xF000);
}

TEST_CASE("EBCDIC: alternate collation compare") {
    auto alt = ebcdic::make_danish_collation();
    uint8_t a[] = {0xC1}; // A
    uint8_t b[] = {0xC2}; // B
    CHECK(alt.compare(a, b, 1) < 0);
}

TEST_CASE("EBCDIC: validate_tables static assertion passed") {
    // This is a compile-time check; if we get here, tables are valid
    CHECK(ebcdic::validate_tables());
}

TEST_CASE("EBCDIC: special characters roundtrip") {
    std::string original = "!@#$%^&*()";
    auto ebc = ebcdic::ascii_to_ebcdic(original);
    auto back = ebcdic::ebcdic_to_ascii(ebc);
    CHECK(back == original);
}

// ================================================================
// TRANSLATOR TESTS (15+)
// ================================================================

TEST_CASE("Translator: generate from simple SORT card") {
    auto code = translator::translate("SORT FIELDS=(1,10,CH,A)");
    CHECK(code.find("sort_fields") != std::string::npos);
    CHECK(code.find("stable_sort") != std::string::npos);
}

TEST_CASE("Translator: generate includes engine header") {
    auto code = translator::translate("SORT FIELDS=(1,5,CH,A)");
    CHECK(code.find("#include \"lazarus/sort/engine.h\"") != std::string::npos);
}

TEST_CASE("Translator: generate descending sort") {
    auto code = translator::translate("SORT FIELDS=(1,5,CH,D)");
    CHECK(code.find("DESC") != std::string::npos);
}

TEST_CASE("Translator: generate OPTION COPY") {
    auto code = translator::translate("OPTION COPY");
    CHECK(code.find("OPTION COPY") != std::string::npos);
}

TEST_CASE("Translator: generate with INCLUDE filter") {
    auto code = translator::translate(
        "SORT FIELDS=(1,10,CH,A)\nINCLUDE COND=((1,3,CH,EQ,'ABC'))");
    CHECK(code.find("include") != std::string::npos);
    CHECK(code.find("remove_if") != std::string::npos);
}

TEST_CASE("Translator: generate with OMIT filter") {
    auto code = translator::translate(
        "SORT FIELDS=(1,10,CH,A)\nOMIT COND=((1,3,CH,EQ,'XXX'))");
    CHECK(code.find("omit") != std::string::npos);
}

TEST_CASE("Translator: generate with OUTREC") {
    auto code = translator::translate(
        "SORT FIELDS=(1,5,CH,A)\nOUTREC BUILD=(1,5,'|',6,5)");
    CHECK(code.find("outrec") != std::string::npos);
    CHECK(code.find("BUILD") != std::string::npos);
}

TEST_CASE("Translator: generate with SUM") {
    auto code = translator::translate(
        "SORT FIELDS=(1,5,CH,A)\nSUM FIELDS=(10,4,BI)");
    CHECK(code.find("SUM") != std::string::npos);
    CHECK(code.find("SumAggregator") != std::string::npos);
}

TEST_CASE("Translator: generate JCL wrapper") {
    translator::CodeGenerator gen;
    auto jcl = gen.generate_jcl_wrapper(
        "SORT FIELDS=(1,10,CH,A)", "MY.INPUT.FILE", "MY.OUTPUT.FILE");
    CHECK(jcl.find("EXEC PGM=SORT") != std::string::npos);
    CHECK(jcl.find("MY.INPUT.FILE") != std::string::npos);
    CHECK(jcl.find("MY.OUTPUT.FILE") != std::string::npos);
    CHECK(jcl.find("SYSIN") != std::string::npos);
}

TEST_CASE("Translator: typed comparator CH") {
    auto code = translator::typed_comparator<parser::FieldFormat::CH>(1, 10);
    CHECK(code.find("compare_ch") != std::string::npos);
}

TEST_CASE("Translator: typed comparator ZD") {
    auto code = translator::typed_comparator<parser::FieldFormat::ZD>(5, 3);
    CHECK(code.find("decode_zd") != std::string::npos);
}

TEST_CASE("Translator: typed comparator PD") {
    auto code = translator::typed_comparator<parser::FieldFormat::PD>(1, 4);
    CHECK(code.find("decode_pd") != std::string::npos);
}

TEST_CASE("Translator: typed comparator BI") {
    auto code = translator::typed_comparator<parser::FieldFormat::BI>(1, 4);
    CHECK(code.find("decode_bi") != std::string::npos);
}

TEST_CASE("Translator: typed comparator FI") {
    auto code = translator::typed_comparator<parser::FieldFormat::FI>(1, 4);
    CHECK(code.find("decode_fi") != std::string::npos);
}

TEST_CASE("Translator: typed comparator FL") {
    auto code = translator::typed_comparator<parser::FieldFormat::FL>(1, 8);
    CHECK(code.find("decode_fl") != std::string::npos);
}

TEST_CASE("Translator: format_decoder returns correct function names") {
    CHECK(translator::CodeGenerator::format_decoder(parser::FieldFormat::CH) == "compare_ch");
    CHECK(translator::CodeGenerator::format_decoder(parser::FieldFormat::ZD) == "compare_zd");
    CHECK(translator::CodeGenerator::format_decoder(parser::FieldFormat::PD) == "compare_pd");
    CHECK(translator::CodeGenerator::format_decoder(parser::FieldFormat::BI) == "compare_bi");
    CHECK(translator::CodeGenerator::format_decoder(parser::FieldFormat::FI) == "compare_fi");
    CHECK(translator::CodeGenerator::format_decoder(parser::FieldFormat::FL) == "compare_fl");
    CHECK(translator::CodeGenerator::format_decoder(parser::FieldFormat::CSF) == "compare_csf");
    CHECK(translator::CodeGenerator::format_decoder(parser::FieldFormat::CSL) == "compare_csl");
    CHECK(translator::CodeGenerator::format_decoder(parser::FieldFormat::CST) == "compare_cst");
}

TEST_CASE("Translator: multi-key sort generation") {
    auto code = translator::translate("SORT FIELDS=(1,5,CH,A,10,3,ZD,D)");
    CHECK(code.find("FieldFormat::CH") != std::string::npos);
    CHECK(code.find("FieldFormat::ZD") != std::string::npos);
}

// ================================================================
// INTEGRATION TESTS (10+)
// ================================================================

TEST_CASE("Integration: full pipeline sort+include+outrec") {
    std::vector<engine::Record> recs = {
        make_rec("BBKEEP DATA2"),
        make_rec("AADROP DATA1"),
        make_rec("CCKEEP DATA3"),
        make_rec("AAKEEP DATA4")
    };
    auto result = engine::sort_records(
        "SORT FIELDS=(1,2,CH,A)\n"
        "INCLUDE COND=((3,4,CH,EQ,'KEEP'))\n"
        "OUTREC BUILD=(1,2,'|',8,5)",
        recs);
    CHECK(result.size() == 3);
    CHECK(result[0].substr(0, 7) == "AA|DATA");
    CHECK(result[1].substr(0, 7) == "BB|DATA");
}

TEST_CASE("Integration: sort with SKIPREC and STOPAFT") {
    std::vector<engine::Record> recs;
    for (int i = 0; i < 10; ++i) {
        recs.push_back(make_rec(std::string(1, static_cast<char>('A' + i))));
    }
    auto result = engine::sort_records(
        "SORT FIELDS=(1,1,CH,A)\nOPTION SKIPREC=2,STOPAFT=5", recs);
    CHECK(result.size() == 5);
}

TEST_CASE("Integration: sort then SUM aggregation") {
    auto r1 = std::string("AA") + encode_bi32(10) + std::string(74, ' ');
    auto r2 = std::string("BB") + encode_bi32(20) + std::string(74, ' ');
    auto r3 = std::string("AA") + encode_bi32(30) + std::string(74, ' ');
    auto r4 = std::string("BB") + encode_bi32(40) + std::string(74, ' ');
    std::vector<engine::Record> recs = {r1, r2, r3, r4};
    auto result = engine::sort_records(
        "SORT FIELDS=(1,2,CH,A)\nSUM FIELDS=(3,4,BI)", recs);
    CHECK(result.size() == 2);
    CHECK(engine::decode_bi(std::string_view(result[0]).substr(2, 4)) == 40);
    CHECK(engine::decode_bi(std::string_view(result[1]).substr(2, 4)) == 60);
}

TEST_CASE("Integration: ICETOOL SORT then UNIQUE") {
    icetool::IcetoolExecutor exec;

    // Sort
    icetool::Sort sort_op;
    sort_op.fields = {{1, 3, parser::FieldFormat::CH, parser::SortOrder::ASC}};
    icetool::Operation op1 = sort_op;
    std::vector<engine::Record> recs = {
        make_rec("CCC"), make_rec("AAA"), make_rec("BBB"),
        make_rec("AAA"), make_rec("CCC")
    };
    auto sorted = exec.execute(op1, recs);

    // Unique
    icetool::Unique uniq;
    uniq.position = 1; uniq.length = 3; uniq.format = parser::FieldFormat::CH;
    icetool::Operation op2 = uniq;
    auto result = exec.execute(op2, sorted);
    CHECK(result.size() == 3);
}

TEST_CASE("Integration: ICETOOL SORT then STATS") {
    icetool::IcetoolExecutor exec;
    icetool::Sort sort_op;
    sort_op.fields = {{1, 4, parser::FieldFormat::BI, parser::SortOrder::ASC}};
    icetool::Operation op1 = sort_op;
    std::vector<engine::Record> recs;
    for (int v : {50, 10, 30, 20, 40}) {
        recs.push_back(encode_bi32(static_cast<uint32_t>(v)) + std::string(76, ' '));
    }
    auto sorted = exec.execute(op1, recs);

    icetool::Stats stats_op;
    stats_op.position = 1; stats_op.length = 4;
    stats_op.format = parser::FieldFormat::BI;
    icetool::Operation op2 = stats_op;
    exec.execute(op2, sorted);
    auto& s = std::get<icetool::Stats>(op2);
    CHECK(s.result.min_val == 10);
    CHECK(s.result.max_val == 50);
    CHECK(s.result.sum == 150);
}

TEST_CASE("Integration: MERGE + OUTREC post-processing") {
    parser::SortParser p;
    auto spec = p.parse("MERGE FIELDS=(1,3,CH,A)\nOUTREC BUILD=(1,3)");
    engine::SortEngine eng(spec);
    std::vector<std::vector<engine::Record>> inputs = {
        {make_rec("AAA"), make_rec("CCC")},
        {make_rec("BBB"), make_rec("DDD")}
    };
    auto result = eng.execute_merge(inputs);
    CHECK(result.size() == 4);
    CHECK(result[0] == "AAA");
    CHECK(result[3] == "DDD");
}

TEST_CASE("Integration: OUTFIL multi-route with sort") {
    std::vector<engine::Record> recs = {
        make_rec("ASALES001"), make_rec("BWAREHS002"),
        make_rec("ASALES003"), make_rec("CRETURN004")
    };
    // Sort first
    auto sorted = engine::sort_records("SORT FIELDS=(1,9,CH,A)", recs);

    // Route
    icetool::Outfil outfil;
    parser::SimpleCondition cond_a;
    cond_a.position = 1; cond_a.length = 1; cond_a.format = parser::FieldFormat::CH;
    cond_a.op = parser::CondOp::EQ; cond_a.value = "A";
    outfil.routes.push_back({cond_a, "SALES"});

    parser::SimpleCondition cond_b;
    cond_b.position = 1; cond_b.length = 1; cond_b.format = parser::FieldFormat::CH;
    cond_b.op = parser::CondOp::EQ; cond_b.value = "B";
    outfil.routes.push_back({cond_b, "WAREHOUSE"});

    auto routed = outfil.route(sorted);
    CHECK(routed["SALES"].size() == 2);
    CHECK(routed["WAREHOUSE"].size() == 1);
}

TEST_CASE("Integration: EBCDIC encode + sort as CH") {
    // Encode names to EBCDIC, sort by EBCDIC order (CH just does byte compare)
    auto r1 = ebcdic::ascii_to_ebcdic("CHARLIE") + std::string(73, ' ');
    auto r2 = ebcdic::ascii_to_ebcdic("ALPHA  ") + std::string(73, ' ');
    auto r3 = ebcdic::ascii_to_ebcdic("BRAVO  ") + std::string(73, ' ');

    std::vector<engine::Record> recs = {r1, r2, r3};
    auto result = engine::sort_records("SORT FIELDS=(1,7,CH,A)", recs);

    auto first = ebcdic::ebcdic_to_ascii(std::string_view(result[0]).substr(0, 7));
    auto last = ebcdic::ebcdic_to_ascii(std::string_view(result[2]).substr(0, 7));
    CHECK(first == "ALPHA  ");
    CHECK(last == "CHARLIE");
}

TEST_CASE("Integration: translator generates compilable structure") {
    auto code = translator::translate(
        "SORT FIELDS=(1,5,CH,A,10,3,ZD,D)\n"
        "INCLUDE COND=((1,3,CH,EQ,'ABC'))\n"
        "OUTREC BUILD=(1,5,'|',10,3)\n"
        "SUM FIELDS=(10,3,ZD)");
    // Verify all major sections present
    CHECK(code.find("sort_fields") != std::string::npos);
    CHECK(code.find("include") != std::string::npos);
    CHECK(code.find("outrec") != std::string::npos);
    CHECK(code.find("SumAggregator") != std::string::npos);
    CHECK(code.find("return records") != std::string::npos);
}

TEST_CASE("Integration: large dataset sort 1000 records") {
    std::vector<engine::Record> recs;
    recs.reserve(1000);
    for (int i = 999; i >= 0; --i) {
        char buf[4];
        buf[0] = static_cast<char>('A' + (i / 100) % 26);
        buf[1] = static_cast<char>('A' + (i / 10) % 26);
        buf[2] = static_cast<char>('A' + i % 26);
        buf[3] = '\0';
        recs.push_back(make_rec(std::string(buf, 3)));
    }
    auto result = engine::sort_records("SORT FIELDS=(1,3,CH,A)", recs);
    CHECK(result.size() == 1000);
    // Verify sorted order
    for (size_t i = 1; i < result.size(); ++i) {
        CHECK(result[i - 1].substr(0, 3) <= result[i].substr(0, 3));
    }
}
