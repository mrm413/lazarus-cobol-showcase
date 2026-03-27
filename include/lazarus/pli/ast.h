#ifndef LAZARUS_PLI_AST_H
#define LAZARUS_PLI_AST_H

#include <string>
#include <vector>
#include <variant>
#include <memory>
#include <optional>
#include <cstdint>

// Windows headers define UNALIGNED as a macro; neutralize it
#ifdef UNALIGNED
#undef UNALIGNED
#endif

namespace lazarus { namespace pli {

// Forward declarations
struct Expression;
struct Statement;
struct Declaration;

using ExprPtr = std::shared_ptr<Expression>;
using StmtPtr = std::shared_ptr<Statement>;
using DeclPtr = std::shared_ptr<Declaration>;

// ---------- Data attributes ----------

enum class BaseType {
    FIXED, FLOAT, CHAR, BIT, POINTER, ENTRY, FILE, LABEL,
    AREA, OFFSET, PICTURE, COMPLEX, UNKNOWN
};

enum class Scale { DECIMAL, BINARY, NONE };

enum class StorageClass { AUTOMATIC, STATIC, CONTROLLED, BASED, DEFINED, NONE };

enum class Alignment { ALIGNED, UNALIGNED, NONE };

enum class Scope { INTERNAL, EXTERNAL, NONE };

struct DataAttributes {
    BaseType base_type = BaseType::UNKNOWN;
    Scale scale = Scale::NONE;
    int precision = 0;
    int scale_factor = 0;
    bool varying = false;
    int char_length = 0;
    int bit_length = 0;
    StorageClass storage = StorageClass::NONE;
    Alignment alignment = Alignment::NONE;
    Scope scope = Scope::NONE;
    std::string picture;
    std::string based_var;
    std::string defined_var;
    bool is_initial = false;
    std::string initial_value;
    bool returns_specified = false;
    BaseType returns_type = BaseType::UNKNOWN;
    bool recursive = false;
    bool options_main = false;
};

// ---------- Expression nodes ----------

struct LiteralExpr {
    enum Kind { INTEGER, FLOAT_LIT, STRING, BIT_STRING, COMPLEX_LIT };
    Kind kind;
    std::string value;
};

struct IdentifierExpr {
    std::string name;
};

enum class BinOp {
    ADD, SUB, MUL, DIV, POWER, CONCAT,
    EQ, NE, LT, GT, LE, GE,
    AND, OR, NOT // NOT used as unary too
};

inline const char* binop_name(BinOp op) {
    switch (op) {
        case BinOp::ADD: return "+";
        case BinOp::SUB: return "-";
        case BinOp::MUL: return "*";
        case BinOp::DIV: return "/";
        case BinOp::POWER: return "**";
        case BinOp::CONCAT: return "||";
        case BinOp::EQ: return "=";
        case BinOp::NE: return "^=";
        case BinOp::LT: return "<";
        case BinOp::GT: return ">";
        case BinOp::LE: return "<=";
        case BinOp::GE: return ">=";
        case BinOp::AND: return "&";
        case BinOp::OR: return "|";
        case BinOp::NOT: return "^";
    }
    return "?";
}

enum class UnaryOp { NEGATE, NOT, PLUS };

struct BinaryOpExpr {
    BinOp op;
    ExprPtr left;
    ExprPtr right;
};

struct UnaryOpExpr {
    UnaryOp op;
    ExprPtr operand;
};

struct FunctionCallExpr {
    std::string name;
    std::vector<ExprPtr> args;
};

struct SubscriptExpr {
    ExprPtr base;
    std::vector<ExprPtr> indices;
};

struct MemberAccessExpr {
    ExprPtr object;
    std::string member;
};

struct BuiltinCallExpr {
    std::string name;
    std::vector<ExprPtr> args;
};

using ExprVariant = std::variant<
    LiteralExpr,
    IdentifierExpr,
    BinaryOpExpr,
    UnaryOpExpr,
    FunctionCallExpr,
    SubscriptExpr,
    MemberAccessExpr,
    BuiltinCallExpr
>;

struct Expression {
    ExprVariant node;
    int line = 0;

    Expression() : node(LiteralExpr{LiteralExpr::INTEGER, "0"}) {}
    explicit Expression(ExprVariant n, int ln = 0) : node(std::move(n)), line(ln) {}
};

inline ExprPtr make_expr(ExprVariant v, int line = 0) {
    return std::make_shared<Expression>(std::move(v), line);
}

inline ExprPtr make_literal(LiteralExpr::Kind k, const std::string& val, int line = 0) {
    return make_expr(LiteralExpr{k, val}, line);
}

inline ExprPtr make_ident(const std::string& name, int line = 0) {
    return make_expr(IdentifierExpr{name}, line);
}

inline ExprPtr make_binop(BinOp op, ExprPtr l, ExprPtr r, int line = 0) {
    return make_expr(BinaryOpExpr{op, std::move(l), std::move(r)}, line);
}

inline ExprPtr make_unaryop(UnaryOp op, ExprPtr operand, int line = 0) {
    return make_expr(UnaryOpExpr{op, std::move(operand)}, line);
}

inline ExprPtr make_call(const std::string& name, std::vector<ExprPtr> args, int line = 0) {
    return make_expr(FunctionCallExpr{name, std::move(args)}, line);
}

inline ExprPtr make_builtin(const std::string& name, std::vector<ExprPtr> args, int line = 0) {
    return make_expr(BuiltinCallExpr{name, std::move(args)}, line);
}

// ---------- Format items for PUT/GET EDIT ----------

struct FormatItem {
    enum Kind { A, F, E, B, P, X, SKIP, PAGE, LINE_FMT, COLUMN, TAB, STRING_FMT, R };
    Kind kind;
    int w = 0;
    int d = 0;
    int s = 0;
    std::string picture;
};

// ---------- Statement nodes ----------

struct AssignmentStmt {
    ExprPtr target;
    ExprPtr value;
};

struct IfStmt {
    ExprPtr condition;
    std::vector<StmtPtr> then_body;
    std::vector<StmtPtr> else_body;
};

struct WhenClause {
    std::vector<ExprPtr> values;  // empty = OTHERWISE
    std::vector<StmtPtr> body;
};

struct SelectStmt {
    ExprPtr expr;  // nullptr = SELECT without expression
    std::vector<WhenClause> whens;
    std::vector<StmtPtr> otherwise_body;
};

struct DoLoopStmt {
    enum Kind { SIMPLE, ITERATIVE, WHILE, UNTIL, FOREVER };
    Kind kind;
    std::string variable;
    ExprPtr from;
    ExprPtr to;
    ExprPtr by;
    ExprPtr while_cond;
    ExprPtr until_cond;
    std::vector<StmtPtr> body;
};

struct CallStmt {
    std::string target;
    std::vector<ExprPtr> args;
};

struct ReturnStmt {
    ExprPtr value;  // may be nullptr
};

struct PutListStmt {
    std::vector<ExprPtr> items;
    bool skip = false;
    std::string target_string;  // for PUT STRING
};

struct PutEditStmt {
    std::vector<ExprPtr> items;
    std::vector<FormatItem> formats;
    std::string target_string;
};

struct GetListStmt {
    std::vector<ExprPtr> targets;
    std::string source_string;  // for GET STRING
};

struct GetEditStmt {
    std::vector<ExprPtr> targets;
    std::vector<FormatItem> formats;
    std::string source_string;
};

struct OpenStmt {
    std::string file_name;
    enum Mode { INPUT, OUTPUT, UPDATE, STREAM_INPUT, STREAM_OUTPUT, PRINT };
    Mode mode;
};

struct CloseStmt {
    std::string file_name;
};

struct ReadStmt {
    std::string file_name;
    ExprPtr into;
    ExprPtr key;
};

struct WriteStmt {
    std::string file_name;
    ExprPtr from;
    ExprPtr keyfrom;
};

struct RewriteStmt {
    std::string file_name;
    ExprPtr from;
};

struct DeleteStmt {
    std::string file_name;
    ExprPtr key;
};

struct AllocateStmt {
    std::string variable;
    ExprPtr set_target;  // pointer to SET into
};

struct FreeStmt {
    std::string variable;
};

struct OnStmt {
    std::string condition;
    std::vector<StmtPtr> body;  // empty = ON condition SYSTEM
    bool is_system = false;
};

struct SignalStmt {
    std::string condition;
};

struct RevertStmt {
    std::string condition;
};

struct StopStmt {
    // no fields needed
};

struct DisplayStmt {
    std::vector<ExprPtr> items;
};

struct GoToStmt {
    std::string label;
};

struct NullStmt {
    // no-op
};

struct LeaveStmt {
    std::string label;  // optional label
};

struct IterateStmt {
    std::string label;  // optional label
};

struct EntryStmt {
    std::string name;
    std::vector<std::string> params;
};

using StmtVariant = std::variant<
    AssignmentStmt,
    IfStmt,
    SelectStmt,
    DoLoopStmt,
    CallStmt,
    ReturnStmt,
    PutListStmt,
    PutEditStmt,
    GetListStmt,
    GetEditStmt,
    OpenStmt,
    CloseStmt,
    ReadStmt,
    WriteStmt,
    RewriteStmt,
    DeleteStmt,
    AllocateStmt,
    FreeStmt,
    OnStmt,
    SignalStmt,
    RevertStmt,
    StopStmt,
    DisplayStmt,
    GoToStmt,
    NullStmt,
    LeaveStmt,
    IterateStmt,
    EntryStmt
>;

struct Statement {
    StmtVariant node;
    int line = 0;

    Statement() : node(NullStmt{}) {}
    explicit Statement(StmtVariant n, int ln = 0) : node(std::move(n)), line(ln) {}
};

inline StmtPtr make_stmt(StmtVariant v, int line = 0) {
    return std::make_shared<Statement>(std::move(v), line);
}

// ---------- Declaration nodes ----------

struct ScalarDecl {
    std::string name;
    DataAttributes attrs;
};

struct ArrayDimension {
    int lower = 1;
    int upper = 0;
};

struct ArrayDecl {
    std::string name;
    std::vector<ArrayDimension> dimensions;
    DataAttributes attrs;
};

struct StructureMember {
    int level;
    std::string name;
    DataAttributes attrs;
    std::vector<ArrayDimension> dimensions;  // empty if not array
};

struct StructureDecl {
    int level;
    std::string name;
    std::vector<StructureMember> members;
    std::string like_name;  // for LIKE attribute
};

struct EntryDecl {
    std::string name;
    std::vector<DataAttributes> param_types;
    DataAttributes returns;
};

struct LabelDecl {
    std::string name;
};

struct FileDecl {
    std::string name;
    bool is_stream = true;
    bool is_input = true;
    bool is_print = false;
    bool is_record = false;
    bool is_sequential = true;
    bool is_keyed = false;
    std::string env;
};

struct ProcedureDecl {
    std::string name;
    std::vector<std::string> params;
    DataAttributes attrs;
    std::vector<DeclPtr> local_decls;
    std::vector<StmtPtr> body;
};

using DeclVariant = std::variant<
    ScalarDecl,
    ArrayDecl,
    StructureDecl,
    EntryDecl,
    LabelDecl,
    FileDecl,
    ProcedureDecl
>;

struct Declaration {
    DeclVariant node;
    int line = 0;

    Declaration() : node(ScalarDecl{}) {}
    explicit Declaration(DeclVariant n, int ln = 0) : node(std::move(n)), line(ln) {}
};

inline DeclPtr make_decl(DeclVariant v, int line = 0) {
    return std::make_shared<Declaration>(std::move(v), line);
}

// ---------- Program / Compilation Unit ----------

struct Program {
    std::string name;
    std::vector<DeclPtr> declarations;
    std::vector<StmtPtr> statements;
    bool options_main = false;
};

}} // namespace lazarus::pli

#endif // LAZARUS_PLI_AST_H
