#ifndef LAZARUS_DB2_DDL_PARSER_H
#define LAZARUS_DB2_DDL_PARSER_H

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace lazarus { namespace db2 {

// ==================================================================
//  AST Types
// ==================================================================

enum class ColumnTypeKind {
    INTEGER, SMALLINT, BIGINT, DECIMAL, CHAR, VARCHAR,
    DATE, TIME, TIMESTAMP, BLOB, CLOB, DOUBLE, REAL,
    GRAPHIC, VARGRAPHIC, XML, BOOLEAN
};

struct ColumnType {
    ColumnTypeKind kind = ColumnTypeKind::INTEGER;
    int precision = 0;    // for DECIMAL(p,s), CHAR(n), VARCHAR(n), GRAPHIC(n), VARGRAPHIC(n)
    int scale = 0;        // for DECIMAL(p,s)

    bool operator==(const ColumnType& o) const {
        return kind == o.kind && precision == o.precision && scale == o.scale;
    }
};

enum class ConstraintKind {
    PRIMARY_KEY, FOREIGN_KEY, UNIQUE, CHECK, NOT_NULL, DEFAULT_VAL, WITH_DEFAULT
};

enum class OnDeleteAction {
    NO_ACTION, CASCADE, SET_NULL, RESTRICT
};

struct Constraint {
    ConstraintKind kind = ConstraintKind::NOT_NULL;
    std::string name;           // constraint name (optional)
    std::vector<std::string> columns; // PK/UNIQUE columns, or FK local columns
    std::string ref_table;      // FK reference table
    std::vector<std::string> ref_columns; // FK reference columns
    OnDeleteAction on_delete = OnDeleteAction::NO_ACTION;
    std::string check_expr;     // CHECK expression
    std::string default_value;  // DEFAULT value
};

struct ColumnDef {
    std::string name;
    ColumnType type;
    bool nullable = true;
    bool not_null = false;
    bool has_default = false;
    bool with_default = false;
    std::string default_value;
    bool identity = false;       // GENERATED ALWAYS AS IDENTITY
    std::vector<Constraint> inline_constraints;
};

struct CreateTable {
    std::string schema;
    std::string name;
    std::vector<ColumnDef> columns;
    std::vector<Constraint> constraints;
    // DB2-specific
    std::string editproc;
    std::string validproc;
    std::string audit;     // NONE, CHANGE, ALL
    std::string ccsid;     // ASCII, EBCDIC, UNICODE
    bool compress = false;
    std::string partition_by; // raw PARTITION BY clause
    std::string in_tablespace; // IN tablespace_name
};

struct IndexColumn {
    std::string name;
    bool descending = false;
};

struct CreateIndex {
    std::string schema;
    std::string name;
    bool unique = false;
    std::string table_schema;
    std::string table_name;
    std::vector<IndexColumn> columns;
    std::vector<std::string> include_columns;
};

struct CreateView {
    std::string schema;
    std::string name;
    std::vector<std::string> column_list; // optional column alias list
    std::string as_select;                // the raw SELECT statement
};

struct CreateTablespace {
    std::string name;
    std::string in_database;
    std::string bufferpool;
    int pagesize = 0;
    std::string locksize;
    bool compress = false;
    std::string segsize;
};

enum class AlterAction {
    ADD_COLUMN, DROP_COLUMN, ALTER_COLUMN
};

struct AlterColumn {
    AlterAction action = AlterAction::ADD_COLUMN;
    ColumnDef column;        // for ADD/ALTER
    std::string column_name; // for DROP
    // ALTER specifics
    std::string set_data_type;
    std::string set_default;
    bool set_not_null = false;
    bool drop_not_null = false;
};

struct AlterTable {
    std::string schema;
    std::string name;
    std::vector<AlterColumn> actions;
};

using DDLStatement = std::variant<
    CreateTable, CreateIndex, CreateView, CreateTablespace, AlterTable
>;

// ==================================================================
//  Parser Utilities (string_view based, zero extra allocation where possible)
// ==================================================================

namespace detail {

inline bool ichar_eq(char a, char b) {
    return std::toupper(static_cast<unsigned char>(a)) ==
           std::toupper(static_cast<unsigned char>(b));
}

inline bool iequal(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (!ichar_eq(a[i], b[i])) return false;
    return true;
}

inline bool istarts_with(std::string_view haystack, std::string_view prefix) {
    if (haystack.size() < prefix.size()) return false;
    return iequal(haystack.substr(0, prefix.size()), prefix);
}

inline std::string_view trim(std::string_view sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) sv.remove_suffix(1);
    return sv;
}

inline std::string to_upper_str(std::string_view sv) {
    std::string s(sv);
    for (auto& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

inline std::string to_lower_str(std::string_view sv) {
    std::string s(sv);
    for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// Advance past whitespace
inline void skip_ws(std::string_view& sv) {
    while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
}

// Advance past SQL single-line comments (--) and whitespace
inline void skip_ws_comments(std::string_view& sv) {
    while (true) {
        skip_ws(sv);
        if (sv.size() >= 2 && sv[0] == '-' && sv[1] == '-') {
            auto nl = sv.find('\n');
            if (nl == std::string_view::npos) { sv = {}; return; }
            sv.remove_prefix(nl + 1);
        } else break;
    }
}

// Consume expected keyword (case-insensitive), returns false if not found
inline bool consume_keyword(std::string_view& sv, std::string_view kw) {
    skip_ws_comments(sv);
    if (!istarts_with(sv, kw)) return false;
    if (sv.size() > kw.size()) {
        char after = sv[kw.size()];
        if (std::isalnum(static_cast<unsigned char>(after)) || after == '_') return false;
    }
    sv.remove_prefix(kw.size());
    return true;
}

// Consume a character
inline bool consume_char(std::string_view& sv, char c) {
    skip_ws_comments(sv);
    if (sv.empty() || sv.front() != c) return false;
    sv.remove_prefix(1);
    return true;
}

// Read an identifier (possibly quoted)
inline std::string read_identifier(std::string_view& sv) {
    skip_ws_comments(sv);
    if (sv.empty()) return "";

    if (sv.front() == '"') {
        sv.remove_prefix(1);
        auto end = sv.find('"');
        if (end == std::string_view::npos) return "";
        std::string id(sv.substr(0, end));
        sv.remove_prefix(end + 1);
        return id;
    }

    std::size_t i = 0;
    while (i < sv.size() && (std::isalnum(static_cast<unsigned char>(sv[i])) || sv[i] == '_' || sv[i] == '#' || sv[i] == '$')) ++i;
    if (i == 0) return "";
    std::string id = to_upper_str(sv.substr(0, i));
    sv.remove_prefix(i);
    return id;
}

// Read schema.name or just name
inline void read_qualified_name(std::string_view& sv, std::string& schema, std::string& name) {
    schema.clear();
    name = read_identifier(sv);
    skip_ws_comments(sv);
    if (!sv.empty() && sv.front() == '.') {
        sv.remove_prefix(1);
        schema = name;
        name = read_identifier(sv);
    }
}

// Read a number
inline int read_int(std::string_view& sv) {
    skip_ws_comments(sv);
    int val = 0;
    bool found = false;
    while (!sv.empty() && std::isdigit(static_cast<unsigned char>(sv.front()))) {
        val = val * 10 + (sv.front() - '0');
        sv.remove_prefix(1);
        found = true;
    }
    if (!found) return -1;
    return val;
}

// Read until a closing paren, respecting nesting
inline std::string read_until_close_paren(std::string_view& sv) {
    int depth = 1;
    std::size_t i = 0;
    while (i < sv.size() && depth > 0) {
        if (sv[i] == '(') ++depth;
        else if (sv[i] == ')') --depth;
        if (depth > 0) ++i;
    }
    std::string result(sv.substr(0, i));
    sv.remove_prefix(i);
    return result;
}

// Read a string literal (single-quoted)
inline std::string read_string_literal(std::string_view& sv) {
    skip_ws_comments(sv);
    if (sv.empty() || sv.front() != '\'') return "";
    sv.remove_prefix(1);
    std::string result;
    while (!sv.empty()) {
        if (sv.front() == '\'') {
            sv.remove_prefix(1);
            if (!sv.empty() && sv.front() == '\'') {
                result += '\'';
                sv.remove_prefix(1);
            } else break;
        } else {
            result += sv.front();
            sv.remove_prefix(1);
        }
    }
    return result;
}

// Read token (non-space, non-paren, non-comma, non-semicolon)
inline std::string read_token(std::string_view& sv) {
    skip_ws_comments(sv);
    std::size_t i = 0;
    while (i < sv.size() && !std::isspace(static_cast<unsigned char>(sv[i]))
           && sv[i] != '(' && sv[i] != ')' && sv[i] != ',' && sv[i] != ';') ++i;
    std::string tok = to_upper_str(sv.substr(0, i));
    sv.remove_prefix(i);
    return tok;
}

// Parse column type
inline ColumnType parse_column_type(std::string_view& sv) {
    ColumnType ct;
    std::string type_name = read_token(sv);

    if (type_name == "INTEGER" || type_name == "INT") ct.kind = ColumnTypeKind::INTEGER;
    else if (type_name == "SMALLINT") ct.kind = ColumnTypeKind::SMALLINT;
    else if (type_name == "BIGINT") ct.kind = ColumnTypeKind::BIGINT;
    else if (type_name == "DECIMAL" || type_name == "DEC" || type_name == "NUMERIC") {
        ct.kind = ColumnTypeKind::DECIMAL;
        skip_ws_comments(sv);
        if (!sv.empty() && sv.front() == '(') {
            sv.remove_prefix(1);
            ct.precision = read_int(sv);
            skip_ws_comments(sv);
            if (!sv.empty() && sv.front() == ',') {
                sv.remove_prefix(1);
                ct.scale = read_int(sv);
            }
            consume_char(sv, ')');
        }
    }
    else if (type_name == "CHAR" || type_name == "CHARACTER") {
        ct.kind = ColumnTypeKind::CHAR;
        skip_ws_comments(sv);
        if (!sv.empty() && sv.front() == '(') {
            sv.remove_prefix(1);
            ct.precision = read_int(sv);
            consume_char(sv, ')');
        } else ct.precision = 1;
    }
    else if (type_name == "VARCHAR") {
        ct.kind = ColumnTypeKind::VARCHAR;
        skip_ws_comments(sv);
        if (!sv.empty() && sv.front() == '(') {
            sv.remove_prefix(1);
            ct.precision = read_int(sv);
            consume_char(sv, ')');
        }
    }
    else if (type_name == "DATE") ct.kind = ColumnTypeKind::DATE;
    else if (type_name == "TIME") ct.kind = ColumnTypeKind::TIME;
    else if (type_name == "TIMESTAMP") ct.kind = ColumnTypeKind::TIMESTAMP;
    else if (type_name == "BLOB") {
        ct.kind = ColumnTypeKind::BLOB;
        skip_ws_comments(sv);
        if (!sv.empty() && sv.front() == '(') {
            sv.remove_prefix(1);
            ct.precision = read_int(sv);
            // skip optional K/M/G suffix
            skip_ws_comments(sv);
            if (!sv.empty() && (sv.front() == 'K' || sv.front() == 'M' || sv.front() == 'G'
                || sv.front() == 'k' || sv.front() == 'm' || sv.front() == 'g'))
                sv.remove_prefix(1);
            consume_char(sv, ')');
        }
    }
    else if (type_name == "CLOB") {
        ct.kind = ColumnTypeKind::CLOB;
        skip_ws_comments(sv);
        if (!sv.empty() && sv.front() == '(') {
            sv.remove_prefix(1);
            ct.precision = read_int(sv);
            skip_ws_comments(sv);
            if (!sv.empty() && (sv.front() == 'K' || sv.front() == 'M' || sv.front() == 'G'
                || sv.front() == 'k' || sv.front() == 'm' || sv.front() == 'g'))
                sv.remove_prefix(1);
            consume_char(sv, ')');
        }
    }
    else if (type_name == "DOUBLE") {
        ct.kind = ColumnTypeKind::DOUBLE;
        // optional PRECISION keyword
        skip_ws_comments(sv);
        auto save = sv;
        std::string maybe = read_token(sv);
        if (maybe != "PRECISION") sv = save;
    }
    else if (type_name == "FLOAT") ct.kind = ColumnTypeKind::DOUBLE;
    else if (type_name == "REAL") ct.kind = ColumnTypeKind::REAL;
    else if (type_name == "GRAPHIC") {
        ct.kind = ColumnTypeKind::GRAPHIC;
        skip_ws_comments(sv);
        if (!sv.empty() && sv.front() == '(') {
            sv.remove_prefix(1);
            ct.precision = read_int(sv);
            consume_char(sv, ')');
        }
    }
    else if (type_name == "VARGRAPHIC") {
        ct.kind = ColumnTypeKind::VARGRAPHIC;
        skip_ws_comments(sv);
        if (!sv.empty() && sv.front() == '(') {
            sv.remove_prefix(1);
            ct.precision = read_int(sv);
            consume_char(sv, ')');
        }
    }
    else if (type_name == "XML") ct.kind = ColumnTypeKind::XML;
    else if (type_name == "BOOLEAN") ct.kind = ColumnTypeKind::BOOLEAN;
    else {
        // Unknown type — try INTEGER as fallback
        ct.kind = ColumnTypeKind::INTEGER;
    }

    return ct;
}

// Parse a single column definition within CREATE TABLE
inline ColumnDef parse_column_def(std::string_view& sv) {
    ColumnDef col;
    col.name = read_identifier(sv);
    col.type = parse_column_type(sv);

    // Parse column-level modifiers
    while (true) {
        skip_ws_comments(sv);
        if (sv.empty()) break;

        auto save = sv;

        if (consume_keyword(sv, "NOT")) {
            if (consume_keyword(sv, "NULL")) {
                col.not_null = true;
                col.nullable = false;
                Constraint c; c.kind = ConstraintKind::NOT_NULL;
                col.inline_constraints.push_back(c);
                continue;
            }
            sv = save;
            break;
        }

        if (consume_keyword(sv, "WITH")) {
            if (consume_keyword(sv, "DEFAULT")) {
                col.with_default = true;
                col.has_default = true;
                Constraint c; c.kind = ConstraintKind::WITH_DEFAULT;
                // Check if there's a value
                skip_ws_comments(sv);
                if (!sv.empty() && sv.front() == '\'') {
                    col.default_value = read_string_literal(sv);
                    c.default_value = col.default_value;
                } else if (!sv.empty() && (std::isdigit(static_cast<unsigned char>(sv.front())) || sv.front() == '-')) {
                    std::string val;
                    if (sv.front() == '-') { val += '-'; sv.remove_prefix(1); }
                    while (!sv.empty() && (std::isdigit(static_cast<unsigned char>(sv.front())) || sv.front() == '.')) {
                        val += sv.front(); sv.remove_prefix(1);
                    }
                    col.default_value = val;
                    c.default_value = val;
                }
                col.inline_constraints.push_back(c);
                continue;
            }
            sv = save;
            break;
        }

        if (consume_keyword(sv, "DEFAULT")) {
            col.has_default = true;
            skip_ws_comments(sv);
            Constraint c; c.kind = ConstraintKind::DEFAULT_VAL;
            if (!sv.empty() && sv.front() == '\'') {
                col.default_value = read_string_literal(sv);
            } else if (!sv.empty() && (std::isdigit(static_cast<unsigned char>(sv.front())) || sv.front() == '-')) {
                std::string val;
                if (sv.front() == '-') { val += '-'; sv.remove_prefix(1); }
                while (!sv.empty() && (std::isdigit(static_cast<unsigned char>(sv.front())) || sv.front() == '.')) {
                    val += sv.front(); sv.remove_prefix(1);
                }
                col.default_value = val;
            } else {
                // Could be NULL or CURRENT_TIMESTAMP etc
                auto tok = read_token(sv);
                col.default_value = tok;
            }
            c.default_value = col.default_value;
            col.inline_constraints.push_back(c);
            continue;
        }

        if (consume_keyword(sv, "GENERATED")) {
            // GENERATED ALWAYS AS IDENTITY
            consume_keyword(sv, "ALWAYS");
            consume_keyword(sv, "AS");
            consume_keyword(sv, "IDENTITY");
            col.identity = true;
            // Skip optional (START WITH n, INCREMENT BY n)
            skip_ws_comments(sv);
            if (!sv.empty() && sv.front() == '(') {
                sv.remove_prefix(1);
                read_until_close_paren(sv);
                consume_char(sv, ')');
            }
            continue;
        }

        if (consume_keyword(sv, "PRIMARY")) {
            if (consume_keyword(sv, "KEY")) {
                Constraint c; c.kind = ConstraintKind::PRIMARY_KEY;
                c.columns.push_back(col.name);
                col.inline_constraints.push_back(c);
                col.not_null = true;
                col.nullable = false;
                continue;
            }
            sv = save;
            break;
        }

        if (consume_keyword(sv, "UNIQUE")) {
            Constraint c; c.kind = ConstraintKind::UNIQUE;
            c.columns.push_back(col.name);
            col.inline_constraints.push_back(c);
            continue;
        }

        if (consume_keyword(sv, "REFERENCES")) {
            Constraint c; c.kind = ConstraintKind::FOREIGN_KEY;
            c.columns.push_back(col.name);
            read_qualified_name(sv, c.ref_table, c.ref_table); // simplified
            skip_ws_comments(sv);
            if (!sv.empty() && sv.front() == '(') {
                sv.remove_prefix(1);
                while (true) {
                    auto rcol = read_identifier(sv);
                    if (rcol.empty()) break;
                    c.ref_columns.push_back(rcol);
                    skip_ws_comments(sv);
                    if (sv.empty() || sv.front() != ',') break;
                    sv.remove_prefix(1);
                }
                consume_char(sv, ')');
            }
            // ON DELETE
            skip_ws_comments(sv);
            auto saved2 = sv;
            if (consume_keyword(sv, "ON")) {
                if (consume_keyword(sv, "DELETE")) {
                    auto act = read_token(sv);
                    if (act == "CASCADE") c.on_delete = OnDeleteAction::CASCADE;
                    else if (act == "SET") { read_token(sv); c.on_delete = OnDeleteAction::SET_NULL; }
                    else if (act == "RESTRICT") c.on_delete = OnDeleteAction::RESTRICT;
                    else c.on_delete = OnDeleteAction::NO_ACTION;
                } else sv = saved2;
            } else sv = saved2;
            col.inline_constraints.push_back(c);
            continue;
        }

        if (consume_keyword(sv, "CHECK")) {
            skip_ws_comments(sv);
            Constraint c; c.kind = ConstraintKind::CHECK;
            if (!sv.empty() && sv.front() == '(') {
                sv.remove_prefix(1);
                c.check_expr = read_until_close_paren(sv);
                consume_char(sv, ')');
            }
            col.inline_constraints.push_back(c);
            continue;
        }

        if (consume_keyword(sv, "CONSTRAINT")) {
            // Named constraint on column
            std::string cname = read_identifier(sv);
            skip_ws_comments(sv);
            auto saved3 = sv;
            if (consume_keyword(sv, "CHECK")) {
                Constraint c; c.kind = ConstraintKind::CHECK; c.name = cname;
                if (!sv.empty() && sv.front() == '(') {
                    sv.remove_prefix(1);
                    c.check_expr = read_until_close_paren(sv);
                    consume_char(sv, ')');
                }
                col.inline_constraints.push_back(c);
                continue;
            }
            sv = saved3;
        }

        // None of the above matched
        break;
    }

    return col;
}

// Parse table-level constraint
inline Constraint parse_table_constraint(std::string_view& sv) {
    Constraint c;
    skip_ws_comments(sv);

    // Optional CONSTRAINT name
    auto save = sv;
    if (consume_keyword(sv, "CONSTRAINT")) {
        c.name = read_identifier(sv);
    } else {
        sv = save;
    }

    skip_ws_comments(sv);
    save = sv;

    if (consume_keyword(sv, "PRIMARY")) {
        consume_keyword(sv, "KEY");
        c.kind = ConstraintKind::PRIMARY_KEY;
        consume_char(sv, '(');
        while (true) {
            auto col = read_identifier(sv);
            if (col.empty()) break;
            c.columns.push_back(col);
            skip_ws_comments(sv);
            if (sv.empty() || sv.front() != ',') break;
            sv.remove_prefix(1);
        }
        consume_char(sv, ')');
    } else if (consume_keyword(sv, "FOREIGN")) {
        consume_keyword(sv, "KEY");
        c.kind = ConstraintKind::FOREIGN_KEY;
        consume_char(sv, '(');
        while (true) {
            auto col = read_identifier(sv);
            if (col.empty()) break;
            c.columns.push_back(col);
            skip_ws_comments(sv);
            if (sv.empty() || sv.front() != ',') break;
            sv.remove_prefix(1);
        }
        consume_char(sv, ')');
        consume_keyword(sv, "REFERENCES");
        std::string dummy_schema;
        read_qualified_name(sv, dummy_schema, c.ref_table);
        if (!dummy_schema.empty()) {
            c.ref_table = dummy_schema + "." + c.ref_table;
        }
        skip_ws_comments(sv);
        if (!sv.empty() && sv.front() == '(') {
            sv.remove_prefix(1);
            while (true) {
                auto rcol = read_identifier(sv);
                if (rcol.empty()) break;
                c.ref_columns.push_back(rcol);
                skip_ws_comments(sv);
                if (sv.empty() || sv.front() != ',') break;
                sv.remove_prefix(1);
            }
            consume_char(sv, ')');
        }
        // ON DELETE
        auto saved2 = sv;
        if (consume_keyword(sv, "ON")) {
            if (consume_keyword(sv, "DELETE")) {
                skip_ws_comments(sv);
                auto saved3 = sv;
                if (consume_keyword(sv, "CASCADE")) c.on_delete = OnDeleteAction::CASCADE;
                else if (consume_keyword(sv, "SET")) {
                    consume_keyword(sv, "NULL");
                    c.on_delete = OnDeleteAction::SET_NULL;
                }
                else if (consume_keyword(sv, "RESTRICT")) c.on_delete = OnDeleteAction::RESTRICT;
                else if (consume_keyword(sv, "NO")) {
                    consume_keyword(sv, "ACTION");
                    c.on_delete = OnDeleteAction::NO_ACTION;
                }
                else sv = saved3;
            } else sv = saved2;
        } else sv = saved2;
    } else if (consume_keyword(sv, "UNIQUE")) {
        c.kind = ConstraintKind::UNIQUE;
        consume_char(sv, '(');
        while (true) {
            auto col = read_identifier(sv);
            if (col.empty()) break;
            c.columns.push_back(col);
            skip_ws_comments(sv);
            if (sv.empty() || sv.front() != ',') break;
            sv.remove_prefix(1);
        }
        consume_char(sv, ')');
    } else if (consume_keyword(sv, "CHECK")) {
        c.kind = ConstraintKind::CHECK;
        consume_char(sv, '(');
        c.check_expr = read_until_close_paren(sv);
        consume_char(sv, ')');
    } else {
        sv = save;
    }

    return c;
}

} // namespace detail

// ==================================================================
//  DDL Parser — public interface
// ==================================================================

class DDLParser {
public:
    // Parse a single DDL statement from the input
    static std::optional<DDLStatement> parse(std::string_view input) {
        auto sv = input;
        detail::skip_ws_comments(sv);

        if (detail::consume_keyword(sv, "CREATE")) {
            detail::skip_ws_comments(sv);

            auto save = sv;
            if (detail::consume_keyword(sv, "TABLE")) {
                return parse_create_table(sv);
            }
            sv = save;

            bool is_unique = false;
            if (detail::consume_keyword(sv, "UNIQUE")) {
                is_unique = true;
            }
            save = sv;
            if (detail::consume_keyword(sv, "INDEX")) {
                return parse_create_index(sv, is_unique);
            }
            sv = save;

            if (detail::consume_keyword(sv, "VIEW")) {
                return parse_create_view(sv);
            }
            sv = save;

            if (detail::consume_keyword(sv, "TABLESPACE")) {
                return parse_create_tablespace(sv);
            }
        }

        detail::skip_ws_comments(sv);
        if (detail::consume_keyword(sv, "ALTER")) {
            if (detail::consume_keyword(sv, "TABLE")) {
                return parse_alter_table(sv);
            }
        }

        return std::nullopt;
    }

    // Parse multiple semicolon-separated DDL statements
    static std::vector<DDLStatement> parse_script(std::string_view input) {
        std::vector<DDLStatement> results;
        auto sv = input;
        while (!sv.empty()) {
            detail::skip_ws_comments(sv);
            if (sv.empty()) break;

            // Find the next semicolon (respecting parens)
            std::size_t pos = 0;
            int depth = 0;
            bool in_string = false;
            while (pos < sv.size()) {
                if (sv[pos] == '\'' && !in_string) in_string = true;
                else if (sv[pos] == '\'' && in_string) in_string = false;
                else if (!in_string) {
                    if (sv[pos] == '(') ++depth;
                    else if (sv[pos] == ')') --depth;
                    else if (sv[pos] == ';' && depth == 0) break;
                }
                ++pos;
            }

            auto stmt_sv = sv.substr(0, pos);
            if (pos < sv.size()) sv.remove_prefix(pos + 1);
            else sv = {};

            detail::skip_ws_comments(stmt_sv);
            if (stmt_sv.empty()) continue;

            auto result = parse(stmt_sv);
            if (result) results.push_back(std::move(*result));
        }
        return results;
    }

private:
    static std::optional<DDLStatement> parse_create_table(std::string_view& sv) {
        CreateTable tbl;
        detail::read_qualified_name(sv, tbl.schema, tbl.name);
        detail::skip_ws_comments(sv);

        if (!detail::consume_char(sv, '(')) return std::nullopt;

        // Parse columns and table constraints
        while (true) {
            detail::skip_ws_comments(sv);
            if (sv.empty() || sv.front() == ')') break;

            // Check if this is a table-level constraint
            bool is_constraint = false;
            if (detail::istarts_with(sv, "PRIMARY") || detail::istarts_with(sv, "FOREIGN") ||
                detail::istarts_with(sv, "UNIQUE") || detail::istarts_with(sv, "CHECK") ||
                detail::istarts_with(sv, "CONSTRAINT")) {
                is_constraint = true;
            }

            if (is_constraint) {
                auto c = detail::parse_table_constraint(sv);
                tbl.constraints.push_back(c);
            } else {
                auto col = detail::parse_column_def(sv);
                if (col.name.empty()) break;
                tbl.columns.push_back(std::move(col));
            }

            detail::skip_ws_comments(sv);
            if (sv.empty() || sv.front() == ')') break;
            detail::consume_char(sv, ',');
        }
        detail::consume_char(sv, ')');

        // Parse post-paren options
        while (true) {
            detail::skip_ws_comments(sv);
            if (sv.empty()) break;

            auto save = sv;
            if (detail::consume_keyword(sv, "IN")) {
                tbl.in_tablespace = detail::read_identifier(sv);
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "EDITPROC")) {
                tbl.editproc = detail::read_identifier(sv);
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "VALIDPROC")) {
                tbl.validproc = detail::read_identifier(sv);
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "AUDIT")) {
                tbl.audit = detail::read_identifier(sv);
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "CCSID")) {
                tbl.ccsid = detail::read_identifier(sv);
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "COMPRESS")) {
                if (detail::consume_keyword(sv, "YES")) tbl.compress = true;
                else detail::read_token(sv); // NO or other
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "PARTITION")) {
                if (detail::consume_keyword(sv, "BY")) {
                    // Read rest as partition clause
                    detail::skip_ws_comments(sv);
                    if (!sv.empty() && sv.front() == '(') {
                        sv.remove_prefix(1);
                        tbl.partition_by = detail::read_until_close_paren(sv);
                        detail::consume_char(sv, ')');
                    } else {
                        tbl.partition_by = detail::read_token(sv);
                    }
                    continue;
                }
                sv = save;
            }
            break;
        }

        return DDLStatement{tbl};
    }

    static std::optional<DDLStatement> parse_create_index(std::string_view& sv, bool unique) {
        CreateIndex idx;
        idx.unique = unique;
        detail::read_qualified_name(sv, idx.schema, idx.name);
        detail::skip_ws_comments(sv);
        detail::consume_keyword(sv, "ON");
        detail::read_qualified_name(sv, idx.table_schema, idx.table_name);

        detail::skip_ws_comments(sv);
        if (!detail::consume_char(sv, '(')) return std::nullopt;

        while (true) {
            detail::skip_ws_comments(sv);
            if (sv.empty() || sv.front() == ')') break;
            IndexColumn ic;
            ic.name = detail::read_identifier(sv);
            if (ic.name.empty()) break;
            detail::skip_ws_comments(sv);
            auto save = sv;
            if (detail::consume_keyword(sv, "DESC")) ic.descending = true;
            else if (detail::consume_keyword(sv, "ASC")) ic.descending = false;
            else sv = save;
            idx.columns.push_back(ic);
            detail::skip_ws_comments(sv);
            if (sv.empty() || sv.front() == ')') break;
            detail::consume_char(sv, ',');
        }
        detail::consume_char(sv, ')');

        // INCLUDE columns
        detail::skip_ws_comments(sv);
        auto save = sv;
        if (detail::consume_keyword(sv, "INCLUDE")) {
            detail::consume_char(sv, '(');
            while (true) {
                detail::skip_ws_comments(sv);
                if (sv.empty() || sv.front() == ')') break;
                auto col = detail::read_identifier(sv);
                if (col.empty()) break;
                idx.include_columns.push_back(col);
                detail::skip_ws_comments(sv);
                if (sv.empty() || sv.front() == ')') break;
                detail::consume_char(sv, ',');
            }
            detail::consume_char(sv, ')');
        } else {
            sv = save;
        }

        return DDLStatement{idx};
    }

    static std::optional<DDLStatement> parse_create_view(std::string_view& sv) {
        CreateView view;
        detail::read_qualified_name(sv, view.schema, view.name);

        // Optional column list
        detail::skip_ws_comments(sv);
        if (!sv.empty() && sv.front() == '(') {
            sv.remove_prefix(1);
            while (true) {
                auto col = detail::read_identifier(sv);
                if (col.empty()) break;
                view.column_list.push_back(col);
                detail::skip_ws_comments(sv);
                if (sv.empty() || sv.front() == ')') break;
                detail::consume_char(sv, ',');
            }
            detail::consume_char(sv, ')');
        }

        detail::consume_keyword(sv, "AS");
        detail::skip_ws_comments(sv);
        // Rest is the SELECT statement
        view.as_select = std::string(detail::trim(sv));
        return DDLStatement{view};
    }

    static std::optional<DDLStatement> parse_create_tablespace(std::string_view& sv) {
        CreateTablespace ts;
        ts.name = detail::read_identifier(sv);

        while (true) {
            detail::skip_ws_comments(sv);
            if (sv.empty()) break;

            auto save = sv;
            if (detail::consume_keyword(sv, "IN")) {
                ts.in_database = detail::read_identifier(sv);
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "BUFFERPOOL")) {
                ts.bufferpool = detail::read_identifier(sv);
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "PAGESIZE")) {
                ts.pagesize = detail::read_int(sv);
                // skip optional K suffix
                detail::skip_ws_comments(sv);
                if (!sv.empty() && (sv.front() == 'K' || sv.front() == 'k')) sv.remove_prefix(1);
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "LOCKSIZE")) {
                ts.locksize = detail::read_identifier(sv);
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "COMPRESS")) {
                if (detail::consume_keyword(sv, "YES")) ts.compress = true;
                else detail::read_token(sv);
                continue;
            }
            sv = save;
            if (detail::consume_keyword(sv, "SEGSIZE")) {
                ts.segsize = std::to_string(detail::read_int(sv));
                continue;
            }
            break;
        }

        return DDLStatement{ts};
    }

    static std::optional<DDLStatement> parse_alter_table(std::string_view& sv) {
        AlterTable alt;
        detail::read_qualified_name(sv, alt.schema, alt.name);

        while (true) {
            detail::skip_ws_comments(sv);
            if (sv.empty()) break;

            auto save = sv;
            if (detail::consume_keyword(sv, "ADD")) {
                detail::skip_ws_comments(sv);
                auto save2 = sv;
                if (detail::consume_keyword(sv, "COLUMN")) {
                    // explicit COLUMN keyword
                } else {
                    sv = save2;
                }
                auto col = detail::parse_column_def(sv);
                AlterColumn ac;
                ac.action = AlterAction::ADD_COLUMN;
                ac.column = std::move(col);
                alt.actions.push_back(std::move(ac));
                continue;
            }
            sv = save;

            if (detail::consume_keyword(sv, "DROP")) {
                detail::skip_ws_comments(sv);
                auto save2 = sv;
                if (detail::consume_keyword(sv, "COLUMN")) {
                    // explicit COLUMN keyword
                } else {
                    sv = save2;
                }
                AlterColumn ac;
                ac.action = AlterAction::DROP_COLUMN;
                ac.column_name = detail::read_identifier(sv);
                alt.actions.push_back(std::move(ac));
                continue;
            }
            sv = save;

            if (detail::consume_keyword(sv, "ALTER")) {
                detail::skip_ws_comments(sv);
                auto save2 = sv;
                if (detail::consume_keyword(sv, "COLUMN")) {
                    // explicit COLUMN keyword
                } else {
                    sv = save2;
                }
                AlterColumn ac;
                ac.action = AlterAction::ALTER_COLUMN;
                ac.column_name = detail::read_identifier(sv);
                ac.column.name = ac.column_name;

                // Parse alter specifics
                detail::skip_ws_comments(sv);
                auto save3 = sv;
                if (detail::consume_keyword(sv, "SET")) {
                    detail::skip_ws_comments(sv);
                    auto save4 = sv;
                    if (detail::consume_keyword(sv, "DATA")) {
                        detail::consume_keyword(sv, "TYPE");
                        auto t = detail::parse_column_type(sv);
                        ac.column.type = t;
                        ac.set_data_type = "SET";
                    } else if (detail::consume_keyword(sv, "NOT")) {
                        detail::consume_keyword(sv, "NULL");
                        ac.set_not_null = true;
                    } else if (detail::consume_keyword(sv, "DEFAULT")) {
                        detail::skip_ws_comments(sv);
                        if (!sv.empty() && sv.front() == '\'') {
                            ac.set_default = detail::read_string_literal(sv);
                        } else {
                            ac.set_default = detail::read_token(sv);
                        }
                    } else {
                        sv = save4;
                    }
                } else if (detail::consume_keyword(sv, "DROP")) {
                    detail::skip_ws_comments(sv);
                    if (detail::consume_keyword(sv, "NOT")) {
                        detail::consume_keyword(sv, "NULL");
                        ac.drop_not_null = true;
                    } else if (detail::consume_keyword(sv, "DEFAULT")) {
                        ac.set_default = "__DROP__";
                    }
                } else {
                    sv = save3;
                }

                alt.actions.push_back(std::move(ac));
                continue;
            }

            break;
        }

        return DDLStatement{alt};
    }
};

}} // namespace lazarus::db2

#endif // LAZARUS_DB2_DDL_PARSER_H
