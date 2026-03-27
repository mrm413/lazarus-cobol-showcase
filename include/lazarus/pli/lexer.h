#ifndef LAZARUS_PLI_LEXER_H
#define LAZARUS_PLI_LEXER_H

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>
#include <stdexcept>

// Windows headers define UNALIGNED as a macro; neutralize it
#ifdef UNALIGNED
#undef UNALIGNED
#endif

namespace lazarus { namespace pli {

enum class TokenType {
    // Keywords - control
    PROCEDURE, BEGIN, END, DO, WHILE, UNTIL, TO, BY,
    RETURN, CALL, GO_TO, IF, THEN, ELSE,
    SELECT, WHEN, OTHERWISE,
    LEAVE, ITERATE, ENTRY, STOP,

    // Declaration keywords
    DCL, DECLARE, RETURNS, RECURSIVE, OPTIONS,

    // Data types
    FIXED, FLOAT, CHAR, BIT, POINTER, LABEL, FILE_KW,
    AREA, OFFSET, PICTURE, COMPLEX,

    // Attributes
    DEC, BIN, PRECISION, VARYING,
    BASED, DEFINED, CONTROLLED, AUTOMATIC, STATIC,
    EXTERNAL, INTERNAL, ALIGNED, UNALIGNED,
    INITIAL, LIKE,

    // I/O
    PUT, GET, OPEN, CLOSE, READ, WRITE, REWRITE, DELETE_KW,
    LIST, EDIT, DATA, PRINT, INPUT, OUTPUT, UPDATE,
    STREAM, RECORD, SEQUENTIAL, DIRECT, KEYED,
    INTO, FROM, SET, KEYTO, KEYFROM,

    // Condition / signal
    ON, SIGNAL, REVERT, DISPLAY,

    // Memory
    ALLOCATE, FREE,

    // Format
    SKIP, PAGE, LINE_KW, COLUMN, TAB, STRING_KW,

    // Preprocessor
    PCT_INCLUDE, PCT_IF, PCT_THEN, PCT_ELSE, PCT_DECLARE,
    PCT_REPLACE, PCT_ACTIVATE, PCT_DEACTIVATE, PCT_DO, PCT_END,

    // Literals
    INTEGER_LIT, FLOAT_LIT, STRING_LIT, BIT_LIT, COMPLEX_LIT,

    // Operators
    PLUS, MINUS, STAR, SLASH, POWER, CONCAT,
    EQ, NE, GT, LT, GE, LE,
    NOT, AND_OP, OR_OP,
    ASSIGN,

    // Punctuation
    LPAREN, RPAREN, COMMA, SEMICOLON, DOT, COLON,

    // Identifiers
    IDENTIFIER,

    // Special
    COMMENT, END_OF_INPUT,

    // Additional keywords
    MAIN, NULL_KW
};

inline const char* token_type_name(TokenType t) {
    switch (t) {
        case TokenType::PROCEDURE: return "PROCEDURE";
        case TokenType::BEGIN: return "BEGIN";
        case TokenType::END: return "END";
        case TokenType::DO: return "DO";
        case TokenType::WHILE: return "WHILE";
        case TokenType::UNTIL: return "UNTIL";
        case TokenType::TO: return "TO";
        case TokenType::BY: return "BY";
        case TokenType::RETURN: return "RETURN";
        case TokenType::CALL: return "CALL";
        case TokenType::GO_TO: return "GO_TO";
        case TokenType::IF: return "IF";
        case TokenType::THEN: return "THEN";
        case TokenType::ELSE: return "ELSE";
        case TokenType::SELECT: return "SELECT";
        case TokenType::WHEN: return "WHEN";
        case TokenType::OTHERWISE: return "OTHERWISE";
        case TokenType::LEAVE: return "LEAVE";
        case TokenType::ITERATE: return "ITERATE";
        case TokenType::ENTRY: return "ENTRY";
        case TokenType::STOP: return "STOP";
        case TokenType::DCL: return "DCL";
        case TokenType::DECLARE: return "DECLARE";
        case TokenType::RETURNS: return "RETURNS";
        case TokenType::RECURSIVE: return "RECURSIVE";
        case TokenType::OPTIONS: return "OPTIONS";
        case TokenType::FIXED: return "FIXED";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::CHAR: return "CHAR";
        case TokenType::BIT: return "BIT";
        case TokenType::POINTER: return "POINTER";
        case TokenType::LABEL: return "LABEL";
        case TokenType::FILE_KW: return "FILE";
        case TokenType::AREA: return "AREA";
        case TokenType::OFFSET: return "OFFSET";
        case TokenType::PICTURE: return "PICTURE";
        case TokenType::COMPLEX: return "COMPLEX";
        case TokenType::DEC: return "DEC";
        case TokenType::BIN: return "BIN";
        case TokenType::PRECISION: return "PRECISION";
        case TokenType::VARYING: return "VARYING";
        case TokenType::BASED: return "BASED";
        case TokenType::DEFINED: return "DEFINED";
        case TokenType::CONTROLLED: return "CONTROLLED";
        case TokenType::AUTOMATIC: return "AUTOMATIC";
        case TokenType::STATIC: return "STATIC";
        case TokenType::EXTERNAL: return "EXTERNAL";
        case TokenType::INTERNAL: return "INTERNAL";
        case TokenType::ALIGNED: return "ALIGNED";
        case TokenType::UNALIGNED: return "UNALIGNED";
        case TokenType::INITIAL: return "INITIAL";
        case TokenType::LIKE: return "LIKE";
        case TokenType::PUT: return "PUT";
        case TokenType::GET: return "GET";
        case TokenType::OPEN: return "OPEN";
        case TokenType::CLOSE: return "CLOSE";
        case TokenType::READ: return "READ";
        case TokenType::WRITE: return "WRITE";
        case TokenType::REWRITE: return "REWRITE";
        case TokenType::DELETE_KW: return "DELETE";
        case TokenType::LIST: return "LIST";
        case TokenType::EDIT: return "EDIT";
        case TokenType::DATA: return "DATA";
        case TokenType::PRINT: return "PRINT";
        case TokenType::INPUT: return "INPUT";
        case TokenType::OUTPUT: return "OUTPUT";
        case TokenType::UPDATE: return "UPDATE";
        case TokenType::STREAM: return "STREAM";
        case TokenType::RECORD: return "RECORD";
        case TokenType::SEQUENTIAL: return "SEQUENTIAL";
        case TokenType::DIRECT: return "DIRECT";
        case TokenType::KEYED: return "KEYED";
        case TokenType::INTO: return "INTO";
        case TokenType::FROM: return "FROM";
        case TokenType::SET: return "SET";
        case TokenType::KEYTO: return "KEYTO";
        case TokenType::KEYFROM: return "KEYFROM";
        case TokenType::ON: return "ON";
        case TokenType::SIGNAL: return "SIGNAL";
        case TokenType::REVERT: return "REVERT";
        case TokenType::DISPLAY: return "DISPLAY";
        case TokenType::ALLOCATE: return "ALLOCATE";
        case TokenType::FREE: return "FREE";
        case TokenType::SKIP: return "SKIP";
        case TokenType::PAGE: return "PAGE";
        case TokenType::LINE_KW: return "LINE";
        case TokenType::COLUMN: return "COLUMN";
        case TokenType::TAB: return "TAB";
        case TokenType::STRING_KW: return "STRING";
        case TokenType::PCT_INCLUDE: return "%INCLUDE";
        case TokenType::PCT_IF: return "%IF";
        case TokenType::PCT_THEN: return "%THEN";
        case TokenType::PCT_ELSE: return "%ELSE";
        case TokenType::PCT_DECLARE: return "%DECLARE";
        case TokenType::PCT_REPLACE: return "%REPLACE";
        case TokenType::PCT_ACTIVATE: return "%ACTIVATE";
        case TokenType::PCT_DEACTIVATE: return "%DEACTIVATE";
        case TokenType::PCT_DO: return "%DO";
        case TokenType::PCT_END: return "%END";
        case TokenType::INTEGER_LIT: return "INTEGER_LIT";
        case TokenType::FLOAT_LIT: return "FLOAT_LIT";
        case TokenType::STRING_LIT: return "STRING_LIT";
        case TokenType::BIT_LIT: return "BIT_LIT";
        case TokenType::COMPLEX_LIT: return "COMPLEX_LIT";
        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::STAR: return "STAR";
        case TokenType::SLASH: return "SLASH";
        case TokenType::POWER: return "POWER";
        case TokenType::CONCAT: return "CONCAT";
        case TokenType::EQ: return "EQ";
        case TokenType::NE: return "NE";
        case TokenType::GT: return "GT";
        case TokenType::LT: return "LT";
        case TokenType::GE: return "GE";
        case TokenType::LE: return "LE";
        case TokenType::NOT: return "NOT";
        case TokenType::AND_OP: return "AND_OP";
        case TokenType::OR_OP: return "OR_OP";
        case TokenType::ASSIGN: return "ASSIGN";
        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::COMMA: return "COMMA";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::DOT: return "DOT";
        case TokenType::COLON: return "COLON";
        case TokenType::IDENTIFIER: return "IDENTIFIER";
        case TokenType::COMMENT: return "COMMENT";
        case TokenType::END_OF_INPUT: return "END_OF_INPUT";
        case TokenType::MAIN: return "MAIN";
        case TokenType::NULL_KW: return "NULL";
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

// PL/I keywords are case-insensitive
constexpr KeywordEntry kPliKeywords[] = {
    {"PROCEDURE", TokenType::PROCEDURE}, {"PROC", TokenType::PROCEDURE},
    {"BEGIN", TokenType::BEGIN}, {"END", TokenType::END},
    {"DO", TokenType::DO}, {"WHILE", TokenType::WHILE},
    {"UNTIL", TokenType::UNTIL}, {"TO", TokenType::TO},
    {"BY", TokenType::BY}, {"RETURN", TokenType::RETURN},
    {"CALL", TokenType::CALL}, {"GOTO", TokenType::GO_TO},
    {"GO", TokenType::GO_TO},
    {"IF", TokenType::IF}, {"THEN", TokenType::THEN},
    {"ELSE", TokenType::ELSE}, {"SELECT", TokenType::SELECT},
    {"WHEN", TokenType::WHEN}, {"OTHERWISE", TokenType::OTHERWISE},
    {"OTHER", TokenType::OTHERWISE},
    {"LEAVE", TokenType::LEAVE}, {"ITERATE", TokenType::ITERATE},
    {"ENTRY", TokenType::ENTRY}, {"STOP", TokenType::STOP},
    {"DCL", TokenType::DCL}, {"DECLARE", TokenType::DECLARE},
    {"RETURNS", TokenType::RETURNS}, {"RECURSIVE", TokenType::RECURSIVE},
    {"OPTIONS", TokenType::OPTIONS},
    {"FIXED", TokenType::FIXED}, {"FLOAT", TokenType::FLOAT},
    {"CHAR", TokenType::CHAR}, {"CHARACTER", TokenType::CHAR},
    {"BIT", TokenType::BIT},
    {"POINTER", TokenType::POINTER}, {"PTR", TokenType::POINTER},
    {"LABEL", TokenType::LABEL},
    {"FILE", TokenType::FILE_KW}, {"AREA", TokenType::AREA},
    {"OFFSET", TokenType::OFFSET}, {"PICTURE", TokenType::PICTURE},
    {"PIC", TokenType::PICTURE},
    {"COMPLEX", TokenType::COMPLEX}, {"CPLX", TokenType::COMPLEX},
    {"DECIMAL", TokenType::DEC}, {"DEC", TokenType::DEC},
    {"BINARY", TokenType::BIN}, {"BIN", TokenType::BIN},
    {"PRECISION", TokenType::PRECISION}, {"PREC", TokenType::PRECISION},
    {"VARYING", TokenType::VARYING}, {"VAR", TokenType::VARYING},
    {"BASED", TokenType::BASED}, {"DEFINED", TokenType::DEFINED},
    {"DEF", TokenType::DEFINED},
    {"CONTROLLED", TokenType::CONTROLLED}, {"CTL", TokenType::CONTROLLED},
    {"AUTOMATIC", TokenType::AUTOMATIC}, {"AUTO", TokenType::AUTOMATIC},
    {"STATIC", TokenType::STATIC},
    {"EXTERNAL", TokenType::EXTERNAL}, {"EXT", TokenType::EXTERNAL},
    {"INTERNAL", TokenType::INTERNAL}, {"INT", TokenType::INTERNAL},
    {"ALIGNED", TokenType::ALIGNED},
    {"UNALIGNED", TokenType::UNALIGNED}, {"UNAL", TokenType::UNALIGNED},
    {"INITIAL", TokenType::INITIAL}, {"INIT", TokenType::INITIAL},
    {"LIKE", TokenType::LIKE},
    {"PUT", TokenType::PUT}, {"GET", TokenType::GET},
    {"OPEN", TokenType::OPEN}, {"CLOSE", TokenType::CLOSE},
    {"READ", TokenType::READ}, {"WRITE", TokenType::WRITE},
    {"REWRITE", TokenType::REWRITE}, {"DELETE", TokenType::DELETE_KW},
    {"LIST", TokenType::LIST}, {"EDIT", TokenType::EDIT},
    {"DATA", TokenType::DATA}, {"PRINT", TokenType::PRINT},
    {"INPUT", TokenType::INPUT}, {"OUTPUT", TokenType::OUTPUT},
    {"UPDATE", TokenType::UPDATE},
    {"STREAM", TokenType::STREAM}, {"RECORD", TokenType::RECORD},
    {"SEQUENTIAL", TokenType::SEQUENTIAL}, {"SEQL", TokenType::SEQUENTIAL},
    {"DIRECT", TokenType::DIRECT},
    {"KEYED", TokenType::KEYED},
    {"INTO", TokenType::INTO}, {"FROM", TokenType::FROM},
    {"SET", TokenType::SET}, {"KEYTO", TokenType::KEYTO},
    {"KEYFROM", TokenType::KEYFROM},
    {"ON", TokenType::ON}, {"SIGNAL", TokenType::SIGNAL},
    {"REVERT", TokenType::REVERT}, {"DISPLAY", TokenType::DISPLAY},
    {"ALLOCATE", TokenType::ALLOCATE}, {"ALLOC", TokenType::ALLOCATE},
    {"FREE", TokenType::FREE},
    {"SKIP", TokenType::SKIP}, {"PAGE", TokenType::PAGE},
    {"LINE", TokenType::LINE_KW}, {"COLUMN", TokenType::COLUMN},
    {"COL", TokenType::COLUMN},
    {"TAB", TokenType::TAB}, {"STRING", TokenType::STRING_KW},
    {"MAIN", TokenType::MAIN}, {"NULL", TokenType::NULL_KW},
};

class Lexer {
public:
    explicit Lexer(std::string_view source) : src_(source) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        while (pos_ < src_.size()) {
            skip_whitespace();
            if (pos_ >= src_.size()) break;

            int start_line = line_;
            int start_col = col_;

            char c = src_[pos_];

            // Comments: /* ... */
            if (c == '/' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '*') {
                tokens.push_back(lex_comment(start_line, start_col));
                continue;
            }

            // Preprocessor directives: %KEYWORD
            if (c == '%') {
                tokens.push_back(lex_preprocessor(start_line, start_col));
                continue;
            }

            // String literals: 'text'
            if (c == '\'') {
                tokens.push_back(lex_string(start_line, start_col));
                continue;
            }

            // Numbers
            if (std::isdigit(static_cast<unsigned char>(c))) {
                tokens.push_back(lex_number(start_line, start_col));
                continue;
            }

            // Identifiers and keywords
            if (std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '#' || c == '@' || c == '$') {
                tokens.push_back(lex_identifier(start_line, start_col));
                continue;
            }

            // Multi-char operators
            if (c == '*' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '*') {
                advance(); advance();
                tokens.emplace_back(TokenType::POWER, "**", start_line, start_col);
                continue;
            }
            if (c == '|' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '|') {
                advance(); advance();
                tokens.emplace_back(TokenType::CONCAT, "||", start_line, start_col);
                continue;
            }
            if (c == '^' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '=') {
                advance(); advance();
                tokens.emplace_back(TokenType::NE, "^=", start_line, start_col);
                continue;
            }
            if (c == '>' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '=') {
                advance(); advance();
                tokens.emplace_back(TokenType::GE, ">=", start_line, start_col);
                continue;
            }
            if (c == '<' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '=') {
                advance(); advance();
                tokens.emplace_back(TokenType::LE, "<=", start_line, start_col);
                continue;
            }

            // Single-char tokens
            switch (c) {
                case '+': advance(); tokens.emplace_back(TokenType::PLUS, "+", start_line, start_col); continue;
                case '-': advance(); tokens.emplace_back(TokenType::MINUS, "-", start_line, start_col); continue;
                case '*': advance(); tokens.emplace_back(TokenType::STAR, "*", start_line, start_col); continue;
                case '/': advance(); tokens.emplace_back(TokenType::SLASH, "/", start_line, start_col); continue;
                case '=': advance(); tokens.emplace_back(TokenType::EQ, "=", start_line, start_col); continue;
                case '>': advance(); tokens.emplace_back(TokenType::GT, ">", start_line, start_col); continue;
                case '<': advance(); tokens.emplace_back(TokenType::LT, "<", start_line, start_col); continue;
                case '^': advance(); tokens.emplace_back(TokenType::NOT, "^", start_line, start_col); continue;
                case '&': advance(); tokens.emplace_back(TokenType::AND_OP, "&", start_line, start_col); continue;
                case '|': advance(); tokens.emplace_back(TokenType::OR_OP, "|", start_line, start_col); continue;
                case '(': advance(); tokens.emplace_back(TokenType::LPAREN, "(", start_line, start_col); continue;
                case ')': advance(); tokens.emplace_back(TokenType::RPAREN, ")", start_line, start_col); continue;
                case ',': advance(); tokens.emplace_back(TokenType::COMMA, ",", start_line, start_col); continue;
                case ';': advance(); tokens.emplace_back(TokenType::SEMICOLON, ";", start_line, start_col); continue;
                case '.': advance(); tokens.emplace_back(TokenType::DOT, ".", start_line, start_col); continue;
                case ':': advance(); tokens.emplace_back(TokenType::COLON, ":", start_line, start_col); continue;
                default: break;
            }

            // Unknown character, skip
            advance();
        }
        tokens.emplace_back(TokenType::END_OF_INPUT, "", line_, col_);
        return tokens;
    }

private:
    std::string_view src_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;

    char peek() const { return pos_ < src_.size() ? src_[pos_] : '\0'; }

    char advance() {
        char c = src_[pos_++];
        if (c == '\n') { line_++; col_ = 1; }
        else { col_++; }
        return c;
    }

    void skip_whitespace() {
        while (pos_ < src_.size() && std::isspace(static_cast<unsigned char>(src_[pos_]))) {
            advance();
        }
    }

    Token lex_comment(int ln, int col) {
        // Skip /*
        advance(); advance();
        std::string text;
        while (pos_ < src_.size()) {
            if (src_[pos_] == '*' && pos_ + 1 < src_.size() && src_[pos_ + 1] == '/') {
                advance(); advance();
                return Token(TokenType::COMMENT, text, ln, col);
            }
            text += advance();
        }
        return Token(TokenType::COMMENT, text, ln, col);
    }

    Token lex_string(int ln, int col) {
        advance(); // skip opening quote
        std::string text;
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (c == '\'') {
                advance();
                // Check for doubled quote (escape)
                if (pos_ < src_.size() && src_[pos_] == '\'') {
                    text += '\'';
                    advance();
                } else {
                    // Check for B suffix (bit string)
                    if (pos_ < src_.size() &&
                        (src_[pos_] == 'B' || src_[pos_] == 'b')) {
                        advance();
                        return Token(TokenType::BIT_LIT, text, ln, col);
                    }
                    // Check for I suffix (imaginary/complex)
                    if (pos_ < src_.size() &&
                        (src_[pos_] == 'I' || src_[pos_] == 'i')) {
                        advance();
                        return Token(TokenType::COMPLEX_LIT, text, ln, col);
                    }
                    return Token(TokenType::STRING_LIT, text, ln, col);
                }
            } else {
                text += advance();
            }
        }
        return Token(TokenType::STRING_LIT, text, ln, col);
    }

    Token lex_number(int ln, int col) {
        std::string num;
        bool has_dot = false;
        bool has_e = false;

        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (std::isdigit(static_cast<unsigned char>(c))) {
                num += advance();
            } else if (c == '.' && !has_dot && !has_e) {
                // Check it's not a member access following number
                // A float needs a digit after the dot
                if (pos_ + 1 < src_.size() && std::isdigit(static_cast<unsigned char>(src_[pos_ + 1]))) {
                    has_dot = true;
                    num += advance();
                } else {
                    break;
                }
            } else if ((c == 'E' || c == 'e') && !has_e) {
                has_e = true;
                num += advance();
                if (pos_ < src_.size() && (src_[pos_] == '+' || src_[pos_] == '-')) {
                    num += advance();
                }
            } else {
                break;
            }
        }

        if (has_dot || has_e) {
            return Token(TokenType::FLOAT_LIT, num, ln, col);
        }
        return Token(TokenType::INTEGER_LIT, num, ln, col);
    }

    Token lex_identifier(int ln, int col) {
        std::string ident;
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '#' || c == '@' || c == '$') {
                ident += advance();
            } else {
                break;
            }
        }

        // Uppercase for keyword match
        std::string upper = ident;
        for (auto& ch : upper) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

        for (const auto& kw : kPliKeywords) {
            if (upper == kw.keyword) {
                return Token(kw.type, ident, ln, col);
            }
        }
        return Token(TokenType::IDENTIFIER, ident, ln, col);
    }

    Token lex_preprocessor(int ln, int col) {
        advance(); // skip %
        std::string directive;
        while (pos_ < src_.size() && std::isalpha(static_cast<unsigned char>(src_[pos_]))) {
            directive += advance();
        }
        std::string upper = directive;
        for (auto& ch : upper) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));

        if (upper == "INCLUDE") return Token(TokenType::PCT_INCLUDE, "%" + directive, ln, col);
        if (upper == "IF") return Token(TokenType::PCT_IF, "%" + directive, ln, col);
        if (upper == "THEN") return Token(TokenType::PCT_THEN, "%" + directive, ln, col);
        if (upper == "ELSE") return Token(TokenType::PCT_ELSE, "%" + directive, ln, col);
        if (upper == "DECLARE" || upper == "DCL") return Token(TokenType::PCT_DECLARE, "%" + directive, ln, col);
        if (upper == "REPLACE") return Token(TokenType::PCT_REPLACE, "%" + directive, ln, col);
        if (upper == "ACTIVATE") return Token(TokenType::PCT_ACTIVATE, "%" + directive, ln, col);
        if (upper == "DEACTIVATE") return Token(TokenType::PCT_DEACTIVATE, "%" + directive, ln, col);
        if (upper == "DO") return Token(TokenType::PCT_DO, "%" + directive, ln, col);
        if (upper == "END") return Token(TokenType::PCT_END, "%" + directive, ln, col);

        return Token(TokenType::IDENTIFIER, "%" + directive, ln, col);
    }
};

}} // namespace lazarus::pli

#endif // LAZARUS_PLI_LEXER_H
