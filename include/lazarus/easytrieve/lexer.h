#ifndef LAZARUS_EASYTRIEVE_LEXER_H
#define LAZARUS_EASYTRIEVE_LEXER_H

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <cstring>

namespace lazarus { namespace easytrieve {

// ============================================================
// Token types for Easytrieve language
// ============================================================
enum class TokenType {
    // Declarations
    KW_FILE, KW_RECORD, KW_FIELD, KW_W,
    KW_DEFINE, KW_MACRO, KW_PARM, KW_PROC,
    // Activities
    KW_JOB, KW_SORT, KW_REPORT,
    // Report elements
    KW_SEQUENCE, KW_CONTROL, KW_LINE, KW_HEADING,
    KW_SUM, KW_TITLE, KW_PRINT,
    // Control flow
    KW_IF, KW_ELSE, KW_END_IF, KW_DO, KW_END_DO,
    KW_PERFORM, KW_GO, KW_TO, KW_GOTO, KW_STOP,
    // I/O
    KW_GET, KW_PUT, KW_POINT, KW_READ, KW_WRITE,
    KW_DISPLAY, KW_MOVE, KW_SEARCH,
    // File qualifiers
    KW_LRECL, KW_VIRTUAL, KW_INDEXED, KW_KEY,
    // Comparison operators as keywords
    KW_EQ, KW_NE, KW_LT, KW_LE, KW_GT, KW_GE,
    KW_AND, KW_OR, KW_NOT, KW_RANGE, KW_MASK,
    // Field types
    KW_TYPE_A, KW_TYPE_N, KW_TYPE_P, KW_TYPE_B, KW_TYPE_U,
    // End markers
    KW_END, KW_END_PROC,
    // Literals and identifiers
    IDENTIFIER, STRING_LITERAL, NUMERIC_LITERAL,
    // Symbols
    DOT, COLON, COMMA, LPAREN, RPAREN,
    PLUS, MINUS, STAR, SLASH, EQUALS,
    // Special
    COMMENT, CONTINUATION, NEWLINE, END_OF_INPUT
};

inline const char* token_type_name(TokenType t) {
    switch (t) {
        case TokenType::KW_FILE:     return "FILE";
        case TokenType::KW_RECORD:   return "RECORD";
        case TokenType::KW_FIELD:    return "FIELD";
        case TokenType::KW_W:        return "W";
        case TokenType::KW_DEFINE:   return "DEFINE";
        case TokenType::KW_MACRO:    return "MACRO";
        case TokenType::KW_PARM:     return "PARM";
        case TokenType::KW_PROC:     return "PROC";
        case TokenType::KW_JOB:      return "JOB";
        case TokenType::KW_SORT:     return "SORT";
        case TokenType::KW_REPORT:   return "REPORT";
        case TokenType::KW_SEQUENCE: return "SEQUENCE";
        case TokenType::KW_CONTROL:  return "CONTROL";
        case TokenType::KW_LINE:     return "LINE";
        case TokenType::KW_HEADING:  return "HEADING";
        case TokenType::KW_SUM:      return "SUM";
        case TokenType::KW_TITLE:    return "TITLE";
        case TokenType::KW_PRINT:    return "PRINT";
        case TokenType::KW_IF:       return "IF";
        case TokenType::KW_ELSE:     return "ELSE";
        case TokenType::KW_END_IF:   return "END-IF";
        case TokenType::KW_DO:       return "DO";
        case TokenType::KW_END_DO:   return "END-DO";
        case TokenType::KW_PERFORM:  return "PERFORM";
        case TokenType::KW_GO:       return "GO";
        case TokenType::KW_TO:       return "TO";
        case TokenType::KW_GOTO:     return "GOTO";
        case TokenType::KW_STOP:     return "STOP";
        case TokenType::KW_GET:      return "GET";
        case TokenType::KW_PUT:      return "PUT";
        case TokenType::KW_POINT:    return "POINT";
        case TokenType::KW_READ:     return "READ";
        case TokenType::KW_WRITE:    return "WRITE";
        case TokenType::KW_DISPLAY:  return "DISPLAY";
        case TokenType::KW_MOVE:     return "MOVE";
        case TokenType::KW_SEARCH:   return "SEARCH";
        case TokenType::KW_LRECL:    return "LRECL";
        case TokenType::KW_VIRTUAL:  return "VIRTUAL";
        case TokenType::KW_INDEXED:  return "INDEXED";
        case TokenType::KW_KEY:      return "KEY";
        case TokenType::KW_EQ:       return "EQ";
        case TokenType::KW_NE:       return "NE";
        case TokenType::KW_LT:       return "LT";
        case TokenType::KW_LE:       return "LE";
        case TokenType::KW_GT:       return "GT";
        case TokenType::KW_GE:       return "GE";
        case TokenType::KW_AND:      return "AND";
        case TokenType::KW_OR:       return "OR";
        case TokenType::KW_NOT:      return "NOT";
        case TokenType::KW_RANGE:    return "RANGE";
        case TokenType::KW_MASK:     return "MASK";
        case TokenType::KW_TYPE_A:   return "TYPE_A";
        case TokenType::KW_TYPE_N:   return "TYPE_N";
        case TokenType::KW_TYPE_P:   return "TYPE_P";
        case TokenType::KW_TYPE_B:   return "TYPE_B";
        case TokenType::KW_TYPE_U:   return "TYPE_U";
        case TokenType::KW_END:      return "END";
        case TokenType::KW_END_PROC: return "END-PROC";
        case TokenType::IDENTIFIER:      return "IDENTIFIER";
        case TokenType::STRING_LITERAL:  return "STRING_LITERAL";
        case TokenType::NUMERIC_LITERAL: return "NUMERIC_LITERAL";
        case TokenType::DOT:         return "DOT";
        case TokenType::COLON:       return "COLON";
        case TokenType::COMMA:       return "COMMA";
        case TokenType::LPAREN:      return "LPAREN";
        case TokenType::RPAREN:      return "RPAREN";
        case TokenType::PLUS:        return "PLUS";
        case TokenType::MINUS:       return "MINUS";
        case TokenType::STAR:        return "STAR";
        case TokenType::SLASH:       return "SLASH";
        case TokenType::EQUALS:      return "EQUALS";
        case TokenType::COMMENT:     return "COMMENT";
        case TokenType::CONTINUATION:return "CONTINUATION";
        case TokenType::NEWLINE:     return "NEWLINE";
        case TokenType::END_OF_INPUT:return "END_OF_INPUT";
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

// ============================================================
// Keyword lookup
// ============================================================
struct KeywordEntry {
    const char* keyword;
    TokenType type;
};

// Compound keywords checked first
inline TokenType lookup_compound_keyword(const std::string& w1, const std::string& w2) {
    if (w1 == "END" && w2 == "IF")   return TokenType::KW_END_IF;
    if (w1 == "END" && w2 == "DO")   return TokenType::KW_END_DO;
    if (w1 == "END" && w2 == "PROC") return TokenType::KW_END_PROC;
    if (w1 == "GO"  && w2 == "TO")   return TokenType::KW_GOTO;
    return TokenType::END_OF_INPUT; // sentinel: no match
}

inline TokenType lookup_keyword(const std::string& word) {
    static const KeywordEntry table[] = {
        {"FILE",     TokenType::KW_FILE},
        {"RECORD",   TokenType::KW_RECORD},
        {"FIELD",    TokenType::KW_FIELD},
        {"W",        TokenType::KW_W},
        {"DEFINE",   TokenType::KW_DEFINE},
        {"MACRO",    TokenType::KW_MACRO},
        {"PARM",     TokenType::KW_PARM},
        {"PROC",     TokenType::KW_PROC},
        {"JOB",      TokenType::KW_JOB},
        {"SORT",     TokenType::KW_SORT},
        {"REPORT",   TokenType::KW_REPORT},
        {"SEQUENCE", TokenType::KW_SEQUENCE},
        {"CONTROL",  TokenType::KW_CONTROL},
        {"LINE",     TokenType::KW_LINE},
        {"HEADING",  TokenType::KW_HEADING},
        {"SUM",      TokenType::KW_SUM},
        {"TITLE",    TokenType::KW_TITLE},
        {"PRINT",    TokenType::KW_PRINT},
        {"IF",       TokenType::KW_IF},
        {"ELSE",     TokenType::KW_ELSE},
        {"END-IF",   TokenType::KW_END_IF},
        {"DO",       TokenType::KW_DO},
        {"END-DO",   TokenType::KW_END_DO},
        {"PERFORM",  TokenType::KW_PERFORM},
        {"GO",       TokenType::KW_GO},
        {"TO",       TokenType::KW_TO},
        {"GOTO",     TokenType::KW_GOTO},
        {"STOP",     TokenType::KW_STOP},
        {"GET",      TokenType::KW_GET},
        {"PUT",      TokenType::KW_PUT},
        {"POINT",    TokenType::KW_POINT},
        {"READ",     TokenType::KW_READ},
        {"WRITE",    TokenType::KW_WRITE},
        {"DISPLAY",  TokenType::KW_DISPLAY},
        {"MOVE",     TokenType::KW_MOVE},
        {"SEARCH",   TokenType::KW_SEARCH},
        {"LRECL",    TokenType::KW_LRECL},
        {"VIRTUAL",  TokenType::KW_VIRTUAL},
        {"INDEXED",  TokenType::KW_INDEXED},
        {"KEY",      TokenType::KW_KEY},
        {"EQ",       TokenType::KW_EQ},
        {"NE",       TokenType::KW_NE},
        {"LT",       TokenType::KW_LT},
        {"LE",       TokenType::KW_LE},
        {"GT",       TokenType::KW_GT},
        {"GE",       TokenType::KW_GE},
        {"AND",      TokenType::KW_AND},
        {"OR",       TokenType::KW_OR},
        {"NOT",      TokenType::KW_NOT},
        {"RANGE",    TokenType::KW_RANGE},
        {"MASK",     TokenType::KW_MASK},
        {"END",      TokenType::KW_END},
        {"END-PROC", TokenType::KW_END_PROC},
    };
    for (const auto& entry : table) {
        if (word == entry.keyword) return entry.type;
    }
    return TokenType::IDENTIFIER;
}

inline bool is_type_char(char c) {
    return c == 'A' || c == 'N' || c == 'P' || c == 'B' || c == 'U';
}

// ============================================================
// Lexer: string_view based, zero-copy where possible
// ============================================================
class Lexer {
public:
    explicit Lexer(std::string_view source) : source_(source) {}

    std::vector<Token> tokenize() {
        tokens_.clear();
        pos_ = 0;
        line_ = 1;
        col_ = 1;
        at_line_start_ = true;

        while (pos_ < source_.size()) {
            // Check for comment (* in column 1)
            if (at_line_start_ && pos_ < source_.size() && source_[pos_] == '*') {
                scan_comment();
                continue;
            }

            // Check for continuation (+ in column 1)
            if (at_line_start_ && pos_ < source_.size() && source_[pos_] == '+') {
                tokens_.emplace_back(TokenType::CONTINUATION, "+", line_, col_);
                advance();
                at_line_start_ = false;
                continue;
            }

            char c = source_[pos_];

            // Newline
            if (c == '\n') {
                tokens_.emplace_back(TokenType::NEWLINE, "\\n", line_, col_);
                advance();
                at_line_start_ = true;
                continue;
            }
            if (c == '\r') {
                advance();
                if (pos_ < source_.size() && source_[pos_] == '\n') {
                    advance();
                }
                tokens_.emplace_back(TokenType::NEWLINE, "\\n", line_ - 1, col_);
                at_line_start_ = true;
                continue;
            }

            // Whitespace
            if (c == ' ' || c == '\t') {
                advance();
                at_line_start_ = false;
                continue;
            }

            at_line_start_ = false;

            // String literal (single-quoted)
            if (c == '\'') {
                scan_string();
                continue;
            }

            // Numeric literal
            if (std::isdigit(static_cast<unsigned char>(c)) ||
                (c == '-' && pos_ + 1 < source_.size() &&
                 std::isdigit(static_cast<unsigned char>(source_[pos_ + 1])) &&
                 should_be_negative_number())) {
                scan_number();
                continue;
            }

            // Symbols
            switch (c) {
                case '.': emit(TokenType::DOT, "."); advance(); continue;
                case ':': emit(TokenType::COLON, ":"); advance(); continue;
                case ',': emit(TokenType::COMMA, ","); advance(); continue;
                case '(': emit(TokenType::LPAREN, "("); advance(); continue;
                case ')': emit(TokenType::RPAREN, ")"); advance(); continue;
                case '+': emit(TokenType::PLUS, "+"); advance(); continue;
                case '-':
                    // Check for hyphenated keywords like END-IF
                    if (pos_ + 1 < source_.size() &&
                        std::isalpha(static_cast<unsigned char>(source_[pos_ + 1]))) {
                        // May be part of a compound identifier; let identifier scanner handle
                        scan_identifier_or_keyword();
                        continue;
                    }
                    emit(TokenType::MINUS, "-");
                    advance();
                    continue;
                case '*': emit(TokenType::STAR, "*"); advance(); continue;
                case '/': emit(TokenType::SLASH, "/"); advance(); continue;
                case '=': emit(TokenType::EQUALS, "="); advance(); continue;
                default: break;
            }

            // Identifier or keyword
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '#' || c == '@' || c == '$') {
                scan_identifier_or_keyword();
                continue;
            }

            // Unknown character -- skip
            advance();
        }

        tokens_.emplace_back(TokenType::END_OF_INPUT, "", line_, col_);
        return tokens_;
    }

    // Convenience: tokenize and strip comments/newlines/continuations
    std::vector<Token> tokenize_clean() {
        auto all = tokenize();
        std::vector<Token> result;
        result.reserve(all.size());
        for (auto& t : all) {
            if (t.type != TokenType::COMMENT &&
                t.type != TokenType::NEWLINE &&
                t.type != TokenType::CONTINUATION) {
                result.push_back(std::move(t));
            }
        }
        return result;
    }

    // Join continuation lines: merge a physical line ending with + on next line
    static std::string join_continuations(std::string_view source) {
        std::string result;
        result.reserve(source.size());
        size_t i = 0;
        while (i < source.size()) {
            if (source[i] == '\n' || (source[i] == '\r' && i + 1 < source.size() && source[i+1] == '\n')) {
                size_t nl_end = (source[i] == '\r') ? i + 2 : i + 1;
                if (nl_end < source.size() && source[nl_end] == '+') {
                    // Skip newline and continuation char
                    result += ' ';
                    i = nl_end + 1;
                    continue;
                }
            }
            result += source[i];
            ++i;
        }
        return result;
    }

private:
    std::string_view source_;
    std::vector<Token> tokens_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;
    bool at_line_start_ = true;

    char peek() const {
        return (pos_ < source_.size()) ? source_[pos_] : '\0';
    }

    void advance() {
        if (pos_ < source_.size()) {
            if (source_[pos_] == '\n') {
                line_++;
                col_ = 1;
            } else {
                col_++;
            }
            pos_++;
        }
    }

    void emit(TokenType type, const std::string& val) {
        tokens_.emplace_back(type, val, line_, col_);
    }

    bool should_be_negative_number() const {
        // A minus is a negative number sign if preceded by an operator or start-of-expression context
        if (tokens_.empty()) return true;
        auto last = tokens_.back().type;
        return last == TokenType::EQUALS || last == TokenType::LPAREN ||
               last == TokenType::COMMA || last == TokenType::PLUS ||
               last == TokenType::MINUS || last == TokenType::STAR ||
               last == TokenType::SLASH || last == TokenType::NEWLINE;
    }

    void scan_comment() {
        int start_line = line_;
        int start_col = col_;
        std::string text;
        // Skip *
        advance();
        while (pos_ < source_.size() && source_[pos_] != '\n' && source_[pos_] != '\r') {
            text += source_[pos_];
            advance();
        }
        tokens_.emplace_back(TokenType::COMMENT, text, start_line, start_col);
    }

    void scan_string() {
        int start_line = line_;
        int start_col = col_;
        advance(); // skip opening quote
        std::string val;
        while (pos_ < source_.size()) {
            if (source_[pos_] == '\'') {
                advance();
                // Doubled quote = escaped quote
                if (pos_ < source_.size() && source_[pos_] == '\'') {
                    val += '\'';
                    advance();
                    continue;
                }
                break;
            }
            val += source_[pos_];
            advance();
        }
        tokens_.emplace_back(TokenType::STRING_LITERAL, val, start_line, start_col);
    }

    void scan_number() {
        int start_line = line_;
        int start_col = col_;
        std::string val;
        if (source_[pos_] == '-') {
            val += '-';
            advance();
        }
        while (pos_ < source_.size() &&
               (std::isdigit(static_cast<unsigned char>(source_[pos_])) || source_[pos_] == '.')) {
            val += source_[pos_];
            advance();
        }
        tokens_.emplace_back(TokenType::NUMERIC_LITERAL, val, start_line, start_col);
    }

    void scan_identifier_or_keyword() {
        int start_line = line_;
        int start_col = col_;
        std::string val;

        // Leading minus already consumed if compound (e.g., -IF after END)
        // Actually, we enter here from alpha/# start
        // Handle potential prefix from back-merge of hyphenated token
        if (!tokens_.empty() && tokens_.back().type == TokenType::IDENTIFIER) {
            // Check if this was triggered by a hyphen continuation
        }

        while (pos_ < source_.size()) {
            char c = source_[pos_];
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' ||
                c == '#' || c == '@' || c == '$') {
                val += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                advance();
            } else {
                break;
            }
        }

        // Check for single-char field type in context: standalone A, N, P, B, U
        // Only if exactly one character and it's a type char
        if (val.size() == 1 && is_type_char(val[0])) {
            switch (val[0]) {
                case 'A': tokens_.emplace_back(TokenType::KW_TYPE_A, "A", start_line, start_col); return;
                case 'N': tokens_.emplace_back(TokenType::KW_TYPE_N, "N", start_line, start_col); return;
                case 'P': tokens_.emplace_back(TokenType::KW_TYPE_P, "P", start_line, start_col); return;
                case 'B': tokens_.emplace_back(TokenType::KW_TYPE_B, "B", start_line, start_col); return;
                case 'U': tokens_.emplace_back(TokenType::KW_TYPE_U, "U", start_line, start_col); return;
                default: break;
            }
        }

        TokenType kw = lookup_keyword(val);
        tokens_.emplace_back(kw, val, start_line, start_col);
    }
};

}} // namespace lazarus::easytrieve

#endif // LAZARUS_EASYTRIEVE_LEXER_H
