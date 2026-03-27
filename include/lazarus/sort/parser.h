// Lazarus Systems — DFSORT/SYNCSORT control card parser
// Tier 2: Sort control card parsing with full AST generation

#ifndef LAZARUS_SORT_PARSER_H
#define LAZARUS_SORT_PARSER_H

#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <cstdint>
#include <sstream>

namespace lazarus { namespace sort { namespace parser {

// ---------- Format types used in SORT FIELDS ----------
enum class FieldFormat {
    CH,   // Character
    ZD,   // Zoned Decimal
    PD,   // Packed Decimal
    BI,   // Binary (unsigned)
    FI,   // Fixed-point Integer (signed)
    FL,   // Floating point
    AC,   // ASCII Character
    CSF,  // Numeric with optional leading sign
    CSL,  // Numeric with leading separate sign
    CST   // Numeric with trailing separate sign
};

inline std::string format_to_string(FieldFormat f) {
    switch (f) {
        case FieldFormat::CH:  return "CH";
        case FieldFormat::ZD:  return "ZD";
        case FieldFormat::PD:  return "PD";
        case FieldFormat::BI:  return "BI";
        case FieldFormat::FI:  return "FI";
        case FieldFormat::FL:  return "FL";
        case FieldFormat::AC:  return "AC";
        case FieldFormat::CSF: return "CSF";
        case FieldFormat::CSL: return "CSL";
        case FieldFormat::CST: return "CST";
    }
    return "CH";
}

inline FieldFormat parse_format(std::string_view s) {
    if (s == "CH")  return FieldFormat::CH;
    if (s == "ZD")  return FieldFormat::ZD;
    if (s == "PD")  return FieldFormat::PD;
    if (s == "BI")  return FieldFormat::BI;
    if (s == "FI")  return FieldFormat::FI;
    if (s == "FL")  return FieldFormat::FL;
    if (s == "AC")  return FieldFormat::AC;
    if (s == "CSF") return FieldFormat::CSF;
    if (s == "CSL") return FieldFormat::CSL;
    if (s == "CST") return FieldFormat::CST;
    throw std::runtime_error(std::string("Unknown format: ") + std::string(s));
}

// Sort order
enum class SortOrder { ASC, DESC };

// ---------- Sort field definition ----------
struct SortField {
    int position;      // 1-based byte position
    int length;        // byte length
    FieldFormat format;
    SortOrder order;
};

// ---------- Condition operators ----------
enum class CondOp {
    EQ, NE, GT, GE, LT, LE,
    SS,  // Substring
    NUM, // Numeric check
};

inline CondOp parse_cond_op(std::string_view s) {
    if (s == "EQ") return CondOp::EQ;
    if (s == "NE") return CondOp::NE;
    if (s == "GT") return CondOp::GT;
    if (s == "GE") return CondOp::GE;
    if (s == "LT") return CondOp::LT;
    if (s == "LE") return CondOp::LE;
    if (s == "SS") return CondOp::SS;
    if (s == "NUM") return CondOp::NUM;
    throw std::runtime_error(std::string("Unknown condition operator: ") + std::string(s));
}

inline std::string condop_to_string(CondOp op) {
    switch (op) {
        case CondOp::EQ: return "EQ";
        case CondOp::NE: return "NE";
        case CondOp::GT: return "GT";
        case CondOp::GE: return "GE";
        case CondOp::LT: return "LT";
        case CondOp::LE: return "LE";
        case CondOp::SS: return "SS";
        case CondOp::NUM: return "NUM";
    }
    return "EQ";
}

// Logical connectives for compound conditions
enum class LogicOp { AND, OR };

// ---------- Condition node ----------
struct SimpleCondition {
    int position;       // field position (1-based)
    int length;         // field length
    FieldFormat format;
    CondOp op;
    std::string value;  // comparison value (literal)
};

struct CompoundCondition;

using Condition = std::variant<SimpleCondition, CompoundCondition>;

struct CompoundCondition {
    LogicOp logic;
    std::vector<Condition> children;
};

// ---------- INREC/OUTREC field reference ----------
struct FieldRef {
    int position;   // 1-based; 0 means literal
    int length;
    std::string literal; // non-empty if this is a literal insertion
};

struct RecordReformat {
    enum class Type { BUILD, OVERLAY };
    Type type;
    std::vector<FieldRef> fields;
};

// ---------- SUM field ----------
struct SumField {
    int position;
    int length;
    FieldFormat format;
};

// ---------- OPTION flags ----------
struct SortOptions {
    bool copy = false;
    bool equals = true;       // EQUALS is default
    int skip_rec = 0;         // SKIPREC=n
    int stop_aft = 0;         // STOPAFT=n
};

// ---------- Complete sort specification ----------
struct SortSpec {
    enum class Operation { SORT, MERGE, COPY };
    Operation operation = Operation::SORT;

    std::vector<SortField> sort_fields;
    std::optional<Condition> include_cond;
    std::optional<Condition> omit_cond;
    std::optional<RecordReformat> inrec;
    std::optional<RecordReformat> outrec;
    std::vector<SumField> sum_fields;
    SortOptions options;
};

// ================================================================
// Tokenizer: splits control card text into tokens
// ================================================================
class Tokenizer {
public:
    explicit Tokenizer(std::string_view input) : src_(input), pos_(0) {}

    std::string_view peek() {
        skip_ws();
        if (pos_ >= src_.size()) return {};
        size_t start = pos_;
        // Delimiters
        char c = src_[pos_];
        if (c == '(' || c == ')' || c == ',' || c == '=') {
            return src_.substr(start, 1);
        }
        // Quoted string
        if (c == '\'' || c == '"') {
            return scan_quoted();
        }
        // Token: run of non-delimiter, non-ws
        size_t end = pos_;
        while (end < src_.size() && !is_delim(src_[end]) && !std::isspace(static_cast<unsigned char>(src_[end]))) {
            ++end;
        }
        return src_.substr(start, end - start);
    }

    std::string_view next() {
        auto tok = peek();
        pos_ += tok.size();
        if (!tok.empty() && (tok.front() == '\'' || tok.front() == '"')) {
            // size includes quotes
        }
        return tok;
    }

    bool match(std::string_view expected) {
        auto tok = peek();
        if (tok == expected) { next(); return true; }
        return false;
    }

    void expect(std::string_view expected) {
        auto tok = next();
        if (tok != expected) {
            throw std::runtime_error(
                std::string("Expected '") + std::string(expected) +
                "' but got '" + std::string(tok) + "'");
        }
    }

    bool at_end() { skip_ws(); return pos_ >= src_.size(); }

    size_t position() const { return pos_; }

private:
    std::string_view src_;
    size_t pos_;

    void skip_ws() {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
            ++pos_;
        }
    }

    bool is_delim(char c) const {
        return c == '(' || c == ')' || c == ',' || c == '=';
    }

    std::string_view scan_quoted() {
        char q = src_[pos_];
        size_t start = pos_;
        size_t end = pos_ + 1;
        while (end < src_.size() && src_[end] != q) ++end;
        if (end < src_.size()) ++end; // include closing quote
        return src_.substr(start, end - start);
    }
};

// Utility: uppercase a string_view into a string
inline std::string to_upper(std::string_view s) {
    std::string r(s);
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return r;
}

// Utility: parse integer from string_view
inline int parse_int(std::string_view s) {
    int val = 0;
    bool neg = false;
    size_t i = 0;
    if (i < s.size() && s[i] == '-') { neg = true; ++i; }
    for (; i < s.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(s[i])))
            throw std::runtime_error(std::string("Invalid integer: ") + std::string(s));
        val = val * 10 + (s[i] - '0');
    }
    return neg ? -val : val;
}

// Utility: strip quotes from a string_view
inline std::string_view strip_quotes(std::string_view s) {
    if (s.size() >= 2 && (s.front() == '\'' || s.front() == '"') && s.back() == s.front()) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

// ================================================================
// Parser: parses control card statements into SortSpec
// ================================================================
class SortParser {
public:
    SortSpec parse(std::string_view input) {
        SortSpec spec;
        // Split into lines/statements
        auto lines = split_lines(input);
        for (auto& line : lines) {
            parse_statement(line, spec);
        }
        return spec;
    }

    // Parse individual statement types
    std::vector<SortField> parse_sort_fields(std::string_view input) {
        Tokenizer tok(input);
        return parse_field_list(tok);
    }

    Condition parse_condition(std::string_view input) {
        Tokenizer tok(input);
        return parse_cond_expr(tok);
    }

    RecordReformat parse_reformat(std::string_view input) {
        Tokenizer tok(input);
        return parse_reformat_spec(tok);
    }

private:
    std::vector<std::string> split_lines(std::string_view input) {
        std::vector<std::string> lines;
        std::string current;
        for (size_t i = 0; i < input.size(); ++i) {
            if (input[i] == '\n' || input[i] == '\r') {
                if (!current.empty()) {
                    // Trim
                    auto trimmed = trim(current);
                    if (!trimmed.empty()) lines.push_back(std::string(trimmed));
                    current.clear();
                }
                if (input[i] == '\r' && i + 1 < input.size() && input[i + 1] == '\n') ++i;
            } else {
                current += input[i];
            }
        }
        if (!current.empty()) {
            auto trimmed = trim(current);
            if (!trimmed.empty()) lines.push_back(std::string(trimmed));
        }
        return lines;
    }

    std::string_view trim(const std::string& s) {
        size_t start = 0, end = s.size();
        while (start < end && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
        while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
        return std::string_view(s).substr(start, end - start);
    }

    void parse_statement(const std::string& line, SortSpec& spec) {
        Tokenizer tok(line);
        auto keyword = to_upper(tok.next());

        if (keyword == "SORT") {
            spec.operation = SortSpec::Operation::SORT;
            tok.expect("FIELDS");
            tok.expect("=");
            if (tok.match("COPY")) {
                spec.options.copy = true;
            } else {
                tok.expect("(");
                spec.sort_fields = parse_field_list_inner(tok);
                tok.expect(")");
            }
        } else if (keyword == "MERGE") {
            spec.operation = SortSpec::Operation::MERGE;
            tok.expect("FIELDS");
            tok.expect("=");
            tok.expect("(");
            spec.sort_fields = parse_field_list_inner(tok);
            tok.expect(")");
        } else if (keyword == "INCLUDE") {
            tok.expect("COND");
            tok.expect("=");
            tok.expect("(");
            spec.include_cond = parse_cond_expr(tok);
            tok.expect(")");
        } else if (keyword == "OMIT") {
            tok.expect("COND");
            tok.expect("=");
            tok.expect("(");
            spec.omit_cond = parse_cond_expr(tok);
            tok.expect(")");
        } else if (keyword == "INREC") {
            spec.inrec = parse_reformat_clause(tok);
        } else if (keyword == "OUTREC") {
            spec.outrec = parse_reformat_clause(tok);
        } else if (keyword == "SUM") {
            tok.expect("FIELDS");
            tok.expect("=");
            if (tok.match("NONE")) {
                // SUM FIELDS=NONE means remove duplicates (no summation)
            } else {
                tok.expect("(");
                spec.sum_fields = parse_sum_fields(tok);
                tok.expect(")");
            }
        } else if (keyword == "OPTION") {
            parse_options(tok, spec.options);
        }
    }

    std::vector<SortField> parse_field_list(Tokenizer& tok) {
        tok.expect("(");
        auto fields = parse_field_list_inner(tok);
        tok.expect(")");
        return fields;
    }

    std::vector<SortField> parse_field_list_inner(Tokenizer& tok) {
        std::vector<SortField> fields;
        while (true) {
            SortField f{};
            f.position = parse_int(tok.next());
            tok.expect(",");
            f.length = parse_int(tok.next());
            tok.expect(",");
            f.format = parse_format(to_upper(tok.next()));
            tok.expect(",");
            auto ord = to_upper(tok.next());
            f.order = (ord == "D" || ord == "DESC") ? SortOrder::DESC : SortOrder::ASC;
            fields.push_back(f);
            if (!tok.match(",")) break;
            // Check if next token looks like a position (digit)
            auto pk = tok.peek();
            if (pk.empty() || pk == ")") break;
        }
        return fields;
    }

    Condition parse_cond_expr(Tokenizer& tok) {
        auto left = parse_simple_cond(tok);
        auto pk = tok.peek();
        auto pku = to_upper(pk);
        if (pku == "AND" || pku == "OR") {
            CompoundCondition comp;
            comp.logic = (pku == "AND") ? LogicOp::AND : LogicOp::OR;
            comp.children.push_back(left);
            while (true) {
                pk = tok.peek();
                pku = to_upper(pk);
                if (pku == "AND" || pku == "OR") {
                    tok.next();
                    comp.children.push_back(parse_simple_cond(tok));
                } else {
                    break;
                }
            }
            return comp;
        }
        return left;
    }

    Condition parse_simple_cond(Tokenizer& tok) {
        SimpleCondition sc{};
        // (pos,len,fmt,op,value) — but opening paren may already be consumed
        bool had_paren = tok.match("(");
        sc.position = parse_int(tok.next());
        tok.expect(",");
        sc.length = parse_int(tok.next());
        tok.expect(",");
        sc.format = parse_format(to_upper(tok.next()));
        tok.expect(",");
        sc.op = parse_cond_op(to_upper(tok.next()));
        tok.expect(",");
        auto val = tok.next();
        sc.value = std::string(strip_quotes(val));
        if (had_paren) tok.expect(")");
        return sc;
    }

    RecordReformat parse_reformat_clause(Tokenizer& tok) {
        RecordReformat rf;
        auto kw = to_upper(tok.peek());
        if (kw == "BUILD" || kw == "FIELDS") {
            tok.next();
            rf.type = RecordReformat::Type::BUILD;
        } else if (kw == "OVERLAY") {
            tok.next();
            rf.type = RecordReformat::Type::OVERLAY;
        } else {
            rf.type = RecordReformat::Type::BUILD;
        }
        tok.expect("=");
        tok.expect("(");
        rf.fields = parse_field_refs(tok);
        tok.expect(")");
        return rf;
    }

    RecordReformat parse_reformat_spec(Tokenizer& tok) {
        RecordReformat rf;
        auto kw = to_upper(tok.peek());
        if (kw == "BUILD" || kw == "FIELDS") {
            tok.next();
            rf.type = RecordReformat::Type::BUILD;
        } else if (kw == "OVERLAY") {
            tok.next();
            rf.type = RecordReformat::Type::OVERLAY;
        } else {
            rf.type = RecordReformat::Type::BUILD;
        }
        tok.expect("=");
        tok.expect("(");
        rf.fields = parse_field_refs(tok);
        tok.expect(")");
        return rf;
    }

    std::vector<FieldRef> parse_field_refs(Tokenizer& tok) {
        std::vector<FieldRef> refs;
        while (true) {
            auto pk = tok.peek();
            if (pk.empty() || pk == ")") break;
            FieldRef fr{};
            if (pk.front() == '\'' || pk.front() == '"') {
                // Literal string
                auto lit = tok.next();
                fr.position = 0;
                fr.length = 0;
                fr.literal = std::string(strip_quotes(lit));
            } else if (std::isdigit(static_cast<unsigned char>(pk.front()))) {
                // pos,len or pos:len
                fr.position = parse_int(tok.next());
                tok.expect(",");
                fr.length = parse_int(tok.next());
                fr.literal.clear();
            } else {
                // skip unknown
                tok.next();
                continue;
            }
            refs.push_back(fr);
            if (!tok.match(",")) break;
        }
        return refs;
    }

    std::vector<SumField> parse_sum_fields(Tokenizer& tok) {
        std::vector<SumField> fields;
        while (true) {
            SumField sf{};
            sf.position = parse_int(tok.next());
            tok.expect(",");
            sf.length = parse_int(tok.next());
            tok.expect(",");
            sf.format = parse_format(to_upper(tok.next()));
            fields.push_back(sf);
            if (!tok.match(",")) break;
            auto pk = tok.peek();
            if (pk.empty() || pk == ")") break;
        }
        return fields;
    }

    void parse_options(Tokenizer& tok, SortOptions& opts) {
        while (!tok.at_end()) {
            auto kw = to_upper(tok.next());
            if (kw == "COPY") {
                opts.copy = true;
            } else if (kw == "EQUALS") {
                opts.equals = true;
            } else if (kw == "NOEQUALS") {
                opts.equals = false;
            } else if (kw == "SKIPREC") {
                tok.expect("=");
                opts.skip_rec = parse_int(tok.next());
            } else if (kw == "STOPAFT") {
                tok.expect("=");
                opts.stop_aft = parse_int(tok.next());
            }
            tok.match(",");
        }
    }
};

}}} // namespace lazarus::sort::parser

#endif // LAZARUS_SORT_PARSER_H
