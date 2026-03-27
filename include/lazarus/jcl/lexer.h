#ifndef LAZARUS_JCL_LEXER_H
#define LAZARUS_JCL_LEXER_H

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace lazarus { namespace jcl {

enum class TokenType {
    JOB, EXEC, DD, PROC, PEND, IF, THEN, ELSE, ENDIF,
    COMMENT, DELIMITER, CONTINUATION, OPER, PARAM, LITERAL,
    SET, INCLUDE, JCLLIB, OUTPUT, COND, NOTIFY,
    LABEL, EQUALS, COMMA, LPAREN, RPAREN, ASTERISK,
    INSTREAM_DATA, END_OF_INPUT
};

inline const char* token_type_name(TokenType t) {
    switch (t) {
        case TokenType::JOB:          return "JOB";
        case TokenType::EXEC:         return "EXEC";
        case TokenType::DD:           return "DD";
        case TokenType::PROC:         return "PROC";
        case TokenType::PEND:         return "PEND";
        case TokenType::IF:           return "IF";
        case TokenType::THEN:         return "THEN";
        case TokenType::ELSE:         return "ELSE";
        case TokenType::ENDIF:        return "ENDIF";
        case TokenType::COMMENT:      return "COMMENT";
        case TokenType::DELIMITER:    return "DELIMITER";
        case TokenType::CONTINUATION: return "CONTINUATION";
        case TokenType::OPER:         return "OPER";
        case TokenType::PARAM:        return "PARAM";
        case TokenType::LITERAL:      return "LITERAL";
        case TokenType::SET:          return "SET";
        case TokenType::INCLUDE:      return "INCLUDE";
        case TokenType::JCLLIB:       return "JCLLIB";
        case TokenType::OUTPUT:       return "OUTPUT";
        case TokenType::COND:         return "COND";
        case TokenType::NOTIFY:       return "NOTIFY";
        case TokenType::LABEL:        return "LABEL";
        case TokenType::EQUALS:       return "EQUALS";
        case TokenType::COMMA:        return "COMMA";
        case TokenType::LPAREN:       return "LPAREN";
        case TokenType::RPAREN:       return "RPAREN";
        case TokenType::ASTERISK:     return "ASTERISK";
        case TokenType::INSTREAM_DATA:return "INSTREAM_DATA";
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

// constexpr keyword lookup table
struct KeywordEntry {
    const char* keyword;
    TokenType type;
};

constexpr KeywordEntry kKeywords[] = {
    {"JOB",     TokenType::JOB},
    {"EXEC",    TokenType::EXEC},
    {"DD",      TokenType::DD},
    {"PROC",    TokenType::PROC},
    {"PEND",    TokenType::PEND},
    {"IF",      TokenType::IF},
    {"THEN",    TokenType::THEN},
    {"ELSE",    TokenType::ELSE},
    {"ENDIF",   TokenType::ENDIF},
    {"SET",     TokenType::SET},
    {"INCLUDE", TokenType::INCLUDE},
    {"JCLLIB",  TokenType::JCLLIB},
    {"OUTPUT",  TokenType::OUTPUT},
    {"COND",    TokenType::COND},
    {"NOTIFY",  TokenType::NOTIFY},
};

constexpr size_t kKeywordCount = sizeof(kKeywords) / sizeof(kKeywords[0]);

inline std::string to_upper(std::string_view sv) {
    std::string result(sv);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

inline TokenType classify_keyword(std::string_view word) {
    std::string upper = to_upper(word);
    for (size_t i = 0; i < kKeywordCount; ++i) {
        if (upper == kKeywords[i].keyword) {
            return kKeywords[i].type;
        }
    }
    return TokenType::OPER;
}

inline bool is_jcl_name_char(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '@' || c == '#' || c == '$' || c == '_';
}

inline bool is_jcl_statement(std::string_view line) {
    return line.size() >= 2 && line[0] == '/' && line[1] == '/';
}

inline bool is_comment(std::string_view line) {
    return line.size() >= 3 && line[0] == '/' && line[1] == '/' && line[2] == '*';
}

inline bool is_delimiter(std::string_view line) {
    return line.size() >= 2 && line[0] == '/' && line[1] == '*';
}

inline bool is_continuation_line(std::string_view line) {
    // A continuation line starts with // and has leading spaces before content
    // (no label at column 3)
    if (line.size() < 3) return false;
    if (line[0] != '/' || line[1] != '/') return false;
    if (line[2] == '*') return false;  // comment
    if (line[2] == ' ' && line.size() > 3) {
        // Check if there's content after spaces (continuation of previous)
        size_t pos = 2;
        while (pos < line.size() && line[pos] == ' ') ++pos;
        return pos > 3 && pos < line.size();
    }
    return false;
}

inline bool has_continuation_mark(std::string_view line) {
    // In 80-column card image, column 72 (0-indexed: 71) non-blank = continuation
    if (line.size() >= 72) {
        return line[71] != ' ';
    }
    // For shorter lines, check if line ends with comma (implicit continuation)
    if (line.empty()) return false;
    auto trimmed = line;
    while (!trimmed.empty() && trimmed.back() == ' ') {
        trimmed.remove_suffix(1);
    }
    return !trimmed.empty() && trimmed.back() == ',';
}

class Lexer {
public:
    explicit Lexer(std::string_view input) : input_(input), line_num_(1) {
        split_lines();
        tokenize();
    }

    const std::vector<Token>& tokens() const { return tokens_; }

    std::vector<Token> tokens_of_type(TokenType type) const {
        std::vector<Token> result;
        for (auto& t : tokens_) {
            if (t.type == type) result.push_back(t);
        }
        return result;
    }

    bool has_token(TokenType type) const {
        for (auto& t : tokens_) {
            if (t.type == type) return true;
        }
        return false;
    }

    size_t count_tokens(TokenType type) const {
        size_t n = 0;
        for (auto& t : tokens_) {
            if (t.type == type) ++n;
        }
        return n;
    }

private:
    std::string input_storage_;
    std::string_view input_;
    std::vector<std::string> lines_;
    std::vector<Token> tokens_;
    int line_num_;

    void split_lines() {
        input_storage_ = std::string(input_);
        std::string_view sv(input_storage_);
        while (!sv.empty()) {
            auto nl = sv.find('\n');
            if (nl == std::string_view::npos) {
                lines_.emplace_back(sv);
                break;
            }
            std::string line(sv.substr(0, nl));
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            lines_.push_back(std::move(line));
            sv.remove_prefix(nl + 1);
        }
    }

    void emit(TokenType type, std::string value, int line, int col) {
        tokens_.emplace_back(type, std::move(value), line, col);
    }

    std::string trim(std::string_view sv) const {
        while (!sv.empty() && sv.front() == ' ') sv.remove_prefix(1);
        while (!sv.empty() && sv.back() == ' ') sv.remove_suffix(1);
        return std::string(sv);
    }

    // Extract content from the operand field, truncating at col 72 for 80-col cards
    std::string_view operand_field(std::string_view line) const {
        if (line.size() > 71) {
            return line.substr(0, 71);
        }
        return line;
    }

    void tokenize_params(std::string_view params, int line, int base_col) {
        size_t pos = 0;
        while (pos < params.size()) {
            char c = params[pos];
            if (c == ' ') { ++pos; continue; }
            if (c == '=') {
                emit(TokenType::EQUALS, "=", line, base_col + static_cast<int>(pos));
                ++pos;
                continue;
            }
            if (c == ',') {
                emit(TokenType::COMMA, ",", line, base_col + static_cast<int>(pos));
                ++pos;
                continue;
            }
            if (c == '(') {
                emit(TokenType::LPAREN, "(", line, base_col + static_cast<int>(pos));
                ++pos;
                continue;
            }
            if (c == ')') {
                emit(TokenType::RPAREN, ")", line, base_col + static_cast<int>(pos));
                ++pos;
                continue;
            }
            if (c == '*') {
                // Could be DD * for instream or *.referback
                emit(TokenType::ASTERISK, "*", line, base_col + static_cast<int>(pos));
                ++pos;
                continue;
            }
            if (c == '\'') {
                // Quoted literal
                size_t start = pos;
                ++pos;
                std::string lit;
                lit += '\'';
                while (pos < params.size()) {
                    if (params[pos] == '\'') {
                        lit += '\'';
                        ++pos;
                        if (pos < params.size() && params[pos] == '\'') {
                            lit += '\'';
                            ++pos;
                            continue;
                        }
                        break;
                    }
                    lit += params[pos];
                    ++pos;
                }
                emit(TokenType::LITERAL, lit, line, base_col + static_cast<int>(start));
                continue;
            }
            // Identifier or value
            size_t start = pos;
            while (pos < params.size() && params[pos] != ' ' && params[pos] != '='
                   && params[pos] != ',' && params[pos] != '(' && params[pos] != ')'
                   && params[pos] != '\'') {
                ++pos;
            }
            if (pos > start) {
                std::string val(params.substr(start, pos - start));
                // Check if it's a keyword in parameter position
                std::string upper = to_upper(val);
                if (upper == "COND") {
                    emit(TokenType::COND, val, line, base_col + static_cast<int>(start));
                } else if (upper == "NOTIFY") {
                    emit(TokenType::NOTIFY, val, line, base_col + static_cast<int>(start));
                } else if (upper == "THEN") {
                    emit(TokenType::THEN, val, line, base_col + static_cast<int>(start));
                } else if (upper == "ELSE") {
                    emit(TokenType::ELSE, val, line, base_col + static_cast<int>(start));
                } else if (upper == "ENDIF") {
                    emit(TokenType::ENDIF, val, line, base_col + static_cast<int>(start));
                } else {
                    emit(TokenType::PARAM, val, line, base_col + static_cast<int>(start));
                }
            }
        }
    }

    void tokenize() {
        size_t i = 0;
        bool in_instream = false;

        while (i < lines_.size()) {
            int ln = static_cast<int>(i) + 1;
            const std::string& line = lines_[i];

            // Handle instream data mode
            if (in_instream) {
                if (is_delimiter(line)) {
                    emit(TokenType::DELIMITER, line, ln, 0);
                    in_instream = false;
                    ++i;
                    continue;
                }
                emit(TokenType::INSTREAM_DATA, line, ln, 0);
                ++i;
                continue;
            }

            // Empty line
            if (line.empty()) {
                ++i;
                continue;
            }

            // Delimiter statement /*
            if (is_delimiter(line) && !is_jcl_statement(line)) {
                emit(TokenType::DELIMITER, line, ln, 0);
                ++i;
                continue;
            }

            // Comment //*
            if (is_comment(line)) {
                std::string_view content = std::string_view(line).substr(3);
                emit(TokenType::COMMENT, trim(content), ln, 3);
                ++i;
                continue;
            }

            // JCL statement //
            if (is_jcl_statement(line)) {
                std::string_view sv(line);
                std::string_view field = operand_field(sv);
                size_t pos = 2;

                // Extract label (name field) - starts at column 3
                std::string label;
                if (pos < field.size() && field[pos] != ' ') {
                    size_t start = pos;
                    while (pos < field.size() && is_jcl_name_char(field[pos])) {
                        ++pos;
                    }
                    label = std::string(field.substr(start, pos - start));
                    emit(TokenType::LABEL, label, ln, static_cast<int>(start));
                }

                // Skip spaces to operation field
                while (pos < field.size() && field[pos] == ' ') ++pos;

                if (pos >= field.size()) {
                    // Null statement or continuation-only
                    ++i;
                    continue;
                }

                // Extract operation
                size_t op_start = pos;
                while (pos < field.size() && field[pos] != ' ') ++pos;
                std::string operation(field.substr(op_start, pos - op_start));
                TokenType op_type = classify_keyword(operation);
                emit(op_type, operation, ln, static_cast<int>(op_start));

                // Skip spaces to operand field
                while (pos < field.size() && field[pos] == ' ') ++pos;

                // Gather operand (may span continuation lines)
                std::string operand;
                if (pos < field.size()) {
                    operand = trim(std::string(field.substr(pos)));
                }

                // Check for continuation
                bool continued = has_continuation_mark(line);

                // Collect continuation lines
                while (continued && (i + 1) < lines_.size()) {
                    ++i;
                    ln = static_cast<int>(i) + 1;
                    const std::string& cline = lines_[i];

                    if (cline.size() < 3 || cline[0] != '/' || cline[1] != '/') break;
                    if (is_comment(cline)) {
                        emit(TokenType::COMMENT, trim(std::string_view(cline).substr(3)), ln, 3);
                        continue;
                    }

                    emit(TokenType::CONTINUATION, cline, ln, 0);

                    std::string_view cfield = operand_field(std::string_view(cline));
                    size_t cpos = 2;
                    // Skip leading spaces on continuation
                    while (cpos < cfield.size() && cfield[cpos] == ' ') ++cpos;

                    if (cpos < cfield.size()) {
                        std::string cont_text = trim(std::string(cfield.substr(cpos)));
                        if (!cont_text.empty()) {
                            operand += cont_text;
                        }
                    }

                    continued = has_continuation_mark(cline);
                }

                // Tokenize the operand field
                if (!operand.empty()) {
                    tokenize_params(operand, static_cast<int>(i) + 1, static_cast<int>(pos));
                }

                // Check if DD * or DD DATA -> enter instream mode
                if (op_type == TokenType::DD) {
                    std::string upper_op = to_upper(operand);
                    if (upper_op == "*" || upper_op == "DATA" ||
                        upper_op.find("*") == 0 || upper_op.find("DATA") == 0) {
                        // Check if operand is just * or DATA (not DSN=...)
                        auto params_trimmed = trim(operand);
                        if (params_trimmed == "*" || params_trimmed == "DATA") {
                            in_instream = true;
                        }
                    }
                }

                ++i;
                continue;
            }

            // Non-JCL line (possible instream data without explicit DD *)
            emit(TokenType::INSTREAM_DATA, line, ln, 0);
            ++i;
        }

        emit(TokenType::END_OF_INPUT, "", static_cast<int>(lines_.size()) + 1, 0);
    }
};

}} // namespace lazarus::jcl

#endif // LAZARUS_JCL_LEXER_H
