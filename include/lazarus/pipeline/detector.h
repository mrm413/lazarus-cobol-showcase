#ifndef LAZARUS_PIPELINE_DETECTOR_H
#define LAZARUS_PIPELINE_DETECTOR_H

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <unordered_map>

namespace lazarus { namespace pipeline {

enum class SourceLang {
    COBOL, HLASM, JCL, PLI, REXX, Easytrieve, SAS, DB2DDL,
    SortControl, RACFUnload, TWSSchedule, CA7Schedule, Copybook, BMSMap, Unknown
};

inline const char* source_lang_name(SourceLang lang) {
    switch (lang) {
        case SourceLang::COBOL:        return "COBOL";
        case SourceLang::HLASM:        return "HLASM";
        case SourceLang::JCL:          return "JCL";
        case SourceLang::PLI:          return "PL/I";
        case SourceLang::REXX:         return "REXX";
        case SourceLang::Easytrieve:   return "Easytrieve";
        case SourceLang::SAS:          return "SAS";
        case SourceLang::DB2DDL:       return "DB2 DDL";
        case SourceLang::SortControl:  return "Sort Control";
        case SourceLang::RACFUnload:   return "RACF Unload";
        case SourceLang::TWSSchedule:  return "TWS Schedule";
        case SourceLang::CA7Schedule:  return "CA7 Schedule";
        case SourceLang::Copybook:     return "Copybook";
        case SourceLang::BMSMap:       return "BMS Map";
        case SourceLang::Unknown:      return "Unknown";
    }
    return "Unknown";
}

struct Detection {
    SourceLang lang = SourceLang::Unknown;
    float confidence = 0.0f;
    std::string reason;
};

namespace detail {

inline std::string to_upper(std::string_view sv) {
    std::string result(sv);
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return result;
}

inline std::string get_extension(std::string_view filename) {
    auto pos = filename.rfind('.');
    if (pos == std::string_view::npos) return "";
    return to_upper(filename.substr(pos));
}

inline std::string get_basename(std::string_view filename) {
    auto slash = filename.rfind('/');
    auto bslash = filename.rfind('\\');
    size_t start = 0;
    if (slash != std::string_view::npos) start = slash + 1;
    if (bslash != std::string_view::npos && bslash + 1 > start) start = bslash + 1;
    return std::string(filename.substr(start));
}

inline size_t count_occurrences(std::string_view text, std::string_view pattern) {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = text.find(pattern, pos)) != std::string_view::npos) {
        ++count;
        pos += pattern.size();
    }
    return count;
}

inline size_t count_occurrences_ci(std::string_view text, std::string_view pattern) {
    std::string upper_text = to_upper(text);
    std::string upper_pattern = to_upper(pattern);
    return count_occurrences(upper_text, upper_pattern);
}

struct LangScore {
    SourceLang lang = SourceLang::Unknown;
    float score = 0.0f;
    std::string reason;
};

inline LangScore detect_by_extension(std::string_view filename) {
    std::string ext = get_extension(filename);
    std::string base = to_upper(get_basename(filename));

    if (ext == ".CBL" || ext == ".COB" || ext == ".COBOL")
        return {SourceLang::COBOL, 0.85f, "file extension " + std::string(ext)};
    if (ext == ".JCL")
        return {SourceLang::JCL, 0.85f, "file extension .JCL"};
    if (ext == ".PLI" || ext == ".PL1")
        return {SourceLang::PLI, 0.85f, "file extension " + ext};
    if (ext == ".REXX" || ext == ".RX" || ext == ".EXEC")
        return {SourceLang::REXX, 0.85f, "file extension " + ext};
    if (ext == ".ASM" || ext == ".HLASM")
        return {SourceLang::HLASM, 0.85f, "file extension " + ext};
    if (ext == ".EZT" || ext == ".EASYTRIEVE")
        return {SourceLang::Easytrieve, 0.85f, "file extension " + ext};
    if (ext == ".SAS")
        return {SourceLang::SAS, 0.85f, "file extension .SAS"};
    if (ext == ".DDL" || ext == ".SQL")
        return {SourceLang::DB2DDL, 0.80f, "file extension " + ext};
    if (ext == ".SORT" || ext == ".SRT")
        return {SourceLang::SortControl, 0.85f, "file extension " + ext};
    if (ext == ".CPY" || ext == ".COPY")
        return {SourceLang::Copybook, 0.85f, "file extension " + ext};
    if (ext == ".BMS")
        return {SourceLang::BMSMap, 0.85f, "file extension .BMS"};

    // Check basename patterns
    if (base.find("RACF") != std::string::npos)
        return {SourceLang::RACFUnload, 0.60f, "filename contains RACF"};
    if (base.find("TWS") != std::string::npos || base.find("OPC") != std::string::npos)
        return {SourceLang::TWSSchedule, 0.55f, "filename contains TWS/OPC"};
    if (base.find("CA7") != std::string::npos)
        return {SourceLang::CA7Schedule, 0.55f, "filename contains CA7"};

    return {SourceLang::Unknown, 0.0f, ""};
}

struct KeywordDef {
    std::string_view keyword;
    float weight;
};

inline float keyword_density(std::string_view content, const std::vector<KeywordDef>& keywords) {
    float total = 0.0f;
    for (const auto& kw : keywords) {
        size_t hits = count_occurrences_ci(content, kw.keyword);
        if (hits > 0) {
            total += kw.weight * std::min(static_cast<float>(hits), 5.0f);
        }
    }
    return total;
}

inline bool has_cobol_column_structure(std::string_view content) {
    // COBOL: columns 1-6 sequence, 7 indicator, 8-11 area A, 12-72 area B
    size_t line_start = 0;
    int structured_lines = 0;
    int total_lines = 0;
    while (line_start < content.size()) {
        auto line_end = content.find('\n', line_start);
        if (line_end == std::string_view::npos) line_end = content.size();
        auto line = content.substr(line_start, line_end - line_start);
        if (line.size() >= 7) {
            ++total_lines;
            char col7 = line[6];
            if (col7 == ' ' || col7 == '*' || col7 == '/' || col7 == '-' || col7 == 'D' || col7 == 'd') {
                bool seq_ok = true;
                for (size_t i = 0; i < 6 && i < line.size(); ++i) {
                    if (!std::isdigit(static_cast<unsigned char>(line[i])) && line[i] != ' ') {
                        seq_ok = false;
                        break;
                    }
                }
                if (seq_ok) ++structured_lines;
            }
        }
        line_start = line_end + 1;
    }
    return total_lines > 2 && structured_lines > total_lines / 2;
}

inline bool has_jcl_structure(std::string_view content) {
    size_t jcl_lines = 0;
    size_t total_lines = 0;
    size_t pos = 0;
    while (pos < content.size()) {
        auto eol = content.find('\n', pos);
        if (eol == std::string_view::npos) eol = content.size();
        auto line = content.substr(pos, eol - pos);
        if (!line.empty()) {
            ++total_lines;
            if (line.size() >= 2 && line[0] == '/' && line[1] == '/') {
                ++jcl_lines;
            } else if (line.size() >= 2 && line[0] == '/' && line[1] == '*') {
                ++jcl_lines;
            }
        }
        pos = eol + 1;
    }
    return total_lines > 1 && jcl_lines > total_lines / 3;
}

inline LangScore score_cobol(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"IDENTIFICATION DIVISION", 3.0f}, {"PROCEDURE DIVISION", 3.0f},
        {"DATA DIVISION", 2.5f}, {"ENVIRONMENT DIVISION", 2.5f},
        {"WORKING-STORAGE", 2.0f}, {"PIC ", 1.5f}, {"PERFORM ", 1.5f},
        {"MOVE ", 1.0f}, {"DISPLAY ", 1.0f}, {"ACCEPT ", 1.0f},
        {"STOP RUN", 2.0f}, {"EXEC CICS", 2.0f}, {"EXEC SQL", 1.5f},
        {"EVALUATE ", 1.0f}, {"COMPUTE ", 1.0f}, {"01 ", 0.5f}
    };
    float density = keyword_density(content, kws);
    bool col_ok = has_cobol_column_structure(content);
    float score = density + (col_ok ? 5.0f : 0.0f);
    return {SourceLang::COBOL, score, "COBOL keyword density=" + std::to_string(density)};
}

inline LangScore score_jcl(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {" JOB ", 3.0f}, {" EXEC ", 2.5f}, {" DD ", 2.5f},
        {"PROC", 2.0f}, {"PEND", 2.0f}, {"DSN=", 1.5f},
        {"DISP=", 1.5f}, {"SPACE=", 1.0f}, {"SYSOUT=", 1.0f},
        {"REGION=", 1.0f}, {"CLASS=", 1.0f}, {"MSGCLASS=", 1.0f},
        {"COND=", 1.0f}, {"NOTIFY=", 0.8f}
    };
    float density = keyword_density(content, kws);
    bool struct_ok = has_jcl_structure(content);
    float score = density + (struct_ok ? 6.0f : 0.0f);
    return {SourceLang::JCL, score, "JCL keyword density=" + std::to_string(density)};
}

inline LangScore score_pli(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"PROCEDURE OPTIONS(MAIN)", 4.0f}, {"PROC OPTIONS", 3.5f},
        {"DCL ", 2.0f}, {"DECLARE ", 2.0f}, {"%INCLUDE", 2.5f},
        {"PUT LIST", 1.5f}, {"GET LIST", 1.5f}, {"FIXED BIN", 1.5f},
        {"CHAR(", 1.0f}, {"BIT(", 1.0f}, {"ALLOCATE ", 1.0f},
        {"BEGIN;", 1.5f}, {"END;", 0.5f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::PLI, density, "PL/I keyword density=" + std::to_string(density)};
}

inline LangScore score_rexx(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"/* REXX */", 5.0f}, {"/* REXX", 4.0f}, {"SAY ", 2.0f},
        {"PARSE ", 2.5f}, {"PARSE ARG", 3.0f}, {"PARSE VAR", 2.5f},
        {"PULL ", 1.5f}, {"DO ", 1.0f}, {"SIGNAL ", 1.5f},
        {"INTERPRET ", 1.5f}, {"ADDRESS ", 1.5f}, {"QUEUE ", 1.0f},
        {"PUSH ", 1.0f}, {"ARG ", 1.0f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::REXX, density, "REXX keyword density=" + std::to_string(density)};
}

inline LangScore score_easytrieve(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"FILE ", 2.0f}, {"RECORD ", 1.5f}, {"FIELD ", 1.5f},
        {"JOB INPUT", 3.0f}, {"SORT ", 2.0f}, {"REPORT ", 2.0f},
        {"DEFINE ", 1.5f}, {"LIST ", 1.0f}, {"PRINT ", 1.0f},
        {"GET ", 1.0f}, {"PUT ", 1.0f}, {"HEADING ", 1.0f},
        {"LINE ", 0.8f}, {"TITLE ", 0.8f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::Easytrieve, density, "Easytrieve keyword density=" + std::to_string(density)};
}

inline LangScore score_sas(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"DATA ", 2.0f}, {"PROC ", 2.5f}, {"RUN;", 2.5f},
        {"%MACRO", 3.0f}, {"%MEND", 2.5f}, {"%LET", 2.0f},
        {"LIBNAME ", 2.0f}, {"FILENAME ", 1.5f}, {"SET ", 1.0f},
        {"MERGE ", 1.0f}, {"OUTPUT;", 1.5f}, {"CARDS;", 1.5f},
        {"PROC SORT", 2.5f}, {"PROC PRINT", 2.0f}, {"PROC MEANS", 2.0f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::SAS, density, "SAS keyword density=" + std::to_string(density)};
}

inline LangScore score_db2ddl(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"CREATE TABLE", 4.0f}, {"CREATE INDEX", 3.5f}, {"ALTER TABLE", 3.0f},
        {"DROP TABLE", 2.5f}, {"CREATE VIEW", 3.0f}, {"CREATE TABLESPACE", 3.5f},
        {"CREATE DATABASE", 3.0f}, {"GRANT ", 1.5f}, {"REVOKE ", 1.5f},
        {"NOT NULL", 1.0f}, {"PRIMARY KEY", 1.5f}, {"FOREIGN KEY", 1.5f},
        {"VARCHAR(", 1.0f}, {"INTEGER", 0.8f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::DB2DDL, density, "DB2 DDL keyword density=" + std::to_string(density)};
}

inline LangScore score_sort(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"SORT FIELDS=", 5.0f}, {"MERGE FIELDS=", 5.0f},
        {"INCLUDE COND=", 3.0f}, {"OMIT COND=", 3.0f},
        {"INREC ", 2.5f}, {"OUTREC ", 2.5f}, {"SUM FIELDS=", 2.5f},
        {"OPTION ", 1.5f}, {"RECORD TYPE=", 1.5f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::SortControl, density, "Sort keyword density=" + std::to_string(density)};
}

inline LangScore score_hlasm(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"CSECT", 4.0f}, {"DSECT", 3.5f}, {"USING ", 3.0f},
        {"BALR ", 2.5f}, {"STM ", 2.0f}, {"LM ", 2.0f},
        {"LA ", 1.5f}, {"LR ", 1.5f}, {"SR ", 1.5f},
        {"BR ", 1.5f}, {"DC ", 1.0f}, {"DS ", 1.0f},
        {"EQU ", 1.0f}, {"LTORG", 1.5f}, {"END ", 1.0f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::HLASM, density, "HLASM keyword density=" + std::to_string(density)};
}

inline LangScore score_racf(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"0100", 3.0f}, {"0200", 3.0f}, {"0400", 3.0f},
        {"0500", 2.5f}, {"USBD", 2.5f}, {"GRBD", 2.5f},
        {"DSBD", 2.5f}, {"ADDUSER ", 2.0f}, {"ALTUSER ", 2.0f},
        {"PERMIT ", 1.5f}, {"CONNECT ", 1.0f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::RACFUnload, density, "RACF keyword density=" + std::to_string(density)};
}

inline LangScore score_bms(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"DFHMSD", 5.0f}, {"DFHMDI", 4.5f}, {"DFHMDF", 4.0f},
        {"TYPE=MAP", 3.0f}, {"TYPE=DSECT", 3.0f}, {"TYPE=FINAL", 2.5f},
        {"SIZE=", 1.5f}, {"POS=", 1.5f}, {"LENGTH=", 1.0f},
        {"ATTRB=", 1.5f}, {"INITIAL=", 1.0f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::BMSMap, density, "BMS keyword density=" + std::to_string(density)};
}

inline LangScore score_tws(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"APPL ", 3.0f}, {"OPER ", 2.5f}, {"JOBNAME ", 3.0f},
        {"SCHEDULE ", 2.0f}, {"DEPENDENCY ", 2.0f}, {"PERIOD ", 1.5f},
        {"CALENDAR ", 1.5f}, {"RUN ", 1.0f}, {"WORKSTATION ", 1.5f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::TWSSchedule, density, "TWS keyword density=" + std::to_string(density)};
}

inline LangScore score_ca7(std::string_view content) {
    std::vector<KeywordDef> kws = {
        {"SCHID ", 3.0f}, {"LEADTM ", 2.5f}, {"DEADTM ", 2.5f},
        {"JDEP ", 3.0f}, {"DSDEP ", 2.5f}, {"NXTCYC ", 2.0f},
        {"NXTRDT ", 2.0f}, {"SCHDCYC ", 2.0f}, {"REQJOB ", 2.0f}
    };
    float density = keyword_density(content, kws);
    return {SourceLang::CA7Schedule, density, "CA7 keyword density=" + std::to_string(density)};
}

inline LangScore score_copybook(std::string_view content) {
    // Copybook: has PIC/data definitions but NO divisions and NO procedure statements
    std::string upper = to_upper(content);
    bool has_pic = count_occurrences(upper, "PIC ") > 0 || count_occurrences(upper, "PIC(") > 0;
    bool has_division = count_occurrences(upper, "DIVISION") > 0;
    bool has_level = count_occurrences(upper, "01 ") > 0 || count_occurrences(upper, "05 ") > 0;
    bool has_proc_stmt = count_occurrences(upper, "PERFORM ") > 0 ||
                         count_occurrences(upper, "MOVE ") > 0 ||
                         count_occurrences(upper, "DISPLAY ") > 0 ||
                         count_occurrences(upper, "CALL ") > 0 ||
                         count_occurrences(upper, "STOP RUN") > 0 ||
                         count_occurrences(upper, "COMPUTE ") > 0;
    float score = 0.0f;
    if (has_pic && !has_division && !has_proc_stmt) score += 8.0f;
    if (has_level && !has_division && !has_proc_stmt) score += 4.0f;
    if (has_pic && has_division) score += 2.0f; // might be a full program
    return {SourceLang::Copybook, score, "Copybook pattern analysis"};
}

} // namespace detail

inline Detection detect(std::string_view filename, std::string_view content) {
    // Phase 1: extension-based detection
    auto ext_result = detail::detect_by_extension(filename);

    // Phase 2: content-based scoring
    struct ScoredLang {
        SourceLang lang;
        float score;
        std::string reason;
    };

    std::vector<ScoredLang> scores;
    auto add = [&](detail::LangScore ls) {
        if (ls.score > 0.5f) {
            scores.push_back({ls.lang, ls.score, ls.reason});
        }
    };

    if (!content.empty()) {
        add(detail::score_cobol(content));
        add(detail::score_jcl(content));
        add(detail::score_pli(content));
        add(detail::score_rexx(content));
        add(detail::score_easytrieve(content));
        add(detail::score_sas(content));
        add(detail::score_db2ddl(content));
        add(detail::score_sort(content));
        add(detail::score_hlasm(content));
        add(detail::score_racf(content));
        add(detail::score_bms(content));
        add(detail::score_tws(content));
        add(detail::score_ca7(content));
        add(detail::score_copybook(content));
    }

    // Sort by score descending
    std::sort(scores.begin(), scores.end(),
              [](const ScoredLang& a, const ScoredLang& b) { return a.score > b.score; });

    // Combine extension + content
    if (!scores.empty()) {
        auto& best = scores[0];
        float content_conf = std::min(best.score / 15.0f, 1.0f);

        if (ext_result.lang != SourceLang::Unknown) {
            if (ext_result.lang == best.lang) {
                // Extension + content agree
                float combined = std::min(ext_result.score * 0.4f + content_conf * 0.6f + 0.1f, 1.0f);
                return {best.lang, combined, ext_result.reason + " + " + best.reason};
            } else {
                // Disagreement: content wins if strong enough
                if (content_conf > 0.7f) {
                    return {best.lang, content_conf * 0.8f, best.reason + " (overrides extension)"};
                }
                return {ext_result.lang, ext_result.score * 0.7f,
                        ext_result.reason + " (content suggests " + std::string(source_lang_name(best.lang)) + ")"};
            }
        }
        return {best.lang, content_conf, best.reason};
    }

    if (ext_result.lang != SourceLang::Unknown) {
        return {ext_result.lang, ext_result.score, ext_result.reason};
    }

    return {SourceLang::Unknown, 0.0f, "no recognizable patterns"};
}

struct FileEntry {
    std::string filename;
    std::string content;
};

inline std::vector<Detection> detect_batch(const std::vector<FileEntry>& files) {
    std::vector<Detection> results;
    results.reserve(files.size());
    for (const auto& f : files) {
        results.push_back(detect(f.filename, f.content));
    }
    return results;
}

}} // namespace lazarus::pipeline

#endif // LAZARUS_PIPELINE_DETECTOR_H
