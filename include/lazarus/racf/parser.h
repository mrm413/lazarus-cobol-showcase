// Lazarus Systems -- RACF Dump Parser (IRRDBU00 format)
// Tier 8: RACF Profile Importer -- zero-copy field extraction from IRRDBU00 unload

#ifndef LAZARUS_RACF_PARSER_H
#define LAZARUS_RACF_PARSER_H

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <optional>
#include <map>

#include "lazarus/racf/profile.h"

namespace lazarus { namespace racf {

// IRRDBU00 record type codes (first 4 bytes of each unload record)
struct RecordType {
    static constexpr std::string_view USER_BASIC    = "0100";
    static constexpr std::string_view USER_OMVS     = "0101";
    static constexpr std::string_view USER_CICS     = "0102";
    static constexpr std::string_view USER_DFP      = "0103";
    static constexpr std::string_view GROUP_BASIC   = "0200";
    static constexpr std::string_view GROUP_OMVS    = "0205";
    static constexpr std::string_view DATASET       = "0400";
    static constexpr std::string_view GENERAL_RES   = "0500";
    static constexpr std::string_view CONNECT       = "0600";
    static constexpr std::string_view ACCESS_LIST   = "0404"; // dataset access
    static constexpr std::string_view GEN_ACC_LIST  = "0505"; // general resource access
};

// Zero-copy field extractor: works on fixed-position IRRDBU00 fields
// Fields are space-delimited within records, with known column offsets
class FieldExtractor {
public:
    explicit FieldExtractor(std::string_view line) : line_(line) {}

    // Extract field by start position and length (0-based)
    std::string_view field(size_t start, size_t len) const {
        if (start >= line_.size()) return {};
        size_t actual = std::min(len, line_.size() - start);
        return trim(line_.substr(start, actual));
    }

    // Extract field and return as string (trimmed)
    std::string field_str(size_t start, size_t len) const {
        return std::string(field(start, len));
    }

    // Extract field as integer
    int field_int(size_t start, size_t len) const {
        auto f = field(start, len);
        if (f.empty()) return 0;
        int val = 0;
        bool neg = false;
        size_t i = 0;
        if (i < f.size() && f[i] == '-') { neg = true; ++i; }
        for (; i < f.size(); ++i) {
            char c = f[i];
            if (c >= '0' && c <= '9') val = val * 10 + (c - '0');
        }
        return neg ? -val : val;
    }

    // Check if a flag field is YES/Y/TRUE/1
    bool field_bool(size_t start, size_t len) const {
        auto f = field(start, len);
        if (f.empty()) return false;
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(f[0])));
        return c == 'Y' || c == '1' || c == 'T';
    }

    // Get the record type (first 4 characters)
    std::string_view record_type() const {
        return field(0, 4);
    }

    // Full line
    std::string_view line() const { return line_; }

    // Line length
    size_t size() const { return line_.size(); }

private:
    std::string_view line_;

    static std::string_view trim(std::string_view sv) {
        while (!sv.empty() && sv.front() == ' ') sv.remove_prefix(1);
        while (!sv.empty() && sv.back() == ' ')  sv.remove_suffix(1);
        return sv;
    }
};

// Delimiter-based field extractor for space/comma separated IRRDBU00 records
// Many IRRDBU00 tools produce space-separated fields after the record type
class DelimitedExtractor {
public:
    explicit DelimitedExtractor(std::string_view line, char delim = ' ')
        : line_(line)
    {
        parse_fields(delim);
    }

    size_t count() const { return fields_.size(); }

    std::string_view get(size_t idx) const {
        if (idx >= fields_.size()) return {};
        return fields_[idx];
    }

    std::string get_str(size_t idx) const {
        return std::string(get(idx));
    }

    int get_int(size_t idx) const {
        auto f = get(idx);
        if (f.empty()) return 0;
        int val = 0;
        bool neg = false;
        size_t i = 0;
        if (i < f.size() && f[i] == '-') { neg = true; ++i; }
        for (; i < f.size(); ++i) {
            if (f[i] >= '0' && f[i] <= '9') val = val * 10 + (f[i] - '0');
        }
        return neg ? -val : val;
    }

    bool get_bool(size_t idx) const {
        auto f = get(idx);
        if (f.empty()) return false;
        char c = static_cast<char>(std::toupper(static_cast<unsigned char>(f[0])));
        return c == 'Y' || c == '1' || c == 'T';
    }

    std::string_view record_type() const {
        return get(0);
    }

private:
    std::string_view line_;
    std::vector<std::string_view> fields_;

    void parse_fields(char delim) {
        size_t pos = 0;
        while (pos < line_.size()) {
            // skip delimiters
            while (pos < line_.size() && line_[pos] == delim) ++pos;
            if (pos >= line_.size()) break;
            size_t start = pos;
            while (pos < line_.size() && line_[pos] != delim) ++pos;
            auto fld = line_.substr(start, pos - start);
            // trim trailing whitespace
            while (!fld.empty() && fld.back() == ' ') fld.remove_suffix(1);
            fields_.push_back(fld);
        }
    }
};

// Parse result
struct ParseResult {
    std::vector<RacfProfile> profiles;
    std::vector<std::string> warnings;
    size_t lines_processed = 0;
    size_t lines_skipped   = 0;

    ProfileDatabase to_database() const {
        ProfileDatabase db;
        for (auto& p : profiles) {
            db.add(p);
        }
        return db;
    }
};

// Main IRRDBU00 parser
// Supports both fixed-position and delimited field layouts.
// The IRRDBU00 unload utility produces records where:
//   - First 4 chars: record type code
//   - Remaining fields: space-delimited values in defined order per record type
class IrrdbuParser {
public:
    // Parse full IRRDBU00 dump text (multiple lines)
    ParseResult parse(std::string_view text) const {
        ParseResult result;
        auto lines = split_lines(text);
        // Intermediate maps for merging segments into base profiles
        std::map<std::string, size_t> user_index;    // userid -> index in result
        std::map<std::string, size_t> group_index;   // group -> index in result
        // Track dataset access lists pending attachment
        std::map<std::string, std::vector<AccessEntry>> ds_access_pending;
        std::map<std::string, std::vector<AccessEntry>> gen_access_pending;

        for (auto& line : lines) {
            if (line.empty() || line.size() < 4) {
                result.lines_skipped++;
                continue;
            }
            result.lines_processed++;
            DelimitedExtractor ex(line);
            auto rt = ex.record_type();

            if (rt == RecordType::USER_BASIC) {
                auto prof = parse_user_basic(ex);
                user_index[prof.userid] = result.profiles.size();
                result.profiles.push_back(prof);
            } else if (rt == RecordType::USER_OMVS) {
                merge_user_omvs(ex, result.profiles, user_index);
            } else if (rt == RecordType::USER_CICS) {
                merge_user_cics(ex, result.profiles, user_index);
            } else if (rt == RecordType::USER_DFP) {
                merge_user_dfp(ex, result.profiles, user_index);
            } else if (rt == RecordType::GROUP_BASIC) {
                auto prof = parse_group_basic(ex);
                group_index[prof.group] = result.profiles.size();
                result.profiles.push_back(prof);
            } else if (rt == RecordType::GROUP_OMVS) {
                merge_group_omvs(ex, result.profiles, group_index);
            } else if (rt == RecordType::DATASET) {
                auto prof = parse_dataset(ex);
                // attach any pending access list entries
                auto it = ds_access_pending.find(prof.dsname);
                if (it != ds_access_pending.end()) {
                    prof.access_list = std::move(it->second);
                    ds_access_pending.erase(it);
                }
                result.profiles.push_back(prof);
            } else if (rt == RecordType::GENERAL_RES) {
                auto prof = parse_general(ex);
                auto key = prof.resource_class + "/" + prof.name;
                auto it = gen_access_pending.find(key);
                if (it != gen_access_pending.end()) {
                    prof.access_list = std::move(it->second);
                    gen_access_pending.erase(it);
                }
                result.profiles.push_back(prof);
            } else if (rt == RecordType::CONNECT) {
                result.profiles.push_back(parse_connect(ex));
            } else if (rt == RecordType::ACCESS_LIST) {
                // Dataset access list entry:
                // 0404 DSNAME ENTITY ACCESS [AUDIT] [COUNT]
                auto ae = parse_access_entry(ex, 2);
                if (ex.count() > 1) {
                    auto dsn = ex.get_str(1);
                    // Try to attach to existing dataset profile
                    bool found = false;
                    for (auto& p : result.profiles) {
                        if (auto* dp = std::get_if<DatasetProfile>(&p)) {
                            if (dp->dsname == dsn) {
                                dp->access_list.push_back(ae);
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found) {
                        ds_access_pending[dsn].push_back(ae);
                    }
                }
            } else if (rt == RecordType::GEN_ACC_LIST) {
                // General resource access list entry:
                // 0505 CLASS NAME ENTITY ACCESS [AUDIT] [COUNT]
                if (ex.count() > 3) {
                    auto key = ex.get_str(1) + "/" + ex.get_str(2);
                    auto ae = parse_access_entry(ex, 3);
                    bool found = false;
                    for (auto& p : result.profiles) {
                        if (auto* gp = std::get_if<GeneralProfile>(&p)) {
                            if ((gp->resource_class + "/" + gp->name) == key) {
                                gp->access_list.push_back(ae);
                                found = true;
                                break;
                            }
                        }
                    }
                    if (!found) {
                        gen_access_pending[key].push_back(ae);
                    }
                }
            } else {
                result.lines_skipped++;
                result.lines_processed--;
            }
        }
        return result;
    }

    // Parse a single IRRDBU00 record line
    std::optional<RacfProfile> parse_record(std::string_view line) const {
        if (line.empty() || line.size() < 4) return std::nullopt;
        DelimitedExtractor ex(line);
        auto rt = ex.record_type();

        if (rt == RecordType::USER_BASIC)  return parse_user_basic(ex);
        if (rt == RecordType::GROUP_BASIC) return parse_group_basic(ex);
        if (rt == RecordType::DATASET)     return parse_dataset(ex);
        if (rt == RecordType::GENERAL_RES) return parse_general(ex);
        if (rt == RecordType::CONNECT)     return parse_connect(ex);
        return std::nullopt;
    }

private:
    // ---- User basic (0100) ----
    // Fields: 0100 USERID NAME DFLTGRP OWNER CREATED PASSDATE PASSINT
    //         REVOKE SPECIAL OPERATIONS AUDITOR UAUDIT [CLAUTH...]
    static UserProfile parse_user_basic(const DelimitedExtractor& ex) {
        UserProfile u;
        u.userid     = ex.get_str(1);
        u.name       = ex.get_str(2);
        u.dfltgrp    = ex.get_str(3);
        u.owner      = ex.get_str(4);
        u.created    = ex.get_str(5);
        u.passdate   = ex.get_str(6);
        u.passint    = ex.get_int(7);
        u.revoke     = ex.get_bool(8);
        u.special    = ex.get_bool(9);
        u.operations = ex.get_bool(10);
        u.auditor    = ex.get_bool(11);
        u.uaudit     = ex.get_bool(12);
        // Remaining fields are CLAUTH classes
        for (size_t i = 13; i < ex.count(); ++i) {
            auto c = ex.get_str(i);
            if (!c.empty()) u.clauth.push_back(c);
        }
        return u;
    }

    // ---- User OMVS segment (0101) ----
    // Fields: 0101 USERID UID HOME SHELL
    static void merge_user_omvs(const DelimitedExtractor& ex,
                                 std::vector<RacfProfile>& profiles,
                                 const std::map<std::string, size_t>& idx) {
        auto uid = ex.get_str(1);
        auto it = idx.find(uid);
        if (it == idx.end()) return;
        if (auto* up = std::get_if<UserProfile>(&profiles[it->second])) {
            up->omvs_uid   = ex.get_int(2);
            up->omvs_home  = ex.get_str(3);
            up->omvs_shell = ex.get_str(4);
        }
    }

    // ---- User CICS segment (0102) ----
    // Fields: 0102 USERID OPIDENT OPPRTY TIMEOUT
    static void merge_user_cics(const DelimitedExtractor& ex,
                                 std::vector<RacfProfile>& profiles,
                                 const std::map<std::string, size_t>& idx) {
        auto uid = ex.get_str(1);
        auto it = idx.find(uid);
        if (it == idx.end()) return;
        if (auto* up = std::get_if<UserProfile>(&profiles[it->second])) {
            up->cics_opident = ex.get_str(2);
            up->cics_opprty  = ex.get_int(3);
            up->cics_timeout = ex.get_int(4);
        }
    }

    // ---- User DFP segment (0103) ----
    // Fields: 0103 USERID DATAAPPL MGMTCLASS STORCLAS
    static void merge_user_dfp(const DelimitedExtractor& ex,
                                std::vector<RacfProfile>& profiles,
                                const std::map<std::string, size_t>& idx) {
        auto uid = ex.get_str(1);
        auto it = idx.find(uid);
        if (it == idx.end()) return;
        if (auto* up = std::get_if<UserProfile>(&profiles[it->second])) {
            up->dfp_dataappl  = ex.get_str(2);
            up->dfp_mgmtclass = ex.get_str(3);
            up->dfp_storclas  = ex.get_str(4);
        }
    }

    // ---- Group basic (0200) ----
    // Fields: 0200 GROUP SUPGROUP OWNER TERMUACC [SUBGROUP...]
    static GroupProfile parse_group_basic(const DelimitedExtractor& ex) {
        GroupProfile g;
        g.group    = ex.get_str(1);
        g.supgroup = ex.get_str(2);
        g.owner    = ex.get_str(3);
        g.termuacc = ex.get_bool(4);
        for (size_t i = 5; i < ex.count(); ++i) {
            auto s = ex.get_str(i);
            if (!s.empty()) g.subgroups.push_back(s);
        }
        return g;
    }

    // ---- Group OMVS segment (0205) ----
    // Fields: 0205 GROUP GID
    static void merge_group_omvs(const DelimitedExtractor& ex,
                                  std::vector<RacfProfile>& profiles,
                                  const std::map<std::string, size_t>& idx) {
        auto grp = ex.get_str(1);
        auto it = idx.find(grp);
        if (it == idx.end()) return;
        if (auto* gp = std::get_if<GroupProfile>(&profiles[it->second])) {
            gp->omvs_gid = ex.get_int(2);
        }
    }

    // ---- Dataset profile (0400) ----
    // Fields: 0400 DSNAME UACC AUDIT GLOBALAUDIT OWNER ERASE WARNING GENERIC
    static DatasetProfile parse_dataset(const DelimitedExtractor& ex) {
        DatasetProfile d;
        d.dsname       = ex.get_str(1);
        d.uacc         = parse_access_level(ex.get(2));
        d.audit_access = ex.get_bool(3);
        d.globalaudit  = ex.get_bool(4);
        d.owner        = ex.get_str(5);
        d.erase        = ex.get_bool(6);
        d.warning      = ex.get_bool(7);
        d.generic      = ex.get_bool(8);
        return d;
    }

    // ---- General resource profile (0500) ----
    // Fields: 0500 CLASS NAME UACC AUDIT OWNER
    static GeneralProfile parse_general(const DelimitedExtractor& ex) {
        GeneralProfile g;
        g.resource_class = ex.get_str(1);
        g.name           = ex.get_str(2);
        g.uacc           = parse_access_level(ex.get(3));
        g.audit_access   = ex.get_bool(4);
        g.owner          = ex.get_str(5);
        return g;
    }

    // ---- Connect profile (0600) ----
    // Fields: 0600 USERID GROUP AUTHORITY REVOKE
    static ConnectProfile parse_connect(const DelimitedExtractor& ex) {
        ConnectProfile c;
        c.userid = ex.get_str(1);
        c.group  = ex.get_str(2);
        c.auth   = parse_authority(ex.get(3));
        c.revoke = ex.get_bool(4);
        return c;
    }

    // ---- Access list entry ----
    // Fields at offset: ENTITY ACCESS [AUDIT] [COUNT]
    static AccessEntry parse_access_entry(const DelimitedExtractor& ex, size_t offset) {
        AccessEntry ae;
        ae.entity = ex.get_str(offset);
        ae.access = parse_access_level(ex.get(offset + 1));
        ae.audit  = ex.get_bool(offset + 2);
        ae.count  = ex.get_int(offset + 3);
        return ae;
    }

    // Split text into lines
    static std::vector<std::string_view> split_lines(std::string_view text) {
        std::vector<std::string_view> lines;
        size_t pos = 0;
        while (pos < text.size()) {
            size_t nl = text.find('\n', pos);
            if (nl == std::string_view::npos) {
                auto line = text.substr(pos);
                if (!line.empty() && line.back() == '\r')
                    line.remove_suffix(1);
                if (!line.empty()) lines.push_back(line);
                break;
            }
            auto line = text.substr(pos, nl - pos);
            if (!line.empty() && line.back() == '\r')
                line.remove_suffix(1);
            if (!line.empty()) lines.push_back(line);
            pos = nl + 1;
        }
        return lines;
    }
};

}} // namespace lazarus::racf

#endif // LAZARUS_RACF_PARSER_H
