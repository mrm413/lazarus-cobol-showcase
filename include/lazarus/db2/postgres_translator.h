#ifndef LAZARUS_DB2_POSTGRES_TRANSLATOR_H
#define LAZARUS_DB2_POSTGRES_TRANSLATOR_H

#include "ddl_parser.h"
#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace lazarus { namespace db2 {

// ==================================================================
//  DB2 -> PostgreSQL Type Mapping
// ==================================================================

inline std::string pg_type_name(const ColumnType& ct) {
    switch (ct.kind) {
        case ColumnTypeKind::INTEGER:    return "INTEGER";
        case ColumnTypeKind::SMALLINT:   return "SMALLINT";
        case ColumnTypeKind::BIGINT:     return "BIGINT";
        case ColumnTypeKind::DECIMAL: {
            if (ct.precision > 0) {
                return "NUMERIC(" + std::to_string(ct.precision) + "," + std::to_string(ct.scale) + ")";
            }
            return "NUMERIC";
        }
        case ColumnTypeKind::CHAR:
            return "CHAR(" + std::to_string(ct.precision > 0 ? ct.precision : 1) + ")";
        case ColumnTypeKind::VARCHAR:
            return "VARCHAR(" + std::to_string(ct.precision > 0 ? ct.precision : 255) + ")";
        case ColumnTypeKind::DATE:       return "DATE";
        case ColumnTypeKind::TIME:       return "TIME";
        case ColumnTypeKind::TIMESTAMP:  return "TIMESTAMP";
        case ColumnTypeKind::BLOB:       return "BYTEA";
        case ColumnTypeKind::CLOB:       return "TEXT";
        case ColumnTypeKind::DOUBLE:     return "DOUBLE PRECISION";
        case ColumnTypeKind::REAL:       return "REAL";
        case ColumnTypeKind::GRAPHIC:
            return "CHAR(" + std::to_string(ct.precision > 0 ? ct.precision : 1) + ")";
        case ColumnTypeKind::VARGRAPHIC:
            return "VARCHAR(" + std::to_string(ct.precision > 0 ? ct.precision : 1) + ")";
        case ColumnTypeKind::XML:        return "XML";
        case ColumnTypeKind::BOOLEAN:    return "BOOLEAN";
    }
    return "TEXT";
}

// ==================================================================
//  DB2 -> PostgreSQL Function Translation
// ==================================================================

inline std::string translate_function(std::string_view db2_func) {
    // Case-insensitive map
    std::string upper(db2_func);
    for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    if (upper == "VALUE")   return "COALESCE";
    if (upper == "STRIP")   return "TRIM";
    if (upper == "DIGITS")  return "TO_CHAR";
    if (upper == "SUBSTR")  return "SUBSTRING";
    if (upper == "HEX")     return "ENCODE";
    if (upper == "CHAR")    return "TO_CHAR";
    if (upper == "LENGTH")  return "LENGTH";
    if (upper == "POSSTR")  return "POSITION";
    if (upper == "RAISE_ERROR") return "RAISE_EXCEPTION";
    if (upper == "DAYOFWEEK")   return "EXTRACT(DOW FROM ";
    if (upper == "YEAR")    return "EXTRACT(YEAR FROM ";
    if (upper == "MONTH")   return "EXTRACT(MONTH FROM ";
    if (upper == "DAY")     return "EXTRACT(DAY FROM ";
    if (upper == "HOUR")    return "EXTRACT(HOUR FROM ";
    if (upper == "MINUTE")  return "EXTRACT(MINUTE FROM ";
    if (upper == "SECOND")  return "EXTRACT(SECOND FROM ";
    if (upper == "CURRENT TIMESTAMP") return "CURRENT_TIMESTAMP";
    if (upper == "CURRENT_TIMESTAMP") return "CURRENT_TIMESTAMP";
    if (upper == "CURRENT DATE")      return "CURRENT_DATE";
    if (upper == "CURRENT_DATE")      return "CURRENT_DATE";
    if (upper == "CURRENT TIME")      return "CURRENT_TIME";
    if (upper == "CURRENT_TIME")      return "CURRENT_TIME";
    if (upper == "CONCAT")  return "CONCAT";
    if (upper == "UCASE")   return "UPPER";
    if (upper == "LCASE")   return "LOWER";
    if (upper == "LTRIM")   return "LTRIM";
    if (upper == "RTRIM")   return "RTRIM";
    if (upper == "REPLACE") return "REPLACE";
    if (upper == "LOCATE")  return "POSITION";
    if (upper == "CEILING") return "CEIL";
    if (upper == "TRUNCATE") return "TRUNC";
    if (upper == "INTEGER") return "INTEGER";
    if (upper == "DECIMAL") return "NUMERIC";
    if (upper == "REAL")    return "REAL";
    if (upper == "DOUBLE")  return "DOUBLE PRECISION";
    if (upper == "FLOAT")   return "DOUBLE PRECISION";
    if (upper == "NULLIF")  return "NULLIF";
    if (upper == "COALESCE") return "COALESCE";

    return std::string(db2_func); // passthrough if unknown
}

// ==================================================================
//  PostgreSQL Translator
// ==================================================================

class PostgresTranslator {
public:
    struct Options {
        bool wrap_transaction;
        bool schema_qualified;
        bool generate_sequences;
        bool preserve_comments;
        std::string target_schema; // if set, overrides source schema

        Options() : wrap_transaction(true), schema_qualified(true),
                    generate_sequences(true), preserve_comments(true) {}
        Options(bool wt, bool sq, bool gs, bool pc, std::string ts)
            : wrap_transaction(wt), schema_qualified(sq),
              generate_sequences(gs), preserve_comments(pc),
              target_schema(std::move(ts)) {}
    };

    explicit PostgresTranslator(Options opts = {}) : opts_(std::move(opts)) {}

    std::string translate(const DDLStatement& stmt) const {
        return std::visit([this](const auto& s) { return translate_impl(s); }, stmt);
    }

    std::string translate_script(const std::vector<DDLStatement>& stmts) const {
        std::ostringstream out;
        if (opts_.wrap_transaction) {
            out << "BEGIN;\n\n";
        }
        for (const auto& stmt : stmts) {
            out << translate(stmt) << "\n\n";
        }
        if (opts_.wrap_transaction) {
            out << "COMMIT;\n";
        }
        return out.str();
    }

private:
    Options opts_;

    std::string qualified_name(const std::string& schema, const std::string& name) const {
        std::string s = opts_.target_schema.empty() ? schema : opts_.target_schema;
        if (opts_.schema_qualified && !s.empty()) {
            return to_lower(s) + "." + to_lower(name);
        }
        return to_lower(name);
    }

    static std::string to_lower(std::string_view sv) {
        std::string s(sv);
        for (auto& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return s;
    }

    std::string on_delete_str(OnDeleteAction a) const {
        switch (a) {
            case OnDeleteAction::CASCADE:  return "CASCADE";
            case OnDeleteAction::SET_NULL: return "SET NULL";
            case OnDeleteAction::RESTRICT: return "RESTRICT";
            case OnDeleteAction::NO_ACTION: return "NO ACTION";
        }
        return "NO ACTION";
    }

    std::string translate_impl(const CreateTable& tbl) const {
        std::ostringstream out;

        if (opts_.preserve_comments) {
            out << "-- Translated from DB2 CREATE TABLE " << tbl.name << "\n";
        }

        // Generate sequences for identity columns
        if (opts_.generate_sequences) {
            for (const auto& col : tbl.columns) {
                if (col.identity) {
                    out << "CREATE SEQUENCE " << qualified_name(tbl.schema, tbl.name + "_" + col.name + "_seq") << ";\n\n";
                }
            }
        }

        out << "CREATE TABLE " << qualified_name(tbl.schema, tbl.name) << " (\n";

        for (std::size_t i = 0; i < tbl.columns.size(); ++i) {
            const auto& col = tbl.columns[i];
            out << "    " << to_lower(col.name) << " ";

            if (col.identity && opts_.generate_sequences) {
                // Use SERIAL or sequence default
                if (col.type.kind == ColumnTypeKind::BIGINT) out << "BIGSERIAL";
                else out << "SERIAL";
            } else {
                out << pg_type_name(col.type);
            }

            if (col.not_null) out << " NOT NULL";
            if (col.has_default && !col.identity) {
                out << " DEFAULT " << translate_default_value(col.default_value);
            }

            // Inline unique
            for (const auto& ic : col.inline_constraints) {
                if (ic.kind == ConstraintKind::UNIQUE) out << " UNIQUE";
                if (ic.kind == ConstraintKind::CHECK) {
                    out << " CHECK (" << ic.check_expr << ")";
                }
            }

            bool last_col = (i == tbl.columns.size() - 1) && tbl.constraints.empty();
            if (!last_col) out << ",";
            out << "\n";
        }

        // Table-level constraints
        for (std::size_t i = 0; i < tbl.constraints.size(); ++i) {
            const auto& c = tbl.constraints[i];
            out << "    ";
            if (!c.name.empty()) out << "CONSTRAINT " << to_lower(c.name) << " ";

            switch (c.kind) {
                case ConstraintKind::PRIMARY_KEY:
                    out << "PRIMARY KEY (";
                    for (std::size_t j = 0; j < c.columns.size(); ++j) {
                        if (j > 0) out << ", ";
                        out << to_lower(c.columns[j]);
                    }
                    out << ")";
                    break;
                case ConstraintKind::FOREIGN_KEY:
                    out << "FOREIGN KEY (";
                    for (std::size_t j = 0; j < c.columns.size(); ++j) {
                        if (j > 0) out << ", ";
                        out << to_lower(c.columns[j]);
                    }
                    out << ") REFERENCES " << to_lower(c.ref_table);
                    if (!c.ref_columns.empty()) {
                        out << " (";
                        for (std::size_t j = 0; j < c.ref_columns.size(); ++j) {
                            if (j > 0) out << ", ";
                            out << to_lower(c.ref_columns[j]);
                        }
                        out << ")";
                    }
                    out << " ON DELETE " << on_delete_str(c.on_delete);
                    break;
                case ConstraintKind::UNIQUE:
                    out << "UNIQUE (";
                    for (std::size_t j = 0; j < c.columns.size(); ++j) {
                        if (j > 0) out << ", ";
                        out << to_lower(c.columns[j]);
                    }
                    out << ")";
                    break;
                case ConstraintKind::CHECK:
                    out << "CHECK (" << c.check_expr << ")";
                    break;
                default: break;
            }

            if (i < tbl.constraints.size() - 1) out << ",";
            out << "\n";
        }

        out << ")";

        // Tablespace
        if (!tbl.in_tablespace.empty()) {
            out << "\nTABLESPACE " << to_lower(tbl.in_tablespace);
        }

        out << ";";
        return out.str();
    }

    std::string translate_impl(const CreateIndex& idx) const {
        std::ostringstream out;

        if (opts_.preserve_comments) {
            out << "-- Translated from DB2 CREATE INDEX " << idx.name << "\n";
        }

        out << "CREATE ";
        if (idx.unique) out << "UNIQUE ";
        out << "INDEX " << qualified_name(idx.schema, idx.name)
            << " ON " << qualified_name(idx.table_schema, idx.table_name) << " (";

        for (std::size_t i = 0; i < idx.columns.size(); ++i) {
            if (i > 0) out << ", ";
            out << to_lower(idx.columns[i].name);
            if (idx.columns[i].descending) out << " DESC";
        }
        out << ")";

        if (!idx.include_columns.empty()) {
            out << " INCLUDE (";
            for (std::size_t i = 0; i < idx.include_columns.size(); ++i) {
                if (i > 0) out << ", ";
                out << to_lower(idx.include_columns[i]);
            }
            out << ")";
        }

        out << ";";
        return out.str();
    }

    std::string translate_impl(const CreateView& view) const {
        std::ostringstream out;

        if (opts_.preserve_comments) {
            out << "-- Translated from DB2 CREATE VIEW " << view.name << "\n";
        }

        out << "CREATE OR REPLACE VIEW " << qualified_name(view.schema, view.name);

        if (!view.column_list.empty()) {
            out << " (";
            for (std::size_t i = 0; i < view.column_list.size(); ++i) {
                if (i > 0) out << ", ";
                out << to_lower(view.column_list[i]);
            }
            out << ")";
        }

        // Translate known DB2 functions in the SELECT
        std::string select_text = translate_sql_functions(view.as_select);
        out << " AS\n" << select_text << ";";
        return out.str();
    }

    std::string translate_impl(const CreateTablespace& ts) const {
        std::ostringstream out;

        if (opts_.preserve_comments) {
            out << "-- Translated from DB2 CREATE TABLESPACE " << ts.name << "\n";
            out << "-- Note: DB2 tablespace options mapped to PostgreSQL equivalents\n";
        }

        out << "CREATE TABLESPACE " << to_lower(ts.name);
        out << "\n    LOCATION '/var/lib/postgresql/data/tbs_" << to_lower(ts.name) << "'";
        out << ";";

        return out.str();
    }

    std::string translate_impl(const AlterTable& alt) const {
        std::ostringstream out;

        if (opts_.preserve_comments) {
            out << "-- Translated from DB2 ALTER TABLE " << alt.name << "\n";
        }

        for (std::size_t i = 0; i < alt.actions.size(); ++i) {
            const auto& ac = alt.actions[i];
            out << "ALTER TABLE " << qualified_name(alt.schema, alt.name);

            switch (ac.action) {
                case AlterAction::ADD_COLUMN:
                    out << " ADD COLUMN " << to_lower(ac.column.name) << " " << pg_type_name(ac.column.type);
                    if (ac.column.not_null) out << " NOT NULL";
                    if (ac.column.has_default) out << " DEFAULT " << translate_default_value(ac.column.default_value);
                    break;
                case AlterAction::DROP_COLUMN:
                    out << " DROP COLUMN " << to_lower(ac.column_name);
                    break;
                case AlterAction::ALTER_COLUMN:
                    if (!ac.set_data_type.empty()) {
                        out << " ALTER COLUMN " << to_lower(ac.column_name) << " TYPE " << pg_type_name(ac.column.type);
                    } else if (ac.set_not_null) {
                        out << " ALTER COLUMN " << to_lower(ac.column_name) << " SET NOT NULL";
                    } else if (ac.drop_not_null) {
                        out << " ALTER COLUMN " << to_lower(ac.column_name) << " DROP NOT NULL";
                    } else if (!ac.set_default.empty() && ac.set_default != "__DROP__") {
                        out << " ALTER COLUMN " << to_lower(ac.column_name) << " SET DEFAULT " << translate_default_value(ac.set_default);
                    } else if (ac.set_default == "__DROP__") {
                        out << " ALTER COLUMN " << to_lower(ac.column_name) << " DROP DEFAULT";
                    }
                    break;
            }

            out << ";";
            if (i < alt.actions.size() - 1) out << "\n";
        }

        return out.str();
    }

    static std::string translate_default_value(const std::string& val) {
        if (val.empty()) return "NULL";
        std::string upper = val;
        for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (upper == "CURRENT_TIMESTAMP" || upper == "CURRENT TIMESTAMP") return "CURRENT_TIMESTAMP";
        if (upper == "CURRENT_DATE" || upper == "CURRENT DATE") return "CURRENT_DATE";
        if (upper == "CURRENT_TIME" || upper == "CURRENT TIME") return "CURRENT_TIME";
        if (upper == "NULL") return "NULL";
        // If it looks numeric, return as-is
        bool is_numeric = true;
        for (char c : val) {
            if (!std::isdigit(static_cast<unsigned char>(c)) && c != '.' && c != '-') {
                is_numeric = false; break;
            }
        }
        if (is_numeric) return val;
        return "'" + val + "'";
    }

    static std::string translate_sql_functions(const std::string& sql) {
        std::string result = sql;
        // Text replacement for DB2 functions — search strings include '(' to avoid partial matches
        auto replace_all = [](std::string& s, const std::string& from, const std::string& to) {
            std::size_t pos = 0;
            while ((pos = s.find(from, pos)) != std::string::npos) {
                // Check start boundary only (end boundary is the '(' in the search string)
                bool start_ok = (pos == 0 || !std::isalnum(static_cast<unsigned char>(s[pos - 1])));
                if (start_ok) {
                    s.replace(pos, from.size(), to);
                    pos += to.size();
                } else {
                    pos += from.size();
                }
            }
        };
        replace_all(result, "VALUE(", "COALESCE(");
        replace_all(result, "STRIP(", "TRIM(");
        replace_all(result, "DIGITS(", "TO_CHAR(");
        replace_all(result, "SUBSTR(", "SUBSTRING(");
        replace_all(result, "HEX(", "ENCODE(");
        replace_all(result, "UCASE(", "UPPER(");
        replace_all(result, "LCASE(", "LOWER(");
        replace_all(result, "POSSTR(", "POSITION(");
        replace_all(result, "CEILING(", "CEIL(");
        replace_all(result, "TRUNCATE(", "TRUNC(");
        return result;
    }
};

}} // namespace lazarus::db2

#endif // LAZARUS_DB2_POSTGRES_TRANSLATOR_H
