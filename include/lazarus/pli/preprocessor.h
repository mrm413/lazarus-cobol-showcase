#ifndef LAZARUS_PLI_PREPROCESSOR_H
#define LAZARUS_PLI_PREPROCESSOR_H

#include "lazarus/pli/lexer.h"
#include <string>
#include <map>
#include <set>
#include <vector>
#include <functional>
#include <sstream>
#include <algorithm>

namespace lazarus { namespace pli {

// Preprocessor for PL/I %directives
class Preprocessor {
public:
    using IncludeResolver = std::function<std::string(const std::string&)>;

    Preprocessor() = default;

    // Set an include resolver (returns file contents given a name)
    void set_include_resolver(IncludeResolver resolver) {
        include_resolver_ = std::move(resolver);
    }

    // Define a compile-time constant (%DECLARE or %REPLACE)
    void define(const std::string& name, const std::string& value) {
        constants_[upper(name)] = value;
    }

    // Get a constant value
    std::string get_constant(const std::string& name) const {
        auto it = constants_.find(upper(name));
        if (it != constants_.end()) return it->second;
        return "";
    }

    bool is_defined(const std::string& name) const {
        return constants_.find(upper(name)) != constants_.end();
    }

    // Activate/deactivate a name for conditional compilation
    void activate(const std::string& name) {
        active_names_.insert(upper(name));
    }

    void deactivate(const std::string& name) {
        active_names_.erase(upper(name));
    }

    bool is_active(const std::string& name) const {
        return active_names_.find(upper(name)) != active_names_.end();
    }

    // Process source through preprocessor
    std::string process(const std::string& source) {
        std::vector<Token> tokens = Lexer(source).tokenize();
        std::string result;
        size_t pos = 0;

        while (pos < tokens.size() && tokens[pos].type != TokenType::END_OF_INPUT) {
            const Token& tok = tokens[pos];

            switch (tok.type) {
                case TokenType::PCT_INCLUDE:
                    pos = process_include(tokens, pos, result);
                    break;
                case TokenType::PCT_IF:
                    pos = process_if(tokens, pos, result);
                    break;
                case TokenType::PCT_DECLARE:
                    pos = process_declare(tokens, pos);
                    break;
                case TokenType::PCT_REPLACE:
                    pos = process_replace(tokens, pos);
                    break;
                case TokenType::PCT_ACTIVATE:
                    pos = process_activate(tokens, pos);
                    break;
                case TokenType::PCT_DEACTIVATE:
                    pos = process_deactivate(tokens, pos);
                    break;
                case TokenType::PCT_DO:
                    pos = process_do(tokens, pos, result);
                    break;
                case TokenType::IDENTIFIER: {
                    // Check for macro substitution
                    std::string uname = upper(tok.value);
                    if (constants_.find(uname) != constants_.end()) {
                        result += constants_[uname];
                    } else {
                        result += tok.value;
                    }
                    pos++;
                    break;
                }
                case TokenType::COMMENT:
                    // Strip comments in preprocessor output or keep them
                    result += "/* " + tok.value + " */";
                    pos++;
                    break;
                default:
                    result += tok.value;
                    if (tok.type == TokenType::SEMICOLON) result += "\n";
                    else result += " ";
                    pos++;
                    break;
            }
        }

        return result;
    }

    // Get all defined constants (for inspection/testing)
    const std::map<std::string, std::string>& constants() const { return constants_; }

    // Evaluate a simple preprocessor expression
    bool evaluate_condition(const std::string& expr) const {
        // Simple expression evaluator for %IF conditions
        // Supports: identifier, integer, =, ^=, &, |, ^
        std::string trimmed = expr;
        // Remove leading/trailing whitespace
        while (!trimmed.empty() && trimmed.front() == ' ') trimmed.erase(trimmed.begin());
        while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();

        // Check for ^= comparison (must check before = to avoid partial match)
        auto ne_pos = trimmed.find("^=");
        if (ne_pos != std::string::npos) {
            std::string lhs = trimmed.substr(0, ne_pos);
            std::string rhs = trimmed.substr(ne_pos + 2);
            while (!lhs.empty() && lhs.back() == ' ') lhs.pop_back();
            while (!rhs.empty() && rhs.front() == ' ') rhs.erase(rhs.begin());
            return resolve_compare_value(lhs) != resolve_compare_value(rhs);
        }

        // Check for = comparison
        auto eq_pos = trimmed.find('=');
        if (eq_pos != std::string::npos && eq_pos > 0) {
            std::string lhs = trimmed.substr(0, eq_pos);
            std::string rhs = trimmed.substr(eq_pos + 1);
            while (!lhs.empty() && lhs.back() == ' ') lhs.pop_back();
            while (!rhs.empty() && rhs.front() == ' ') rhs.erase(rhs.begin());
            return resolve_compare_value(lhs) == resolve_compare_value(rhs);
        }

        // Simple truth check: non-zero integer or non-empty string
        std::string val = resolve_value(trimmed);
        if (val.empty()) return false;
        if (val == "0") return false;
        return true;
    }

private:
    std::map<std::string, std::string> constants_;
    std::set<std::string> active_names_;
    IncludeResolver include_resolver_;
    int include_depth_ = 0;
    static constexpr int MAX_INCLUDE_DEPTH = 16;

    static std::string upper(const std::string& s) {
        std::string u = s;
        for (auto& c : u) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return u;
    }

    std::string resolve_value(const std::string& expr) const {
        std::string trimmed = expr;
        while (!trimmed.empty() && trimmed.front() == ' ') trimmed.erase(trimmed.begin());
        while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();

        // Check if it's a constant reference
        std::string uname = upper(trimmed);
        auto it = constants_.find(uname);
        if (it != constants_.end()) return it->second;

        // Check if it's an active name
        if (active_names_.find(uname) != active_names_.end()) return "1";

        // Return as-is (literal)
        // Strip quotes if present
        if (trimmed.size() >= 2 && trimmed.front() == '\'' && trimmed.back() == '\'') {
            return trimmed.substr(1, trimmed.size() - 2);
        }

        // If it looks like an identifier (not a number), it's undefined — return "0"
        if (!trimmed.empty() && std::isalpha(static_cast<unsigned char>(trimmed[0]))) {
            return "0";
        }

        return trimmed;
    }

    // Resolve value for comparison context: constants expand, but bare identifiers
    // return their name as-is (for matching against other identifiers/values)
    std::string resolve_compare_value(const std::string& expr) const {
        std::string trimmed = expr;
        while (!trimmed.empty() && trimmed.front() == ' ') trimmed.erase(trimmed.begin());
        while (!trimmed.empty() && trimmed.back() == ' ') trimmed.pop_back();

        std::string uname = upper(trimmed);
        auto it = constants_.find(uname);
        if (it != constants_.end()) return it->second;
        if (active_names_.find(uname) != active_names_.end()) return "1";
        if (trimmed.size() >= 2 && trimmed.front() == '\'' && trimmed.back() == '\'') {
            return trimmed.substr(1, trimmed.size() - 2);
        }
        return uname;  // return uppercased name for comparison
    }

    // %INCLUDE processing
    size_t process_include(const std::vector<Token>& tokens, size_t pos, std::string& result) {
        pos++; // skip %INCLUDE
        std::string file_name;

        if (pos < tokens.size()) {
            if (tokens[pos].type == TokenType::STRING_LIT) {
                file_name = tokens[pos].value;
                pos++;
            } else if (tokens[pos].type == TokenType::IDENTIFIER) {
                file_name = tokens[pos].value;
                pos++;
            }
        }

        if (pos < tokens.size() && tokens[pos].type == TokenType::SEMICOLON) pos++;

        if (include_resolver_ && include_depth_ < MAX_INCLUDE_DEPTH) {
            include_depth_++;
            std::string included = include_resolver_(file_name);
            result += process(included);
            include_depth_--;
        }

        return pos;
    }

    // %IF / %THEN / %ELSE processing
    size_t process_if(const std::vector<Token>& tokens, size_t pos, std::string& result) {
        pos++; // skip %IF

        // Gather condition tokens until %THEN
        std::string condition;
        while (pos < tokens.size() && tokens[pos].type != TokenType::PCT_THEN) {
            condition += tokens[pos].value + " ";
            pos++;
        }
        if (pos < tokens.size()) pos++; // skip %THEN

        bool cond_true = evaluate_condition(condition);

        // Gather THEN body until %ELSE or %END or next preprocessor
        std::string then_body;
        std::string else_body;
        int depth = 1;

        // Collect then body
        while (pos < tokens.size() && depth > 0) {
            if (tokens[pos].type == TokenType::PCT_IF) depth++;
            if (tokens[pos].type == TokenType::PCT_END) {
                depth--;
                if (depth == 0) { pos++; break; }
            }
            if (tokens[pos].type == TokenType::PCT_ELSE && depth == 1) {
                pos++;
                // Collect else body
                while (pos < tokens.size() && depth > 0) {
                    if (tokens[pos].type == TokenType::PCT_IF) depth++;
                    if (tokens[pos].type == TokenType::PCT_END) {
                        depth--;
                        if (depth == 0) { pos++; break; }
                    }
                    else_body += tokens[pos].value + " ";
                    pos++;
                }
                break;
            }
            then_body += tokens[pos].value + " ";
            pos++;
        }

        // Skip trailing semicolon
        if (pos < tokens.size() && tokens[pos].type == TokenType::SEMICOLON) pos++;

        if (cond_true) {
            result += then_body;
        } else {
            result += else_body;
        }

        return pos;
    }

    // %DECLARE processing
    size_t process_declare(const std::vector<Token>& tokens, size_t pos) {
        pos++; // skip %DECLARE or %DCL

        std::string name;
        if (pos < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER) {
            name = tokens[pos].value;
            pos++;
        }

        // Skip attributes (FIXED, CHAR, etc.)
        while (pos < tokens.size() && tokens[pos].type != TokenType::SEMICOLON) {
            pos++;
        }
        if (pos < tokens.size()) pos++; // skip ;

        // Register the name (default value empty until assigned)
        if (!name.empty()) {
            constants_[upper(name)] = "";
        }

        return pos;
    }

    // %REPLACE processing
    size_t process_replace(const std::vector<Token>& tokens, size_t pos) {
        pos++; // skip %REPLACE

        std::string name;
        if (pos < tokens.size() && tokens[pos].type == TokenType::IDENTIFIER) {
            name = tokens[pos].value;
            pos++;
        }

        // Expect BY
        if (pos < tokens.size() && tokens[pos].type == TokenType::BY) {
            pos++;
        } else if (pos < tokens.size() && upper(tokens[pos].value) == "BY") {
            pos++;
        }

        // Collect value until semicolon
        std::string value;
        while (pos < tokens.size() && tokens[pos].type != TokenType::SEMICOLON) {
            if (!value.empty()) value += " ";
            value += tokens[pos].value;
            pos++;
        }
        if (pos < tokens.size()) pos++; // skip ;

        if (!name.empty()) {
            constants_[upper(name)] = value;
        }

        return pos;
    }

    // %ACTIVATE processing
    size_t process_activate(const std::vector<Token>& tokens, size_t pos) {
        pos++; // skip %ACTIVATE
        while (pos < tokens.size() && tokens[pos].type != TokenType::SEMICOLON) {
            if (tokens[pos].type == TokenType::IDENTIFIER) {
                activate(tokens[pos].value);
            }
            pos++;
        }
        if (pos < tokens.size()) pos++; // skip ;
        return pos;
    }

    // %DEACTIVATE processing
    size_t process_deactivate(const std::vector<Token>& tokens, size_t pos) {
        pos++; // skip %DEACTIVATE
        while (pos < tokens.size() && tokens[pos].type != TokenType::SEMICOLON) {
            if (tokens[pos].type == TokenType::IDENTIFIER) {
                deactivate(tokens[pos].value);
            }
            pos++;
        }
        if (pos < tokens.size()) pos++; // skip ;
        return pos;
    }

    // %DO / %END iteration
    size_t process_do(const std::vector<Token>& tokens, size_t pos, std::string& result) {
        pos++; // skip %DO

        // Simple %DO ... %END (no iteration variable support yet)
        // Just collect and emit the body once
        std::string body;
        int depth = 1;
        while (pos < tokens.size() && depth > 0) {
            if (tokens[pos].type == TokenType::PCT_DO) depth++;
            if (tokens[pos].type == TokenType::PCT_END) {
                depth--;
                if (depth == 0) { pos++; break; }
            }
            body += tokens[pos].value + " ";
            pos++;
        }
        if (pos < tokens.size() && tokens[pos].type == TokenType::SEMICOLON) pos++;

        result += body;
        return pos;
    }
};

}} // namespace lazarus::pli

#endif // LAZARUS_PLI_PREPROCESSOR_H
