#ifndef LAZARUS_SCHEDULER_SCHEDULE_MODEL_H
#define LAZARUS_SCHEDULER_SCHEDULE_MODEL_H

#include <string>
#include <vector>
#include <variant>
#include <optional>
#include <map>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <functional>
#include <numeric>
#include <cstdint>

namespace lazarus { namespace scheduler {

// --- Enumerations ---

enum class DependencyCondition {
    RC_ZERO,       // RC=0 (success only)
    RC_LT4,        // RC<4 (warnings OK)
    RC_LT8,        // RC<8 (minor errors OK)
    ALWAYS,        // run regardless
    ONLY_FAIL,     // run only on predecessor failure
    CUSTOM         // custom expression
};

enum class RunCycle {
    DAILY,
    WEEKLY,
    MONTHLY,
    YEARLY,
    BUSINESS_DAYS,
    CUSTOM
};

enum class DayOfWeek {
    MONDAY = 1,
    TUESDAY,
    WEDNESDAY,
    THURSDAY,
    FRIDAY,
    SATURDAY,
    SUNDAY
};

enum class ResourceMode {
    SHARED,
    EXCLUSIVE
};

enum class JobPriority {
    LOW = 1,
    NORMAL = 5,
    HIGH = 8,
    CRITICAL = 10
};

enum class ScheduleSource {
    TWS,
    CA7,
    MANUAL
};

// --- Time Window ---

struct TimeWindow {
    std::string earliest_start;    // HH:MM format
    std::string latest_start;      // HH:MM format
    std::string deadline;          // HH:MM format
    int duration_minutes = 0;

    bool is_valid() const {
        if (earliest_start.empty()) return true;
        if (!latest_start.empty() && latest_start < earliest_start) return false;
        if (!deadline.empty() && !earliest_start.empty() && deadline < earliest_start) return false;
        return true;
    }

    bool has_conflict(const TimeWindow& other) const {
        if (earliest_start.empty() || other.earliest_start.empty()) return false;
        if (!deadline.empty() && !other.earliest_start.empty()) {
            if (deadline <= other.earliest_start) return false;
        }
        if (!other.deadline.empty() && !earliest_start.empty()) {
            if (other.deadline <= earliest_start) return false;
        }
        return (!deadline.empty() && !other.earliest_start.empty() &&
                deadline > other.earliest_start) ||
               (!other.deadline.empty() && !earliest_start.empty() &&
                other.deadline > earliest_start);
    }
};

// --- Calendar ---

struct Calendar {
    std::string name;
    RunCycle cycle = RunCycle::DAILY;
    std::set<DayOfWeek> run_days;
    std::vector<std::string> holidays;     // YYYY-MM-DD format
    std::vector<std::string> free_days;    // additional free days
    int monthly_day = 0;                   // 1-31 for monthly; 0 = not set
    int yearly_month = 0;                  // 1-12 for yearly
    int yearly_day = 0;                    // 1-31 for yearly
    std::string shift;                     // shift identifier
    std::string custom_expression;

    bool is_business_day_calendar() const {
        if (run_days.size() != 5) return false;
        return run_days.count(DayOfWeek::MONDAY) &&
               run_days.count(DayOfWeek::TUESDAY) &&
               run_days.count(DayOfWeek::WEDNESDAY) &&
               run_days.count(DayOfWeek::THURSDAY) &&
               run_days.count(DayOfWeek::FRIDAY);
    }

    bool is_holiday(const std::string& date) const {
        return std::find(holidays.begin(), holidays.end(), date) != holidays.end() ||
               std::find(free_days.begin(), free_days.end(), date) != free_days.end();
    }
};

// --- Special Resource ---

struct SpecialResource {
    std::string name;
    ResourceMode mode = ResourceMode::SHARED;
    int quantity = 1;
    int availability = 1;

    bool conflicts_with(const SpecialResource& other) const {
        if (name != other.name) return false;
        if (mode == ResourceMode::EXCLUSIVE || other.mode == ResourceMode::EXCLUSIVE) return true;
        return (quantity + other.quantity) > availability;
    }
};

// --- Dependency ---

struct Dependency {
    std::string predecessor;
    std::string successor;
    DependencyCondition condition = DependencyCondition::RC_ZERO;
    std::string custom_condition;
    int offset_minutes = 0;

    std::string condition_str() const {
        switch (condition) {
            case DependencyCondition::RC_ZERO:   return "RC=0";
            case DependencyCondition::RC_LT4:    return "RC<4";
            case DependencyCondition::RC_LT8:    return "RC<8";
            case DependencyCondition::ALWAYS:     return "ALWAYS";
            case DependencyCondition::ONLY_FAIL:  return "ONLY_FAIL";
            case DependencyCondition::CUSTOM:     return custom_condition;
        }
        return "RC=0";
    }
};

// --- Source Distinction ---

struct TWSSource {
    std::string adid;
    int operation_number = 0;
    std::string workstation;
};

struct CA7Source {
    std::string system;
    std::string jclid;
    int schedule_id = 0;
};

using SourceInfo = std::variant<TWSSource, CA7Source>;

// --- Scheduled Job ---

struct ScheduledJob {
    std::string name;
    std::string description;
    std::string owner;
    Calendar calendar;
    TimeWindow time_window;
    JobPriority priority = JobPriority::NORMAL;
    std::vector<SpecialResource> resources;
    std::map<std::string, std::string> variables;
    ScheduleSource source_type = ScheduleSource::MANUAL;
    std::optional<SourceInfo> source_info;
    std::string group;
    bool enabled = true;
    std::string jcl_member;
    std::string error_handling;

    int priority_value() const { return static_cast<int>(priority); }
};

// --- Schedule (collection of jobs + deps) ---

struct ScheduleValidationResult {
    bool valid = true;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::vector<std::vector<std::string>> cycles;
    std::vector<std::string> unreachable_jobs;
    std::vector<std::pair<std::string, std::string>> timing_conflicts;

    void add_error(const std::string& msg) {
        valid = false;
        errors.push_back(msg);
    }
    void add_warning(const std::string& msg) {
        warnings.push_back(msg);
    }
};

class Schedule {
public:
    std::string name;
    std::string description;

    void add_job(const ScheduledJob& job) {
        jobs_[job.name] = job;
    }

    void add_dependency(const Dependency& dep) {
        dependencies_.push_back(dep);
    }

    const std::map<std::string, ScheduledJob>& jobs() const { return jobs_; }
    const std::vector<Dependency>& dependencies() const { return dependencies_; }

    std::optional<ScheduledJob> get_job(const std::string& name) const {
        auto it = jobs_.find(name);
        if (it != jobs_.end()) return it->second;
        return std::nullopt;
    }

    std::vector<std::string> predecessors(const std::string& job) const {
        std::vector<std::string> result;
        for (const auto& d : dependencies_) {
            if (d.successor == job) result.push_back(d.predecessor);
        }
        return result;
    }

    std::vector<std::string> successors(const std::string& job) const {
        std::vector<std::string> result;
        for (const auto& d : dependencies_) {
            if (d.predecessor == job) result.push_back(d.successor);
        }
        return result;
    }

    // Topological sort — returns empty if cycle exists
    std::vector<std::string> topological_order() const {
        std::map<std::string, int> in_degree;
        std::map<std::string, std::vector<std::string>> adj;

        for (const auto& [name, _] : jobs_) {
            (void)_;
            in_degree[name] = 0;
        }
        for (const auto& d : dependencies_) {
            if (jobs_.count(d.predecessor) && jobs_.count(d.successor)) {
                adj[d.predecessor].push_back(d.successor);
                in_degree[d.successor]++;
            }
        }

        std::vector<std::string> queue;
        for (const auto& [name, deg] : in_degree) {
            if (deg == 0) queue.push_back(name);
        }

        std::vector<std::string> order;
        while (!queue.empty()) {
            std::sort(queue.begin(), queue.end());
            std::string curr = queue.front();
            queue.erase(queue.begin());
            order.push_back(curr);
            for (const auto& next : adj[curr]) {
                in_degree[next]--;
                if (in_degree[next] == 0) queue.push_back(next);
            }
        }

        if (order.size() != jobs_.size()) return {};
        return order;
    }

    // Detect cycles via DFS
    std::vector<std::vector<std::string>> detect_cycles() const {
        std::map<std::string, std::vector<std::string>> adj;
        for (const auto& d : dependencies_) {
            if (jobs_.count(d.predecessor) && jobs_.count(d.successor)) {
                adj[d.predecessor].push_back(d.successor);
            }
        }

        std::vector<std::vector<std::string>> cycles;
        std::set<std::string> visited;
        std::set<std::string> rec_stack;
        std::vector<std::string> path;

        for (const auto& [name, _] : jobs_) {
            (void)_;
            if (!visited.count(name)) {
                dfs_cycles(name, adj, visited, rec_stack, path, cycles);
            }
        }
        return cycles;
    }

    // Find unreachable jobs (no predecessors AND not a root)
    std::vector<std::string> find_unreachable() const {
        if (jobs_.empty()) return {};

        std::set<std::string> has_predecessor;
        std::set<std::string> has_successor;
        for (const auto& d : dependencies_) {
            if (jobs_.count(d.successor)) has_predecessor.insert(d.successor);
            if (jobs_.count(d.predecessor)) has_successor.insert(d.predecessor);
        }

        // Root jobs have no predecessors
        std::set<std::string> roots;
        for (const auto& [name, _] : jobs_) {
            (void)_;
            if (!has_predecessor.count(name)) roots.insert(name);
        }

        // BFS from roots
        std::set<std::string> reachable = roots;
        std::vector<std::string> queue(roots.begin(), roots.end());
        std::map<std::string, std::vector<std::string>> adj;
        for (const auto& d : dependencies_) {
            adj[d.predecessor].push_back(d.successor);
        }
        while (!queue.empty()) {
            std::string curr = queue.back();
            queue.pop_back();
            for (const auto& next : adj[curr]) {
                if (!reachable.count(next)) {
                    reachable.insert(next);
                    queue.push_back(next);
                }
            }
        }

        std::vector<std::string> unreachable;
        for (const auto& [name, _] : jobs_) {
            (void)_;
            if (!reachable.count(name)) unreachable.push_back(name);
        }
        std::sort(unreachable.begin(), unreachable.end());
        return unreachable;
    }

    // Full validation
    ScheduleValidationResult validate() const {
        ScheduleValidationResult result;

        // Check for undefined jobs in dependencies
        for (const auto& d : dependencies_) {
            if (!jobs_.count(d.predecessor)) {
                result.add_error("Dependency references undefined predecessor: " + d.predecessor);
            }
            if (!jobs_.count(d.successor)) {
                result.add_error("Dependency references undefined successor: " + d.successor);
            }
        }

        // Cycle detection
        auto cycles = detect_cycles();
        if (!cycles.empty()) {
            result.cycles = cycles;
            for (const auto& cycle : cycles) {
                std::string msg = "Cycle detected: ";
                for (size_t i = 0; i < cycle.size(); ++i) {
                    if (i > 0) msg += " -> ";
                    msg += cycle[i];
                }
                result.add_error(msg);
            }
        }

        // Unreachable jobs (only in cycles)
        auto unreachable = find_unreachable();
        result.unreachable_jobs = unreachable;
        if (!unreachable.empty()) {
            for (const auto& u : unreachable) {
                result.add_warning("Job unreachable from roots (in cycle?): " + u);
            }
        }

        // Timing conflicts
        for (const auto& d : dependencies_) {
            if (!jobs_.count(d.predecessor) || !jobs_.count(d.successor)) continue;
            const auto& pred = jobs_.at(d.predecessor);
            const auto& succ = jobs_.at(d.successor);
            if (pred.time_window.has_conflict(succ.time_window)) {
                result.timing_conflicts.emplace_back(d.predecessor, d.successor);
                result.add_warning("Timing conflict between " + d.predecessor + " and " + d.successor);
            }
        }

        // Time window validity
        for (const auto& [name, job] : jobs_) {
            if (!job.time_window.is_valid()) {
                result.add_error("Invalid time window for job: " + name);
            }
        }

        // Resource conflicts — exclusive resources held by concurrent root jobs
        auto roots = topological_order();
        for (size_t i = 0; i < roots.size(); ++i) {
            for (size_t j = i + 1; j < roots.size(); ++j) {
                if (!jobs_.count(roots[i]) || !jobs_.count(roots[j])) continue;
                const auto& ji = jobs_.at(roots[i]);
                const auto& jj = jobs_.at(roots[j]);
                for (const auto& ri : ji.resources) {
                    for (const auto& rj : jj.resources) {
                        if (ri.conflicts_with(rj)) {
                            result.add_warning("Resource conflict (" + ri.name +
                                ") between " + roots[i] + " and " + roots[j]);
                        }
                    }
                }
            }
        }

        return result;
    }

    // Merge another schedule into this one
    void merge(const Schedule& other, const std::string& prefix = "") {
        for (const auto& [name, job] : other.jobs_) {
            ScheduledJob j = job;
            if (!prefix.empty()) {
                j.name = prefix + "_" + j.name;
                j.group = prefix;
            }
            add_job(j);
        }
        for (const auto& d : other.dependencies_) {
            Dependency dep = d;
            if (!prefix.empty()) {
                dep.predecessor = prefix + "_" + dep.predecessor;
                dep.successor = prefix + "_" + dep.successor;
            }
            add_dependency(dep);
        }
    }

    size_t job_count() const { return jobs_.size(); }
    size_t dependency_count() const { return dependencies_.size(); }

private:
    std::map<std::string, ScheduledJob> jobs_;
    std::vector<Dependency> dependencies_;

    void dfs_cycles(const std::string& node,
                    const std::map<std::string, std::vector<std::string>>& adj,
                    std::set<std::string>& visited,
                    std::set<std::string>& rec_stack,
                    std::vector<std::string>& path,
                    std::vector<std::vector<std::string>>& cycles) const {
        visited.insert(node);
        rec_stack.insert(node);
        path.push_back(node);

        auto it = adj.find(node);
        if (it != adj.end()) {
            for (const auto& next : it->second) {
                if (!visited.count(next)) {
                    dfs_cycles(next, adj, visited, rec_stack, path, cycles);
                } else if (rec_stack.count(next)) {
                    // Found cycle — extract it
                    std::vector<std::string> cycle;
                    auto start = std::find(path.begin(), path.end(), next);
                    for (auto ci = start; ci != path.end(); ++ci) {
                        cycle.push_back(*ci);
                    }
                    cycle.push_back(next);
                    cycles.push_back(cycle);
                }
            }
        }

        path.pop_back();
        rec_stack.erase(node);
    }
};

}} // namespace lazarus::scheduler

#endif // LAZARUS_SCHEDULER_SCHEDULE_MODEL_H
