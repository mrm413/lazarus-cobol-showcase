#ifndef LAZARUS_REXX_LEXER_H
#define LAZARUS_REXX_LEXER_H

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cstdint>

namespace lazarus { namespace rexx {

enum class TokenType {
    // Keywords
    DO, END, IF, THEN, ELSE, SELECT, WHEN, OTHERWISE,
    CALL, RETURN, EXIT, SAY, PULL, PUSH, QUEUE, ARG,
    PARSE, ADDRESS, SIGNAL, DROP, ITERATE, LEAVE, NOP,
    TRACE, UPPER, PROCEDURE, EXPOSE, INTERPRET,
    NUMERIC, OPTIONS, VALUE, WITH,
    TO, BY, FOR, WHILE, UNTIL, FOREVER,
    ON, OFF,
    // Identifiers and literals
    IDENTIFIER, INTEGER, FLOAT, STRING,
    HEX_STRING, BINARY_STRING,
    // Operators
    PLUS, MINUS, STAR, SLASH, PERCENT, DBLSLASH, POWER,
    EQUAL, BACKSLASH_EQUAL, LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    STRICT_EQUAL, STRICT_BACKSLASH_EQUAL,
    STRICT_LESS, STRICT_GREATER,
    CONCAT, CONCAT_SPACE,
    NOT, AND, OR, XOR,
    // Delimiters
    LPAREN, RPAREN, COMMA, SEMICOLON, COLON, DOT,
    // Label (identifier followed by colon)
    LABEL,
    // Special
    NEWLINE, END_OF_INPUT
};

inline const char* token_type_name(TokenType t) {
    switch (t) {
        case TokenType::DO: return "DO";
        case TokenType::END: return "END";
        case TokenType::IF: return "IF";
        case TokenType::THEN: return "THEN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::SELECT: return "SELECT";
        case TokenType::WHEN: return "WHEN";
        case TokenType::OTHERWISE: return "OTHERWISE";
        case TokenType::CALL: return "CALL";
        case TokenType::RETURN: return "RETURN";
        case TokenType::EXIT: return "EXIT";
        case TokenType::SAY: return "SAY";
        case TokenType::PULL: return "PULL";
        case TokenType::PUSH: return "PUSH";
        case TokenType::QUEUE: return "QUEUE";
        case TokenType::ARG: return "ARG";
        case TokenType::PARSE: return "PARSE";
        case TokenType::ADDRESS: return "ADDRESS";
        case TokenType::SIGNAL: return "SIGNAL";
        case TokenType::DROP: return "DROP";
        case TokenType::ITERATE: return "ITERATE";
        case TokenType::LEAVE: return "LEAVE";
        case TokenType::NOP: return "NOP";
        case TokenType::TRACE: return "TRACE";
        case TokenType::UPPER: return "UPPER";
        case TokenType::PROCEDURE: return "PROCEDURE";
        case TokenType::EXPOSE: return "EXPOSE";
        case TokenType::INTERPRET: return "INTERPRET";
        case TokenType::NUMERIC: return "NUMERIC";
        case TokenType::OPTIONS: return "OPTIONS";
        case TokenType::VALUE: return "VALUE";
        case TokenType::WITH: return "WITH";
        case TokenType::TO: return "TO";
        case TokenType::BY: return "BY";
        case TokenType::FOR: return "FOR";
        case TokenType::WHILE: return "WHILE";
        case TokenType::UNTIL: return "UNTIL";
        case TokenType::FOREVER: return "FOREVER";
        case TokenType::ON: return "ON";
        case TokenType::OFF: return "OFF";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::INTEGER: return "INTEGER";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::STRING: return "STRING";
        case TokenType::HEX_STRING: return "HEX_STRING";
        case TokenType::BINARY_STRING: return "BINARY_STRING";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::PERCENT: return "PERCENT";
        case TokenType::DBLSLASH: return "DBLSLASH";
        case TokenType::POWER: return "POWER";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::BACKSLASH_EQUAL: return "BACKSLASH_EQUAL";
        case TokenType::LESS: return "LESS";
        case TokenType::GREATER: return "GREATER";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::STRICT_EQUAL: return "STRICT_EQUAL";
        case TokenType::STRICT_BACKSLASH_EQUAL: return "STRICT_BACKSLASH_EQUAL";
        case TokenType::STRICT_LESS: return "STRICT_LESS";
        case TokenType::STRICT_GREATER: return "STRICT_GREATER";
        case TokenType::CONCAT: return "CONCAT";
        case TokenType::CONCAT_SPACE: return "CONCAT_SPACE";
        case TokenType::NOT: return "NOT";
        case TokenType::AND: return "AND";
        case TokenType::OR: return "OR";
        case TokenType::XOR: return "XOR";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COLON: return "COLON";
        case TokenType::DOT: return "DOT";
        case TokenType::LABEL: return "LABEL";
        case TokenType::NEWLINE: return "NEWLINE";
        case TokenType::END_OF_INPUT: return "END_OF_INPUT";
    }
    return "UNKNOWN";
}

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;

    Token() : type(TokenType::END_OF_INPUT), line(0), column(0) {}
    Token(TokenType t, std::string v, int ln, int col)
        : type(t), value(std::move(v)), line(ln), column(col) {}
};

struct KeywordEntry {
    const char* keyword;
    TokenType type;
};

constexpr KeywordEntry kRexxKeywords[] = {
    {"DO",        TokenType::DO},
    {"END",       TokenType::END},
    {"IF",        TokenType::IF},
    {"THEN",      TokenType::THEN},
    {"ELSE",      TokenType::ELSE},
    {"SELECT",    TokenType::SELECT},
    {"WHEN",      TokenType::WHEN},
    {"OTHERWISE",  TokenType::OTHERWISE},
    {"CALL",      TokenType::CALL},
    {"RETURN",    TokenType::RETURN},
    {"EXIT",      TokenType::EXIT},
    {"SAY",       TokenType::SAY},
    {"PULL",      TokenType::PULL},
    {"PUSH",      TokenType::PUSH},
    {"QUEUE",     TokenType::QUEUE},
    {"ARG",       TokenType::ARG},
    {"PARSE",     TokenType::PARSE},
    {"ADDRESS",   TokenType::ADDRESS},
    {"SIGNAL",    TokenType::SIGNAL},
    {"DROP",      TokenType::DROP},
    {"ITERATE",   TokenType::ITERATE},
    {"LEAVE",     TokenType::LEAVE},
    {"NOP",       TokenType::NOP},
    {"TRACE",     TokenType::TRACE},
    {"UPPER",     TokenType::UPPER},
    {"PROCEDURE", TokenType::PROCEDURE},
    {"EXPOSE",    TokenType::EXPOSE},
    {"INTERPRET", TokenType::INTERPRET},
    {"NUMERIC",   TokenType::NUMERIC},
    {"OPTIONS",   TokenType::OPTIONS},
    {"VALUE",     TokenType::VALUE},
    {"WITH",      TokenType::WITH},
    {"TO",        TokenType::TO},
    {"BY",        TokenType::BY},
    {"FOR",       TokenType::FOR},
    {"WHILE",     TokenType::WHILE},
    {"UNTIL",     TokenType::UNTIL},
    {"FOREVER",   TokenType::FOREVER},
    {"ON",        TokenType::ON},
    {"OFF",       TokenType::OFF},
};

class Lexer {
public:
    explicit Lexer(std::string_view source) : src_(source) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (pos_ < src_.size()) {
            skip_whitespace_not_newline();
            if (pos_ >= src_.size()) break;

            char c = src_[pos_];

            // Newline
            if (c == '\n') {
                tokens.emplace_back(TokenType::NEWLINE, "\\n", line_, col_);
                advance();
                continue;
            }
            if (c == '\r') {
                advance();
                if (pos_ < src_.size() && src_[pos_] == '\n') advance();
                tokens.emplace_back(TokenType::NEWLINE, "\\n", line_, col_);
                continue;
            }

            // Block comments /* ... */ (nested)
            if (c == '/' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '*') {
                skip_block_comment();
                continue;
            }

            // Line comments: -- style (not standard REXX but common)
            if (c == '-' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '-') {
                while (pos_ < src_.size() && src_[pos_] != '\n') advance();
                continue;
            }

            // Strings
            if (c == '\'' || c == '"') {
                tokens.push_back(scan_string());
                continue;
            }

            // Numbers
            if (std::isdigit(static_cast<unsigned char>(c))) {
                tokens.push_back(scan_number());
                continue;
            }

            // Identifiers / keywords / labels
            if (is_ident_start(c)) {
                tokens.push_back(scan_identifier_or_keyword());
                // Check for label: identifier immediately followed by ':'
                if (!tokens.empty() && tokens.back().type == TokenType::IDENTIFIER) {
                    size_t save = pos_;
                    skip_whitespace_not_newline();
                    if (pos_ < src_.size() && src_[pos_] == ':') {
                        // It's a label
                        tokens.back().type = TokenType::LABEL;
                        advance(); // consume ':'
                    } else {
                        pos_ = save; // restore
                    }
                }
                continue;
            }

            // Operators and punctuation
            int startline = line_, startcol = col_;
            switch (c) {
                case '+': advance(); tokens.emplace_back(TokenType::PLUS, "+", startline, startcol); break;
                case '-': advance(); tokens.emplace_back(TokenType::MINUS, "-", startline, startcol); break;
                case '%': advance(); tokens.emplace_back(TokenType::PERCENT, "%", startline, startcol); break;
                case '(':  advance(); tokens.emplace_back(TokenType::LPAREN, "(", startline, startcol); break;
                case ')':  advance(); tokens.emplace_back(TokenType::RPAREN, ")", startline, startcol); break;
                case ',':
                    advance();
                    // Comma at end of line = continuation
                    {
                        size_t save = pos_;
                        int sline = line_;
                        skip_whitespace_not_newline();
                        if (pos_ < src_.size() && (src_[pos_] == '\n' || src_[pos_] == '\r')) {
                            // line continuation: skip the newline
                            if (src_[pos_] == '\r') advance();
                            if (pos_ < src_.size() && src_[pos_] == '\n') advance();
                            // Don't emit the comma or newline
                        } else {
                            pos_ = save;
                            line_ = sline;
                            tokens.emplace_back(TokenType::COMMA, ",", startline, startcol);
                        }
                    }
                    break;
                case ';':  advance(); tokens.emplace_back(TokenType::SEMICOLON, ";", startline, startcol); break;
                case '.':  advance(); tokens.emplace_back(TokenType::DOT, ".", startline, startcol); break;
                case '*':
                    advance();
                    if (pos_ < src_.size() && src_[pos_] == '*') {
                        advance();
                        tokens.emplace_back(TokenType::POWER, "**", startline, startcol);
                    } else {
                        tokens.emplace_back(TokenType::STAR, "*", startline, startcol);
                    }
                    break;
                case '/':
                    advance();
                    if (pos_ < src_.size() && src_[pos_] == '/') {
                        advance();
                        tokens.emplace_back(TokenType::DBLSLASH, "//", startline, startcol);
                    } else {
                        tokens.emplace_back(TokenType::SLASH, "/", startline, startcol);
                    }
                    break;
                case '=':
                    advance();
                    if (pos_ < src_.size() && src_[pos_] == '=') {
                        advance();
                        tokens.emplace_back(TokenType::STRICT_EQUAL, "==", startline, startcol);
                    } else {
                        tokens.emplace_back(TokenType::EQUAL, "=", startline, startcol);
                    }
                    break;
                case '\\':
                case '^':
                    advance();
                    if (pos_ < src_.size() && src_[pos_] == '=') {
                        advance();
                        if (pos_ < src_.size() && src_[pos_] == '=') {
                            advance();
                            tokens.emplace_back(TokenType::STRICT_BACKSLASH_EQUAL, "\\==", startline, startcol);
                        } else {
                            tokens.emplace_back(TokenType::BACKSLASH_EQUAL, "\\=", startline, startcol);
                        }
                    } else {
                        tokens.emplace_back(TokenType::NOT, "\\", startline, startcol);
                    }
                    break;
                case '<':
                    advance();
                    if (pos_ < src_.size() && src_[pos_] == '=') {
                        advance();
                        tokens.emplace_back(TokenType::LESS_EQUAL, "<=", startline, startcol);
                    } else if (pos_ < src_.size() && src_[pos_] == '<') {
                        advance();
                        tokens.emplace_back(TokenType::STRICT_LESS, "<<", startline, startcol);
                    } else {
                        tokens.emplace_back(TokenType::LESS, "<", startline, startcol);
                    }
                    break;
                case '>':
                    advance();
                    if (pos_ < src_.size() && src_[pos_] == '=') {
                        advance();
                        tokens.emplace_back(TokenType::GREATER_EQUAL, ">=", startline, startcol);
                    } else if (pos_ < src_.size() && src_[pos_] == '>') {
                        advance();
                        tokens.emplace_back(TokenType::STRICT_GREATER, ">>", startline, startcol);
                    } else {
                        tokens.emplace_back(TokenType::GREATER, ">", startline, startcol);
                    }
                    break;
                case '|':
                    advance();
                    if (pos_ < src_.size() && src_[pos_] == '|') {
                        advance();
                        tokens.emplace_back(TokenType::CONCAT, "||", startline, startcol);
                    } else {
                        tokens.emplace_back(TokenType::OR, "|", startline, startcol);
                    }
                    break;
                case '&':
                    advance();
                    if (pos_ < src_.size() && src_[pos_] == '&') {
                        advance();
                        tokens.emplace_back(TokenType::XOR, "&&", startline, startcol);
                    } else {
                        tokens.emplace_back(TokenType::AND, "&", startline, startcol);
                    }
                    break;
                default:
                    advance(); // skip unknown
                    break;
            }
        }
        tokens.emplace_back(TokenType::END_OF_INPUT, "", line_, col_);
        return tokens;
    }

private:
    std::string_view src_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;

    void advance() {
        if (pos_ < src_.size()) {
            if (src_[pos_] == '\n') { ++line_; col_ = 1; }
            else { ++col_; }
            ++pos_;
        }
    }

    void skip_whitespace_not_newline() {
        while (pos_ < src_.size() && (src_[pos_] == ' ' || src_[pos_] == '\t'))
            advance();
    }

    void skip_block_comment() {
        // Already at '/', next is '*'
        advance(); advance(); // skip /*
        int depth = 1;
        while (pos_ < src_.size() && depth > 0) {
            if (src_[pos_] == '/' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '*') {
                advance(); advance();
                ++depth;
            } else if (src_[pos_] == '*' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '/') {
                advance(); advance();
                --depth;
            } else {
                advance();
            }
        }
    }

    static bool is_ident_start(char c) {
        return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '!' || c == '?' || c == '@' || c == '#' || c == '$';
    }

    static bool is_ident_char(char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '.' || c == '!' || c == '?' || c == '@' || c == '#' || c == '$';
    }

    Token scan_string() {
        int sl = line_, sc = col_;
        char quote = src_[pos_];
        advance(); // skip opening quote
        std::string val;
        while (pos_ < src_.size()) {
            if (src_[pos_] == quote) {
                advance();
                // doubled quote = escaped
                if (pos_ < src_.size() && src_[pos_] == quote) {
                    val += quote;
                    advance();
                } else {
                    break;
                }
            } else {
                val += src_[pos_];
                advance();
            }
        }
        // Check for hex/binary suffix
        if (pos_ < src_.size()) {
            char suf = static_cast<char>(std::toupper(static_cast<unsigned char>(src_[pos_])));
            if (suf == 'X') {
                advance();
                return Token(TokenType::HEX_STRING, val, sl, sc);
            }
            if (suf == 'B') {
                advance();
                return Token(TokenType::BINARY_STRING, val, sl, sc);
            }
        }
        return Token(TokenType::STRING, val, sl, sc);
    }

    Token scan_number() {
        int sl = line_, sc = col_;
        std::string val;
        while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
            val += src_[pos_];
            advance();
        }
        if (pos_ < src_.size() && src_[pos_] == '.' &&
            (pos_ + 1 >= src_.size() || !is_ident_start(src_[pos_ + 1]))) {
            // Check it's a decimal not a stem dot
            size_t next = pos_ + 1;
            if (next < src_.size() && std::isdigit(static_cast<unsigned char>(src_[next]))) {
                val += '.';
                advance();
                while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                    val += src_[pos_];
                    advance();
                }
                // Scientific notation
                if (pos_ < src_.size() && (src_[pos_] == 'e' || src_[pos_] == 'E')) {
                    val += src_[pos_]; advance();
                    if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) {
                        val += src_[pos_]; advance();
                    }
                    while (pos_ < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_]))) {
                        val += src_[pos_]; advance();
                    }
                }
                return Token(TokenType::FLOAT, val, sl, sc);
            }
        }
        return Token(TokenType::INTEGER, val, sl, sc);
    }

    Token scan_identifier_or_keyword() {
        int sl = line_, sc = col_;
        std::string val;
        while (pos_ < src_.size() && is_ident_char(src_[pos_])) {
            val += src_[pos_];
            advance();
        }
        // Check keywords (case-insensitive)
        std::string upper = val;
        std::transform(upper.begin(), upper.end(), upper.begin(),
                       [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });

        for (const auto& kw : kRexxKeywords) {
            if (upper == kw.keyword) {
                return Token(kw.type, upper, sl, sc);
            }
        }
        return Token(TokenType::IDENTIFIER, val, sl, sc);
    }
};

}} // namespace lazarus::rexx

#endif // LAZARUS_REXX_LEXER_H
