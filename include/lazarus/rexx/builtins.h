#ifndef LAZARUS_REXX_BUILTINS_H
#define LAZARUS_REXX_BUILTINS_H

#include "lazarus/rexx/ast.h"
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <numeric>
#include <functional>
#include <unordered_map>
#include <stdexcept>

namespace lazarus { namespace rexx { namespace builtins {

// ── String helpers ─────────────────────────────────────────────────────────
inline std::string to_upper(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return r;
}

inline std::string to_lower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

// Count words in a string
inline int count_words(const std::string& s) {
    int count = 0;
    bool in_word = false;
    for (char c : s) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            ++count;
        }
    }
    return count;
}

// Get nth word (1-based)
inline std::string get_word(const std::string& s, int n) {
    int count = 0;
    size_t start = 0;
    bool in_word = false;
    for (size_t i = 0; i <= s.size(); ++i) {
        bool ws = (i == s.size()) || std::isspace(static_cast<unsigned char>(s[i]));
        if (!ws && !in_word) { in_word = true; start = i; ++count; }
        if (ws && in_word) {
            in_word = false;
            if (count == n) return s.substr(start, i - start);
        }
    }
    return "";
}

// Get position of nth word (1-based)
inline int word_index(const std::string& s, int n) {
    int count = 0;
    bool in_word = false;
    for (size_t i = 0; i < s.size(); ++i) {
        bool ws = std::isspace(static_cast<unsigned char>(s[i]));
        if (!ws && !in_word) {
            in_word = true;
            ++count;
            if (count == n) return static_cast<int>(i) + 1; // 1-based
        }
        if (ws) in_word = false;
    }
    return 0;
}

// Word length
inline int word_length(const std::string& s, int n) {
    return static_cast<int>(get_word(s, n).size());
}

// ── STRING FUNCTIONS ───────────────────────────────────────────────────────

// ABBREV(info, info_abbrev [,length])
inline RexxValue fn_abbrev(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(int64_t(0));
    std::string info = args[0].to_string();
    std::string abbr = args[1].to_string();
    int minlen = args.size() > 2 ? static_cast<int>(args[2].to_integer()) : static_cast<int>(abbr.size());
    if (static_cast<int>(abbr.size()) < minlen) return RexxValue(int64_t(0));
    if (abbr.size() > info.size()) return RexxValue(int64_t(0));
    std::string upper_info = to_upper(info.substr(0, abbr.size()));
    std::string upper_abbr = to_upper(abbr);
    return RexxValue(int64_t(upper_info == upper_abbr ? 1 : 0));
}

// CENTER/CENTRE(string, length [,pad])
inline RexxValue fn_center(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    int len = static_cast<int>(args[1].to_integer());
    char pad = args.size() > 2 ? args[2].to_string()[0] : ' ';
    if (len <= 0) return RexxValue(std::string(""));
    int slen = static_cast<int>(s.size());
    if (slen >= len) {
        int start = (slen - len) / 2;
        return RexxValue(s.substr(static_cast<size_t>(start), static_cast<size_t>(len)));
    }
    int left_pad = (len - slen) / 2;
    int right_pad = len - slen - left_pad;
    return RexxValue(std::string(static_cast<size_t>(left_pad), pad) + s +
                     std::string(static_cast<size_t>(right_pad), pad));
}

// CHANGESTR(needle, haystack, new)
inline RexxValue fn_changestr(const std::vector<RexxValue>& args) {
    if (args.size() < 3) return RexxValue(std::string(""));
    std::string needle = args[0].to_string();
    std::string haystack = args[1].to_string();
    std::string repl = args[2].to_string();
    if (needle.empty()) return RexxValue(haystack);
    std::string result;
    size_t pos = 0;
    while (pos < haystack.size()) {
        size_t found = haystack.find(needle, pos);
        if (found == std::string::npos) {
            result += haystack.substr(pos);
            break;
        }
        result += haystack.substr(pos, found - pos) + repl;
        pos = found + needle.size();
    }
    return RexxValue(result);
}

// COMPARE(string1, string2 [,pad])
inline RexxValue fn_compare(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(int64_t(0));
    std::string s1 = args[0].to_string();
    std::string s2 = args[1].to_string();
    char pad = args.size() > 2 ? args[2].to_string()[0] : ' ';
    size_t maxlen = std::max(s1.size(), s2.size());
    while (s1.size() < maxlen) s1 += pad;
    while (s2.size() < maxlen) s2 += pad;
    for (size_t i = 0; i < maxlen; ++i) {
        if (s1[i] != s2[i]) return RexxValue(static_cast<int64_t>(i + 1));
    }
    return RexxValue(int64_t(0));
}

// COPIES(string, n)
inline RexxValue fn_copies(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    int n = static_cast<int>(args[1].to_integer());
    std::string result;
    for (int i = 0; i < n; ++i) result += s;
    return RexxValue(result);
}

// COUNTSTR(needle, haystack)
inline RexxValue fn_countstr(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(int64_t(0));
    std::string needle = args[0].to_string();
    std::string haystack = args[1].to_string();
    if (needle.empty()) return RexxValue(int64_t(0));
    int count = 0;
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return RexxValue(static_cast<int64_t>(count));
}

// DELSTR(string, start [,length])
inline RexxValue fn_delstr(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    int start = args.size() > 1 ? static_cast<int>(args[1].to_integer()) : 1;
    if (start < 1) start = 1;
    size_t idx = static_cast<size_t>(start - 1);
    if (idx >= s.size()) return RexxValue(s);
    if (args.size() > 2) {
        int len = static_cast<int>(args[2].to_integer());
        return RexxValue(s.substr(0, idx) + s.substr(std::min(idx + static_cast<size_t>(len), s.size())));
    }
    return RexxValue(s.substr(0, idx));
}

// DELWORD(string, start [,length])
inline RexxValue fn_delword(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    int start_word = static_cast<int>(args[1].to_integer());
    int del_count = args.size() > 2 ? static_cast<int>(args[2].to_integer()) : count_words(s);

    // Split into words with positions
    std::vector<std::pair<size_t, size_t>> words; // start, end
    bool in_word = false;
    size_t ws = 0;
    for (size_t i = 0; i <= s.size(); ++i) {
        bool is_ws = (i == s.size()) || std::isspace(static_cast<unsigned char>(s[i]));
        if (!is_ws && !in_word) { in_word = true; ws = i; }
        if (is_ws && in_word) { in_word = false; words.emplace_back(ws, i); }
    }

    int first = start_word - 1;
    int last = first + del_count;
    if (first < 0) first = 0;
    if (last > static_cast<int>(words.size())) last = static_cast<int>(words.size());

    std::string result;
    if (first > 0) {
        result = s.substr(0, words[static_cast<size_t>(first)].first);
    }
    if (last < static_cast<int>(words.size())) {
        if (!result.empty() && result.back() != ' ') result += ' ';
        result += s.substr(words[static_cast<size_t>(last)].first);
    }
    // Trim trailing space
    while (!result.empty() && result.back() == ' ') result.pop_back();
    return RexxValue(result);
}

// INSERT(new, target [,position [,length [,pad]]])
inline RexxValue fn_insert(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(std::string(""));
    std::string ins = args[0].to_string();
    std::string target = args[1].to_string();
    int pos = args.size() > 2 ? static_cast<int>(args[2].to_integer()) : 0;
    int len = args.size() > 3 ? static_cast<int>(args[3].to_integer()) : static_cast<int>(ins.size());
    char pad = args.size() > 4 ? args[4].to_string()[0] : ' ';
    while (static_cast<int>(ins.size()) < len) ins += pad;
    if (static_cast<int>(ins.size()) > len) ins = ins.substr(0, static_cast<size_t>(len));
    while (static_cast<int>(target.size()) < pos) target += pad;
    return RexxValue(target.substr(0, static_cast<size_t>(pos)) + ins +
                     target.substr(static_cast<size_t>(pos)));
}

// LASTPOS(needle, haystack [,start])
inline RexxValue fn_lastpos(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(int64_t(0));
    std::string needle = args[0].to_string();
    std::string haystack = args[1].to_string();
    size_t start = args.size() > 2 ? static_cast<size_t>(args[2].to_integer()) - 1 : std::string::npos;
    size_t pos = haystack.rfind(needle, start);
    return RexxValue(pos == std::string::npos ? int64_t(0) : static_cast<int64_t>(pos + 1));
}

// LEFT(string, length [,pad])
inline RexxValue fn_left(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    int len = static_cast<int>(args[1].to_integer());
    char pad = args.size() > 2 ? args[2].to_string()[0] : ' ';
    if (len <= 0) return RexxValue(std::string(""));
    while (static_cast<int>(s.size()) < len) s += pad;
    return RexxValue(s.substr(0, static_cast<size_t>(len)));
}

// LENGTH(string)
inline RexxValue fn_length(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(int64_t(0));
    return RexxValue(static_cast<int64_t>(args[0].to_string().size()));
}

// OVERLAY(new, target [,position [,length [,pad]]])
inline RexxValue fn_overlay(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(std::string(""));
    std::string ov = args[0].to_string();
    std::string target = args[1].to_string();
    int pos = args.size() > 2 ? static_cast<int>(args[2].to_integer()) : 1;
    int len = args.size() > 3 ? static_cast<int>(args[3].to_integer()) : static_cast<int>(ov.size());
    char pad = args.size() > 4 ? args[4].to_string()[0] : ' ';
    if (pos < 1) pos = 1;
    while (static_cast<int>(ov.size()) < len) ov += pad;
    if (static_cast<int>(ov.size()) > len) ov = ov.substr(0, static_cast<size_t>(len));
    size_t idx = static_cast<size_t>(pos - 1);
    while (target.size() < idx) target += pad;
    std::string result = target.substr(0, idx) + ov;
    if (idx + static_cast<size_t>(len) < target.size())
        result += target.substr(idx + static_cast<size_t>(len));
    return RexxValue(result);
}

// POS(needle, haystack [,start])
inline RexxValue fn_pos(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(int64_t(0));
    std::string needle = args[0].to_string();
    std::string haystack = args[1].to_string();
    size_t start = args.size() > 2 ? static_cast<size_t>(args[2].to_integer()) - 1 : 0;
    size_t pos = haystack.find(needle, start);
    return RexxValue(pos == std::string::npos ? int64_t(0) : static_cast<int64_t>(pos + 1));
}

// REVERSE(string)
inline RexxValue fn_reverse(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    std::reverse(s.begin(), s.end());
    return RexxValue(s);
}

// RIGHT(string, length [,pad])
inline RexxValue fn_right(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    int len = static_cast<int>(args[1].to_integer());
    char pad = args.size() > 2 ? args[2].to_string()[0] : ' ';
    if (len <= 0) return RexxValue(std::string(""));
    while (static_cast<int>(s.size()) < len) s = pad + s;
    if (static_cast<int>(s.size()) > len)
        s = s.substr(s.size() - static_cast<size_t>(len));
    return RexxValue(s);
}

// SPACE(string [,n [,pad]])
inline RexxValue fn_space(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    int n = args.size() > 1 ? static_cast<int>(args[1].to_integer()) : 1;
    char pad = args.size() > 2 ? args[2].to_string()[0] : ' ';
    int wc = count_words(s);
    std::string result;
    for (int i = 1; i <= wc; ++i) {
        if (i > 1) result += std::string(static_cast<size_t>(n), pad);
        result += get_word(s, i);
    }
    return RexxValue(result);
}

// STRIP(string [,option [,char]])
inline RexxValue fn_strip(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    char opt = 'B';
    if (args.size() > 1) {
        std::string o = to_upper(args[1].to_string());
        if (!o.empty()) opt = o[0];
    }
    char ch = ' ';
    if (args.size() > 2 && !args[2].to_string().empty())
        ch = args[2].to_string()[0];

    if (opt == 'L' || opt == 'B') {
        while (!s.empty() && s.front() == ch) s.erase(s.begin());
    }
    if (opt == 'T' || opt == 'B') {
        while (!s.empty() && s.back() == ch) s.pop_back();
    }
    return RexxValue(s);
}

// SUBSTR(string, position [,length [,pad]])
inline RexxValue fn_substr(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    int pos = static_cast<int>(args[1].to_integer());
    char pad = args.size() > 3 ? args[3].to_string()[0] : ' ';
    if (pos < 1) pos = 1;
    size_t idx = static_cast<size_t>(pos - 1);
    if (args.size() > 2) {
        int len = static_cast<int>(args[2].to_integer());
        while (s.size() < idx + static_cast<size_t>(len)) s += pad;
        return RexxValue(s.substr(idx, static_cast<size_t>(len)));
    }
    if (idx >= s.size()) return RexxValue(std::string(""));
    return RexxValue(s.substr(idx));
}

// SUBWORD(string, position [,length])
inline RexxValue fn_subword(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    int start = static_cast<int>(args[1].to_integer());
    int wc = count_words(s);
    int len = args.size() > 2 ? static_cast<int>(args[2].to_integer()) : (wc - start + 1);
    std::string result;
    for (int i = start; i < start + len && i <= wc; ++i) {
        if (!result.empty()) result += ' ';
        result += get_word(s, i);
    }
    return RexxValue(result);
}

// TRANSLATE(string [,output [,input [,pad]]])
inline RexxValue fn_translate(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    if (args.size() == 1) return RexxValue(to_upper(s));
    std::string output = args.size() > 1 ? args[1].to_string() : "";
    std::string input = args.size() > 2 ? args[2].to_string() : "";
    char pad = args.size() > 3 ? args[3].to_string()[0] : ' ';
    std::string result;
    for (char c : s) {
        size_t idx = input.find(c);
        if (idx != std::string::npos) {
            result += (idx < output.size()) ? output[idx] : pad;
        } else {
            result += c;
        }
    }
    return RexxValue(result);
}

// VERIFY(string, reference [,option [,start]])
inline RexxValue fn_verify(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(int64_t(0));
    std::string s = args[0].to_string();
    std::string ref = args[1].to_string();
    bool match_mode = false; // 'N' = nomatch (default), 'M' = match
    if (args.size() > 2) {
        std::string opt = to_upper(args[2].to_string());
        match_mode = (!opt.empty() && opt[0] == 'M');
    }
    int start = args.size() > 3 ? static_cast<int>(args[3].to_integer()) : 1;
    for (size_t i = static_cast<size_t>(start - 1); i < s.size(); ++i) {
        bool found = (ref.find(s[i]) != std::string::npos);
        if (match_mode && found) return RexxValue(static_cast<int64_t>(i + 1));
        if (!match_mode && !found) return RexxValue(static_cast<int64_t>(i + 1));
    }
    return RexxValue(int64_t(0));
}

// WORD(string, n)
inline RexxValue fn_word(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(std::string(""));
    return RexxValue(get_word(args[0].to_string(), static_cast<int>(args[1].to_integer())));
}

// WORDINDEX(string, n)
inline RexxValue fn_wordindex(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(int64_t(0));
    return RexxValue(static_cast<int64_t>(word_index(args[0].to_string(),
                                                      static_cast<int>(args[1].to_integer()))));
}

// WORDLENGTH(string, n)
inline RexxValue fn_wordlength(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(int64_t(0));
    return RexxValue(static_cast<int64_t>(word_length(args[0].to_string(),
                                                       static_cast<int>(args[1].to_integer()))));
}

// WORDPOS(phrase, string [,start])
inline RexxValue fn_wordpos(const std::vector<RexxValue>& args) {
    if (args.size() < 2) return RexxValue(int64_t(0));
    std::string phrase = args[0].to_string();
    std::string s = args[1].to_string();
    int start = args.size() > 2 ? static_cast<int>(args[2].to_integer()) : 1;
    int phrase_wc = count_words(phrase);
    int s_wc = count_words(s);
    for (int i = start; i <= s_wc - phrase_wc + 1; ++i) {
        bool match = true;
        for (int j = 0; j < phrase_wc; ++j) {
            if (to_upper(get_word(s, i + j)) != to_upper(get_word(phrase, j + 1))) {
                match = false;
                break;
            }
        }
        if (match) return RexxValue(static_cast<int64_t>(i));
    }
    return RexxValue(int64_t(0));
}

// WORDS(string)
inline RexxValue fn_words(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(int64_t(0));
    return RexxValue(static_cast<int64_t>(count_words(args[0].to_string())));
}

// XRANGE([start [,end]])
inline RexxValue fn_xrange(const std::vector<RexxValue>& args) {
    int start = 0, end = 255;
    if (!args.empty()) start = static_cast<int>(args[0].to_integer()) & 0xFF;
    if (args.size() > 1) end = static_cast<int>(args[1].to_integer()) & 0xFF;
    std::string result;
    for (int i = start; ; ) {
        result += static_cast<char>(i);
        if (i == end) break;
        i = (i + 1) & 0xFF;
    }
    return RexxValue(result);
}

// ── CONVERSION FUNCTIONS ───────────────────────────────────────────────────

// B2X(binary_string)
inline RexxValue fn_b2x(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    std::string bin = args[0].to_string();
    // Remove spaces
    std::string clean;
    for (char c : bin) { if (c == '0' || c == '1') clean += c; }
    while (clean.size() % 4 != 0) clean = "0" + clean;
    std::string hex;
    for (size_t i = 0; i < clean.size(); i += 4) {
        int val = 0;
        for (int j = 0; j < 4; ++j) val = (val << 1) | (clean[i + static_cast<size_t>(j)] - '0');
        hex += "0123456789ABCDEF"[val];
    }
    return RexxValue(hex);
}

// C2D(string [,n])
inline RexxValue fn_c2d(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(int64_t(0));
    std::string s = args[0].to_string();
    if (s.empty()) return RexxValue(int64_t(0));
    int64_t result = 0;
    for (char c : s) {
        result = (result << 8) | static_cast<unsigned char>(c);
    }
    return RexxValue(result);
}

// C2X(string)
inline RexxValue fn_c2x(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    std::string s = args[0].to_string();
    std::string hex;
    for (unsigned char c : s) {
        hex += "0123456789ABCDEF"[c >> 4];
        hex += "0123456789ABCDEF"[c & 0xF];
    }
    return RexxValue(hex);
}

// D2C(integer [,n])
inline RexxValue fn_d2c(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    int64_t val = args[0].to_integer();
    if (args.size() > 1) {
        int n = static_cast<int>(args[1].to_integer());
        std::string result(static_cast<size_t>(n), '\0');
        for (int i = n - 1; i >= 0; --i) {
            result[static_cast<size_t>(i)] = static_cast<char>(val & 0xFF);
            val >>= 8;
        }
        return RexxValue(result);
    }
    return RexxValue(std::string(1, static_cast<char>(val & 0xFF)));
}

// D2X(integer [,n])
inline RexxValue fn_d2x(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    int64_t val = args[0].to_integer();
    if (val < 0) val = -val; // simplified
    std::ostringstream os;
    os << std::uppercase << std::hex << val;
    std::string hex = os.str();
    if (args.size() > 1) {
        int n = static_cast<int>(args[1].to_integer());
        while (static_cast<int>(hex.size()) < n) hex = "0" + hex;
        if (static_cast<int>(hex.size()) > n)
            hex = hex.substr(hex.size() - static_cast<size_t>(n));
    }
    return RexxValue(hex);
}

// X2B(hex_string)
inline RexxValue fn_x2b(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    std::string hex = to_upper(args[0].to_string());
    std::string bin;
    for (char c : hex) {
        if (std::isspace(static_cast<unsigned char>(c))) continue;
        int val = 0;
        if (c >= '0' && c <= '9') val = c - '0';
        else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
        for (int b = 3; b >= 0; --b) bin += ((val >> b) & 1) ? '1' : '0';
    }
    return RexxValue(bin);
}

// X2C(hex_string)
inline RexxValue fn_x2c(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    std::string hex = args[0].to_string();
    std::string clean;
    for (char c : hex) {
        if (!std::isspace(static_cast<unsigned char>(c))) clean += c;
    }
    if (clean.size() % 2 != 0) clean = "0" + clean;
    std::string result;
    for (size_t i = 0; i + 1 < clean.size(); i += 2) {
        auto hd = [](char ch) -> int {
            if (ch >= '0' && ch <= '9') return ch - '0';
            ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
            return 0;
        };
        result += static_cast<char>((hd(clean[i]) << 4) | hd(clean[i + 1]));
    }
    return RexxValue(result);
}

// X2D(hex_string [,n])
inline RexxValue fn_x2d(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(int64_t(0));
    std::string hex = args[0].to_string();
    std::string clean;
    for (char c : hex) {
        if (!std::isspace(static_cast<unsigned char>(c))) clean += c;
    }
    int64_t val = 0;
    for (char c : clean) {
        val <<= 4;
        if (c >= '0' && c <= '9') val |= (c - '0');
        else {
            char uc = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            if (uc >= 'A' && uc <= 'F') val |= (uc - 'A' + 10);
        }
    }
    return RexxValue(val);
}

// FORMAT(number [,before [,after]])
inline RexxValue fn_format(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string("0"));
    double val = args[0].to_double();
    int after = args.size() > 2 ? static_cast<int>(args[2].to_integer()) : -1;
    int before = args.size() > 1 ? static_cast<int>(args[1].to_integer()) : -1;
    std::ostringstream os;
    if (after >= 0) os << std::fixed << std::setprecision(after);
    os << val;
    std::string result = os.str();
    if (before > 0) {
        // Find position of decimal point or end
        auto dot = result.find('.');
        size_t int_len = (dot != std::string::npos) ? dot : result.size();
        bool neg = (!result.empty() && result[0] == '-');
        if (neg) --int_len;
        while (static_cast<int>(int_len) < before) {
            if (neg) result = "-" + std::string(" ") + result.substr(1);
            else result = " " + result;
            ++int_len;
        }
    }
    return RexxValue(result);
}

// ── MATH FUNCTIONS ─────────────────────────────────────────────────────────

// ABS(number)
inline RexxValue fn_abs(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(int64_t(0));
    if (args[0].is_integer()) return RexxValue(std::abs(args[0].to_integer()));
    return RexxValue(std::fabs(args[0].to_double()));
}

// MAX(number, number [,...])
inline RexxValue fn_max(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(int64_t(0));
    double m = args[0].to_double();
    for (size_t i = 1; i < args.size(); ++i) {
        double v = args[i].to_double();
        if (v > m) m = v;
    }
    // Return integer if possible
    if (m == std::floor(m) && m >= -1e18 && m <= 1e18)
        return RexxValue(static_cast<int64_t>(m));
    return RexxValue(m);
}

// MIN(number, number [,...])
inline RexxValue fn_min(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(int64_t(0));
    double m = args[0].to_double();
    for (size_t i = 1; i < args.size(); ++i) {
        double v = args[i].to_double();
        if (v < m) m = v;
    }
    if (m == std::floor(m) && m >= -1e18 && m <= 1e18)
        return RexxValue(static_cast<int64_t>(m));
    return RexxValue(m);
}

// SIGN(number)
inline RexxValue fn_sign(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(int64_t(0));
    double v = args[0].to_double();
    if (v < 0) return RexxValue(int64_t(-1));
    if (v > 0) return RexxValue(int64_t(1));
    return RexxValue(int64_t(0));
}

// TRUNC(number [,n])
inline RexxValue fn_trunc(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(int64_t(0));
    double v = args[0].to_double();
    int n = args.size() > 1 ? static_cast<int>(args[1].to_integer()) : 0;
    if (n == 0) return RexxValue(static_cast<int64_t>(v));
    double factor = std::pow(10.0, n);
    double truncated = static_cast<int64_t>(v * factor) / factor;
    std::ostringstream os;
    os << std::fixed << std::setprecision(n) << truncated;
    return RexxValue(os.str());
}

// RANDOM([min] [,max] [,seed])
inline RexxValue fn_random(const std::vector<RexxValue>& args) {
    int mn = 0, mx = 999;
    if (args.size() >= 2) {
        mn = static_cast<int>(args[0].to_integer());
        mx = static_cast<int>(args[1].to_integer());
    } else if (args.size() == 1) {
        mx = static_cast<int>(args[0].to_integer());
    }
    if (args.size() >= 3) {
        std::srand(static_cast<unsigned>(args[2].to_integer()));
    }
    if (mn > mx) std::swap(mn, mx);
    int range = mx - mn + 1;
    return RexxValue(static_cast<int64_t>(mn + (std::rand() % range)));
}

// ── COMPARISON/INFORMATION FUNCTIONS ───────────────────────────────────────

// DATATYPE(string [,type])
inline RexxValue fn_datatype(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string("CHAR"));
    std::string s = args[0].to_string();
    if (args.size() == 1) {
        // Return 'NUM' or 'CHAR'
        RexxValue v(s);
        return RexxValue(std::string(v.is_numeric() ? "NUM" : "CHAR"));
    }
    std::string type = to_upper(args[1].to_string());
    if (type.empty()) return RexxValue(int64_t(0));
    char t = type[0];
    bool result = false;
    switch (t) {
        case 'A': // alphanumeric
            result = std::all_of(s.begin(), s.end(),
                [](unsigned char c) { return std::isalnum(c); });
            break;
        case 'B': // binary
            result = std::all_of(s.begin(), s.end(),
                [](char c) { return c == '0' || c == '1'; });
            break;
        case 'L': // lowercase
            result = std::all_of(s.begin(), s.end(),
                [](unsigned char c) { return std::islower(c); });
            break;
        case 'M': // mixed case alpha
            result = std::all_of(s.begin(), s.end(),
                [](unsigned char c) { return std::isalpha(c); });
            break;
        case 'N': // number
            result = RexxValue(s).is_numeric();
            break;
        case 'S': // symbol (valid REXX symbol characters)
            result = !s.empty() && std::all_of(s.begin(), s.end(),
                [](unsigned char c) { return std::isalnum(c) || c == '_' || c == '.' || c == '!' || c == '?' || c == '@' || c == '#' || c == '$'; });
            break;
        case 'U': // uppercase
            result = std::all_of(s.begin(), s.end(),
                [](unsigned char c) { return std::isupper(c); });
            break;
        case 'W': // whole number
            result = RexxValue(s).is_numeric() && std::floor(RexxValue(s).to_double()) == RexxValue(s).to_double();
            break;
        case 'X': // hex
            result = std::all_of(s.begin(), s.end(),
                [](unsigned char c) { return std::isxdigit(c) || std::isspace(c); });
            break;
        default: break;
    }
    return RexxValue(static_cast<int64_t>(result ? 1 : 0));
}

// SYMBOL(name) — returns 'VAR', 'LIT', or 'BAD'
inline RexxValue fn_symbol(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string("BAD"));
    std::string s = args[0].to_string();
    if (s.empty()) return RexxValue(std::string("BAD"));
    // Simple check
    if (std::isdigit(static_cast<unsigned char>(s[0]))) return RexxValue(std::string("LIT"));
    bool valid = true;
    for (char c : s) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.' && c != '!' && c != '?')
            valid = false;
    }
    return RexxValue(std::string(valid ? "VAR" : "BAD"));
}

// ERRORTEXT(n) — returns error message for REXX error code
inline RexxValue fn_errortext(const std::vector<RexxValue>& args) {
    if (args.empty()) return RexxValue(std::string(""));
    int code = static_cast<int>(args[0].to_integer());
    switch (code) {
        case 0:  return RexxValue(std::string("No error"));
        case 4:  return RexxValue(std::string("Program interrupted"));
        case 5:  return RexxValue(std::string("Machine storage exhausted"));
        case 6:  return RexxValue(std::string("Unmatched /*"));
        case 7:  return RexxValue(std::string("Expected WHEN or OTHERWISE"));
        case 8:  return RexxValue(std::string("Unexpected THEN or ELSE"));
        case 9:  return RexxValue(std::string("Unexpected WHEN or OTHERWISE"));
        case 10: return RexxValue(std::string("Unexpected END"));
        case 14: return RexxValue(std::string("Incomplete DO/IF/SELECT"));
        case 15: return RexxValue(std::string("Invalid hex or binary string"));
        case 16: return RexxValue(std::string("Label not found"));
        case 17: return RexxValue(std::string("Unexpected PROCEDURE"));
        case 18: return RexxValue(std::string("Expected THEN"));
        case 19: return RexxValue(std::string("Expected string or symbol"));
        case 20: return RexxValue(std::string("Name expected"));
        case 26: return RexxValue(std::string("Invalid whole number"));
        case 40: return RexxValue(std::string("Incorrect call to routine"));
        case 41: return RexxValue(std::string("Bad arithmetic conversion"));
        case 42: return RexxValue(std::string("Arithmetic overflow/underflow"));
        case 44: return RexxValue(std::string("Function did not return data"));
        case 48: return RexxValue(std::string("Failure in system service"));
        default: return RexxValue(std::string("Error " + std::to_string(code)));
    }
}

// DIGITS() — returns current NUMERIC DIGITS setting
inline RexxValue fn_digits(const std::vector<RexxValue>&) {
    return RexxValue(int64_t(9));
}

// FUZZ() — returns current NUMERIC FUZZ setting
inline RexxValue fn_fuzz(const std::vector<RexxValue>&) {
    return RexxValue(int64_t(0));
}

// FORM() — returns current NUMERIC FORM setting
inline RexxValue fn_form(const std::vector<RexxValue>&) {
    return RexxValue(std::string("SCIENTIFIC"));
}

// QUEUED() — items in external data queue
inline RexxValue fn_queued(const std::vector<RexxValue>&) {
    return RexxValue(int64_t(0));
}

// DATE([option])
inline RexxValue fn_date(const std::vector<RexxValue>& args) {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char buf[64];
    std::string opt = "N";
    if (!args.empty()) opt = to_upper(args[0].to_string());
    if (opt == "N" || opt.empty()) {
        std::strftime(buf, sizeof(buf), "%d %b %Y", tm);
    } else if (opt == "U") {
        std::strftime(buf, sizeof(buf), "%m/%d/%y", tm);
    } else if (opt == "S") {
        std::strftime(buf, sizeof(buf), "%Y%m%d", tm);
    } else if (opt == "O") {
        std::strftime(buf, sizeof(buf), "%y/%m/%d", tm);
    } else if (opt == "E") {
        std::strftime(buf, sizeof(buf), "%d/%m/%y", tm);
    } else {
        std::strftime(buf, sizeof(buf), "%d %b %Y", tm);
    }
    return RexxValue(std::string(buf));
}

// TIME([option])
inline RexxValue fn_time(const std::vector<RexxValue>& args) {
    std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);
    char buf[32];
    std::string opt = "N";
    if (!args.empty()) opt = to_upper(args[0].to_string());
    if (opt == "N" || opt.empty()) {
        std::strftime(buf, sizeof(buf), "%H:%M:%S", tm);
    } else if (opt == "C") {
        int h = tm->tm_hour % 12;
        if (h == 0) h = 12;
        std::snprintf(buf, sizeof(buf), "%d:%02d%s", h, tm->tm_min,
                     tm->tm_hour < 12 ? "am" : "pm");
    } else if (opt == "L") {
        std::snprintf(buf, sizeof(buf), "%02d:%02d:%02d.000000", tm->tm_hour, tm->tm_min, tm->tm_sec);
    } else if (opt == "H") {
        std::snprintf(buf, sizeof(buf), "%d", tm->tm_hour);
    } else if (opt == "M") {
        std::snprintf(buf, sizeof(buf), "%d", tm->tm_hour * 60 + tm->tm_min);
    } else if (opt == "S") {
        std::snprintf(buf, sizeof(buf), "%d", tm->tm_hour * 3600 + tm->tm_min * 60 + tm->tm_sec);
    } else {
        std::strftime(buf, sizeof(buf), "%H:%M:%S", tm);
    }
    return RexxValue(std::string(buf));
}

// ── Dispatch table ─────────────────────────────────────────────────────────
using BuiltinFunc = RexxValue(*)(const std::vector<RexxValue>&);

inline const std::unordered_map<std::string, BuiltinFunc>& get_builtins() {
    static const std::unordered_map<std::string, BuiltinFunc> table = {
        // String
        {"ABBREV",     fn_abbrev},
        {"CENTER",     fn_center},
        {"CENTRE",     fn_center},
        {"CHANGESTR",  fn_changestr},
        {"COMPARE",    fn_compare},
        {"COPIES",     fn_copies},
        {"COUNTSTR",   fn_countstr},
        {"DELSTR",     fn_delstr},
        {"DELWORD",    fn_delword},
        {"INSERT",     fn_insert},
        {"LASTPOS",    fn_lastpos},
        {"LEFT",       fn_left},
        {"LENGTH",     fn_length},
        {"OVERLAY",    fn_overlay},
        {"POS",        fn_pos},
        {"REVERSE",    fn_reverse},
        {"RIGHT",      fn_right},
        {"SPACE",      fn_space},
        {"STRIP",      fn_strip},
        {"SUBSTR",     fn_substr},
        {"SUBWORD",    fn_subword},
        {"TRANSLATE",  fn_translate},
        {"VERIFY",     fn_verify},
        {"WORD",       fn_word},
        {"WORDINDEX",  fn_wordindex},
        {"WORDLENGTH", fn_wordlength},
        {"WORDPOS",    fn_wordpos},
        {"WORDS",      fn_words},
        {"XRANGE",     fn_xrange},
        // Conversion
        {"B2X",        fn_b2x},
        {"C2D",        fn_c2d},
        {"C2X",        fn_c2x},
        {"D2C",        fn_d2c},
        {"D2X",        fn_d2x},
        {"X2B",        fn_x2b},
        {"X2C",        fn_x2c},
        {"X2D",        fn_x2d},
        {"FORMAT",     fn_format},
        // Math
        {"ABS",        fn_abs},
        {"MAX",        fn_max},
        {"MIN",        fn_min},
        {"SIGN",       fn_sign},
        {"TRUNC",      fn_trunc},
        {"RANDOM",     fn_random},
        // Info/Comparison
        {"DATATYPE",   fn_datatype},
        {"SYMBOL",     fn_symbol},
        {"ERRORTEXT",  fn_errortext},
        {"DIGITS",     fn_digits},
        {"FUZZ",       fn_fuzz},
        {"FORM",       fn_form},
        {"QUEUED",     fn_queued},
        {"DATE",       fn_date},
        {"TIME",       fn_time},
    };
    return table;
}

inline RexxValue call_builtin(const std::string& name, const std::vector<RexxValue>& args) {
    std::string upper = to_upper(name);
    const auto& table = get_builtins();
    auto it = table.find(upper);
    if (it != table.end()) {
        return it->second(args);
    }
    throw std::runtime_error("Unknown builtin function: " + name);
}

inline bool is_builtin(const std::string& name) {
    std::string upper = to_upper(name);
    return get_builtins().count(upper) > 0;
}

}}} // namespace lazarus::rexx::builtins

#endif // LAZARUS_REXX_BUILTINS_H
