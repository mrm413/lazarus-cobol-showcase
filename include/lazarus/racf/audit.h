// Lazarus Systems -- RACF Audit & Conflict Detection
// Tier 8: RACF Profile Importer -- permission graph, escalation, orphan, report

#ifndef LAZARUS_RACF_AUDIT_H
#define LAZARUS_RACF_AUDIT_H

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <sstream>
#include <map>
#include <set>
#include <functional>
#include <ctime>
#include <numeric>

#include "lazarus/racf/profile.h"

namespace lazarus { namespace racf {

// Severity for audit findings
enum class Severity {
    INFO     = 0,
    WARNING  = 1,
    CRITICAL = 2
};

inline const char* severity_name(Severity s) {
    switch (s) {
        case Severity::INFO:     return "INFO";
        case Severity::WARNING:  return "WARNING";
        case Severity::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

// A single audit finding
struct AuditFinding {
    Severity    severity;
    std::string category;
    std::string description;
    std::string entity;       // user, group, or dataset involved
    std::string detail;       // additional context

    bool operator==(const AuditFinding& o) const {
        return severity == o.severity && category == o.category
            && description == o.description && entity == o.entity;
    }
};

// Summary statistics
struct AuditSummary {
    size_t total_users    = 0;
    size_t total_groups   = 0;
    size_t total_datasets = 0;
    size_t total_generals = 0;
    size_t total_connects = 0;
    size_t total_access_rules = 0;
    size_t total_findings     = 0;
    size_t critical_count     = 0;
    size_t warning_count      = 0;
    size_t info_count         = 0;
};

// Permission graph node (for escalation analysis)
struct PermNode {
    std::string name;
    std::string type;  // "user", "group", "dataset"
    std::vector<size_t> edges;  // indices into graph node list
};

// Directed permission graph
class PermissionGraph {
public:
    // Add node, return index
    size_t add_node(const std::string& name, const std::string& type) {
        auto key = type + ":" + name;
        auto it = index_.find(key);
        if (it != index_.end()) return it->second;
        size_t idx = nodes_.size();
        nodes_.push_back({name, type, {}});
        index_[key] = idx;
        return idx;
    }

    // Add directed edge from -> to
    void add_edge(size_t from, size_t to) {
        if (from < nodes_.size()) {
            nodes_[from].edges.push_back(to);
        }
    }

    // Find node index by name+type
    std::optional<size_t> find(const std::string& name,
                                const std::string& type) const {
        auto key = type + ":" + name;
        auto it = index_.find(key);
        if (it != index_.end()) return it->second;
        return std::nullopt;
    }

    // BFS reachability: which nodes can be reached from 'start'
    std::set<size_t> reachable(size_t start) const {
        std::set<size_t> visited;
        std::vector<size_t> queue;
        queue.push_back(start);
        visited.insert(start);
        while (!queue.empty()) {
            auto cur = queue.back();
            queue.pop_back();
            if (cur < nodes_.size()) {
                for (auto next : nodes_[cur].edges) {
                    if (visited.insert(next).second) {
                        queue.push_back(next);
                    }
                }
            }
        }
        return visited;
    }

    const std::vector<PermNode>& nodes() const { return nodes_; }
    size_t size() const { return nodes_.size(); }

private:
    std::vector<PermNode> nodes_;
    std::map<std::string, size_t> index_;
};

// Full audit report
struct AuditReport {
    AuditSummary summary;
    std::vector<AuditFinding> findings;
    PermissionGraph graph;

    // Filter findings by severity
    std::vector<AuditFinding> by_severity(Severity s) const {
        std::vector<AuditFinding> result;
        for (auto& f : findings) {
            if (f.severity == s) result.push_back(f);
        }
        return result;
    }

    // Filter findings by category
    std::vector<AuditFinding> by_category(std::string_view cat) const {
        std::vector<AuditFinding> result;
        for (auto& f : findings) {
            if (f.category == cat) result.push_back(f);
        }
        return result;
    }

    // Generate structured text report
    std::string to_text() const {
        std::ostringstream os;
        os << "=== RACF Security Audit Report ===\n\n";
        os << "Summary:\n";
        os << "  Users:        " << summary.total_users << "\n";
        os << "  Groups:       " << summary.total_groups << "\n";
        os << "  Datasets:     " << summary.total_datasets << "\n";
        os << "  Resources:    " << summary.total_generals << "\n";
        os << "  Connections:  " << summary.total_connects << "\n";
        os << "  Access Rules: " << summary.total_access_rules << "\n";
        os << "  Findings:     " << summary.total_findings << "\n";
        os << "    CRITICAL:   " << summary.critical_count << "\n";
        os << "    WARNING:    " << summary.warning_count << "\n";
        os << "    INFO:       " << summary.info_count << "\n";
        os << "\n";

        if (!findings.empty()) {
            os << "Findings:\n";
            os << std::string(60, '-') << "\n";
            for (size_t i = 0; i < findings.size(); ++i) {
                auto& f = findings[i];
                os << "[" << severity_name(f.severity) << "] "
                   << f.category << ": " << f.description << "\n";
                if (!f.entity.empty())
                    os << "  Entity: " << f.entity << "\n";
                if (!f.detail.empty())
                    os << "  Detail: " << f.detail << "\n";
            }
        }
        return os.str();
    }

    // Generate JSON report
    std::string to_json() const {
        std::ostringstream os;
        os << "{\n";
        os << "  \"summary\": {\n";
        os << "    \"total_users\": " << summary.total_users << ",\n";
        os << "    \"total_groups\": " << summary.total_groups << ",\n";
        os << "    \"total_datasets\": " << summary.total_datasets << ",\n";
        os << "    \"total_resources\": " << summary.total_generals << ",\n";
        os << "    \"total_connections\": " << summary.total_connects << ",\n";
        os << "    \"total_access_rules\": " << summary.total_access_rules << ",\n";
        os << "    \"total_findings\": " << summary.total_findings << ",\n";
        os << "    \"critical\": " << summary.critical_count << ",\n";
        os << "    \"warnings\": " << summary.warning_count << ",\n";
        os << "    \"info\": " << summary.info_count << "\n";
        os << "  },\n";
        os << "  \"findings\": [\n";
        for (size_t i = 0; i < findings.size(); ++i) {
            auto& f = findings[i];
            os << "    {\n";
            os << "      \"severity\": \"" << severity_name(f.severity) << "\",\n";
            os << "      \"category\": \"" << f.category << "\",\n";
            os << "      \"description\": \"" << f.description << "\",\n";
            os << "      \"entity\": \"" << f.entity << "\",\n";
            os << "      \"detail\": \"" << f.detail << "\"\n";
            os << "    }";
            if (i + 1 < findings.size()) os << ",";
            os << "\n";
        }
        os << "  ]\n";
        os << "}";
        return os.str();
    }
};

// Main auditor: analyzes a ProfileDatabase for security issues
class SecurityAuditor {
public:
    AuditReport audit(const ProfileDatabase& db) const {
        AuditReport report;

        // Build summary
        report.summary.total_users    = db.users.size();
        report.summary.total_groups   = db.groups.size();
        report.summary.total_datasets = db.datasets.size();
        report.summary.total_generals = db.generals.size();
        report.summary.total_connects = db.connects.size();
        for (auto& ds : db.datasets) {
            report.summary.total_access_rules += ds.access_list.size();
        }
        for (auto& g : db.generals) {
            report.summary.total_access_rules += g.access_list.size();
        }

        // Build permission graph
        build_graph(report.graph, db);

        // Run all detectors
        detect_privilege_escalation(report, db);
        detect_overlapping_datasets(report, db);
        detect_contradictory_memberships(report, db);
        detect_orphaned_resources(report, db);
        detect_unused_accounts(report, db);
        detect_excessive_permissions(report, db);
        detect_revoked_connects(report, db);

        // Tally findings
        report.summary.total_findings = report.findings.size();
        for (auto& f : report.findings) {
            switch (f.severity) {
                case Severity::CRITICAL: report.summary.critical_count++; break;
                case Severity::WARNING:  report.summary.warning_count++; break;
                case Severity::INFO:     report.summary.info_count++; break;
            }
        }
        return report;
    }

    // Set a "stale days" threshold for unused accounts (default 90)
    void set_stale_days(int days) { stale_days_ = days; }

    // Set a reference date for stale checking (YYYY-MM-DD)
    void set_reference_date(const std::string& date) { ref_date_ = date; }

private:
    int stale_days_ = 90;
    std::string ref_date_;

    void build_graph(PermissionGraph& g, const ProfileDatabase& db) const {
        // Add all entities as nodes
        for (auto& u : db.users)    g.add_node(u.userid, "user");
        for (auto& grp : db.groups) g.add_node(grp.group, "group");
        for (auto& ds : db.datasets) g.add_node(ds.dsname, "dataset");

        // Connect edges: user -> group (via connect records)
        for (auto& c : db.connects) {
            auto ui = g.find(c.userid, "user");
            auto gi = g.find(c.group, "group");
            if (ui && gi) g.add_edge(*ui, *gi);
        }

        // Group hierarchy: group -> supgroup
        for (auto& grp : db.groups) {
            if (!grp.supgroup.empty()) {
                auto ci = g.find(grp.group, "group");
                auto si = g.find(grp.supgroup, "group");
                if (ci && si) g.add_edge(*ci, *si);
            }
        }

        // Dataset access: group/user -> dataset
        for (auto& ds : db.datasets) {
            auto di = g.find(ds.dsname, "dataset");
            if (!di) continue;
            for (auto& ae : ds.access_list) {
                auto ui = g.find(ae.entity, "user");
                auto gi = g.find(ae.entity, "group");
                if (ui) g.add_edge(*ui, *di);
                else if (gi) g.add_edge(*gi, *di);
            }
        }
    }

    // CRITICAL: SPECIAL + OPERATIONS on same user
    void detect_privilege_escalation(AuditReport& report,
                                      const ProfileDatabase& db) const {
        for (auto& u : db.users) {
            if (u.special && u.operations) {
                report.findings.push_back({
                    Severity::CRITICAL,
                    "PRIVILEGE_ESCALATION",
                    "User has both SPECIAL and OPERATIONS attributes",
                    u.userid,
                    "Combined SPECIAL+OPERATIONS grants unrestricted system access"
                });
            }
            if (u.special && u.auditor) {
                report.findings.push_back({
                    Severity::WARNING,
                    "PRIVILEGE_ESCALATION",
                    "User has both SPECIAL and AUDITOR attributes",
                    u.userid,
                    "Admin can modify audit trails (separation of duties violation)"
                });
            }
        }
    }

    // WARNING: overlapping dataset rules (generic vs discrete for same HLQ)
    void detect_overlapping_datasets(AuditReport& report,
                                      const ProfileDatabase& db) const {
        // Build map of HLQ -> profiles
        std::map<std::string, std::vector<const DatasetProfile*>> hlq_map;
        for (auto& ds : db.datasets) {
            auto hlq = extract_hlq(ds.dsname);
            hlq_map[hlq].push_back(&ds);
        }
        for (auto& [hlq, profiles] : hlq_map) {
            bool has_generic = false;
            bool has_discrete = false;
            std::string gen_name, disc_name;
            for (auto* p : profiles) {
                if (p->generic) { has_generic = true; gen_name = p->dsname; }
                else { has_discrete = true; disc_name = p->dsname; }
            }
            if (has_generic && has_discrete) {
                report.findings.push_back({
                    Severity::WARNING,
                    "OVERLAPPING_DATASET",
                    "Generic and discrete profiles overlap for same HLQ",
                    hlq,
                    "Generic: " + gen_name + ", Discrete: " + disc_name
                });
            }
        }
    }

    // WARNING: user in groups with contradictory access to same dataset
    void detect_contradictory_memberships(AuditReport& report,
                                           const ProfileDatabase& db) const {
        for (auto& u : db.users) {
            auto conns = db.user_connects(u.userid);
            if (conns.size() < 2) continue;

            // For each dataset, collect access levels from each group
            for (auto& ds : db.datasets) {
                std::map<std::string, AccessLevel> group_access;
                for (auto& ae : ds.access_list) {
                    for (auto& c : conns) {
                        if (ae.entity == c.group) {
                            group_access[c.group] = ae.access;
                        }
                    }
                }
                if (group_access.size() < 2) continue;

                // Check for contradictions: one grants ALTER, another NONE
                AccessLevel min_a = AccessLevel::ALTER;
                AccessLevel max_a = AccessLevel::NONE;
                std::string min_g, max_g;
                for (auto& [grp, acc] : group_access) {
                    if (acc < min_a) { min_a = acc; min_g = grp; }
                    if (acc > max_a) { max_a = acc; max_g = grp; }
                }
                // Report if there is a meaningful spread (e.g. NONE vs UPDATE+)
                if (min_a == AccessLevel::NONE && max_a >= AccessLevel::UPDATE) {
                    report.findings.push_back({
                        Severity::WARNING,
                        "CONTRADICTORY_MEMBERSHIP",
                        "User groups have contradictory access to dataset",
                        u.userid,
                        "Dataset: " + ds.dsname + ", " + min_g + "=" +
                        std::string(access_level_name(min_a)) + " vs " +
                        max_g + "=" + std::string(access_level_name(max_a))
                    });
                }
            }
        }
    }

    // WARNING: datasets with no valid owner
    void detect_orphaned_resources(AuditReport& report,
                                    const ProfileDatabase& db) const {
        std::set<std::string> valid_owners;
        for (auto& u : db.users) valid_owners.insert(u.userid);
        for (auto& g : db.groups) valid_owners.insert(g.group);

        for (auto& ds : db.datasets) {
            if (!ds.owner.empty() && valid_owners.find(ds.owner) == valid_owners.end()) {
                report.findings.push_back({
                    Severity::WARNING,
                    "ORPHANED_RESOURCE",
                    "Dataset owner does not exist in RACF",
                    ds.dsname,
                    "Owner: " + ds.owner
                });
            }
        }
        for (auto& g : db.generals) {
            if (!g.owner.empty() && valid_owners.find(g.owner) == valid_owners.end()) {
                report.findings.push_back({
                    Severity::WARNING,
                    "ORPHANED_RESOURCE",
                    "General resource owner does not exist in RACF",
                    g.resource_class + "/" + g.name,
                    "Owner: " + g.owner
                });
            }
        }
    }

    // INFO: accounts with stale PASSDATE
    void detect_unused_accounts(AuditReport& report,
                                 const ProfileDatabase& db) const {
        for (auto& u : db.users) {
            if (u.revoke) continue;  // already revoked
            if (u.passdate.empty()) {
                report.findings.push_back({
                    Severity::INFO,
                    "UNUSED_ACCOUNT",
                    "User has no recorded password change date",
                    u.userid,
                    "No PASSDATE set -- may be a service account or unused"
                });
                continue;
            }
            if (!ref_date_.empty() && is_stale(u.passdate, ref_date_, stale_days_)) {
                report.findings.push_back({
                    Severity::INFO,
                    "UNUSED_ACCOUNT",
                    "User password not changed within threshold",
                    u.userid,
                    "PASSDATE: " + u.passdate + ", threshold: "
                    + std::to_string(stale_days_) + " days"
                });
            }
        }
    }

    // WARNING: datasets with UACC of ALTER or CONTROL
    void detect_excessive_permissions(AuditReport& report,
                                       const ProfileDatabase& db) const {
        for (auto& ds : db.datasets) {
            if (ds.uacc >= AccessLevel::CONTROL) {
                report.findings.push_back({
                    Severity::WARNING,
                    "EXCESSIVE_PERMISSION",
                    "Dataset has high default access (UACC)",
                    ds.dsname,
                    "UACC: " + std::string(access_level_name(ds.uacc))
                });
            }
        }
    }

    // INFO: revoked connect records
    void detect_revoked_connects(AuditReport& report,
                                  const ProfileDatabase& db) const {
        for (auto& c : db.connects) {
            if (c.revoke) {
                report.findings.push_back({
                    Severity::INFO,
                    "REVOKED_CONNECT",
                    "Connect record is revoked",
                    c.userid + "/" + c.group,
                    "User " + c.userid + " revoked from group " + c.group
                });
            }
        }
    }

    // Extract high-level qualifier from dataset name
    static std::string extract_hlq(const std::string& dsname) {
        auto dot = dsname.find('.');
        if (dot == std::string::npos) return dsname;
        return dsname.substr(0, dot);
    }

    // Simple date comparison: is passdate more than N days before ref_date?
    // Dates in YYYY-MM-DD format
    static bool is_stale(const std::string& passdate,
                          const std::string& ref_date,
                          int threshold_days) {
        if (passdate.size() < 10 || ref_date.size() < 10) return false;
        // Parse YYYY-MM-DD
        auto parse_ymd = [](const std::string& d) -> int {
            int y = 0, m = 0, day = 0;
            if (d.size() >= 10 && d[4] == '-' && d[7] == '-') {
                y   = std::stoi(d.substr(0, 4));
                m   = std::stoi(d.substr(5, 2));
                day = std::stoi(d.substr(8, 2));
            }
            // Approximate day count (good enough for threshold)
            return y * 365 + m * 30 + day;
        };
        int pd = parse_ymd(passdate);
        int rd = parse_ymd(ref_date);
        return (rd - pd) > threshold_days;
    }
};

}} // namespace lazarus::racf

#endif // LAZARUS_RACF_AUDIT_H
