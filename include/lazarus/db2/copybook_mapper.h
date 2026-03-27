#ifndef LAZARUS_DB2_COPYBOOK_MAPPER_H
#define LAZARUS_DB2_COPYBOOK_MAPPER_H

#include "ddl_parser.h"
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace lazarus { namespace db2 {

// ==================================================================
//  COBOL Copybook AST
// ==================================================================

enum class CobolUsage {
    DISPLAY,    // default — zoned decimal or alphanumeric
    COMP,       // binary (big-endian), same as COMP_4
    COMP_1,     // single-precision float
    COMP_2,     // double-precision float
    COMP_3,     // packed decimal
    COMP_4,     // binary
    COMP_5,     // native binary
    INDEX_USAGE // INDEX
};

struct PicClause {
    std::string raw;          // original PIC string e.g. "X(10)"
    bool is_alpha = false;    // X or A type
    bool is_numeric = false;  // 9 type
    bool is_signed = false;   // S prefix
    int integer_digits = 0;   // 9(n) before V
    int decimal_digits = 0;   // 9(m) after V
    int total_size = 0;       // total character positions
};

struct CopybookField {
    int level = 0;            // 01-49, 77, 88
    std::string name;
    PicClause pic;
    CobolUsage usage = CobolUsage::DISPLAY;
    std::string redefines;    // name of redefined field
    int occurs = 0;           // OCCURS count (0 = not array)
    std::string occurs_depending_on; // DEPENDING ON field
    std::string value;        // VALUE clause (for 88-level)
    std::vector<std::string> values_88; // 88-level values
    bool is_filler = false;

    // Tree structure
    std::vector<CopybookField> children;
};

// ==================================================================
//  PIC clause parser
// ==================================================================

namespace detail_cb {

inline PicClause parse_pic(std::string_view raw) {
    PicClause pic;
    pic.raw = std::string(raw);

    // Uppercase for parsing
    std::string upper(raw);
    for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    std::string_view sv(upper);
    bool before_v = true;

    // Check for sign
    if (!sv.empty() && sv.front() == 'S') {
        pic.is_signed = true;
        sv.remove_prefix(1);
    }

    while (!sv.empty()) {
        char ch = sv.front();
        sv.remove_prefix(1);

        if (ch == 'V') {
            before_v = false;
            continue;
        }

        int count = 1;
        // Check for (n) repeat
        if (!sv.empty() && sv.front() == '(') {
            sv.remove_prefix(1);
            count = 0;
            while (!sv.empty() && std::isdigit(static_cast<unsigned char>(sv.front()))) {
                count = count * 10 + (sv.front() - '0');
                sv.remove_prefix(1);
            }
            if (!sv.empty() && sv.front() == ')') sv.remove_prefix(1);
        }

        if (ch == 'X' || ch == 'A' || ch == 'B') {
            pic.is_alpha = true;
            pic.total_size += count;
        } else if (ch == '9') {
            pic.is_numeric = true;
            if (before_v) pic.integer_digits += count;
            else pic.decimal_digits += count;
            pic.total_size += count;
        } else if (ch == 'Z' || ch == '*') {
            // Edited numeric — suppress zeros
            pic.is_numeric = true;
            if (before_v) pic.integer_digits += count;
            else pic.decimal_digits += count;
            pic.total_size += count;
        } else if (ch == '.' || ch == ',') {
            pic.total_size += count;
        }
    }

    return pic;
}

inline CobolUsage parse_usage(std::string_view sv) {
    std::string upper(sv);
    for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    // Remove "USAGE IS " prefix if present
    if (upper.find("COMP-3") != std::string::npos || upper.find("COMPUTATIONAL-3") != std::string::npos)
        return CobolUsage::COMP_3;
    if (upper.find("COMP-5") != std::string::npos || upper.find("COMPUTATIONAL-5") != std::string::npos)
        return CobolUsage::COMP_5;
    if (upper.find("COMP-4") != std::string::npos || upper.find("COMPUTATIONAL-4") != std::string::npos)
        return CobolUsage::COMP_4;
    if (upper.find("COMP-1") != std::string::npos || upper.find("COMPUTATIONAL-1") != std::string::npos)
        return CobolUsage::COMP_1;
    if (upper.find("COMP-2") != std::string::npos || upper.find("COMPUTATIONAL-2") != std::string::npos)
        return CobolUsage::COMP_2;
    if (upper.find("COMP") != std::string::npos || upper.find("COMPUTATIONAL") != std::string::npos ||
        upper.find("BINARY") != std::string::npos)
        return CobolUsage::COMP;
    if (upper.find("INDEX") != std::string::npos)
        return CobolUsage::INDEX_USAGE;
    return CobolUsage::DISPLAY;
}

} // namespace detail_cb

// ==================================================================
//  Copybook Parser
// ==================================================================

class CopybookParser {
public:
    static std::vector<CopybookField> parse(std::string_view input) {
        std::vector<CopybookField> fields;
        auto lines = split_lines(input);

        for (auto& line : lines) {
            auto sv = trim_sv(line);
            if (sv.empty()) continue;
            // Skip comment lines (column 7 = *)
            if (line.size() > 6 && line[6] == '*') continue;
            // Skip sequence numbers — take columns 7-72 if line is long enough
            if (line.size() > 7) {
                // Check if first 6 chars are all digits or spaces (sequence number area)
                bool has_seq = true;
                for (int i = 0; i < 6 && i < static_cast<int>(line.size()); ++i) {
                    if (!std::isdigit(static_cast<unsigned char>(line[static_cast<std::size_t>(i)])) &&
                        line[static_cast<std::size_t>(i)] != ' ') {
                        has_seq = false; break;
                    }
                }
                if (has_seq && line.size() > 7) {
                    sv = trim_sv(line.substr(6));
                }
            }

            // Remove trailing period
            auto sv_str = std::string(sv);
            while (!sv_str.empty() && sv_str.back() == '.') sv_str.pop_back();
            sv = trim_sv(sv_str);
            if (sv.empty()) continue;

            auto field = parse_field(sv);
            if (field.level > 0 || !field.name.empty()) {
                fields.push_back(std::move(field));
            }
        }

        // Build tree
        return build_tree(fields);
    }

private:
    static std::string_view trim_sv(std::string_view sv) {
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) sv.remove_suffix(1);
        return sv;
    }

    static std::vector<std::string> split_lines(std::string_view input) {
        std::vector<std::string> lines;
        std::size_t pos = 0;
        while (pos < input.size()) {
            auto nl = input.find('\n', pos);
            if (nl == std::string_view::npos) {
                lines.emplace_back(input.substr(pos));
                break;
            }
            lines.emplace_back(input.substr(pos, nl - pos));
            pos = nl + 1;
        }
        return lines;
    }

    static std::string upper_str(std::string_view sv) {
        std::string s(sv);
        for (auto& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return s;
    }

    static std::string next_token(std::string_view& sv) {
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
        if (sv.empty()) return "";
        std::size_t i = 0;
        // Handle quoted strings
        if (sv.front() == '\'' || sv.front() == '"') {
            char q = sv.front();
            i = 1;
            while (i < sv.size() && sv[i] != q) ++i;
            if (i < sv.size()) ++i;
            std::string tok(sv.substr(0, i));
            sv.remove_prefix(i);
            return tok;
        }
        while (i < sv.size() && !std::isspace(static_cast<unsigned char>(sv[i]))) ++i;
        std::string tok(sv.substr(0, i));
        sv.remove_prefix(i);
        return tok;
    }

    static CopybookField parse_field(std::string_view line) {
        CopybookField field;
        auto sv = line;

        // Read level number
        auto level_str = next_token(sv);
        if (level_str.empty()) return field;
        try {
            field.level = std::stoi(level_str);
        } catch (...) {
            return field;
        }

        // Read name
        auto name = next_token(sv);
        if (name.empty()) return field;
        field.name = upper_str(name);
        if (field.name == "FILLER") field.is_filler = true;

        // Parse remaining clauses
        while (true) {
            auto tok = upper_str(next_token(sv));
            if (tok.empty()) break;

            if (tok == "PIC" || tok == "PICTURE") {
                // next token may be "IS"
                auto pic_tok = next_token(sv);
                auto pic_upper = upper_str(pic_tok);
                if (pic_upper == "IS") {
                    pic_tok = next_token(sv);
                }
                field.pic = detail_cb::parse_pic(pic_tok);
            } else if (tok == "USAGE") {
                auto usage_tok = next_token(sv);
                auto usage_upper = upper_str(usage_tok);
                if (usage_upper == "IS") {
                    usage_tok = next_token(sv);
                }
                field.usage = detail_cb::parse_usage(usage_tok);
            } else if (tok == "COMP" || tok == "COMPUTATIONAL" || tok == "BINARY") {
                field.usage = CobolUsage::COMP;
            } else if (tok == "COMP-3" || tok == "COMPUTATIONAL-3" || tok == "PACKED-DECIMAL") {
                field.usage = CobolUsage::COMP_3;
            } else if (tok == "COMP-4" || tok == "COMPUTATIONAL-4") {
                field.usage = CobolUsage::COMP_4;
            } else if (tok == "COMP-5" || tok == "COMPUTATIONAL-5") {
                field.usage = CobolUsage::COMP_5;
            } else if (tok == "COMP-1" || tok == "COMPUTATIONAL-1") {
                field.usage = CobolUsage::COMP_1;
            } else if (tok == "COMP-2" || tok == "COMPUTATIONAL-2") {
                field.usage = CobolUsage::COMP_2;
            } else if (tok == "REDEFINES") {
                field.redefines = upper_str(next_token(sv));
            } else if (tok == "OCCURS") {
                auto count_str = next_token(sv);
                try { field.occurs = std::stoi(count_str); } catch (...) {}
                // Check for TIMES and DEPENDING ON
                auto occurs_save = sv;
                auto next_kw = upper_str(next_token(sv));
                if (next_kw == "TIMES") {
                    // ok, consumed
                    occurs_save = sv;
                    next_kw = upper_str(next_token(sv));
                }
                if (next_kw == "DEPENDING") {
                    auto on_tok = upper_str(next_token(sv));
                    if (on_tok == "ON") {
                        field.occurs_depending_on = upper_str(next_token(sv));
                    }
                } else {
                    sv = occurs_save;
                }
            } else if (tok == "VALUE" || tok == "VALUES") {
                // Consume optional IS/ARE
                auto val_tok = upper_str(next_token(sv));
                if (val_tok == "IS" || val_tok == "ARE") {
                    val_tok = upper_str(next_token(sv));
                }
                // For 88-level, can have multiple values separated by spaces/THRU
                if (field.level == 88) {
                    field.values_88.push_back(val_tok);
                    while (true) {
                        auto more = upper_str(next_token(sv));
                        if (more.empty()) break;
                        if (more == "THRU" || more == "THROUGH") {
                            auto end_val = upper_str(next_token(sv));
                            field.values_88.push_back("THRU");
                            field.values_88.push_back(end_val);
                        } else {
                            field.values_88.push_back(more);
                        }
                    }
                } else {
                    field.value = val_tok;
                }
            }
        }

        return field;
    }

    static std::vector<CopybookField> build_tree(const std::vector<CopybookField>& flat) {
        std::vector<CopybookField> roots;
        if (flat.empty()) return roots;

        // Stack-based tree building
        struct StackEntry {
            CopybookField* node;
            int level;
        };

        std::vector<StackEntry> stack;

        for (const auto& f : flat) {
            CopybookField field = f;

            if (field.level == 88) {
                // Attach to most recent field
                if (!stack.empty()) {
                    stack.back().node->children.push_back(std::move(field));
                }
                continue;
            }

            // Pop stack until we find a parent with lower level
            while (!stack.empty() && stack.back().level >= field.level) {
                stack.pop_back();
            }

            if (stack.empty()) {
                roots.push_back(std::move(field));
                stack.push_back({&roots.back(), roots.back().level});
            } else {
                stack.back().node->children.push_back(std::move(field));
                auto* child = &stack.back().node->children.back();
                stack.push_back({child, child->level});
            }
        }

        return roots;
    }
};

// ==================================================================
//  Copybook -> DB2 Schema Mapper
// ==================================================================

struct MappedColumn {
    std::string name;
    ColumnType type;
    bool nullable = true;
    std::string check_constraint; // from 88-level
    bool is_group = false;
    int occurs = 0;
};

struct MappedTable {
    std::string table_name;
    std::vector<MappedColumn> columns;
    std::vector<Constraint> constraints; // CHECK constraints from 88-level
    // Child tables from OCCURS
    std::vector<MappedTable> child_tables;
};

class CopybookMapper {
public:
    struct Options {
        bool flatten_groups;
        bool normalize_occurs;
        bool create_check_constraints;
        std::string table_prefix;

        Options() : flatten_groups(true), normalize_occurs(true),
                    create_check_constraints(true) {}
        Options(bool fg, bool no, bool cc, std::string tp)
            : flatten_groups(fg), normalize_occurs(no),
              create_check_constraints(cc), table_prefix(std::move(tp)) {}
    };

    explicit CopybookMapper(Options opts = {}) : opts_(std::move(opts)) {}

    MappedTable map(const std::vector<CopybookField>& fields) const {
        MappedTable tbl;

        for (const auto& root : fields) {
            if (root.level == 1 || root.level == 77) {
                if (tbl.table_name.empty()) {
                    tbl.table_name = opts_.table_prefix + to_sql_name(root.name);
                }
                map_field(root, "", tbl);
            }
        }

        if (tbl.table_name.empty()) tbl.table_name = opts_.table_prefix + "COPYBOOK_TABLE";
        return tbl;
    }

    static ColumnType pic_to_column_type(const PicClause& pic, CobolUsage usage) {
        ColumnType ct;

        switch (usage) {
            case CobolUsage::COMP:
            case CobolUsage::COMP_4: {
                // Binary: size depends on digits
                int digits = pic.integer_digits + pic.decimal_digits;
                if (digits <= 4) ct.kind = ColumnTypeKind::SMALLINT;
                else if (digits <= 9) ct.kind = ColumnTypeKind::INTEGER;
                else ct.kind = ColumnTypeKind::BIGINT;
                if (pic.decimal_digits > 0) {
                    ct.kind = ColumnTypeKind::DECIMAL;
                    ct.precision = digits;
                    ct.scale = pic.decimal_digits;
                }
                break;
            }
            case CobolUsage::COMP_5: {
                // Native binary: size from PIC length
                int digits = pic.integer_digits + pic.decimal_digits;
                if (digits <= 4) ct.kind = ColumnTypeKind::SMALLINT;
                else if (digits <= 9) ct.kind = ColumnTypeKind::INTEGER;
                else ct.kind = ColumnTypeKind::BIGINT;
                break;
            }
            case CobolUsage::COMP_3: {
                // Packed decimal
                int total_digits = pic.integer_digits + pic.decimal_digits;
                ct.kind = ColumnTypeKind::DECIMAL;
                ct.precision = total_digits;
                ct.scale = pic.decimal_digits;
                break;
            }
            case CobolUsage::COMP_1:
                ct.kind = ColumnTypeKind::REAL;
                break;
            case CobolUsage::COMP_2:
                ct.kind = ColumnTypeKind::DOUBLE;
                break;
            case CobolUsage::DISPLAY:
            default:
                if (pic.is_alpha) {
                    ct.kind = ColumnTypeKind::VARCHAR;
                    ct.precision = pic.total_size;
                } else if (pic.is_numeric) {
                    if (pic.decimal_digits > 0) {
                        ct.kind = ColumnTypeKind::DECIMAL;
                        ct.precision = pic.integer_digits + pic.decimal_digits;
                        ct.scale = pic.decimal_digits;
                    } else {
                        ct.kind = ColumnTypeKind::DECIMAL;
                        ct.precision = pic.integer_digits;
                        ct.scale = 0;
                    }
                } else {
                    ct.kind = ColumnTypeKind::VARCHAR;
                    ct.precision = pic.total_size > 0 ? pic.total_size : 1;
                }
                break;
        }

        return ct;
    }

private:
    Options opts_;

    static std::string to_sql_name(const std::string& cobol_name) {
        std::string result;
        for (char c : cobol_name) {
            if (c == '-') result += '_';
            else result += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        return result;
    }

    void map_field(const CopybookField& field, const std::string& prefix, MappedTable& tbl) const {
        std::string col_name = prefix.empty() ? to_sql_name(field.name) : prefix + "_" + to_sql_name(field.name);

        // Skip FILLER
        if (field.is_filler) return;

        // REDEFINES: skip (or create view) — mapped as comment note only
        if (!field.redefines.empty()) {
            // For simplicity, we skip redefines but note it
            return;
        }

        // OCCURS with normalization: create child table
        if (field.occurs > 0 && opts_.normalize_occurs) {
            MappedTable child;
            child.table_name = tbl.table_name + "_" + to_sql_name(field.name);

            // Add index column
            MappedColumn idx_col;
            idx_col.name = "IDX";
            idx_col.type.kind = ColumnTypeKind::INTEGER;
            child.columns.push_back(idx_col);

            if (field.children.empty()) {
                // Leaf OCCURS — the element itself
                MappedColumn mc;
                mc.name = to_sql_name(field.name);
                mc.type = pic_to_column_type(field.pic, field.usage);
                mc.occurs = field.occurs;
                child.columns.push_back(mc);
            } else {
                // Group OCCURS — map children
                for (const auto& ch : field.children) {
                    map_field_flat(ch, "", child);
                }
            }

            // FK constraint
            Constraint fk;
            fk.kind = ConstraintKind::FOREIGN_KEY;
            fk.columns.push_back("PARENT_ID");
            fk.ref_table = tbl.table_name;
            child.constraints.push_back(fk);

            tbl.child_tables.push_back(std::move(child));
            return;
        }

        // Group item with children
        if (!field.children.empty()) {
            if (opts_.flatten_groups) {
                for (const auto& child : field.children) {
                    if (child.level == 88) {
                        // 88-level on group — skip
                        continue;
                    }
                    map_field(child, col_name, tbl);
                }
            } else {
                MappedColumn mc;
                mc.name = col_name;
                mc.is_group = true;
                mc.type.kind = ColumnTypeKind::VARCHAR;
                mc.type.precision = 255;
                tbl.columns.push_back(mc);
            }
            return;
        }

        // Leaf field
        MappedColumn mc;
        mc.name = col_name;
        mc.type = pic_to_column_type(field.pic, field.usage);

        // 88-level children -> CHECK constraint
        if (opts_.create_check_constraints) {
            for (const auto& child : field.children) {
                if (child.level == 88) {
                    Constraint chk;
                    chk.kind = ConstraintKind::CHECK;
                    std::string expr = col_name + " IN (";
                    for (std::size_t i = 0; i < child.values_88.size(); ++i) {
                        if (child.values_88[i] == "THRU") continue;
                        if (i > 0 && child.values_88[i - 1] != "THRU" && child.values_88[i] != "THRU") expr += ", ";
                        expr += "'" + child.values_88[i] + "'";
                    }
                    expr += ")";
                    chk.check_expr = expr;
                    chk.name = "CHK_" + child.name;
                    tbl.constraints.push_back(chk);
                }
            }
        }

        tbl.columns.push_back(mc);
    }

    void map_field_flat(const CopybookField& field, const std::string& prefix, MappedTable& tbl) const {
        if (field.is_filler) return;
        std::string col_name = prefix.empty() ? to_sql_name(field.name) : prefix + "_" + to_sql_name(field.name);

        if (!field.children.empty()) {
            for (const auto& child : field.children) {
                if (child.level != 88)
                    map_field_flat(child, col_name, tbl);
            }
            return;
        }

        MappedColumn mc;
        mc.name = col_name;
        mc.type = pic_to_column_type(field.pic, field.usage);
        tbl.columns.push_back(mc);
    }
};

}} // namespace lazarus::db2

#endif // LAZARUS_DB2_COPYBOOK_MAPPER_H
