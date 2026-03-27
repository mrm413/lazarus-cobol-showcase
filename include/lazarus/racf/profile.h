// Lazarus Systems -- RACF Profile Model
// Tier 8: RACF Profile Importer -- profile types, querying, hierarchy, effective permissions

#ifndef LAZARUS_RACF_PROFILE_H
#define LAZARUS_RACF_PROFILE_H

#include <string>
#include <string_view>
#include <vector>
#include <variant>
#include <optional>
#include <algorithm>
#include <map>
#include <set>
#include <regex>
#include <sstream>
#include <ctime>
#include <functional>

namespace lazarus { namespace racf {

// Access levels in ascending order of privilege
enum class AccessLevel {
    NONE    = 0,
    READ    = 1,
    UPDATE  = 2,
    CONTROL = 3,
    ALTER   = 4
};

inline const char* access_level_name(AccessLevel a) {
    switch (a) {
        case AccessLevel::NONE:    return "NONE";
        case AccessLevel::READ:    return "READ";
        case AccessLevel::UPDATE:  return "UPDATE";
        case AccessLevel::CONTROL: return "CONTROL";
        case AccessLevel::ALTER:   return "ALTER";
    }
    return "UNKNOWN";
}

inline AccessLevel parse_access_level(std::string_view sv) {
    if (sv == "ALTER")   return AccessLevel::ALTER;
    if (sv == "CONTROL") return AccessLevel::CONTROL;
    if (sv == "UPDATE")  return AccessLevel::UPDATE;
    if (sv == "READ")    return AccessLevel::READ;
    return AccessLevel::NONE;
}

// Connect authority levels
enum class Authority {
    USE     = 0,
    CREATE  = 1,
    CONNECT = 2,
    JOIN    = 3
};

inline const char* authority_name(Authority a) {
    switch (a) {
        case Authority::USE:     return "USE";
        case Authority::CREATE:  return "CREATE";
        case Authority::CONNECT: return "CONNECT";
        case Authority::JOIN:    return "JOIN";
    }
    return "UNKNOWN";
}

inline Authority parse_authority(std::string_view sv) {
    if (sv == "JOIN")    return Authority::JOIN;
    if (sv == "CONNECT") return Authority::CONNECT;
    if (sv == "CREATE")  return Authority::CREATE;
    return Authority::USE;
}

// Access list entry: entity + access level + optional audit flag
struct AccessEntry {
    std::string entity;
    AccessLevel access = AccessLevel::NONE;
    bool        audit  = false;
    int         count  = 0;  // access count from SMF

    bool operator==(const AccessEntry& o) const {
        return entity == o.entity && access == o.access
            && audit == o.audit && count == o.count;
    }
};

// User profile (record types 0100, 0101, 0102, 0103)
struct UserProfile {
    std::string userid;
    std::string name;
    std::string dfltgrp;
    std::string owner;
    std::string created;       // YYYY-MM-DD or Julian
    std::string passdate;      // last password change date
    int         passint = 0;   // password interval (days)
    bool        revoke     = false;
    bool        special    = false;
    bool        operations = false;
    bool        auditor    = false;
    bool        uaudit     = false;
    std::vector<std::string> clauth;   // class authorities

    // OMVS segment (0101)
    int         omvs_uid  = -1;
    std::string omvs_home;
    std::string omvs_shell;

    // CICS segment (0102)
    std::string cics_opident;
    int         cics_opprty = 0;
    int         cics_timeout = 0;

    // DFP segment (0103)
    std::string dfp_dataappl;
    std::string dfp_mgmtclass;
    std::string dfp_storclas;

    bool operator==(const UserProfile& o) const { return userid == o.userid; }
};

// Group profile (record types 0200, 0205)
struct GroupProfile {
    std::string group;
    std::string supgroup;      // superior group
    std::string owner;
    bool        termuacc = false;
    std::vector<std::string> subgroups;

    // OMVS segment (0205)
    int omvs_gid = -1;

    bool operator==(const GroupProfile& o) const { return group == o.group; }
};

// Dataset profile (record type 0400)
struct DatasetProfile {
    std::string dsname;
    AccessLevel uacc = AccessLevel::NONE;
    bool        audit_access   = false;
    bool        globalaudit    = false;
    std::string owner;
    bool        erase   = false;
    bool        warning = false;
    bool        generic = false;  // true = generic profile (contains wildcards)
    std::vector<AccessEntry> access_list;

    bool operator==(const DatasetProfile& o) const { return dsname == o.dsname; }
};

// General resource profile (record type 0500)
struct GeneralProfile {
    std::string resource_class;
    std::string name;
    AccessLevel uacc = AccessLevel::NONE;
    bool        audit_access = false;
    std::string owner;
    std::vector<AccessEntry> access_list;

    bool operator==(const GeneralProfile& o) const {
        return resource_class == o.resource_class && name == o.name;
    }
};

// Connect profile (record type 0600)
struct ConnectProfile {
    std::string userid;
    std::string group;
    Authority   auth = Authority::USE;
    bool        revoke = false;

    bool operator==(const ConnectProfile& o) const {
        return userid == o.userid && group == o.group;
    }
};

// Variant holding any profile type
using RacfProfile = std::variant<
    UserProfile, GroupProfile, DatasetProfile,
    GeneralProfile, ConnectProfile
>;

// Profile database: holds all parsed profiles and supports queries
class ProfileDatabase {
public:
    std::vector<UserProfile>    users;
    std::vector<GroupProfile>   groups;
    std::vector<DatasetProfile> datasets;
    std::vector<GeneralProfile> generals;
    std::vector<ConnectProfile> connects;

    void add(const RacfProfile& p) {
        std::visit([this](const auto& prof) { add_impl(prof); }, p);
    }

    void add(const UserProfile& u)    { users.push_back(u); }
    void add(const GroupProfile& g)   { groups.push_back(g); }
    void add(const DatasetProfile& d) { datasets.push_back(d); }
    void add(const GeneralProfile& g) { generals.push_back(g); }
    void add(const ConnectProfile& c) { connects.push_back(c); }

    // Find user by exact userid (case-insensitive)
    std::optional<UserProfile> find_user(std::string_view uid) const {
        auto upper = to_upper(uid);
        for (auto& u : users) {
            if (to_upper(u.userid) == upper) return u;
        }
        return std::nullopt;
    }

    // Find group by exact name
    std::optional<GroupProfile> find_group(std::string_view gname) const {
        auto upper = to_upper(gname);
        for (auto& g : groups) {
            if (to_upper(g.group) == upper) return g;
        }
        return std::nullopt;
    }

    // Find datasets matching a pattern (supports * wildcard)
    std::vector<DatasetProfile> find_datasets(std::string_view pattern) const {
        std::vector<DatasetProfile> result;
        auto re = wildcard_to_regex(pattern);
        for (auto& d : datasets) {
            if (std::regex_match(d.dsname, re)) result.push_back(d);
        }
        return result;
    }

    // Find users matching a pattern
    std::vector<UserProfile> find_users(std::string_view pattern) const {
        std::vector<UserProfile> result;
        auto re = wildcard_to_regex(pattern);
        for (auto& u : users) {
            if (std::regex_match(u.userid, re)) result.push_back(u);
        }
        return result;
    }

    // Find groups matching a pattern
    std::vector<GroupProfile> find_groups(std::string_view pattern) const {
        std::vector<GroupProfile> result;
        auto re = wildcard_to_regex(pattern);
        for (auto& g : groups) {
            if (std::regex_match(g.group, re)) result.push_back(g);
        }
        return result;
    }

    // Get connect records for a user
    std::vector<ConnectProfile> user_connects(std::string_view uid) const {
        std::vector<ConnectProfile> result;
        auto upper = to_upper(uid);
        for (auto& c : connects) {
            if (to_upper(c.userid) == upper) result.push_back(c);
        }
        return result;
    }

    // Get connect records for a group
    std::vector<ConnectProfile> group_connects(std::string_view gname) const {
        std::vector<ConnectProfile> result;
        auto upper = to_upper(gname);
        for (auto& c : connects) {
            if (to_upper(c.group) == upper) result.push_back(c);
        }
        return result;
    }

    // Group hierarchy: walk supgroup chain up to root (SYS1 or empty)
    std::vector<std::string> group_hierarchy(std::string_view gname) const {
        std::vector<std::string> chain;
        std::set<std::string> visited;
        auto cur = to_upper(gname);
        while (!cur.empty()) {
            if (visited.count(cur)) break;  // cycle guard
            visited.insert(cur);
            chain.push_back(cur);
            auto g = find_group(cur);
            if (!g.has_value() || g->supgroup.empty()) break;
            cur = to_upper(g->supgroup);
            if (cur == chain.back()) break;  // self-referencing
        }
        return chain;
    }

    // Effective permission: highest access a user has to a dataset
    // Checks: SPECIAL flag, UACC, direct access list, group access list
    AccessLevel effective_permission(std::string_view uid,
                                     const DatasetProfile& ds) const {
        auto user = find_user(uid);
        if (!user.has_value()) return AccessLevel::NONE;

        // SPECIAL attribute grants ALTER to everything
        if (user->special) return AccessLevel::ALTER;

        // Owner gets ALTER
        if (to_upper(user->userid) == to_upper(ds.owner)) {
            return AccessLevel::ALTER;
        }

        AccessLevel best = ds.uacc;  // start with UACC

        // Check access list for direct user entry
        auto upper_uid = to_upper(uid);
        for (auto& ae : ds.access_list) {
            if (to_upper(ae.entity) == upper_uid) {
                if (ae.access > best) best = ae.access;
            }
        }

        // Check access list for group entries
        auto conns = user_connects(uid);
        for (auto& conn : conns) {
            auto upper_grp = to_upper(conn.group);
            for (auto& ae : ds.access_list) {
                if (to_upper(ae.entity) == upper_grp) {
                    if (ae.access > best) best = ae.access;
                }
            }
            // Walk up group hierarchy
            auto hier = group_hierarchy(conn.group);
            for (auto& g : hier) {
                for (auto& ae : ds.access_list) {
                    if (to_upper(ae.entity) == g) {
                        if (ae.access > best) best = ae.access;
                    }
                }
            }
        }

        return best;
    }

    // Count all profiles
    size_t total_profiles() const {
        return users.size() + groups.size() + datasets.size()
             + generals.size() + connects.size();
    }

private:
    void add_impl(const UserProfile& u)    { users.push_back(u); }
    void add_impl(const GroupProfile& g)   { groups.push_back(g); }
    void add_impl(const DatasetProfile& d) { datasets.push_back(d); }
    void add_impl(const GeneralProfile& g) { generals.push_back(g); }
    void add_impl(const ConnectProfile& c) { connects.push_back(c); }

    static std::string to_upper(std::string_view sv) {
        std::string s(sv);
        std::transform(s.begin(), s.end(), s.begin(),
                       [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        return s;
    }

    static std::regex wildcard_to_regex(std::string_view pattern) {
        std::string re;
        re.reserve(pattern.size() * 2);
        for (char c : pattern) {
            switch (c) {
                case '*':  re += ".*"; break;
                case '?':  re += "."; break;
                case '.':  re += "\\."; break;
                case '(':  re += "\\("; break;
                case ')':  re += "\\)"; break;
                case '[':  re += "\\["; break;
                case ']':  re += "\\]"; break;
                case '{':  re += "\\{"; break;
                case '}':  re += "\\}"; break;
                case '+':  re += "\\+"; break;
                case '^':  re += "\\^"; break;
                case '$':  re += "\\$"; break;
                case '|':  re += "\\|"; break;
                case '\\': re += "\\\\"; break;
                default:   re += static_cast<char>(std::toupper(static_cast<unsigned char>(c))); break;
            }
        }
        return std::regex(re, std::regex::icase);
    }
};

}} // namespace lazarus::racf

#endif // LAZARUS_RACF_PROFILE_H
