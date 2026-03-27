#ifndef LAZARUS_JCL_PARSER_H
#define LAZARUS_JCL_PARSER_H

#include "lazarus/jcl/lexer.h"
#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <map>
#include <stdexcept>
#include <algorithm>

namespace lazarus { namespace jcl {

// --- AST Node Types ---

struct JclComment {
    std::string text;
    int line;
};

struct DdParams {
    std::string dsn;
    std::string disp;          // e.g. "(NEW,CATLG,DELETE)"
    std::string disp_status;   // NEW, OLD, SHR, MOD
    std::string disp_normal;   // CATLG, KEEP, DELETE, PASS, UNCATLG
    std::string disp_abnormal; // CATLG, KEEP, DELETE, UNCATLG
    std::string space;         // e.g. "(CYL,(10,5),RLSE)"
    std::string dcb;           // e.g. "(RECFM=FB,LRECL=80,BLKSIZE=800)"
    std::string sysout;        // e.g. "*" or "A"
    std::string unit;
    std::string vol;
    bool is_instream = false;
    bool is_dummy = false;
    bool is_referback = false;
    std::string referback;     // e.g. "*.STEP1.SYSUT1"
    std::vector<std::string> instream_data;
    std::map<std::string, std::string> extra_params;
};

struct JclDd {
    std::string name;          // DD name (e.g. SYSUT1, SYSIN)
    DdParams params;
    int line;
    bool is_override = false;  // true if overriding a PROC DD
};

struct JclStep {
    std::string name;          // Step name
    std::string pgm;           // Program name (EXEC PGM=xxx)
    std::string proc;          // Proc name (EXEC procname or EXEC PROC=xxx)
    bool is_proc_call = false;
    std::map<std::string, std::string> exec_params;  // PARM, TIME, REGION, COND, etc.
    std::vector<JclDd> dd_statements;
    int line;
};

struct CondExpr {
    int rc;
    std::string op;            // GT, GE, LT, LE, EQ, NE
    std::string step_name;
};

struct JclJob {
    std::string name;
    std::string accounting;
    std::string programmer;
    std::string job_class;
    std::string msgclass;
    std::string msglevel;
    std::string notify;
    std::string region;
    std::string time;
    std::vector<CondExpr> cond;
    std::map<std::string, std::string> extra_params;
    std::vector<JclStep> steps;
    int line;
};

struct JclProc {
    std::string name;
    std::map<std::string, std::string> defaults;  // symbolic parameter defaults
    std::vector<JclStep> steps;
    int line;
};

struct JclIf {
    std::string condition;     // Raw condition text
    std::vector<JclStep> then_steps;
    std::vector<JclStep> else_steps;
    int line;
};

struct JclSet {
    std::string variable;
    std::string value;
    int line;
};

struct JclInclude {
    std::string member;
    int line;
};

struct JclOutput {
    std::string name;
    std::map<std::string, std::string> params;
    int line;
};

using AstNode = std::variant<
    JclJob, JclStep, JclDd, JclProc, JclIf, JclSet,
    JclInclude, JclOutput, JclComment
>;

// --- Parser ---

class Parser {
public:
    explicit Parser(const std::vector<Token>& tokens)
        : tokens_(tokens), pos_(0) {}

    JclJob parse() {
        JclJob job;
        job.line = 0;

        while (!at_end()) {
            if (peek().type == TokenType::COMMENT) {
                advance();
                continue;
            }
            if (peek().type == TokenType::CONTINUATION) {
                advance();
                continue;
            }
            if (peek().type == TokenType::LABEL) {
                std::string label = advance().value;

                if (at_end()) break;

                TokenType op = peek().type;
                if (op == TokenType::JOB) {
                    job = parse_job(label);
                } else if (op == TokenType::EXEC) {
                    job.steps.push_back(parse_step(label));
                } else if (op == TokenType::DD) {
                    if (!job.steps.empty()) {
                        job.steps.back().dd_statements.push_back(parse_dd(label));
                    } else {
                        parse_dd(label); // Orphan DD, consume it
                    }
                } else if (op == TokenType::PROC) {
                    advance(); // consume PROC
                    skip_params();
                } else if (op == TokenType::OUTPUT) {
                    advance();
                    skip_params();
                } else if (op == TokenType::SET) {
                    advance();
                    skip_params();
                } else {
                    advance();
                    skip_params();
                }
            } else if (peek().type == TokenType::JOB) {
                job = parse_job("");
            } else if (peek().type == TokenType::EXEC) {
                job.steps.push_back(parse_step(""));
            } else if (peek().type == TokenType::DD) {
                if (!job.steps.empty()) {
                    job.steps.back().dd_statements.push_back(parse_dd(""));
                } else {
                    parse_dd("");
                }
            } else if (peek().type == TokenType::SET) {
                advance();
                skip_params();
            } else if (peek().type == TokenType::IF) {
                advance();
                skip_params();
            } else if (peek().type == TokenType::ENDIF) {
                advance();
            } else if (peek().type == TokenType::INCLUDE) {
                advance();
                skip_params();
            } else if (peek().type == TokenType::INSTREAM_DATA) {
                if (!job.steps.empty() && !job.steps.back().dd_statements.empty()) {
                    job.steps.back().dd_statements.back().params.instream_data.push_back(
                        advance().value);
                    job.steps.back().dd_statements.back().params.is_instream = true;
                } else {
                    advance();
                }
            } else if (peek().type == TokenType::DELIMITER) {
                advance();
            } else {
                advance();
            }
        }

        return job;
    }

    // Parse standalone PROC definition
    JclProc parse_proc() {
        JclProc proc;
        proc.line = 0;

        while (!at_end()) {
            if (peek().type == TokenType::COMMENT || peek().type == TokenType::CONTINUATION) {
                advance();
                continue;
            }
            if (peek().type == TokenType::LABEL) {
                std::string label = advance().value;
                if (at_end()) break;

                if (peek().type == TokenType::PROC) {
                    advance();
                    proc.name = label;
                    proc.line = peek().line;
                    // Parse default parameters
                    while (!at_end() && peek().type == TokenType::PARAM) {
                        std::string pname = advance().value;
                        std::string pval;
                        if (!at_end() && peek().type == TokenType::EQUALS) {
                            advance();
                            if (!at_end() && (peek().type == TokenType::PARAM ||
                                              peek().type == TokenType::LITERAL)) {
                                pval = advance().value;
                            }
                        }
                        proc.defaults[pname] = pval;
                        if (!at_end() && peek().type == TokenType::COMMA) advance();
                    }
                } else if (peek().type == TokenType::EXEC) {
                    proc.steps.push_back(parse_step(label));
                } else if (peek().type == TokenType::DD) {
                    if (!proc.steps.empty()) {
                        proc.steps.back().dd_statements.push_back(parse_dd(label));
                    }
                } else if (peek().type == TokenType::PEND) {
                    advance();
                    break;
                } else {
                    advance();
                    skip_params();
                }
            } else if (peek().type == TokenType::PEND) {
                advance();
                break;
            } else {
                advance();
            }
        }
        return proc;
    }

    // Parse into a flat list of AST nodes
    std::vector<AstNode> parse_all() {
        std::vector<AstNode> nodes;
        while (!at_end()) {
            if (peek().type == TokenType::COMMENT) {
                JclComment c;
                c.text = peek().value;
                c.line = peek().line;
                nodes.push_back(c);
                advance();
            } else if (peek().type == TokenType::LABEL) {
                std::string label = advance().value;
                if (at_end()) break;

                if (peek().type == TokenType::JOB) {
                    nodes.push_back(parse_job(label));
                } else if (peek().type == TokenType::EXEC) {
                    nodes.push_back(parse_step(label));
                } else if (peek().type == TokenType::DD) {
                    nodes.push_back(parse_dd(label));
                } else if (peek().type == TokenType::SET) {
                    advance();
                    JclSet s;
                    s.line = peek().line;
                    if (!at_end() && peek().type == TokenType::PARAM) {
                        s.variable = advance().value;
                        if (!at_end() && peek().type == TokenType::EQUALS) {
                            advance();
                            if (!at_end() && (peek().type == TokenType::PARAM ||
                                              peek().type == TokenType::LITERAL)) {
                                s.value = advance().value;
                            }
                        }
                    }
                    nodes.push_back(s);
                } else if (peek().type == TokenType::INCLUDE) {
                    advance();
                    JclInclude inc;
                    inc.line = peek().line;
                    if (!at_end() && peek().type == TokenType::PARAM) {
                        std::string pn = advance().value;
                        if (to_upper(pn) == "MEMBER") {
                            if (!at_end() && peek().type == TokenType::EQUALS) {
                                advance();
                                if (!at_end()) inc.member = advance().value;
                            }
                        } else {
                            inc.member = pn;
                        }
                    }
                    nodes.push_back(inc);
                } else {
                    advance();
                    skip_params();
                }
            } else if (peek().type == TokenType::SET) {
                advance();
                JclSet s;
                s.line = peek().line;
                if (!at_end() && peek().type == TokenType::PARAM) {
                    s.variable = advance().value;
                    if (!at_end() && peek().type == TokenType::EQUALS) {
                        advance();
                        if (!at_end() && (peek().type == TokenType::PARAM ||
                                          peek().type == TokenType::LITERAL)) {
                            s.value = advance().value;
                        }
                    }
                }
                nodes.push_back(s);
            } else {
                advance();
            }
        }
        return nodes;
    }

private:
    std::vector<Token> tokens_;
    size_t pos_;

    bool at_end() const {
        return pos_ >= tokens_.size() || tokens_[pos_].type == TokenType::END_OF_INPUT;
    }

    const Token& peek() const {
        if (pos_ >= tokens_.size()) {
            static Token eof(TokenType::END_OF_INPUT, "", 0, 0);
            return eof;
        }
        return tokens_[pos_];
    }

    Token advance() {
        if (at_end()) return Token(TokenType::END_OF_INPUT, "", 0, 0);
        return tokens_[pos_++];
    }

    void skip_params() {
        while (!at_end()) {
            auto t = peek().type;
            if (t == TokenType::PARAM || t == TokenType::LITERAL ||
                t == TokenType::EQUALS || t == TokenType::COMMA ||
                t == TokenType::LPAREN || t == TokenType::RPAREN ||
                t == TokenType::ASTERISK || t == TokenType::COND ||
                t == TokenType::NOTIFY || t == TokenType::CONTINUATION) {
                advance();
            } else {
                break;
            }
        }
    }

    // Collect a parenthesized group as a flat string
    std::string collect_paren_group() {
        std::string result;
        if (!at_end() && peek().type == TokenType::LPAREN) {
            result += advance().value;
            int depth = 1;
            while (!at_end() && depth > 0) {
                if (peek().type == TokenType::LPAREN) ++depth;
                if (peek().type == TokenType::RPAREN) --depth;
                result += advance().value;
            }
        }
        return result;
    }

    // Parse name=value or name=(value,value) returning the value as string
    std::string parse_kw_value() {
        if (!at_end() && peek().type == TokenType::EQUALS) {
            advance();
            if (!at_end() && peek().type == TokenType::LPAREN) {
                return collect_paren_group();
            }
            if (!at_end() && (peek().type == TokenType::PARAM ||
                              peek().type == TokenType::LITERAL ||
                              peek().type == TokenType::ASTERISK)) {
                return advance().value;
            }
        }
        return "";
    }

    JclJob parse_job(const std::string& name) {
        JclJob job;
        job.name = name;
        job.line = peek().line;
        advance(); // consume JOB

        // Positional: accounting info
        if (!at_end() && peek().type == TokenType::PARAM) {
            job.accounting = advance().value;
            if (!at_end() && peek().type == TokenType::COMMA) advance();
        } else if (!at_end() && peek().type == TokenType::LPAREN) {
            job.accounting = collect_paren_group();
            if (!at_end() && peek().type == TokenType::COMMA) advance();
        } else if (!at_end() && peek().type == TokenType::LITERAL) {
            job.accounting = advance().value;
            if (!at_end() && peek().type == TokenType::COMMA) advance();
        }

        // Positional: programmer name
        if (!at_end() && peek().type == TokenType::LITERAL) {
            job.programmer = advance().value;
            if (!at_end() && peek().type == TokenType::COMMA) advance();
        } else if (!at_end() && peek().type == TokenType::PARAM) {
            // Check if it's a keyword param (has = after)
            size_t save = pos_;
            std::string val = peek().value;
            advance();
            if (!at_end() && peek().type == TokenType::EQUALS) {
                // It's a keyword, backtrack
                pos_ = save;
            } else {
                job.programmer = val;
                if (!at_end() && peek().type == TokenType::COMMA) advance();
            }
        }

        // Keyword parameters
        while (!at_end()) {
            auto t = peek().type;
            if (t == TokenType::CONTINUATION) { advance(); continue; }
            if (t == TokenType::COMMA) { advance(); continue; }

            if (t == TokenType::PARAM || t == TokenType::COND || t == TokenType::NOTIFY) {
                std::string kw = to_upper(peek().value);
                advance();

                if (kw == "CLASS") {
                    job.job_class = parse_kw_value();
                } else if (kw == "MSGCLASS") {
                    job.msgclass = parse_kw_value();
                } else if (kw == "MSGLEVEL") {
                    job.msglevel = parse_kw_value();
                } else if (kw == "NOTIFY") {
                    job.notify = parse_kw_value();
                } else if (kw == "REGION") {
                    job.region = parse_kw_value();
                } else if (kw == "TIME") {
                    job.time = parse_kw_value();
                } else if (kw == "COND") {
                    std::string cval = parse_kw_value();
                    if (!cval.empty()) {
                        parse_cond_expr(cval, job.cond);
                    }
                } else {
                    std::string v = parse_kw_value();
                    if (!v.empty()) {
                        job.extra_params[kw] = v;
                    }
                }
            } else {
                break;
            }
        }

        return job;
    }

    void parse_cond_expr(const std::string& expr, std::vector<CondExpr>& conds) {
        // Parse COND=((rc,op,stepname),...)
        // Simple form: COND=(4,LT,STEP1)
        // Multiple: COND=((4,LT,STEP1),(0,NE,STEP2))
        std::string s = expr;
        // Strip outer parens
        if (!s.empty() && s.front() == '(') s = s.substr(1);
        if (!s.empty() && s.back() == ')') s.pop_back();

        // Check for nested parens (multiple conditions)
        if (!s.empty() && s.front() == '(') {
            // Multiple conditions
            size_t pos = 0;
            while (pos < s.size()) {
                if (s[pos] == '(') {
                    size_t end = s.find(')', pos);
                    if (end != std::string::npos) {
                        parse_single_cond(s.substr(pos + 1, end - pos - 1), conds);
                        pos = end + 1;
                    } else {
                        break;
                    }
                } else {
                    ++pos;
                }
            }
        } else {
            // Single condition
            parse_single_cond(s, conds);
        }
    }

    void parse_single_cond(const std::string& s, std::vector<CondExpr>& conds) {
        // Format: rc,op,stepname
        CondExpr ce{};
        size_t p1 = s.find(',');
        if (p1 == std::string::npos) return;
        try { ce.rc = std::stoi(s.substr(0, p1)); } catch (...) { return; }
        size_t p2 = s.find(',', p1 + 1);
        if (p2 == std::string::npos) return;
        ce.op = to_upper(s.substr(p1 + 1, p2 - p1 - 1));
        ce.step_name = s.substr(p2 + 1);
        // Trim spaces
        while (!ce.step_name.empty() && ce.step_name.back() == ' ') ce.step_name.pop_back();
        conds.push_back(ce);
    }

    JclStep parse_step(const std::string& name) {
        JclStep step;
        step.name = name;
        step.line = peek().line;
        advance(); // consume EXEC

        // Parse PGM= or PROC= or bare proc name
        while (!at_end()) {
            auto t = peek().type;
            if (t == TokenType::CONTINUATION) { advance(); continue; }
            if (t == TokenType::COMMA) { advance(); continue; }

            if (t == TokenType::PARAM) {
                std::string kw = to_upper(peek().value);
                if (kw == "PGM") {
                    advance();
                    step.pgm = parse_kw_value();
                } else if (kw == "PROC") {
                    advance();
                    step.proc = parse_kw_value();
                    step.is_proc_call = true;
                } else if (kw == "PARM") {
                    advance();
                    step.exec_params["PARM"] = parse_kw_value();
                } else if (kw == "TIME") {
                    advance();
                    step.exec_params["TIME"] = parse_kw_value();
                } else if (kw == "REGION") {
                    advance();
                    step.exec_params["REGION"] = parse_kw_value();
                } else {
                    // Check if next is =
                    size_t save = pos_;
                    advance();
                    if (!at_end() && peek().type == TokenType::EQUALS) {
                        std::string v = parse_kw_value();
                        step.exec_params[kw] = v;
                    } else {
                        // Bare proc name
                        pos_ = save;
                        step.proc = advance().value;
                        step.is_proc_call = true;
                    }
                }
            } else if (t == TokenType::COND) {
                advance();
                step.exec_params["COND"] = parse_kw_value();
            } else {
                break;
            }
        }

        return step;
    }

    JclDd parse_dd(const std::string& name) {
        JclDd dd;
        dd.name = name;
        dd.line = peek().line;
        advance(); // consume DD

        // Check for DD * (instream)
        if (!at_end() && peek().type == TokenType::ASTERISK) {
            advance();
            dd.params.is_instream = true;
            return dd;
        }

        // Check for DD DUMMY
        if (!at_end() && peek().type == TokenType::PARAM) {
            if (to_upper(peek().value) == "DUMMY") {
                advance();
                dd.params.is_dummy = true;
                return dd;
            }
            if (to_upper(peek().value) == "DATA") {
                advance();
                dd.params.is_instream = true;
                return dd;
            }
        }

        // Parse keyword parameters
        while (!at_end()) {
            auto t = peek().type;
            if (t == TokenType::CONTINUATION) { advance(); continue; }
            if (t == TokenType::COMMA) { advance(); continue; }

            if (t == TokenType::PARAM) {
                std::string kw = to_upper(peek().value);
                advance();

                if (kw == "DSN" || kw == "DSNAME") {
                    dd.params.dsn = parse_kw_value();
                    // Check for referback
                    if (!dd.params.dsn.empty() && dd.params.dsn[0] == '*') {
                        dd.params.is_referback = true;
                        dd.params.referback = dd.params.dsn;
                    }
                } else if (kw == "DISP") {
                    dd.params.disp = parse_kw_value();
                    parse_disp(dd.params.disp, dd.params);
                } else if (kw == "SPACE") {
                    dd.params.space = parse_kw_value();
                } else if (kw == "DCB") {
                    dd.params.dcb = parse_kw_value();
                } else if (kw == "SYSOUT") {
                    dd.params.sysout = parse_kw_value();
                } else if (kw == "UNIT") {
                    dd.params.unit = parse_kw_value();
                } else if (kw == "VOL") {
                    dd.params.vol = parse_kw_value();
                } else if (kw == "RECFM" || kw == "LRECL" || kw == "BLKSIZE") {
                    // DCB subparams appearing at DD level
                    std::string v = parse_kw_value();
                    dd.params.extra_params[kw] = v;
                } else {
                    std::string v = parse_kw_value();
                    dd.params.extra_params[kw] = v;
                }
            } else if (t == TokenType::ASTERISK) {
                // SYSOUT=*
                advance();
            } else {
                break;
            }
        }

        return dd;
    }

    void parse_disp(const std::string& disp, DdParams& p) {
        // DISP=(status,normal,abnormal) or DISP=SHR
        std::string s = disp;
        if (s.front() == '(') s = s.substr(1);
        if (!s.empty() && s.back() == ')') s.pop_back();

        std::vector<std::string> parts;
        size_t start = 0;
        for (size_t i = 0; i <= s.size(); ++i) {
            if (i == s.size() || s[i] == ',') {
                parts.push_back(s.substr(start, i - start));
                start = i + 1;
            }
        }
        if (parts.size() >= 1) p.disp_status = to_upper(parts[0]);
        if (parts.size() >= 2) p.disp_normal = to_upper(parts[1]);
        if (parts.size() >= 3) p.disp_abnormal = to_upper(parts[2]);
    }
};

// Convenience: parse JCL text into a JclJob AST
inline JclJob parse_jcl(std::string_view input) {
    Lexer lex(input);
    Parser parser(lex.tokens());
    return parser.parse();
}

// Convenience: parse JCL text into a flat node list
inline std::vector<AstNode> parse_jcl_all(std::string_view input) {
    Lexer lex(input);
    Parser parser(lex.tokens());
    return parser.parse_all();
}

}} // namespace lazarus::jcl

#endif // LAZARUS_JCL_PARSER_H
