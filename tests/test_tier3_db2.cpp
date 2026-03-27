#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "include/doctest.h"

#include "include/lazarus/db2/ddl_parser.h"
#include "include/lazarus/db2/postgres_translator.h"
#include "include/lazarus/db2/copybook_mapper.h"
#include "include/lazarus/db2/ebcdic.h"
#include "include/lazarus/db2/record_reader.h"

#include <cmath>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

using namespace lazarus::db2;
using namespace lazarus::db2::ebcdic;

// =====================================================================
//  DDL Parser — CREATE TABLE
// =====================================================================

TEST_CASE("DDL: CREATE TABLE basic") {
    auto result = DDLParser::parse("CREATE TABLE EMPLOYEE (ID INTEGER, NAME VARCHAR(100))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.name == "EMPLOYEE");
    CHECK(tbl.columns.size() == 2);
    CHECK(tbl.columns[0].name == "ID");
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::INTEGER);
    CHECK(tbl.columns[1].name == "NAME");
    CHECK(tbl.columns[1].type.kind == ColumnTypeKind::VARCHAR);
    CHECK(tbl.columns[1].type.precision == 100);
}

TEST_CASE("DDL: CREATE TABLE with schema") {
    auto result = DDLParser::parse("CREATE TABLE HR.EMPLOYEE (ID INTEGER)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.schema == "HR");
    CHECK(tbl.name == "EMPLOYEE");
}

TEST_CASE("DDL: SMALLINT column type") {
    auto result = DDLParser::parse("CREATE TABLE T (A SMALLINT)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::SMALLINT);
}

TEST_CASE("DDL: BIGINT column type") {
    auto result = DDLParser::parse("CREATE TABLE T (A BIGINT)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::BIGINT);
}

TEST_CASE("DDL: DECIMAL with precision and scale") {
    auto result = DDLParser::parse("CREATE TABLE T (AMT DECIMAL(10,2))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::DECIMAL);
    CHECK(tbl.columns[0].type.precision == 10);
    CHECK(tbl.columns[0].type.scale == 2);
}

TEST_CASE("DDL: CHAR and VARCHAR types") {
    auto result = DDLParser::parse("CREATE TABLE T (A CHAR(5), B VARCHAR(200))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::CHAR);
    CHECK(tbl.columns[0].type.precision == 5);
    CHECK(tbl.columns[1].type.kind == ColumnTypeKind::VARCHAR);
    CHECK(tbl.columns[1].type.precision == 200);
}

TEST_CASE("DDL: DATE, TIME, TIMESTAMP types") {
    auto result = DDLParser::parse("CREATE TABLE T (D DATE, T TIME, TS TIMESTAMP)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::DATE);
    CHECK(tbl.columns[1].type.kind == ColumnTypeKind::TIME);
    CHECK(tbl.columns[2].type.kind == ColumnTypeKind::TIMESTAMP);
}

TEST_CASE("DDL: BLOB and CLOB types") {
    auto result = DDLParser::parse("CREATE TABLE T (B BLOB(1M), C CLOB(32K))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::BLOB);
    CHECK(tbl.columns[1].type.kind == ColumnTypeKind::CLOB);
}

TEST_CASE("DDL: DOUBLE and REAL types") {
    auto result = DDLParser::parse("CREATE TABLE T (D DOUBLE, R REAL)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::DOUBLE);
    CHECK(tbl.columns[1].type.kind == ColumnTypeKind::REAL);
}

TEST_CASE("DDL: GRAPHIC and VARGRAPHIC types") {
    auto result = DDLParser::parse("CREATE TABLE T (G GRAPHIC(10), V VARGRAPHIC(50))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::GRAPHIC);
    CHECK(tbl.columns[0].type.precision == 10);
    CHECK(tbl.columns[1].type.kind == ColumnTypeKind::VARGRAPHIC);
    CHECK(tbl.columns[1].type.precision == 50);
}

TEST_CASE("DDL: XML column type") {
    auto result = DDLParser::parse("CREATE TABLE T (X XML)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::XML);
}

TEST_CASE("DDL: NOT NULL constraint") {
    auto result = DDLParser::parse("CREATE TABLE T (ID INTEGER NOT NULL)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].not_null == true);
    CHECK(tbl.columns[0].nullable == false);
}

TEST_CASE("DDL: DEFAULT value") {
    auto result = DDLParser::parse("CREATE TABLE T (STATUS CHAR(1) DEFAULT 'A')");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].has_default == true);
    CHECK(tbl.columns[0].default_value == "A");
}

TEST_CASE("DDL: WITH DEFAULT") {
    auto result = DDLParser::parse("CREATE TABLE T (AMT DECIMAL(10,2) WITH DEFAULT 0)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].with_default == true);
    CHECK(tbl.columns[0].default_value == "0");
}

TEST_CASE("DDL: GENERATED ALWAYS AS IDENTITY") {
    auto result = DDLParser::parse("CREATE TABLE T (ID INTEGER GENERATED ALWAYS AS IDENTITY)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].identity == true);
}

TEST_CASE("DDL: PRIMARY KEY table constraint") {
    auto result = DDLParser::parse(
        "CREATE TABLE T (ID INTEGER NOT NULL, NAME VARCHAR(50), PRIMARY KEY (ID))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    REQUIRE(tbl.constraints.size() >= 1);
    CHECK(tbl.constraints[0].kind == ConstraintKind::PRIMARY_KEY);
    CHECK(tbl.constraints[0].columns[0] == "ID");
}

TEST_CASE("DDL: composite PRIMARY KEY") {
    auto result = DDLParser::parse(
        "CREATE TABLE T (A INTEGER, B INTEGER, PRIMARY KEY (A, B))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    REQUIRE(tbl.constraints.size() >= 1);
    CHECK(tbl.constraints[0].columns.size() == 2);
    CHECK(tbl.constraints[0].columns[0] == "A");
    CHECK(tbl.constraints[0].columns[1] == "B");
}

TEST_CASE("DDL: FOREIGN KEY with REFERENCES") {
    auto result = DDLParser::parse(
        "CREATE TABLE T (ID INTEGER, DEPT_ID INTEGER, FOREIGN KEY (DEPT_ID) REFERENCES DEPT(ID))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    REQUIRE(tbl.constraints.size() >= 1);
    CHECK(tbl.constraints[0].kind == ConstraintKind::FOREIGN_KEY);
    CHECK(tbl.constraints[0].columns[0] == "DEPT_ID");
    CHECK(tbl.constraints[0].ref_table == "DEPT");
    CHECK(tbl.constraints[0].ref_columns[0] == "ID");
}

TEST_CASE("DDL: FOREIGN KEY ON DELETE CASCADE") {
    auto result = DDLParser::parse(
        "CREATE TABLE T (ID INTEGER, FOREIGN KEY (ID) REFERENCES P(ID) ON DELETE CASCADE)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.constraints[0].on_delete == OnDeleteAction::CASCADE);
}

TEST_CASE("DDL: FOREIGN KEY ON DELETE SET NULL") {
    auto result = DDLParser::parse(
        "CREATE TABLE T (ID INTEGER, FOREIGN KEY (ID) REFERENCES P(ID) ON DELETE SET NULL)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.constraints[0].on_delete == OnDeleteAction::SET_NULL);
}

TEST_CASE("DDL: UNIQUE constraint") {
    auto result = DDLParser::parse(
        "CREATE TABLE T (EMAIL VARCHAR(200), UNIQUE (EMAIL))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    REQUIRE(!tbl.constraints.empty());
    CHECK(tbl.constraints[0].kind == ConstraintKind::UNIQUE);
    CHECK(tbl.constraints[0].columns[0] == "EMAIL");
}

TEST_CASE("DDL: CHECK constraint") {
    auto result = DDLParser::parse(
        "CREATE TABLE T (AGE INTEGER, CHECK (AGE > 0))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    REQUIRE(!tbl.constraints.empty());
    CHECK(tbl.constraints[0].kind == ConstraintKind::CHECK);
    CHECK(tbl.constraints[0].check_expr == "AGE > 0");
}

TEST_CASE("DDL: named CONSTRAINT") {
    auto result = DDLParser::parse(
        "CREATE TABLE T (AGE INTEGER, CONSTRAINT CHK_AGE CHECK (AGE >= 0))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    REQUIRE(!tbl.constraints.empty());
    CHECK(tbl.constraints[0].name == "CHK_AGE");
    CHECK(tbl.constraints[0].kind == ConstraintKind::CHECK);
}

TEST_CASE("DDL: EDITPROC option") {
    auto result = DDLParser::parse("CREATE TABLE T (A INTEGER) EDITPROC MYPROC");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.editproc == "MYPROC");
}

TEST_CASE("DDL: VALIDPROC option") {
    auto result = DDLParser::parse("CREATE TABLE T (A INTEGER) VALIDPROC VALPROC1");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.validproc == "VALPROC1");
}

TEST_CASE("DDL: AUDIT option") {
    auto result = DDLParser::parse("CREATE TABLE T (A INTEGER) AUDIT ALL");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.audit == "ALL");
}

TEST_CASE("DDL: CCSID option") {
    auto result = DDLParser::parse("CREATE TABLE T (A INTEGER) CCSID EBCDIC");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.ccsid == "EBCDIC");
}

TEST_CASE("DDL: COMPRESS YES option") {
    auto result = DDLParser::parse("CREATE TABLE T (A INTEGER) COMPRESS YES");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.compress == true);
}

TEST_CASE("DDL: PARTITION BY") {
    auto result = DDLParser::parse(
        "CREATE TABLE T (A INTEGER, B DATE) PARTITION BY (B)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(!tbl.partition_by.empty());
}

TEST_CASE("DDL: IN tablespace") {
    auto result = DDLParser::parse("CREATE TABLE T (A INTEGER) IN TS1");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.in_tablespace == "TS1");
}

TEST_CASE("DDL: multiple columns complex table") {
    auto result = DDLParser::parse(
        "CREATE TABLE HR.EMPLOYEE ("
        "  EMP_ID INTEGER NOT NULL GENERATED ALWAYS AS IDENTITY,"
        "  FIRST_NAME VARCHAR(50) NOT NULL,"
        "  LAST_NAME VARCHAR(50) NOT NULL,"
        "  SALARY DECIMAL(10,2) WITH DEFAULT 0,"
        "  HIRE_DATE DATE,"
        "  DEPT_ID INTEGER,"
        "  PRIMARY KEY (EMP_ID),"
        "  FOREIGN KEY (DEPT_ID) REFERENCES DEPARTMENT(DEPT_ID) ON DELETE SET NULL"
        ")");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.schema == "HR");
    CHECK(tbl.columns.size() == 6);
    CHECK(tbl.columns[0].identity == true);
    CHECK(tbl.columns[3].with_default == true);
    CHECK(tbl.constraints.size() == 2);
}

// =====================================================================
//  DDL Parser — CREATE INDEX
// =====================================================================

TEST_CASE("DDL: CREATE INDEX basic") {
    auto result = DDLParser::parse("CREATE INDEX IDX1 ON T (A)");
    REQUIRE(result.has_value());
    auto& idx = std::get<CreateIndex>(*result);
    CHECK(idx.name == "IDX1");
    CHECK(idx.unique == false);
    CHECK(idx.table_name == "T");
    CHECK(idx.columns.size() == 1);
    CHECK(idx.columns[0].name == "A");
}

TEST_CASE("DDL: CREATE UNIQUE INDEX") {
    auto result = DDLParser::parse("CREATE UNIQUE INDEX IDX2 ON T (A, B)");
    REQUIRE(result.has_value());
    auto& idx = std::get<CreateIndex>(*result);
    CHECK(idx.unique == true);
    CHECK(idx.columns.size() == 2);
}

TEST_CASE("DDL: CREATE INDEX with DESC") {
    auto result = DDLParser::parse("CREATE INDEX IDX3 ON T (A ASC, B DESC)");
    REQUIRE(result.has_value());
    auto& idx = std::get<CreateIndex>(*result);
    CHECK(idx.columns[0].descending == false);
    CHECK(idx.columns[1].descending == true);
}

TEST_CASE("DDL: CREATE INDEX with INCLUDE columns") {
    auto result = DDLParser::parse("CREATE INDEX IDX4 ON T (A) INCLUDE (B, C)");
    REQUIRE(result.has_value());
    auto& idx = std::get<CreateIndex>(*result);
    CHECK(idx.include_columns.size() == 2);
    CHECK(idx.include_columns[0] == "B");
    CHECK(idx.include_columns[1] == "C");
}

TEST_CASE("DDL: CREATE INDEX with schema") {
    auto result = DDLParser::parse("CREATE INDEX SCH.IDX5 ON SCH.T (A)");
    REQUIRE(result.has_value());
    auto& idx = std::get<CreateIndex>(*result);
    CHECK(idx.schema == "SCH");
    CHECK(idx.table_schema == "SCH");
}

// =====================================================================
//  DDL Parser — CREATE VIEW
// =====================================================================

TEST_CASE("DDL: CREATE VIEW basic") {
    auto result = DDLParser::parse("CREATE VIEW V1 AS SELECT ID, NAME FROM EMPLOYEE");
    REQUIRE(result.has_value());
    auto& v = std::get<CreateView>(*result);
    CHECK(v.name == "V1");
    CHECK(v.as_select.find("SELECT") != std::string::npos);
}

TEST_CASE("DDL: CREATE VIEW with column list") {
    auto result = DDLParser::parse("CREATE VIEW V2 (A, B) AS SELECT ID, NAME FROM EMPLOYEE");
    REQUIRE(result.has_value());
    auto& v = std::get<CreateView>(*result);
    CHECK(v.column_list.size() == 2);
    CHECK(v.column_list[0] == "A");
    CHECK(v.column_list[1] == "B");
}

TEST_CASE("DDL: CREATE VIEW with schema") {
    auto result = DDLParser::parse("CREATE VIEW HR.V3 AS SELECT * FROM HR.EMPLOYEE");
    REQUIRE(result.has_value());
    auto& v = std::get<CreateView>(*result);
    CHECK(v.schema == "HR");
    CHECK(v.name == "V3");
}

// =====================================================================
//  DDL Parser — CREATE TABLESPACE
// =====================================================================

TEST_CASE("DDL: CREATE TABLESPACE basic") {
    auto result = DDLParser::parse("CREATE TABLESPACE TS1 IN DBNAME BUFFERPOOL BP0");
    REQUIRE(result.has_value());
    auto& ts = std::get<CreateTablespace>(*result);
    CHECK(ts.name == "TS1");
    CHECK(ts.in_database == "DBNAME");
    CHECK(ts.bufferpool == "BP0");
}

TEST_CASE("DDL: CREATE TABLESPACE with PAGESIZE") {
    auto result = DDLParser::parse("CREATE TABLESPACE TS2 PAGESIZE 32K LOCKSIZE ROW");
    REQUIRE(result.has_value());
    auto& ts = std::get<CreateTablespace>(*result);
    CHECK(ts.pagesize == 32);
    CHECK(ts.locksize == "ROW");
}

TEST_CASE("DDL: CREATE TABLESPACE with SEGSIZE") {
    auto result = DDLParser::parse("CREATE TABLESPACE TS3 SEGSIZE 64");
    REQUIRE(result.has_value());
    auto& ts = std::get<CreateTablespace>(*result);
    CHECK(ts.segsize == "64");
}

TEST_CASE("DDL: CREATE TABLESPACE with COMPRESS") {
    auto result = DDLParser::parse("CREATE TABLESPACE TS4 COMPRESS YES");
    REQUIRE(result.has_value());
    auto& ts = std::get<CreateTablespace>(*result);
    CHECK(ts.compress == true);
}

// =====================================================================
//  DDL Parser — ALTER TABLE
// =====================================================================

TEST_CASE("DDL: ALTER TABLE ADD COLUMN") {
    auto result = DDLParser::parse("ALTER TABLE T ADD COLUMN EMAIL VARCHAR(200)");
    REQUIRE(result.has_value());
    auto& alt = std::get<AlterTable>(*result);
    CHECK(alt.name == "T");
    REQUIRE(alt.actions.size() == 1);
    CHECK(alt.actions[0].action == AlterAction::ADD_COLUMN);
    CHECK(alt.actions[0].column.name == "EMAIL");
    CHECK(alt.actions[0].column.type.kind == ColumnTypeKind::VARCHAR);
}

TEST_CASE("DDL: ALTER TABLE ADD without COLUMN keyword") {
    auto result = DDLParser::parse("ALTER TABLE T ADD PHONE CHAR(15)");
    REQUIRE(result.has_value());
    auto& alt = std::get<AlterTable>(*result);
    CHECK(alt.actions[0].action == AlterAction::ADD_COLUMN);
    CHECK(alt.actions[0].column.name == "PHONE");
}

TEST_CASE("DDL: ALTER TABLE DROP COLUMN") {
    auto result = DDLParser::parse("ALTER TABLE T DROP COLUMN OLD_COL");
    REQUIRE(result.has_value());
    auto& alt = std::get<AlterTable>(*result);
    CHECK(alt.actions[0].action == AlterAction::DROP_COLUMN);
    CHECK(alt.actions[0].column_name == "OLD_COL");
}

TEST_CASE("DDL: ALTER TABLE ALTER COLUMN SET DATA TYPE") {
    auto result = DDLParser::parse("ALTER TABLE T ALTER COLUMN NAME SET DATA TYPE VARCHAR(200)");
    REQUIRE(result.has_value());
    auto& alt = std::get<AlterTable>(*result);
    CHECK(alt.actions[0].action == AlterAction::ALTER_COLUMN);
    CHECK(alt.actions[0].column_name == "NAME");
    CHECK(alt.actions[0].column.type.kind == ColumnTypeKind::VARCHAR);
    CHECK(alt.actions[0].column.type.precision == 200);
}

TEST_CASE("DDL: ALTER TABLE ALTER COLUMN SET NOT NULL") {
    auto result = DDLParser::parse("ALTER TABLE T ALTER COLUMN X SET NOT NULL");
    REQUIRE(result.has_value());
    auto& alt = std::get<AlterTable>(*result);
    CHECK(alt.actions[0].set_not_null == true);
}

TEST_CASE("DDL: ALTER TABLE ALTER COLUMN DROP NOT NULL") {
    auto result = DDLParser::parse("ALTER TABLE T ALTER COLUMN X DROP NOT NULL");
    REQUIRE(result.has_value());
    auto& alt = std::get<AlterTable>(*result);
    CHECK(alt.actions[0].drop_not_null == true);
}

TEST_CASE("DDL: ALTER TABLE with schema") {
    auto result = DDLParser::parse("ALTER TABLE SCH.T ADD COL1 INTEGER");
    REQUIRE(result.has_value());
    auto& alt = std::get<AlterTable>(*result);
    CHECK(alt.schema == "SCH");
    CHECK(alt.name == "T");
}

// =====================================================================
//  DDL Parser — parse_script (multi-statement)
// =====================================================================

TEST_CASE("DDL: parse_script multiple statements") {
    auto stmts = DDLParser::parse_script(
        "CREATE TABLE T1 (A INTEGER);\n"
        "CREATE TABLE T2 (B VARCHAR(50));\n"
        "CREATE INDEX IX1 ON T1 (A);\n");
    CHECK(stmts.size() == 3);
}

TEST_CASE("DDL: parse_script handles comments") {
    auto stmts = DDLParser::parse_script(
        "-- This is a comment\n"
        "CREATE TABLE T1 (A INTEGER);\n");
    CHECK(stmts.size() == 1);
}

// =====================================================================
//  PostgreSQL Translator — Type Mapping
// =====================================================================

TEST_CASE("PG: INTEGER maps to INTEGER") {
    ColumnType ct; ct.kind = ColumnTypeKind::INTEGER;
    CHECK(pg_type_name(ct) == "INTEGER");
}

TEST_CASE("PG: DECIMAL maps to NUMERIC") {
    ColumnType ct; ct.kind = ColumnTypeKind::DECIMAL; ct.precision = 10; ct.scale = 2;
    CHECK(pg_type_name(ct) == "NUMERIC(10,2)");
}

TEST_CASE("PG: BLOB maps to BYTEA") {
    ColumnType ct; ct.kind = ColumnTypeKind::BLOB;
    CHECK(pg_type_name(ct) == "BYTEA");
}

TEST_CASE("PG: CLOB maps to TEXT") {
    ColumnType ct; ct.kind = ColumnTypeKind::CLOB;
    CHECK(pg_type_name(ct) == "TEXT");
}

TEST_CASE("PG: DOUBLE maps to DOUBLE PRECISION") {
    ColumnType ct; ct.kind = ColumnTypeKind::DOUBLE;
    CHECK(pg_type_name(ct) == "DOUBLE PRECISION");
}

TEST_CASE("PG: GRAPHIC maps to CHAR") {
    ColumnType ct; ct.kind = ColumnTypeKind::GRAPHIC; ct.precision = 10;
    CHECK(pg_type_name(ct) == "CHAR(10)");
}

TEST_CASE("PG: VARGRAPHIC maps to VARCHAR") {
    ColumnType ct; ct.kind = ColumnTypeKind::VARGRAPHIC; ct.precision = 50;
    CHECK(pg_type_name(ct) == "VARCHAR(50)");
}

TEST_CASE("PG: XML maps to XML") {
    ColumnType ct; ct.kind = ColumnTypeKind::XML;
    CHECK(pg_type_name(ct) == "XML");
}

TEST_CASE("PG: TIMESTAMP maps to TIMESTAMP") {
    ColumnType ct; ct.kind = ColumnTypeKind::TIMESTAMP;
    CHECK(pg_type_name(ct) == "TIMESTAMP");
}

TEST_CASE("PG: VARCHAR maps to VARCHAR") {
    ColumnType ct; ct.kind = ColumnTypeKind::VARCHAR; ct.precision = 100;
    CHECK(pg_type_name(ct) == "VARCHAR(100)");
}

// =====================================================================
//  PostgreSQL Translator — Function Translation
// =====================================================================

TEST_CASE("PG: VALUE -> COALESCE") {
    CHECK(translate_function("VALUE") == "COALESCE");
}

TEST_CASE("PG: STRIP -> TRIM") {
    CHECK(translate_function("STRIP") == "TRIM");
}

TEST_CASE("PG: DIGITS -> TO_CHAR") {
    CHECK(translate_function("DIGITS") == "TO_CHAR");
}

TEST_CASE("PG: SUBSTR -> SUBSTRING") {
    CHECK(translate_function("SUBSTR") == "SUBSTRING");
}

TEST_CASE("PG: HEX -> ENCODE") {
    CHECK(translate_function("HEX") == "ENCODE");
}

TEST_CASE("PG: CHAR -> TO_CHAR") {
    CHECK(translate_function("CHAR") == "TO_CHAR");
}

TEST_CASE("PG: LENGTH -> LENGTH") {
    CHECK(translate_function("LENGTH") == "LENGTH");
}

TEST_CASE("PG: UCASE -> UPPER") {
    CHECK(translate_function("UCASE") == "UPPER");
}

TEST_CASE("PG: LCASE -> LOWER") {
    CHECK(translate_function("LCASE") == "LOWER");
}

TEST_CASE("PG: CEILING -> CEIL") {
    CHECK(translate_function("CEILING") == "CEIL");
}

TEST_CASE("PG: function case insensitive") {
    CHECK(translate_function("value") == "COALESCE");
    CHECK(translate_function("Strip") == "TRIM");
}

// =====================================================================
//  PostgreSQL Translator — Full Statement Translation
// =====================================================================

TEST_CASE("PG: translate CREATE TABLE") {
    auto stmt = DDLParser::parse("CREATE TABLE T (ID INTEGER NOT NULL, NAME VARCHAR(50))");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("CREATE TABLE t") != std::string::npos);
    CHECK(sql.find("INTEGER") != std::string::npos);
    CHECK(sql.find("NOT NULL") != std::string::npos);
}

TEST_CASE("PG: translate CREATE TABLE with transaction wrapping") {
    auto stmts = DDLParser::parse_script("CREATE TABLE T1 (A INTEGER); CREATE TABLE T2 (B VARCHAR(20));");
    PostgresTranslator tr({true, false, false, false, ""});
    auto sql = tr.translate_script(stmts);
    CHECK(sql.find("BEGIN;") != std::string::npos);
    CHECK(sql.find("COMMIT;") != std::string::npos);
}

TEST_CASE("PG: translate IDENTITY -> SERIAL") {
    auto stmt = DDLParser::parse("CREATE TABLE T (ID INTEGER GENERATED ALWAYS AS IDENTITY)");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, true, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("SERIAL") != std::string::npos);
}

TEST_CASE("PG: translate CREATE INDEX") {
    auto stmt = DDLParser::parse("CREATE UNIQUE INDEX IX1 ON T (A DESC, B)");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("CREATE UNIQUE INDEX") != std::string::npos);
    CHECK(sql.find("DESC") != std::string::npos);
}

TEST_CASE("PG: translate CREATE INDEX with INCLUDE") {
    auto stmt = DDLParser::parse("CREATE INDEX IX2 ON T (A) INCLUDE (B, C)");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("INCLUDE (b, c)") != std::string::npos);
}

TEST_CASE("PG: translate CREATE VIEW") {
    auto stmt = DDLParser::parse("CREATE VIEW V1 AS SELECT ID, NAME FROM EMPLOYEE");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("CREATE OR REPLACE VIEW") != std::string::npos);
    CHECK(sql.find("SELECT") != std::string::npos);
}

TEST_CASE("PG: translate CREATE VIEW function substitution") {
    auto stmt = DDLParser::parse("CREATE VIEW V2 AS SELECT VALUE(A, 0) FROM T");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("COALESCE(") != std::string::npos);
}

TEST_CASE("PG: translate CREATE TABLESPACE") {
    auto stmt = DDLParser::parse("CREATE TABLESPACE TS1 IN DB1 BUFFERPOOL BP0");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("CREATE TABLESPACE") != std::string::npos);
    CHECK(sql.find("LOCATION") != std::string::npos);
}

TEST_CASE("PG: translate ALTER TABLE ADD") {
    auto stmt = DDLParser::parse("ALTER TABLE T ADD COLUMN EMAIL VARCHAR(200)");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("ADD COLUMN") != std::string::npos);
    CHECK(sql.find("VARCHAR(200)") != std::string::npos);
}

TEST_CASE("PG: translate ALTER TABLE DROP") {
    auto stmt = DDLParser::parse("ALTER TABLE T DROP COLUMN OLD_COL");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("DROP COLUMN") != std::string::npos);
}

TEST_CASE("PG: schema-qualified output") {
    auto stmt = DDLParser::parse("CREATE TABLE HR.EMP (ID INTEGER)");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, true, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("hr.emp") != std::string::npos);
}

TEST_CASE("PG: target schema override") {
    auto stmt = DDLParser::parse("CREATE TABLE HR.EMP (ID INTEGER)");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, true, false, false, "public"});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("public.emp") != std::string::npos);
}

TEST_CASE("PG: comment preservation") {
    auto stmt = DDLParser::parse("CREATE TABLE T (A INTEGER)");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, true, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("-- Translated from DB2") != std::string::npos);
}

TEST_CASE("PG: FOREIGN KEY translation") {
    auto stmt = DDLParser::parse(
        "CREATE TABLE T (ID INTEGER, FK_ID INTEGER,"
        " FOREIGN KEY (FK_ID) REFERENCES PARENT(ID) ON DELETE CASCADE)");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("REFERENCES parent") != std::string::npos);
    CHECK(sql.find("ON DELETE CASCADE") != std::string::npos);
}

// =====================================================================
//  Copybook Mapper — PIC parsing
// =====================================================================

TEST_CASE("CB: PIC X(10) -> alpha, size 10") {
    auto pic = detail_cb::parse_pic("X(10)");
    CHECK(pic.is_alpha == true);
    CHECK(pic.total_size == 10);
}

TEST_CASE("CB: PIC 9(5) -> numeric, 5 integer digits") {
    auto pic = detail_cb::parse_pic("9(5)");
    CHECK(pic.is_numeric == true);
    CHECK(pic.integer_digits == 5);
    CHECK(pic.decimal_digits == 0);
}

TEST_CASE("CB: PIC 9(5)V9(2) -> numeric with decimal") {
    auto pic = detail_cb::parse_pic("9(5)V9(2)");
    CHECK(pic.is_numeric == true);
    CHECK(pic.integer_digits == 5);
    CHECK(pic.decimal_digits == 2);
}

TEST_CASE("CB: PIC S9(7) -> signed numeric") {
    auto pic = detail_cb::parse_pic("S9(7)");
    CHECK(pic.is_signed == true);
    CHECK(pic.is_numeric == true);
    CHECK(pic.integer_digits == 7);
}

TEST_CASE("CB: PIC S9(7)V9(2) -> signed decimal") {
    auto pic = detail_cb::parse_pic("S9(7)V9(2)");
    CHECK(pic.is_signed == true);
    CHECK(pic.integer_digits == 7);
    CHECK(pic.decimal_digits == 2);
}

TEST_CASE("CB: PIC XX -> two alpha chars") {
    auto pic = detail_cb::parse_pic("XX");
    CHECK(pic.is_alpha == true);
    CHECK(pic.total_size == 2);
}

TEST_CASE("CB: PIC 999 -> three digits") {
    auto pic = detail_cb::parse_pic("999");
    CHECK(pic.is_numeric == true);
    CHECK(pic.integer_digits == 3);
}

// =====================================================================
//  Copybook Mapper — PIC to Column Type
// =====================================================================

TEST_CASE("CB: PIC X(n) -> VARCHAR") {
    PicClause pic; pic.is_alpha = true; pic.total_size = 20;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::DISPLAY);
    CHECK(ct.kind == ColumnTypeKind::VARCHAR);
    CHECK(ct.precision == 20);
}

TEST_CASE("CB: PIC 9(n) -> DECIMAL(n,0)") {
    PicClause pic; pic.is_numeric = true; pic.integer_digits = 9;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::DISPLAY);
    CHECK(ct.kind == ColumnTypeKind::DECIMAL);
    CHECK(ct.precision == 9);
    CHECK(ct.scale == 0);
}

TEST_CASE("CB: PIC 9(5)V9(2) -> DECIMAL(7,2)") {
    PicClause pic; pic.is_numeric = true; pic.integer_digits = 5; pic.decimal_digits = 2;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::DISPLAY);
    CHECK(ct.kind == ColumnTypeKind::DECIMAL);
    CHECK(ct.precision == 7);
    CHECK(ct.scale == 2);
}

TEST_CASE("CB: COMP small -> SMALLINT") {
    PicClause pic; pic.is_numeric = true; pic.integer_digits = 4;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::COMP);
    CHECK(ct.kind == ColumnTypeKind::SMALLINT);
}

TEST_CASE("CB: COMP medium -> INTEGER") {
    PicClause pic; pic.is_numeric = true; pic.integer_digits = 9;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::COMP);
    CHECK(ct.kind == ColumnTypeKind::INTEGER);
}

TEST_CASE("CB: COMP large -> BIGINT") {
    PicClause pic; pic.is_numeric = true; pic.integer_digits = 18;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::COMP);
    CHECK(ct.kind == ColumnTypeKind::BIGINT);
}

TEST_CASE("CB: COMP-3 -> DECIMAL") {
    PicClause pic; pic.is_numeric = true; pic.integer_digits = 7; pic.decimal_digits = 2;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::COMP_3);
    CHECK(ct.kind == ColumnTypeKind::DECIMAL);
    CHECK(ct.precision == 9);
    CHECK(ct.scale == 2);
}

TEST_CASE("CB: COMP-5 small -> SMALLINT") {
    PicClause pic; pic.is_numeric = true; pic.integer_digits = 4;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::COMP_5);
    CHECK(ct.kind == ColumnTypeKind::SMALLINT);
}

TEST_CASE("CB: COMP-5 large -> BIGINT") {
    PicClause pic; pic.is_numeric = true; pic.integer_digits = 18;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::COMP_5);
    CHECK(ct.kind == ColumnTypeKind::BIGINT);
}

TEST_CASE("CB: COMP-1 -> REAL") {
    PicClause pic;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::COMP_1);
    CHECK(ct.kind == ColumnTypeKind::REAL);
}

TEST_CASE("CB: COMP-2 -> DOUBLE") {
    PicClause pic;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::COMP_2);
    CHECK(ct.kind == ColumnTypeKind::DOUBLE);
}

TEST_CASE("CB: COMP with decimal -> DECIMAL") {
    PicClause pic; pic.is_numeric = true; pic.integer_digits = 5; pic.decimal_digits = 2;
    auto ct = CopybookMapper::pic_to_column_type(pic, CobolUsage::COMP);
    CHECK(ct.kind == ColumnTypeKind::DECIMAL);
    CHECK(ct.precision == 7);
    CHECK(ct.scale == 2);
}

// =====================================================================
//  Copybook Mapper — Full copybook parsing
// =====================================================================

TEST_CASE("CB: parse simple copybook") {
    const char* copybook =
        "       01 CUSTOMER-RECORD.\n"
        "          05 CUST-ID          PIC 9(8).\n"
        "          05 CUST-NAME        PIC X(30).\n"
        "          05 CUST-BALANCE     PIC S9(7)V99 COMP-3.\n";
    auto fields = CopybookParser::parse(copybook);
    REQUIRE(!fields.empty());
    CHECK(fields[0].name == "CUSTOMER-RECORD");
    CHECK(fields[0].children.size() == 3);
    CHECK(fields[0].children[0].name == "CUST-ID");
    CHECK(fields[0].children[1].name == "CUST-NAME");
}

TEST_CASE("CB: parse REDEFINES") {
    const char* copybook =
        "       01 REC.\n"
        "          05 DATE-FIELD       PIC X(8).\n"
        "          05 DATE-NUM REDEFINES DATE-FIELD PIC 9(8).\n";
    auto fields = CopybookParser::parse(copybook);
    REQUIRE(!fields.empty());
    bool found_redefines = false;
    for (auto& ch : fields[0].children) {
        if (ch.name == "DATE-NUM") {
            CHECK(ch.redefines == "DATE-FIELD");
            found_redefines = true;
        }
    }
    CHECK(found_redefines);
}

TEST_CASE("CB: parse OCCURS") {
    const char* copybook =
        "       01 ORDER-REC.\n"
        "          05 ORDER-ID         PIC 9(6).\n"
        "          05 LINE-ITEMS OCCURS 10 TIMES.\n"
        "             10 ITEM-CODE     PIC X(5).\n"
        "             10 ITEM-QTY      PIC 9(3).\n";
    auto fields = CopybookParser::parse(copybook);
    REQUIRE(!fields.empty());
    bool found_occurs = false;
    for (auto& ch : fields[0].children) {
        if (ch.name == "LINE-ITEMS") {
            CHECK(ch.occurs == 10);
            found_occurs = true;
        }
    }
    CHECK(found_occurs);
}

TEST_CASE("CB: parse OCCURS DEPENDING ON") {
    const char* copybook =
        "       01 REC.\n"
        "          05 ITEM-COUNT       PIC 9(3).\n"
        "          05 ITEMS OCCURS 100 DEPENDING ON ITEM-COUNT.\n"
        "             10 ITEM-NAME     PIC X(20).\n";
    auto fields = CopybookParser::parse(copybook);
    REQUIRE(!fields.empty());
    bool found_dep = false;
    for (auto& ch : fields[0].children) {
        if (ch.name == "ITEMS") {
            CHECK(ch.occurs == 100);
            CHECK(ch.occurs_depending_on == "ITEM-COUNT");
            found_dep = true;
        }
    }
    CHECK(found_dep);
}

TEST_CASE("CB: parse level 77") {
    const char* copybook =
        "       77 WORK-FIELD          PIC S9(5)V9(2) COMP-3.\n";
    auto fields = CopybookParser::parse(copybook);
    REQUIRE(!fields.empty());
    CHECK(fields[0].level == 77);
    CHECK(fields[0].name == "WORK-FIELD");
    CHECK(fields[0].usage == CobolUsage::COMP_3);
}

TEST_CASE("CB: parse level 88 values") {
    const char* copybook =
        "       01 REC.\n"
        "          05 STATUS-CODE      PIC X(1).\n"
        "             88 ACTIVE        VALUE 'A'.\n"
        "             88 INACTIVE      VALUE 'I'.\n";
    auto fields = CopybookParser::parse(copybook);
    REQUIRE(!fields.empty());
    // 88-levels attached as children of STATUS-CODE
    bool found_88 = false;
    for (auto& ch : fields[0].children) {
        if (ch.name == "STATUS-CODE") {
            for (auto& sub : ch.children) {
                if (sub.level == 88 && sub.name == "ACTIVE") {
                    found_88 = true;
                    CHECK(!sub.values_88.empty());
                }
            }
        }
    }
    CHECK(found_88);
}

TEST_CASE("CB: parse FILLER field") {
    const char* copybook =
        "       01 REC.\n"
        "          05 FILLER           PIC X(5).\n"
        "          05 DATA-FIELD       PIC X(10).\n";
    auto fields = CopybookParser::parse(copybook);
    REQUIRE(!fields.empty());
    bool found_filler = false;
    for (auto& ch : fields[0].children) {
        if (ch.name == "FILLER") {
            CHECK(ch.is_filler == true);
            found_filler = true;
        }
    }
    CHECK(found_filler);
}

// =====================================================================
//  Copybook Mapper — DB2 Schema Generation
// =====================================================================

TEST_CASE("CB: map copybook to table") {
    const char* copybook =
        "       01 CUSTOMER-RECORD.\n"
        "          05 CUST-ID          PIC 9(8).\n"
        "          05 CUST-NAME        PIC X(30).\n";
    auto fields = CopybookParser::parse(copybook);
    CopybookMapper mapper;
    auto tbl = mapper.map(fields);
    CHECK(!tbl.table_name.empty());
    CHECK(tbl.columns.size() >= 2);
}

TEST_CASE("CB: FILLER fields excluded from mapped table") {
    const char* copybook =
        "       01 REC.\n"
        "          05 FILLER           PIC X(5).\n"
        "          05 DATA-FIELD       PIC X(10).\n";
    auto fields = CopybookParser::parse(copybook);
    CopybookMapper mapper;
    auto tbl = mapper.map(fields);
    // Should only have DATA-FIELD, not FILLER
    for (auto& col : tbl.columns) {
        CHECK(col.name.find("FILLER") == std::string::npos);
    }
}

TEST_CASE("CB: OCCURS creates child table when normalized") {
    const char* copybook =
        "       01 ORDER-REC.\n"
        "          05 ORDER-ID         PIC 9(6).\n"
        "          05 LINE-ITEMS OCCURS 5 TIMES.\n"
        "             10 ITEM-CODE     PIC X(5).\n";
    auto fields = CopybookParser::parse(copybook);
    CopybookMapper mapper({true, true, true, ""});
    auto tbl = mapper.map(fields);
    CHECK(tbl.child_tables.size() >= 1);
}

TEST_CASE("CB: REDEFINES excluded from mapped table") {
    const char* copybook =
        "       01 REC.\n"
        "          05 DATE-STR         PIC X(8).\n"
        "          05 DATE-NUM REDEFINES DATE-STR PIC 9(8).\n";
    auto fields = CopybookParser::parse(copybook);
    CopybookMapper mapper;
    auto tbl = mapper.map(fields);
    // DATE-NUM should be excluded (redefines)
    bool found_redef = false;
    for (auto& col : tbl.columns) {
        if (col.name.find("DATE_NUM") != std::string::npos) found_redef = true;
    }
    CHECK(found_redef == false);
}

TEST_CASE("CB: table prefix option") {
    const char* copybook =
        "       01 REC.\n"
        "          05 F1               PIC X(5).\n";
    auto fields = CopybookParser::parse(copybook);
    CopybookMapper mapper({true, true, true, "TBL_"});
    auto tbl = mapper.map(fields);
    CHECK(tbl.table_name.substr(0, 4) == "TBL_");
}

// =====================================================================
//  EBCDIC — encoding tables
// =====================================================================

TEST_CASE("EBCDIC: letter A roundtrip") {
    uint8_t eb = ascii_to_ebcdic_table[static_cast<unsigned char>('A')];
    CHECK(eb == 0xC1);
    CHECK(ebcdic_to_ascii_table[0xC1] == 'A');
}

TEST_CASE("EBCDIC: letter Z roundtrip") {
    uint8_t eb = ascii_to_ebcdic_table[static_cast<unsigned char>('Z')];
    CHECK(eb == 0xE9);
    CHECK(ebcdic_to_ascii_table[0xE9] == 'Z');
}

TEST_CASE("EBCDIC: lowercase a roundtrip") {
    uint8_t eb = ascii_to_ebcdic_table[static_cast<unsigned char>('a')];
    CHECK(eb == 0x81);
    CHECK(ebcdic_to_ascii_table[0x81] == 'a');
}

TEST_CASE("EBCDIC: digit 0 roundtrip") {
    uint8_t eb = ascii_to_ebcdic_table[static_cast<unsigned char>('0')];
    CHECK(eb == 0xF0);
    CHECK(ebcdic_to_ascii_table[0xF0] == '0');
}

TEST_CASE("EBCDIC: digit 9 roundtrip") {
    uint8_t eb = ascii_to_ebcdic_table[static_cast<unsigned char>('9')];
    CHECK(eb == 0xF9);
    CHECK(ebcdic_to_ascii_table[0xF9] == '9');
}

TEST_CASE("EBCDIC: space roundtrip") {
    uint8_t eb = ascii_to_ebcdic_table[static_cast<unsigned char>(' ')];
    CHECK(eb == 0x40);
    CHECK(ebcdic_to_ascii_table[0x40] == ' ');
}

TEST_CASE("EBCDIC: period roundtrip") {
    uint8_t eb = ascii_to_ebcdic_table[static_cast<unsigned char>('.')];
    CHECK(eb == 0x4B);
    CHECK(ebcdic_to_ascii_table[0x4B] == '.');
}

TEST_CASE("EBCDIC: all uppercase letters mapped") {
    for (char c = 'A'; c <= 'Z'; ++c) {
        uint8_t eb = ascii_to_ebcdic_table[static_cast<unsigned char>(c)];
        CHECK(eb != 0x3F); // 0x3F = unmapped
        CHECK(ebcdic_to_ascii_table[eb] == c);
    }
}

TEST_CASE("EBCDIC: all digits mapped") {
    for (char c = '0'; c <= '9'; ++c) {
        uint8_t eb = ascii_to_ebcdic_table[static_cast<unsigned char>(c)];
        CHECK(eb != 0x3F);
        CHECK(ebcdic_to_ascii_table[eb] == c);
    }
}

TEST_CASE("EBCDIC: string conversion HELLO") {
    std::string ascii_str = "HELLO";
    auto ebcdic_bytes = ascii_to_ebcdic(ascii_str);
    CHECK(ebcdic_bytes.size() == 5);
    CHECK(ebcdic_bytes[0] == 0xC8); // H
    CHECK(ebcdic_bytes[1] == 0xC5); // E
    CHECK(ebcdic_bytes[2] == 0xD3); // L
    CHECK(ebcdic_bytes[3] == 0xD3); // L
    CHECK(ebcdic_bytes[4] == 0xD6); // O
    auto back = ebcdic_to_ascii(ebcdic_bytes.data(), ebcdic_bytes.size());
    CHECK(back == "HELLO");
}

TEST_CASE("EBCDIC: string roundtrip mixed case") {
    std::string original = "Hello World 123";
    auto eb = ascii_to_ebcdic(original);
    auto back = ebcdic_to_ascii(eb.data(), eb.size());
    CHECK(back == original);
}

TEST_CASE("EBCDIC: empty string conversion") {
    auto eb = ascii_to_ebcdic("");
    CHECK(eb.empty());
    auto back = ebcdic_to_ascii(eb.data(), eb.size());
    CHECK(back.empty());
}

TEST_CASE("EBCDIC: NUL byte maps to NUL") {
    CHECK(ebcdic_to_ascii_table[0x00] == '\x00');
}

// =====================================================================
//  EBCDIC — Packed decimal
// =====================================================================

TEST_CASE("EBCDIC: packed decimal decode positive") {
    // 01234C -> +1234
    uint8_t data[] = {0x01, 0x23, 0x4C};
    CHECK(packed_decimal_decode_int(data, 3) == 1234);
}

TEST_CASE("EBCDIC: packed decimal decode negative") {
    // 01234D -> -1234
    uint8_t data[] = {0x01, 0x23, 0x4D};
    CHECK(packed_decimal_decode_int(data, 3) == -1234);
}

TEST_CASE("EBCDIC: packed decimal decode zero") {
    // 0C -> 0
    uint8_t data[] = {0x0C};
    CHECK(packed_decimal_decode_int(data, 1) == 0);
}

TEST_CASE("EBCDIC: packed decimal decode unsigned (F sign)") {
    // Use encode/decode roundtrip for unsigned packed decimal
    auto encoded = packed_decimal_encode(12345, 4, true);
    // Unsigned sign nibble should be 0x0F
    CHECK((encoded.back() & 0x0F) == 0x0F);
    CHECK(packed_decimal_decode_int(encoded.data(), encoded.size()) == 12345);
}

TEST_CASE("EBCDIC: packed decimal decode with scale") {
    // 12345C with scale=2 -> 123.45
    uint8_t data[] = {0x01, 0x23, 0x4C};
    double val = packed_decimal_decode(data, 3, 2);
    CHECK(std::abs(val - 12.34) < 0.001);
}

TEST_CASE("EBCDIC: packed decimal encode positive") {
    auto enc = packed_decimal_encode(1234, 3);
    CHECK(enc.size() == 3);
    // 1234 -> 5 capacity (3*2-1), digits: 0,1,2,3,4 -> but wait, we want 01234
    // Actually: value 1234, max_digits=5, digits=[0,1,2,3,4]
    // byte0: 0|1 = 0x01, byte1: 2|3 = 0x23, byte2: 4|C = 0x4C
    CHECK(enc[0] == 0x01);
    CHECK(enc[1] == 0x23);
    CHECK(enc[2] == 0x4C);
}

TEST_CASE("EBCDIC: packed decimal encode negative") {
    auto enc = packed_decimal_encode(-5678, 3);
    CHECK((enc[2] & 0x0F) == 0x0D); // negative sign
}

TEST_CASE("EBCDIC: packed decimal roundtrip") {
    for (int64_t val : {0LL, 1LL, -1LL, 12345LL, -99999LL, 9999999LL}) {
        auto enc = packed_decimal_encode(val, 5);
        auto dec = packed_decimal_decode_int(enc.data(), enc.size());
        CHECK(dec == val);
    }
}

TEST_CASE("EBCDIC: packed decimal large value roundtrip") {
    int64_t val = 123456789012LL;
    auto enc = packed_decimal_encode(val, 8);
    auto dec = packed_decimal_decode_int(enc.data(), enc.size());
    CHECK(dec == val);
}

// =====================================================================
//  EBCDIC — Binary integers
// =====================================================================

TEST_CASE("EBCDIC: int16 big-endian decode") {
    uint8_t data[] = {0x00, 0x64}; // 100
    CHECK(binary_to_int16(data) == 100);
}

TEST_CASE("EBCDIC: int16 negative decode") {
    uint8_t data[] = {0xFF, 0x9C}; // -100
    CHECK(binary_to_int16(data) == -100);
}

TEST_CASE("EBCDIC: int32 big-endian decode") {
    uint8_t data[] = {0x00, 0x01, 0x00, 0x00}; // 65536
    CHECK(binary_to_int32(data) == 65536);
}

TEST_CASE("EBCDIC: int32 negative decode") {
    uint8_t data[] = {0xFF, 0xFF, 0xFF, 0x9C}; // -100
    CHECK(binary_to_int32(data) == -100);
}

TEST_CASE("EBCDIC: int64 big-endian decode") {
    uint8_t data[8] = {};
    int64_to_binary(1000000000LL, data);
    CHECK(binary_to_int64(data) == 1000000000LL);
}

TEST_CASE("EBCDIC: int16 roundtrip") {
    uint8_t buf[2];
    int16_to_binary(12345, buf);
    CHECK(binary_to_int16(buf) == 12345);
}

TEST_CASE("EBCDIC: int32 roundtrip") {
    uint8_t buf[4];
    int32_to_binary(-987654, buf);
    CHECK(binary_to_int32(buf) == -987654);
}

TEST_CASE("EBCDIC: int64 roundtrip") {
    uint8_t buf[8];
    int64_to_binary(-1234567890123LL, buf);
    CHECK(binary_to_int64(buf) == -1234567890123LL);
}

// =====================================================================
//  EBCDIC — Zoned decimal
// =====================================================================

TEST_CASE("EBCDIC: zoned decimal decode positive") {
    // 123 positive: F1 F2 C3
    uint8_t data[] = {0xF1, 0xF2, 0xC3};
    CHECK(zoned_decimal_decode(data, 3) == 123);
}

TEST_CASE("EBCDIC: zoned decimal decode negative") {
    // 456 negative: F4 F5 D6
    uint8_t data[] = {0xF4, 0xF5, 0xD6};
    CHECK(zoned_decimal_decode(data, 3) == -456);
}

TEST_CASE("EBCDIC: zoned decimal decode unsigned (F zone)") {
    // 789 unsigned: F7 F8 F9
    uint8_t data[] = {0xF7, 0xF8, 0xF9};
    CHECK(zoned_decimal_decode(data, 3) == 789);
}

TEST_CASE("EBCDIC: zoned decimal encode positive") {
    auto enc = zoned_decimal_encode(123, 3);
    CHECK(enc.size() == 3);
    CHECK(enc[0] == 0xF1);
    CHECK(enc[1] == 0xF2);
    CHECK((enc[2] & 0x0F) == 3); // digit 3
    CHECK((enc[2] & 0xF0) == 0xC0); // positive sign
}

TEST_CASE("EBCDIC: zoned decimal encode negative") {
    auto enc = zoned_decimal_encode(-456, 3);
    CHECK((enc[2] & 0xF0) == 0xD0); // negative sign
    CHECK((enc[2] & 0x0F) == 6);
}

TEST_CASE("EBCDIC: zoned decimal roundtrip") {
    for (int64_t val : {0LL, 1LL, -1LL, 999LL, -999LL}) {
        auto enc = zoned_decimal_encode(val, 4);
        auto dec = zoned_decimal_decode(enc.data(), enc.size());
        CHECK(dec == val);
    }
}

TEST_CASE("EBCDIC: zoned decimal zero") {
    auto enc = zoned_decimal_encode(0, 3);
    auto dec = zoned_decimal_decode(enc.data(), enc.size());
    CHECK(dec == 0);
}

// =====================================================================
//  Record Reader — Fixed records
// =====================================================================

TEST_CASE("RR: fixed record count") {
    // 3 records of 4 bytes each
    std::vector<uint8_t> data(12, 0x40); // EBCDIC spaces
    FixedRecordReader reader(data, 4);
    CHECK(reader.record_count() == 3);
}

TEST_CASE("RR: fixed record access by index") {
    std::vector<uint8_t> data = {
        0xC1, 0xC2, 0xC3, 0xC4,  // ABCD
        0xC5, 0xC6, 0xC7, 0xC8   // EFGH
    };
    FixedRecordReader reader(data, 4);
    auto rec = reader.record(0);
    CHECK(rec.length() == 4);
    CHECK(rec[0] == 0xC1);
    auto rec2 = reader.record(1);
    CHECK(rec2[0] == 0xC5);
}

TEST_CASE("RR: fixed record iteration") {
    std::vector<uint8_t> data = {
        0xF1, 0xF2, 0xF3, 0x40,  // "123 "
        0xF4, 0xF5, 0xF6, 0x40   // "456 "
    };
    FixedRecordReader reader(data, 4);
    int count = 0;
    for (auto it = reader.begin(); it != reader.end(); ++it) {
        ++count;
    }
    CHECK(count == 2);
}

TEST_CASE("RR: fixed record field extraction") {
    // "ABCDEF" in EBCDIC
    std::vector<uint8_t> data = {0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6};
    FixedRecordReader reader(data, 6);
    auto rec = reader.record(0);
    auto field = rec.field(0, 3);
    CHECK(field.size() == 3);
}

TEST_CASE("RR: fixed record field_ascii extraction") {
    std::vector<uint8_t> data = {0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6};
    FixedRecordReader reader(data, 6);
    auto rec = reader.record(0);
    auto str = rec.field_ascii(0, 3);
    CHECK(str == "ABC");
}

TEST_CASE("RR: fixed record partial last record") {
    // 10 bytes, record len 4 -> 2 complete records, 2 leftover bytes ignored
    std::vector<uint8_t> data(10, 0x40);
    FixedRecordReader reader(data, 4);
    CHECK(reader.record_count() == 2);
}

// =====================================================================
//  Record Reader — Variable records (RDW)
// =====================================================================

TEST_CASE("RR: variable record count") {
    // Two records: RDW(8) + 4 data bytes, RDW(7) + 3 data bytes
    std::vector<uint8_t> data = {
        0x00, 0x08, 0x00, 0x00,  0xC1, 0xC2, 0xC3, 0xC4,  // len=8, "ABCD"
        0x00, 0x07, 0x00, 0x00,  0xC5, 0xC6, 0xC7           // len=7, "EFG"
    };
    VariableRecordReader reader(data);
    CHECK(reader.record_count() == 2);
}

TEST_CASE("RR: variable record access by index") {
    std::vector<uint8_t> data = {
        0x00, 0x08, 0x00, 0x00,  0xC1, 0xC2, 0xC3, 0xC4,
        0x00, 0x07, 0x00, 0x00,  0xC5, 0xC6, 0xC7
    };
    VariableRecordReader reader(data);
    auto rec0 = reader.record(0);
    CHECK(rec0.length() == 4);
    CHECK(rec0[0] == 0xC1);
    auto rec1 = reader.record(1);
    CHECK(rec1.length() == 3);
    CHECK(rec1[0] == 0xC5);
}

TEST_CASE("RR: variable record iteration") {
    std::vector<uint8_t> data = {
        0x00, 0x06, 0x00, 0x00,  0xF1, 0xF2,
        0x00, 0x06, 0x00, 0x00,  0xF3, 0xF4,
        0x00, 0x06, 0x00, 0x00,  0xF5, 0xF6
    };
    VariableRecordReader reader(data);
    int count = 0;
    for (auto it = reader.begin(); it != reader.end(); ++it) {
        ++count;
    }
    CHECK(count == 3);
}

TEST_CASE("RR: variable record empty data") {
    std::vector<uint8_t> data;
    VariableRecordReader reader(data);
    CHECK(reader.record_count() == 0);
}

// =====================================================================
//  Record Reader — Type-aware extraction
// =====================================================================

TEST_CASE("RR: packed decimal field extraction") {
    std::vector<uint8_t> data = {0x01, 0x23, 0x4C, 0x00, 0x00, 0x00};
    FixedRecordReader reader(data, 6);
    auto rec = reader.record(0);
    CHECK(rec.field_packed_int(0, 3) == 1234);
}

TEST_CASE("RR: binary int32 field extraction") {
    std::vector<uint8_t> data = {0x00, 0x00, 0x01, 0x00, 0x40, 0x40};
    FixedRecordReader reader(data, 6);
    auto rec = reader.record(0);
    CHECK(rec.field_int32(0) == 256);
}

TEST_CASE("RR: zoned decimal field extraction") {
    std::vector<uint8_t> data = {0xF1, 0xF2, 0xC3, 0x40, 0x40, 0x40};
    FixedRecordReader reader(data, 6);
    auto rec = reader.record(0);
    CHECK(rec.field_zoned(0, 3) == 123);
}

TEST_CASE("RR: out of bounds field returns zero/empty") {
    std::vector<uint8_t> data = {0xC1, 0xC2};
    FixedRecordReader reader(data, 2);
    auto rec = reader.record(0);
    CHECK(rec.field_int32(0) == 0); // only 2 bytes, need 4
    CHECK(rec.field(10, 5).empty());
}

// =====================================================================
//  Record Reader — BDW (blocked records)
// =====================================================================

TEST_CASE("RR: blocked record count") {
    // One block: BDW len=19 (4+15), containing 2 variable records
    std::vector<uint8_t> data = {
        // BDW: length=19
        0x00, 0x13, 0x00, 0x00,
        // Record 1: RDW len=8, 4 data bytes
        0x00, 0x08, 0x00, 0x00,  0xC1, 0xC2, 0xC3, 0xC4,
        // Record 2: RDW len=7, 3 data bytes
        0x00, 0x07, 0x00, 0x00,  0xC5, 0xC6, 0xC7
    };
    BlockedRecordReader reader(data);
    CHECK(reader.block_count() == 1);
    CHECK(reader.record_count() == 2);
}

TEST_CASE("RR: blocked record all_records") {
    std::vector<uint8_t> data = {
        0x00, 0x13, 0x00, 0x00,
        0x00, 0x08, 0x00, 0x00,  0xC1, 0xC2, 0xC3, 0xC4,
        0x00, 0x07, 0x00, 0x00,  0xC5, 0xC6, 0xC7
    };
    BlockedRecordReader reader(data);
    auto records = reader.all_records();
    CHECK(records.size() == 2);
    CHECK(records[0].length() == 4);
    CHECK(records[1].length() == 3);
}

// =====================================================================
//  Integration — DDL + copybook -> PostgreSQL pipeline
// =====================================================================

TEST_CASE("INT: DDL parse -> PG translate roundtrip") {
    auto stmt = DDLParser::parse(
        "CREATE TABLE HR.EMPLOYEE ("
        "  EMP_ID INTEGER NOT NULL,"
        "  NAME VARCHAR(50),"
        "  SALARY DECIMAL(10,2),"
        "  PRIMARY KEY (EMP_ID)"
        ")");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({true, true, false, false, ""});
    auto sql = tr.translate_script({*stmt});
    CHECK(sql.find("BEGIN;") != std::string::npos);
    CHECK(sql.find("hr.employee") != std::string::npos);
    CHECK(sql.find("NUMERIC(10,2)") != std::string::npos);
    CHECK(sql.find("PRIMARY KEY") != std::string::npos);
    CHECK(sql.find("COMMIT;") != std::string::npos);
}

TEST_CASE("INT: copybook -> table -> DB2 DDL concept") {
    const char* copybook =
        "       01 ACCOUNT-RECORD.\n"
        "          05 ACCT-NUMBER      PIC 9(10).\n"
        "          05 ACCT-TYPE        PIC X(2).\n"
        "          05 ACCT-BALANCE     PIC S9(9)V99 COMP-3.\n";
    auto fields = CopybookParser::parse(copybook);
    CopybookMapper mapper;
    auto tbl = mapper.map(fields);
    CHECK(tbl.columns.size() >= 3);
    // ACCT-BALANCE should be DECIMAL(11,2)
    bool found_balance = false;
    for (auto& col : tbl.columns) {
        if (col.name.find("ACCT_BALANCE") != std::string::npos) {
            CHECK(col.type.kind == ColumnTypeKind::DECIMAL);
            CHECK(col.type.scale == 2);
            found_balance = true;
        }
    }
    CHECK(found_balance);
}

TEST_CASE("INT: multi-statement DDL script translate") {
    auto stmts = DDLParser::parse_script(
        "CREATE TABLE T1 (ID INTEGER NOT NULL, PRIMARY KEY (ID));\n"
        "CREATE TABLE T2 (ID INTEGER, T1_ID INTEGER,"
        " FOREIGN KEY (T1_ID) REFERENCES T1(ID) ON DELETE CASCADE);\n"
        "CREATE UNIQUE INDEX IX_T2 ON T2 (T1_ID);\n"
    );
    REQUIRE(stmts.size() == 3);
    PostgresTranslator tr({true, false, false, false, ""});
    auto sql = tr.translate_script(stmts);
    CHECK(sql.find("BEGIN;") != std::string::npos);
    CHECK(sql.find("CREATE TABLE t1") != std::string::npos);
    CHECK(sql.find("CREATE TABLE t2") != std::string::npos);
    CHECK(sql.find("CREATE UNIQUE INDEX") != std::string::npos);
    CHECK(sql.find("ON DELETE CASCADE") != std::string::npos);
    CHECK(sql.find("COMMIT;") != std::string::npos);
}

TEST_CASE("INT: EBCDIC record read + field decode pipeline") {
    // Simulate a fixed record: EBCDIC name(10) + packed amount(4) + binary seq(2)
    std::vector<uint8_t> record(16, 0x40); // spaces
    // Name: "SMITH" in EBCDIC
    auto name_eb = ascii_to_ebcdic("SMITH     ");
    std::copy(name_eb.begin(), name_eb.begin() + 10, record.begin());
    // Packed decimal 12345 in 4 bytes
    auto packed = packed_decimal_encode(12345, 4);
    std::copy(packed.begin(), packed.end(), record.begin() + 10);
    // Binary seq 42
    int16_to_binary(42, record.data() + 14);

    FixedRecordReader reader(record, 16);
    auto rec = reader.record(0);
    auto name = rec.field_ascii(0, 10);
    CHECK(name.substr(0, 5) == "SMITH");
    CHECK(rec.field_packed_int(10, 4) == 12345);
    CHECK(rec.field_int16(14) == 42);
}

TEST_CASE("INT: copybook COMP-4 mapping") {
    const char* copybook =
        "       01 REC.\n"
        "          05 SMALL-FIELD      PIC S9(4) COMP-4.\n"
        "          05 MED-FIELD        PIC S9(9) COMP-4.\n"
        "          05 BIG-FIELD        PIC S9(18) COMP-4.\n";
    auto fields = CopybookParser::parse(copybook);
    CopybookMapper mapper;
    auto tbl = mapper.map(fields);
    REQUIRE(tbl.columns.size() >= 3);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::SMALLINT);
    CHECK(tbl.columns[1].type.kind == ColumnTypeKind::INTEGER);
    CHECK(tbl.columns[2].type.kind == ColumnTypeKind::BIGINT);
}

TEST_CASE("INT: full DB2 DDL with all constraint types") {
    auto stmt = DDLParser::parse(
        "CREATE TABLE SCHEMA1.ORDERS ("
        "  ORDER_ID INTEGER NOT NULL GENERATED ALWAYS AS IDENTITY,"
        "  CUSTOMER_ID INTEGER NOT NULL,"
        "  ORDER_DATE DATE DEFAULT CURRENT_DATE,"
        "  TOTAL DECIMAL(12,2) WITH DEFAULT 0,"
        "  STATUS CHAR(1) NOT NULL DEFAULT 'N',"
        "  PRIMARY KEY (ORDER_ID),"
        "  FOREIGN KEY (CUSTOMER_ID) REFERENCES CUSTOMERS(CUST_ID) ON DELETE RESTRICT,"
        "  UNIQUE (CUSTOMER_ID, ORDER_DATE),"
        "  CHECK (TOTAL >= 0)"
        ") IN TS_ORDERS COMPRESS YES CCSID UNICODE");
    REQUIRE(stmt.has_value());
    auto& tbl = std::get<CreateTable>(*stmt);
    CHECK(tbl.schema == "SCHEMA1");
    CHECK(tbl.columns.size() == 5);
    CHECK(tbl.columns[0].identity == true);
    CHECK(tbl.columns[2].default_value == "CURRENT_DATE");
    CHECK(tbl.columns[3].with_default == true);
    CHECK(tbl.constraints.size() == 4);
    CHECK(tbl.in_tablespace == "TS_ORDERS");
    CHECK(tbl.compress == true);
    CHECK(tbl.ccsid == "UNICODE");

    PostgresTranslator tr({true, true, true, true, ""});
    auto sql = tr.translate_script({*stmt});
    CHECK(sql.find("BEGIN;") != std::string::npos);
    CHECK(sql.find("SERIAL") != std::string::npos);
    CHECK(sql.find("CURRENT_DATE") != std::string::npos);
    CHECK(sql.find("ON DELETE RESTRICT") != std::string::npos);
    CHECK(sql.find("COMMIT;") != std::string::npos);
}

TEST_CASE("INT: ALTER TABLE -> PG translation") {
    auto stmts = DDLParser::parse_script(
        "ALTER TABLE HR.EMP ADD COLUMN MIDDLE_NAME VARCHAR(30);\n"
        "ALTER TABLE HR.EMP DROP COLUMN OLD_PHONE;\n"
        "ALTER TABLE HR.EMP ALTER COLUMN SALARY SET DATA TYPE DECIMAL(12,2);\n"
    );
    CHECK(stmts.size() == 3);
    PostgresTranslator tr({false, true, false, false, ""});
    std::string all_sql;
    for (auto& s : stmts) all_sql += tr.translate(s) + "\n";
    CHECK(all_sql.find("ADD COLUMN") != std::string::npos);
    CHECK(all_sql.find("DROP COLUMN") != std::string::npos);
    CHECK(all_sql.find("TYPE NUMERIC(12,2)") != std::string::npos);
}

TEST_CASE("INT: EBCDIC encode/decode all printable ASCII") {
    // Verify every printable ASCII character roundtrips through EBCDIC
    int roundtrip_ok = 0;
    for (int c = 32; c < 127; ++c) {
        uint8_t eb = ascii_to_ebcdic_table[static_cast<std::size_t>(c)];
        if (eb != 0x3F) { // only count mapped chars
            char back = ebcdic_to_ascii_table[eb];
            if (back == static_cast<char>(c)) ++roundtrip_ok;
        }
    }
    // At minimum: 26 upper + 26 lower + 10 digits + space + common punctuation = ~70+
    CHECK(roundtrip_ok >= 70);
}

TEST_CASE("INT: copybook with nested groups -> flattened columns") {
    const char* copybook =
        "       01 EMPLOYEE-REC.\n"
        "          05 EMP-ID           PIC 9(6).\n"
        "          05 EMP-NAME.\n"
        "             10 FIRST-NAME    PIC X(20).\n"
        "             10 LAST-NAME     PIC X(25).\n"
        "          05 EMP-SALARY       PIC S9(7)V99 COMP-3.\n";
    auto fields = CopybookParser::parse(copybook);
    CopybookMapper mapper({true, true, true, ""});
    auto tbl = mapper.map(fields);
    // Flattened: EMP_ID, EMP_NAME_FIRST_NAME, EMP_NAME_LAST_NAME, EMP_SALARY = 4 cols
    CHECK(tbl.columns.size() >= 4);
    bool found_first = false;
    bool found_last = false;
    for (auto& col : tbl.columns) {
        if (col.name.find("FIRST_NAME") != std::string::npos) found_first = true;
        if (col.name.find("LAST_NAME") != std::string::npos) found_last = true;
    }
    CHECK(found_first);
    CHECK(found_last);
}

TEST_CASE("INT: view with DB2 functions translated") {
    auto stmt = DDLParser::parse(
        "CREATE VIEW V1 AS SELECT VALUE(AMT, 0), STRIP(NAME), SUBSTR(CODE, 1, 3) FROM T");
    REQUIRE(stmt.has_value());
    PostgresTranslator tr({false, false, false, false, ""});
    auto sql = tr.translate(*stmt);
    CHECK(sql.find("COALESCE(") != std::string::npos);
    CHECK(sql.find("TRIM(") != std::string::npos);
    CHECK(sql.find("SUBSTRING(") != std::string::npos);
}

TEST_CASE("INT: DOUBLE PRECISION keyword handling") {
    auto result = DDLParser::parse("CREATE TABLE T (D DOUBLE PRECISION)");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::DOUBLE);
}

TEST_CASE("INT: variable record with packed decimals") {
    // Build a VB record with RDW + packed data
    std::vector<uint8_t> data;
    auto packed_val = packed_decimal_encode(99999, 4);
    uint16_t rdw_len = static_cast<uint16_t>(4 + packed_val.size());
    data.push_back(static_cast<uint8_t>((rdw_len >> 8) & 0xFF));
    data.push_back(static_cast<uint8_t>(rdw_len & 0xFF));
    data.push_back(0x00); data.push_back(0x00); // reserved
    data.insert(data.end(), packed_val.begin(), packed_val.end());

    VariableRecordReader reader(data);
    CHECK(reader.record_count() == 1);
    auto rec = reader.record(0);
    CHECK(rec.field_packed_int(0, 4) == 99999);
}

TEST_CASE("INT: EBCDIC special characters roundtrip") {
    // Verify common punctuation roundtrips
    for (char c : std::string(".,;:!$*()+-/<>=@#&%_?")) {
        uint8_t eb = ascii_to_ebcdic_table[static_cast<unsigned char>(c)];
        if (eb != 0x3F) {
            char back = ebcdic_to_ascii_table[eb];
            CHECK(back == c);
        }
    }
}

TEST_CASE("DDL: NUMERIC synonym for DECIMAL") {
    auto result = DDLParser::parse("CREATE TABLE T (A NUMERIC(8,3))");
    REQUIRE(result.has_value());
    auto& tbl = std::get<CreateTable>(*result);
    CHECK(tbl.columns[0].type.kind == ColumnTypeKind::DECIMAL);
    CHECK(tbl.columns[0].type.precision == 8);
    CHECK(tbl.columns[0].type.scale == 3);
}
