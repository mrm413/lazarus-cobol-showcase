#ifndef LAZARUS_REXX_AST_H
#define LAZARUS_REXX_AST_H

#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace lazarus { namespace rexx {

// ── RexxValue: typeless REXX variable ──────────────────────────────────────
struct RexxValue {
    std::variant<int64_t, double, std::string> data;

    RexxValue() : data(std::string("")) {}
    explicit RexxValue(int64_t v) : data(v) {}
    explicit RexxValue(double v) : data(v) {}
    explicit RexxValue(const std::string& v) : data(v) {}
    explicit RexxValue(const char* v) : data(std::string(v)) {}

    bool is_integer() const { return std::holds_alternative<int64_t>(data); }
    bool is_double() const { return std::holds_alternative<double>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }

    int64_t to_integer() const {
        if (is_integer()) return std::get<int64_t>(data);
        if (is_double()) return static_cast<int64_t>(std::get<double>(data));
        const auto& s = std::get<std::string>(data);
        if (s.empty()) return 0;
        try {
            size_t pos = 0;
            int64_t v = std::stoll(s, &pos);
            if (pos == s.size()) return v;
        } catch (...) {}
        try {
            size_t pos = 0;
            double d = std::stod(s, &pos);
            if (pos == s.size()) return static_cast<int64_t>(d);
        } catch (...) {}
        return 0;
    }

    double to_double() const {
        if (is_double()) return std::get<double>(data);
        if (is_integer()) return static_cast<double>(std::get<int64_t>(data));
        const auto& s = std::get<std::string>(data);
        if (s.empty()) return 0.0;
        try {
            size_t pos = 0;
            double v = std::stod(s, &pos);
            if (pos == s.size()) return v;
        } catch (...) {}
        return 0.0;
    }

    std::string to_string() const {
        if (is_string()) return std::get<std::string>(data);
        if (is_integer()) return std::to_string(std::get<int64_t>(data));
        std::ostringstream os;
        os << std::get<double>(data);
        return os.str();
    }

    bool is_numeric() const {
        if (is_integer() || is_double()) return true;
        const auto& s = std::get<std::string>(data);
        if (s.empty()) return false;
        try {
            size_t pos = 0;
            std::stod(s, &pos);
            return pos == s.size();
        } catch (...) { return false; }
    }

    // Arithmetic coercion: if both can be int, use int; else double
    static RexxValue add(const RexxValue& a, const RexxValue& b) {
        if (a.is_numeric() && b.is_numeric()) {
            if (a.is_integer() && b.is_integer())
                return RexxValue(a.to_integer() + b.to_integer());
            return RexxValue(a.to_double() + b.to_double());
        }
        return RexxValue(a.to_string() + b.to_string());
    }

    static RexxValue sub(const RexxValue& a, const RexxValue& b) {
        if (a.is_integer() && b.is_integer())
            return RexxValue(a.to_integer() - b.to_integer());
        return RexxValue(a.to_double() - b.to_double());
    }

    static RexxValue mul(const RexxValue& a, const RexxValue& b) {
        if (a.is_integer() && b.is_integer())
            return RexxValue(a.to_integer() * b.to_integer());
        return RexxValue(a.to_double() * b.to_double());
    }

    static RexxValue div(const RexxValue& a, const RexxValue& b) {
        double bv = b.to_double();
        if (bv == 0.0) throw std::runtime_error("REXX: division by zero");
        return RexxValue(a.to_double() / bv);
    }

    static RexxValue int_div(const RexxValue& a, const RexxValue& b) {
        int64_t bv = b.to_integer();
        if (bv == 0) throw std::runtime_error("REXX: integer division by zero");
        return RexxValue(a.to_integer() / bv);
    }

    static RexxValue mod(const RexxValue& a, const RexxValue& b) {
        int64_t bv = b.to_integer();
        if (bv == 0) throw std::runtime_error("REXX: modulo by zero");
        return RexxValue(a.to_integer() % bv);
    }

    static RexxValue power(const RexxValue& a, const RexxValue& b) {
        return RexxValue(std::pow(a.to_double(), b.to_double()));
    }

    static RexxValue concat(const RexxValue& a, const RexxValue& b) {
        return RexxValue(a.to_string() + b.to_string());
    }

    static RexxValue concat_space(const RexxValue& a, const RexxValue& b) {
        return RexxValue(a.to_string() + " " + b.to_string());
    }

    bool operator==(const RexxValue& o) const {
        if (is_numeric() && o.is_numeric())
            return to_double() == o.to_double();
        return to_string() == o.to_string();
    }

    bool operator<(const RexxValue& o) const {
        if (is_numeric() && o.is_numeric())
            return to_double() < o.to_double();
        return to_string() < o.to_string();
    }

    bool operator>(const RexxValue& o) const { return o < *this; }
    bool operator<=(const RexxValue& o) const { return !(o < *this); }
    bool operator>=(const RexxValue& o) const { return !(*this < o); }
    bool operator!=(const RexxValue& o) const { return !(*this == o); }
};

// ── Expression AST ─────────────────────────────────────────────────────────
enum class ExprType {
    Literal, Variable, StemAccess, BinaryOp, UnaryOp,
    FunctionCall, Comparison, Parenthesized
};

enum class BinOp {
    Add, Sub, Mul, Div, IntDiv, Mod, Power,
    Concat, ConcatSpace,
    Eq, Ne, Lt, Gt, Le, Ge,
    StrictEq, StrictNe, StrictLt, StrictGt,
    LogicalAnd, LogicalOr
};

enum class UnOp { Negate, Not };

struct Expr;
using ExprPtr = std::shared_ptr<Expr>;

struct Expr {
    ExprType type;
    // Literal / Variable
    std::string value;
    // StemAccess
    std::string stem_name;
    std::vector<ExprPtr> tail_exprs;
    // BinaryOp
    BinOp bin_op = BinOp::Add;
    ExprPtr left, right;
    // UnaryOp
    UnOp un_op = UnOp::Negate;
    ExprPtr operand;
    // FunctionCall
    std::string func_name;
    std::vector<ExprPtr> args;
    // Comparison
    // (reuses bin_op, left, right)

    static ExprPtr make_literal(const std::string& v) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::Literal;
        e->value = v;
        return e;
    }
    static ExprPtr make_variable(const std::string& v) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::Variable;
        e->value = v;
        return e;
    }
    static ExprPtr make_stem(const std::string& name, std::vector<ExprPtr> tails) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::StemAccess;
        e->stem_name = name;
        e->tail_exprs = std::move(tails);
        return e;
    }
    static ExprPtr make_binary(BinOp op, ExprPtr l, ExprPtr r) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::BinaryOp;
        e->bin_op = op;
        e->left = std::move(l);
        e->right = std::move(r);
        return e;
    }
    static ExprPtr make_unary(UnOp op, ExprPtr o) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::UnaryOp;
        e->un_op = op;
        e->operand = std::move(o);
        return e;
    }
    static ExprPtr make_call(const std::string& name, std::vector<ExprPtr> a) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::FunctionCall;
        e->func_name = name;
        e->args = std::move(a);
        return e;
    }
    static ExprPtr make_comparison(BinOp op, ExprPtr l, ExprPtr r) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::Comparison;
        e->bin_op = op;
        e->left = std::move(l);
        e->right = std::move(r);
        return e;
    }
    static ExprPtr make_paren(ExprPtr inner) {
        auto e = std::make_shared<Expr>();
        e->type = ExprType::Parenthesized;
        e->operand = std::move(inner);
        return e;
    }
};

// ── PARSE template patterns ────────────────────────────────────────────────
enum class TemplateItemType {
    Variable,       // plain word — assign next token
    LiteralDelim,   // 'string' — skip to literal
    Positional,     // absolute column =n
    RelativePos,    // +n or -n
    Period          // placeholder .
};

struct TemplateItem {
    TemplateItemType type;
    std::string value;      // variable name or literal
    int position = 0;       // for positional / relative

    static TemplateItem var(const std::string& n) {
        return {TemplateItemType::Variable, n, 0};
    }
    static TemplateItem lit(const std::string& s) {
        return {TemplateItemType::LiteralDelim, s, 0};
    }
    static TemplateItem pos(int p) {
        return {TemplateItemType::Positional, "", p};
    }
    static TemplateItem rel(int p) {
        return {TemplateItemType::RelativePos, "", p};
    }
    static TemplateItem dot() {
        return {TemplateItemType::Period, ".", 0};
    }
};

// ── Statement AST ──────────────────────────────────────────────────────────
enum class StmtType {
    Say, Pull, Push, Queue, Assign, If, Select, Do,
    Call, Return, Exit, Signal, Address, Parse, Drop,
    Nop, Iterate, Leave, Interpret, Numeric, Trace,
    Procedure, Label, Block
};

struct Stmt;
using StmtPtr = std::shared_ptr<Stmt>;

enum class DoVariant {
    Simple,     // DO; ...END
    Counted,    // DO n
    Controlled, // DO i = start TO end [BY step]
    While,      // DO WHILE cond
    Until,      // DO UNTIL cond
    Forever     // DO FOREVER
};

enum class ParseSource {
    Arg, Pull, Var, Value, Source, Version, LinIn
};

enum class NumericSetting { Digits, Fuzz, Form };

// SIGNAL/CALL condition types
enum class ConditionType {
    Error, Failure, Halt, NoValue, Syntax, LostDigits, NotReady
};

inline std::string condition_name(ConditionType c) {
    switch (c) {
        case ConditionType::Error:      return "ERROR";
        case ConditionType::Failure:    return "FAILURE";
        case ConditionType::Halt:       return "HALT";
        case ConditionType::NoValue:    return "NOVALUE";
        case ConditionType::Syntax:     return "SYNTAX";
        case ConditionType::LostDigits: return "LOSTDIGITS";
        case ConditionType::NotReady:   return "NOTREADY";
    }
    return "UNKNOWN";
}

struct WhenClause {
    ExprPtr condition;
    std::vector<StmtPtr> body;
};

struct Stmt {
    StmtType type;

    // Say / Push / Queue / Return / Exit / Interpret
    ExprPtr expr;

    // Pull / Parse
    std::vector<TemplateItem> tmpl;
    ParseSource parse_source = ParseSource::Arg;
    std::string parse_var_name;   // for PARSE VAR name

    // Assign
    std::string assign_target;
    ExprPtr assign_value;
    std::string assign_stem;
    std::vector<ExprPtr> assign_tail;

    // If
    ExprPtr condition;
    StmtPtr then_branch;
    StmtPtr else_branch;

    // Select
    std::vector<WhenClause> when_clauses;
    std::vector<StmtPtr> otherwise_body;

    // Do
    DoVariant do_variant = DoVariant::Simple;
    std::string do_var;
    ExprPtr do_start, do_end, do_step, do_count;
    ExprPtr do_condition;  // while / until
    std::vector<StmtPtr> do_body;

    // Call / Signal
    std::string target_name;
    bool is_on = false;
    bool is_off = false;
    ConditionType cond_type = ConditionType::Error;
    std::vector<ExprPtr> call_args;

    // Address
    std::string environment;
    ExprPtr address_command;

    // Drop
    std::vector<std::string> drop_vars;

    // Trace
    std::string trace_setting;

    // Numeric
    NumericSetting numeric_setting = NumericSetting::Digits;
    ExprPtr numeric_value;

    // Procedure
    std::vector<std::string> expose_vars;

    // Label
    std::string label_name;

    // Block
    std::vector<StmtPtr> block_stmts;

    // Factory methods
    static StmtPtr make_say(ExprPtr e) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Say;
        s->expr = std::move(e);
        return s;
    }
    static StmtPtr make_pull(std::vector<TemplateItem> t) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Pull;
        s->tmpl = std::move(t);
        return s;
    }
    static StmtPtr make_push(ExprPtr e) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Push;
        s->expr = std::move(e);
        return s;
    }
    static StmtPtr make_queue(ExprPtr e) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Queue;
        s->expr = std::move(e);
        return s;
    }
    static StmtPtr make_assign(const std::string& name, ExprPtr val) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Assign;
        s->assign_target = name;
        s->assign_value = std::move(val);
        return s;
    }
    static StmtPtr make_stem_assign(const std::string& stem,
                                     std::vector<ExprPtr> tail, ExprPtr val) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Assign;
        s->assign_stem = stem;
        s->assign_tail = std::move(tail);
        s->assign_value = std::move(val);
        return s;
    }
    static StmtPtr make_if(ExprPtr cond, StmtPtr tb, StmtPtr eb = nullptr) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::If;
        s->condition = std::move(cond);
        s->then_branch = std::move(tb);
        s->else_branch = std::move(eb);
        return s;
    }
    static StmtPtr make_select(std::vector<WhenClause> wc,
                                std::vector<StmtPtr> ow = {}) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Select;
        s->when_clauses = std::move(wc);
        s->otherwise_body = std::move(ow);
        return s;
    }
    static StmtPtr make_do_simple(std::vector<StmtPtr> body) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Do;
        s->do_variant = DoVariant::Simple;
        s->do_body = std::move(body);
        return s;
    }
    static StmtPtr make_do_counted(ExprPtr count, std::vector<StmtPtr> body) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Do;
        s->do_variant = DoVariant::Counted;
        s->do_count = std::move(count);
        s->do_body = std::move(body);
        return s;
    }
    static StmtPtr make_do_controlled(const std::string& var, ExprPtr start,
                                       ExprPtr end_e, ExprPtr step,
                                       std::vector<StmtPtr> body) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Do;
        s->do_variant = DoVariant::Controlled;
        s->do_var = var;
        s->do_start = std::move(start);
        s->do_end = std::move(end_e);
        s->do_step = std::move(step);
        s->do_body = std::move(body);
        return s;
    }
    static StmtPtr make_do_while(ExprPtr cond, std::vector<StmtPtr> body) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Do;
        s->do_variant = DoVariant::While;
        s->do_condition = std::move(cond);
        s->do_body = std::move(body);
        return s;
    }
    static StmtPtr make_do_until(ExprPtr cond, std::vector<StmtPtr> body) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Do;
        s->do_variant = DoVariant::Until;
        s->do_condition = std::move(cond);
        s->do_body = std::move(body);
        return s;
    }
    static StmtPtr make_do_forever(std::vector<StmtPtr> body) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Do;
        s->do_variant = DoVariant::Forever;
        s->do_body = std::move(body);
        return s;
    }
    static StmtPtr make_call(const std::string& name, std::vector<ExprPtr> args = {}) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Call;
        s->target_name = name;
        s->call_args = std::move(args);
        return s;
    }
    static StmtPtr make_call_on(ConditionType ct, const std::string& label = "") {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Call;
        s->is_on = true;
        s->cond_type = ct;
        s->target_name = label;
        return s;
    }
    static StmtPtr make_call_off(ConditionType ct) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Call;
        s->is_off = true;
        s->cond_type = ct;
        return s;
    }
    static StmtPtr make_return(ExprPtr e = nullptr) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Return;
        s->expr = std::move(e);
        return s;
    }
    static StmtPtr make_exit(ExprPtr e = nullptr) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Exit;
        s->expr = std::move(e);
        return s;
    }
    static StmtPtr make_signal(const std::string& label) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Signal;
        s->target_name = label;
        return s;
    }
    static StmtPtr make_signal_on(ConditionType ct, const std::string& label = "") {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Signal;
        s->is_on = true;
        s->cond_type = ct;
        s->target_name = label;
        return s;
    }
    static StmtPtr make_signal_off(ConditionType ct) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Signal;
        s->is_off = true;
        s->cond_type = ct;
        return s;
    }
    static StmtPtr make_address(const std::string& env, ExprPtr cmd = nullptr) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Address;
        s->environment = env;
        s->address_command = std::move(cmd);
        return s;
    }
    static StmtPtr make_parse(ParseSource src, std::vector<TemplateItem> t,
                               const std::string& var_name = "") {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Parse;
        s->parse_source = src;
        s->tmpl = std::move(t);
        s->parse_var_name = var_name;
        return s;
    }
    static StmtPtr make_drop(std::vector<std::string> vars) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Drop;
        s->drop_vars = std::move(vars);
        return s;
    }
    static StmtPtr make_nop() {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Nop;
        return s;
    }
    static StmtPtr make_iterate(const std::string& name = "") {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Iterate;
        s->target_name = name;
        return s;
    }
    static StmtPtr make_leave(const std::string& name = "") {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Leave;
        s->target_name = name;
        return s;
    }
    static StmtPtr make_interpret(ExprPtr e) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Interpret;
        s->expr = std::move(e);
        return s;
    }
    static StmtPtr make_numeric(NumericSetting ns, ExprPtr val) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Numeric;
        s->numeric_setting = ns;
        s->numeric_value = std::move(val);
        return s;
    }
    static StmtPtr make_trace(const std::string& setting) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Trace;
        s->trace_setting = setting;
        return s;
    }
    static StmtPtr make_procedure(std::vector<std::string> exposed = {}) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Procedure;
        s->expose_vars = std::move(exposed);
        return s;
    }
    static StmtPtr make_label(const std::string& name) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Label;
        s->label_name = name;
        return s;
    }
    static StmtPtr make_block(std::vector<StmtPtr> stmts) {
        auto s = std::make_shared<Stmt>();
        s->type = StmtType::Block;
        s->block_stmts = std::move(stmts);
        return s;
    }
};

// ── Program root ───────────────────────────────────────────────────────────
struct Program {
    std::vector<StmtPtr> statements;
};

}} // namespace lazarus::rexx

#endif // LAZARUS_REXX_AST_H
