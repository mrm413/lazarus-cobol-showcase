#ifndef LAZARUS_EASYTRIEVE_AST_H
#define LAZARUS_EASYTRIEVE_AST_H

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <memory>
#include <cstdint>

namespace lazarus { namespace easytrieve {

// Forward declarations
struct Expression;
struct Condition;
struct Statement;

// ============================================================
// Field type enumeration (A=alpha, N=numeric, P=packed, B=binary, U=unsigned)
// ============================================================
enum class FieldType { Alpha, Numeric, Packed, Binary, Unsigned };

inline const char* field_type_name(FieldType ft) {
    switch (ft) {
        case FieldType::Alpha:    return "A";
        case FieldType::Numeric:  return "N";
        case FieldType::Packed:   return "P";
        case FieldType::Binary:   return "B";
        case FieldType::Unsigned: return "U";
    }
    return "?";
}

inline FieldType field_type_from_char(char c) {
    switch (c) {
        case 'A': case 'a': return FieldType::Alpha;
        case 'N': case 'n': return FieldType::Numeric;
        case 'P': case 'p': return FieldType::Packed;
        case 'B': case 'b': return FieldType::Binary;
        case 'U': case 'u': return FieldType::Unsigned;
        default: return FieldType::Alpha;
    }
}

// ============================================================
// Expression nodes
// ============================================================
struct FieldRef {
    std::string file_name;   // optional qualifier (FILE-NAME:FIELD-NAME)
    std::string field_name;
};

struct Literal {
    std::string value;
    bool is_numeric;
};

enum class ArithOp { Add, Sub, Mul, Div };

inline const char* arith_op_symbol(ArithOp op) {
    switch (op) {
        case ArithOp::Add: return "+";
        case ArithOp::Sub: return "-";
        case ArithOp::Mul: return "*";
        case ArithOp::Div: return "/";
    }
    return "?";
}

struct Arithmetic {
    std::shared_ptr<Expression> left;
    ArithOp op;
    std::shared_ptr<Expression> right;
};

struct Concatenation {
    std::vector<std::shared_ptr<Expression>> parts;
};

using ExpressionVariant = std::variant<FieldRef, Literal, Arithmetic, Concatenation>;

struct Expression {
    ExpressionVariant value;

    Expression() : value(Literal{"", false}) {}
    explicit Expression(ExpressionVariant v) : value(std::move(v)) {}

    bool is_field_ref() const { return std::holds_alternative<FieldRef>(value); }
    bool is_literal() const { return std::holds_alternative<Literal>(value); }
    bool is_arithmetic() const { return std::holds_alternative<Arithmetic>(value); }
    bool is_concatenation() const { return std::holds_alternative<Concatenation>(value); }

    const FieldRef& as_field_ref() const { return std::get<FieldRef>(value); }
    const Literal& as_literal() const { return std::get<Literal>(value); }
    const Arithmetic& as_arithmetic() const { return std::get<Arithmetic>(value); }
    const Concatenation& as_concatenation() const { return std::get<Concatenation>(value); }
};

inline Expression make_field_ref(const std::string& name, const std::string& file = "") {
    return Expression{FieldRef{file, name}};
}

inline Expression make_literal(const std::string& val, bool numeric = false) {
    return Expression{Literal{val, numeric}};
}

inline Expression make_arithmetic(std::shared_ptr<Expression> l, ArithOp op,
                                   std::shared_ptr<Expression> r) {
    return Expression{Arithmetic{std::move(l), op, std::move(r)}};
}

// ============================================================
// Condition nodes
// ============================================================
enum class CompareOp { Eq, Ne, Lt, Le, Gt, Ge };

inline const char* compare_op_name(CompareOp op) {
    switch (op) {
        case CompareOp::Eq: return "EQ";
        case CompareOp::Ne: return "NE";
        case CompareOp::Lt: return "LT";
        case CompareOp::Le: return "LE";
        case CompareOp::Gt: return "GT";
        case CompareOp::Ge: return "GE";
    }
    return "?";
}

inline const char* compare_op_cpp(CompareOp op) {
    switch (op) {
        case CompareOp::Eq: return "==";
        case CompareOp::Ne: return "!=";
        case CompareOp::Lt: return "<";
        case CompareOp::Le: return "<=";
        case CompareOp::Gt: return ">";
        case CompareOp::Ge: return ">=";
    }
    return "?";
}

struct Compare {
    Expression left;
    CompareOp op;
    Expression right;
};

struct Range {
    Expression field;
    Expression low;
    Expression high;
};

struct Mask {
    Expression field;
    std::string pattern;
};

struct ConditionAnd;
struct ConditionOr;
struct ConditionNot;

using ConditionVariant = std::variant<
    Compare, Range, Mask,
    std::shared_ptr<ConditionAnd>,
    std::shared_ptr<ConditionOr>,
    std::shared_ptr<ConditionNot>
>;

struct Condition {
    ConditionVariant value;

    Condition() : value(Compare{make_literal("0", true), CompareOp::Eq, make_literal("0", true)}) {}
    explicit Condition(ConditionVariant v) : value(std::move(v)) {}

    bool is_compare() const { return std::holds_alternative<Compare>(value); }
    bool is_range() const { return std::holds_alternative<Range>(value); }
    bool is_mask() const { return std::holds_alternative<Mask>(value); }
};

struct ConditionAnd {
    Condition left;
    Condition right;
};

struct ConditionOr {
    Condition left;
    Condition right;
};

struct ConditionNot {
    Condition operand;
};

inline Condition make_compare(Expression l, CompareOp op, Expression r) {
    return Condition{Compare{std::move(l), op, std::move(r)}};
}

inline Condition make_and(Condition l, Condition r) {
    return Condition{std::make_shared<ConditionAnd>(ConditionAnd{std::move(l), std::move(r)})};
}

inline Condition make_or(Condition l, Condition r) {
    return Condition{std::make_shared<ConditionOr>(ConditionOr{std::move(l), std::move(r)})};
}

inline Condition make_not(Condition c) {
    return Condition{std::make_shared<ConditionNot>(ConditionNot{std::move(c)})};
}

inline Condition make_range(Expression f, Expression lo, Expression hi) {
    return Condition{Range{std::move(f), std::move(lo), std::move(hi)}};
}

inline Condition make_mask(Expression f, const std::string& pat) {
    return Condition{Mask{std::move(f), pat}};
}

// ============================================================
// Declaration nodes
// ============================================================
struct FieldDecl {
    std::string name;
    int position;       // 1-based starting position (0 = auto)
    int length;
    FieldType type;
    int decimals;       // decimal places for numeric
    std::string heading; // optional column heading
    std::string initial_value;

    FieldDecl() : position(0), length(0), type(FieldType::Alpha), decimals(0) {}
    FieldDecl(const std::string& n, int pos, int len, FieldType t, int dec = 0)
        : name(n), position(pos), length(len), type(t), decimals(dec) {}
};

struct RecordDecl {
    std::string name;
    std::string file_name;
    std::vector<FieldDecl> fields;
};

struct FileDecl {
    std::string name;
    std::string ddname;             // DD name for JCL association
    int lrecl;                      // logical record length
    bool is_virtual;                // VIRTUAL file (in-memory)
    bool is_indexed;                // indexed (VSAM KSDS)
    std::string key_field;          // key field name for indexed
    std::vector<RecordDecl> records;
    std::vector<FieldDecl> fields;  // direct fields (no explicit record)

    FileDecl() : lrecl(0), is_virtual(false), is_indexed(false) {}
};

struct WorkingField {
    std::string name;
    int length;
    FieldType type;
    int decimals;
    std::string initial_value;
    std::string mask;               // edit mask

    WorkingField() : length(0), type(FieldType::Alpha), decimals(0) {}
    WorkingField(const std::string& n, int len, FieldType t, int dec = 0,
                 const std::string& init = "")
        : name(n), length(len), type(t), decimals(dec), initial_value(init) {}
};

struct TableDecl {
    std::string name;
    std::vector<FieldDecl> key_fields;
    std::vector<FieldDecl> data_fields;
    std::vector<std::vector<std::string>> entries; // row data
};

// ============================================================
// Report elements
// ============================================================
struct TitleLine {
    int line_number;
    std::vector<Expression> elements;
};

struct HeadingLine {
    int line_number;
    std::vector<Expression> elements;
};

struct DetailLine {
    std::string name;
    std::vector<Expression> elements;
};

struct SumField {
    std::string field_name;
    std::string label;
};

struct ControlBreak {
    std::string field_name;
    int level;               // break level (1 = highest)
    std::vector<SumField> sums;
    std::vector<Expression> footer_elements;

    ControlBreak() : level(0) {}
};

// ============================================================
// Activity nodes
// ============================================================
struct JobActivity {
    std::string name;
    std::string input_file;
    std::vector<std::shared_ptr<Statement>> statements;
};

struct SortActivity {
    std::string name;
    std::string input_file;
    std::string output_file;
    std::vector<std::string> sort_fields;
    std::vector<bool> ascending;         // per field
    std::vector<std::shared_ptr<Statement>> before_statements;
    std::vector<std::shared_ptr<Statement>> after_statements;
};

struct ReportActivity {
    std::string name;
    std::string input_file;
    std::vector<TitleLine> titles;
    std::vector<HeadingLine> headings;
    std::vector<DetailLine> detail_lines;
    std::vector<ControlBreak> control_breaks;
    int lines_per_page;
    std::string sequence_field;

    ReportActivity() : lines_per_page(60) {}
};

// ============================================================
// Statement nodes
// ============================================================
struct IfStmt {
    Condition condition;
    std::vector<std::shared_ptr<Statement>> then_branch;
    std::vector<std::shared_ptr<Statement>> else_branch;
};

struct DoStmt {
    std::string var_name;
    Expression start_val;
    Expression end_val;
    Expression step_val;
    std::vector<std::shared_ptr<Statement>> body;
    bool is_while;
    std::optional<Condition> while_condition;

    DoStmt() : is_while(false) {}
};

struct PerformStmt {
    std::string proc_name;
};

struct GoToStmt {
    std::string label;
};

struct GetStmt {
    std::string file_name;
};

struct PutStmt {
    std::string file_name;
};

struct PointStmt {
    std::string file_name;
    Expression key_value;
};

struct ReadStmt {
    std::string file_name;
};

struct WriteStmt {
    std::string file_name;
};

struct DisplayStmt {
    std::vector<Expression> items;
    bool newline;
    DisplayStmt() : newline(true) {}
    DisplayStmt(std::vector<Expression> i, bool nl) : items(std::move(i)), newline(nl) {}
};

struct MoveStmt {
    Expression source;
    Expression target;
};

struct SearchStmt {
    std::string table_name;
    Expression search_key;
    std::string result_field;
};

struct StopStmt {};

struct PrintStmt {
    std::string report_name;
    std::string line_name;
};

using StatementVariant = std::variant<
    IfStmt, DoStmt, PerformStmt, GoToStmt,
    GetStmt, PutStmt, PointStmt, ReadStmt, WriteStmt,
    DisplayStmt, MoveStmt, SearchStmt, StopStmt, PrintStmt
>;

struct Statement {
    StatementVariant value;
    std::string label;   // optional label prefix

    Statement() : value(StopStmt{}) {}
    explicit Statement(StatementVariant v, const std::string& lbl = "")
        : value(std::move(v)), label(lbl) {}

    bool is_if() const { return std::holds_alternative<IfStmt>(value); }
    bool is_do() const { return std::holds_alternative<DoStmt>(value); }
    bool is_perform() const { return std::holds_alternative<PerformStmt>(value); }
    bool is_goto() const { return std::holds_alternative<GoToStmt>(value); }
    bool is_get() const { return std::holds_alternative<GetStmt>(value); }
    bool is_put() const { return std::holds_alternative<PutStmt>(value); }
    bool is_point() const { return std::holds_alternative<PointStmt>(value); }
    bool is_read() const { return std::holds_alternative<ReadStmt>(value); }
    bool is_write() const { return std::holds_alternative<WriteStmt>(value); }
    bool is_display() const { return std::holds_alternative<DisplayStmt>(value); }
    bool is_move() const { return std::holds_alternative<MoveStmt>(value); }
    bool is_search() const { return std::holds_alternative<SearchStmt>(value); }
    bool is_stop() const { return std::holds_alternative<StopStmt>(value); }
    bool is_print() const { return std::holds_alternative<PrintStmt>(value); }
};

inline std::shared_ptr<Statement> make_stmt(StatementVariant v, const std::string& lbl = "") {
    return std::make_shared<Statement>(std::move(v), lbl);
}

// ============================================================
// Procedure definition (PROC)
// ============================================================
struct ProcDecl {
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::shared_ptr<Statement>> body;
};

// ============================================================
// Macro definition
// ============================================================
struct MacroDef {
    std::string name;
    std::vector<std::string> parameters;
    std::string body_text;
};

// ============================================================
// Top-level program
// ============================================================
struct Program {
    std::vector<FileDecl> files;
    std::vector<WorkingField> working_fields;
    std::vector<TableDecl> tables;
    std::vector<JobActivity> jobs;
    std::vector<SortActivity> sorts;
    std::vector<ReportActivity> reports;
    std::vector<ProcDecl> procs;
    std::vector<MacroDef> macros;
};

}} // namespace lazarus::easytrieve

#endif // LAZARUS_EASYTRIEVE_AST_H
