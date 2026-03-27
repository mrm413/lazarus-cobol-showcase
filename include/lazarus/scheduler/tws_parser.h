#ifndef LAZARUS_SCHEDULER_TWS_PARSER_H
#define LAZARUS_SCHEDULER_TWS_PARSER_H

#include "lazarus/scheduler/schedule_model.h"
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <optional>
#include <sstream>
#include <algorithm>
#include <stdexcept>
#include <cctype>

namespace lazarus { namespace scheduler {

// --- TWS/OPC Data Structures ---

struct TWSOperation {
    std::string jobname;
    int jobnr = 0;
    int duration = 0;          // minutes
    std::string opinfo;
    int interval = 0;          // minutes
    std::string workstation;
    std::string status;
    TimeWindow time_restriction;
};

struct TWSDependency {
    std::string pred_adid;
    int pred_opnr = 0;
    std::string succ_adid;
    int succ_opnr = 0;
    char type = 'S';           // S=success, C=check, E=end
};

struct TWSSpecialResource {
    std::string name;
    int quantity = 1;
    int availability = 1;
};

struct TWSVariableEntry {
    std::string name;
    std::string value;
};

struct TWSCalendar {
    std::string name;
    std::vector<std::string> run_days;
    std::vector<std::string> free_days;
    std::string shift;
};

struct TWSApplication {
    std::string adid;
    std::string owner;
    int priority = 5;
    std::string authority;
    std::string calendar_name;
    std::string group;
    std::string description;
    std::vector<TWSOperation> operations;
    std::vector<TWSDependency> dependencies;
    std::vector<TWSSpecialResource> resources;
    std::vector<TWSVariableEntry> variables;
    TWSCalendar calendar;
    bool valid = false;
};

// --- TWS Parser ---

class TWSParser {
public:
    struct ParseResult {
        std::vector<TWSApplication> applications;
        std::vector<std::string> errors;
        bool success = true;

        void add_error(const std::string& msg) {
            success = false;
            errors.push_back(msg);
        }
    };

    ParseResult parse(std::string_view input) const {
        ParseResult result;
        auto lines = split_lines(input);

        TWSApplication current_app;
        bool in_app = false;
        std::string section;

        for (size_t i = 0; i < lines.size(); ++i) {
            auto line = trim(lines[i]);
            if (line.empty() || starts_with(line, "#") || starts_with(line, "*")) {
                continue;
            }

            // Application definition
            if (starts_with(line, "ADID")) {
                if (in_app && !current_app.adid.empty()) {
                    current_app.valid = true;
                    result.applications.push_back(current_app);
                }
                current_app = TWSApplication{};
                in_app = true;
                section = "HEADER";
                current_app.adid = extract_value(line, "ADID");
                // Parse remaining fields on same line
                auto owner = try_extract_value(line, "OWNER");
                if (owner) current_app.owner = *owner;
                auto pri = try_extract_value(line, "PRIORITY");
                if (pri) current_app.priority = std::stoi(*pri);
                auto auth = try_extract_value(line, "AUTHORITY");
                if (auth) current_app.authority = *auth;
                auto cal = try_extract_value(line, "CALENDAR");
                if (cal) current_app.calendar_name = *cal;
                auto grp = try_extract_value(line, "GROUP");
                if (grp) current_app.group = *grp;
                continue;
            }

            if (!in_app) continue;

            // Section markers
            if (starts_with(line, "OPERATIONS") || starts_with(line, "OPS")) {
                section = "OPS";
                continue;
            }
            if (starts_with(line, "DEPENDENCIES") || starts_with(line, "DEPS")) {
                section = "DEPS";
                continue;
            }
            if (starts_with(line, "RESOURCES") || starts_with(line, "RES")) {
                section = "RES";
                continue;
            }
            if (starts_with(line, "VARIABLES") || starts_with(line, "VARS")) {
                section = "VARS";
                continue;
            }
            if (starts_with(line, "CALENDAR") && !starts_with(line, "CALENDAR(")) {
                section = "CAL";
                auto cal_name = try_extract_value(line, "CALENDAR");
                if (cal_name) current_app.calendar.name = *cal_name;
                continue;
            }
            if (starts_with(line, "TIMERESTRICTIONS") || starts_with(line, "TIMERES")) {
                section = "TIME";
                continue;
            }
            if (starts_with(line, "END")) {
                if (in_app && !current_app.adid.empty()) {
                    current_app.valid = true;
                    result.applications.push_back(current_app);
                }
                current_app = TWSApplication{};
                in_app = false;
                section.clear();
                continue;
            }

            // Header continuation
            if (section == "HEADER") {
                auto owner = try_extract_value(line, "OWNER");
                if (owner) current_app.owner = *owner;
                auto pri = try_extract_value(line, "PRIORITY");
                if (pri) current_app.priority = std::stoi(*pri);
                auto auth = try_extract_value(line, "AUTHORITY");
                if (auth) current_app.authority = *auth;
                auto cal = try_extract_value(line, "CALENDAR");
                if (cal) current_app.calendar_name = *cal;
                auto desc = try_extract_value(line, "DESCRIPTION");
                if (desc) current_app.description = *desc;
            }

            // Operations
            if (section == "OPS") {
                auto op = parse_operation(line);
                if (op) {
                    current_app.operations.push_back(*op);
                } else {
                    result.add_error("Failed to parse operation at line " + std::to_string(i + 1));
                }
            }

            // Dependencies
            if (section == "DEPS") {
                auto dep = parse_dependency(line, current_app.adid);
                if (dep) {
                    current_app.dependencies.push_back(*dep);
                } else {
                    result.add_error("Failed to parse dependency at line " + std::to_string(i + 1));
                }
            }

            // Resources
            if (section == "RES") {
                auto res = parse_resource(line);
                if (res) {
                    current_app.resources.push_back(*res);
                }
            }

            // Variables
            if (section == "VARS") {
                auto var = parse_variable(line);
                if (var) {
                    current_app.variables.push_back(*var);
                }
            }

            // Calendar
            if (section == "CAL") {
                parse_calendar_line(line, current_app.calendar);
            }

            // Time restrictions
            if (section == "TIME") {
                parse_time_restriction(line, current_app);
            }
        }

        // Flush last app
        if (in_app && !current_app.adid.empty()) {
            current_app.valid = true;
            result.applications.push_back(current_app);
        }

        return result;
    }

    // Convert parsed TWS applications to a Schedule
    Schedule to_schedule(const ParseResult& pr) const {
        Schedule sched;
        sched.name = "TWS_Schedule";

        for (const auto& app : pr.applications) {
            for (const auto& op : app.operations) {
                ScheduledJob job;
                job.name = app.adid + "." + std::to_string(op.jobnr);
                job.jcl_member = op.jobname;
                job.description = op.opinfo;
                job.owner = app.owner;
                job.priority = map_priority(app.priority);
                job.group = app.group.empty() ? app.adid : app.group;
                job.source_type = ScheduleSource::TWS;
                job.source_info = TWSSource{app.adid, op.jobnr, op.workstation};
                job.time_window = op.time_restriction;
                job.time_window.duration_minutes = op.duration;

                // Calendar
                job.calendar.name = app.calendar_name;
                if (!app.calendar.run_days.empty()) {
                    for (const auto& day : app.calendar.run_days) {
                        auto d = parse_day(day);
                        if (d) job.calendar.run_days.insert(*d);
                    }
                }
                for (const auto& fd : app.calendar.free_days) {
                    job.calendar.holidays.push_back(fd);
                }
                job.calendar.shift = app.calendar.shift;

                // Variables
                for (const auto& v : app.variables) {
                    job.variables[v.name] = v.value;
                }

                // Resources
                for (const auto& r : app.resources) {
                    SpecialResource sr;
                    sr.name = r.name;
                    sr.quantity = r.quantity;
                    sr.availability = r.availability;
                    sr.mode = (r.quantity >= r.availability) ? ResourceMode::EXCLUSIVE : ResourceMode::SHARED;
                    job.resources.push_back(sr);
                }

                sched.add_job(job);
            }

            // Dependencies
            for (const auto& d : app.dependencies) {
                Dependency dep;
                dep.predecessor = d.pred_adid + "." + std::to_string(d.pred_opnr);
                dep.successor = d.succ_adid + "." + std::to_string(d.succ_opnr);
                switch (d.type) {
                    case 'S': dep.condition = DependencyCondition::RC_ZERO; break;
                    case 'C': dep.condition = DependencyCondition::RC_LT4; break;
                    case 'E': dep.condition = DependencyCondition::ALWAYS; break;
                    default:  dep.condition = DependencyCondition::RC_ZERO; break;
                }
                sched.add_dependency(dep);
            }
        }

        return sched;
    }

private:
    static std::vector<std::string_view> split_lines(std::string_view input) {
        std::vector<std::string_view> lines;
        size_t pos = 0;
        while (pos < input.size()) {
            auto nl = input.find('\n', pos);
            if (nl == std::string_view::npos) {
                auto line = input.substr(pos);
                if (!line.empty()) lines.push_back(line);
                break;
            }
            auto line = input.substr(pos, nl - pos);
            // Strip \r
            if (!line.empty() && line.back() == '\r') {
                line = line.substr(0, line.size() - 1);
            }
            lines.push_back(line);
            pos = nl + 1;
        }
        return lines;
    }

    static std::string_view trim(std::string_view sv) {
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
        while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) sv.remove_suffix(1);
        return sv;
    }

    static bool starts_with(std::string_view sv, std::string_view prefix) {
        if (sv.size() < prefix.size()) return false;
        return sv.substr(0, prefix.size()) == prefix;
    }

    static std::string extract_value(std::string_view line, std::string_view key) {
        // Finds KEY(value) or KEY=value or KEY value
        auto result = try_extract_value(line, key);
        return result ? *result : "";
    }

    // Extract value that may contain commas (e.g. RUNDAYS=MON,TUE,WED)
    // Stops at space or end-of-line only, not at commas
    static std::optional<std::string> try_extract_list_value(std::string_view line, std::string_view key) {
        auto pos = line.find(key);
        if (pos == std::string_view::npos) return std::nullopt;
        auto after = pos + key.size();
        if (after >= line.size()) return std::nullopt;
        if (line[after] != '=') return std::nullopt;
        after++;
        if (after >= line.size()) return std::string("");
        auto end = after;
        while (end < line.size() && line[end] != ' ' && line[end] != '\t') {
            end++;
        }
        return std::string(line.substr(after, end - after));
    }

    static std::optional<std::string> try_extract_value(std::string_view line, std::string_view key) {
        auto pos = line.find(key);
        if (pos == std::string_view::npos) return std::nullopt;

        auto after = pos + key.size();
        if (after >= line.size()) return std::nullopt;

        // KEY(value)
        if (line[after] == '(') {
            auto end = line.find(')', after);
            if (end != std::string_view::npos) {
                return std::string(line.substr(after + 1, end - after - 1));
            }
        }

        // KEY=value or KEY="value"
        if (line[after] == '=') {
            after++;
            if (after >= line.size()) return std::string("");
            if (line[after] == '"') {
                auto end = line.find('"', after + 1);
                if (end != std::string_view::npos) {
                    return std::string(line.substr(after + 1, end - after - 1));
                }
            }
            // Unquoted: read to next space or comma
            auto end = after;
            while (end < line.size() && line[end] != ' ' && line[end] != ',' && line[end] != ')') {
                end++;
            }
            return std::string(line.substr(after, end - after));
        }

        // KEY value (space-separated)
        if (line[after] == ' ') {
            auto val_start = after + 1;
            while (val_start < line.size() && line[val_start] == ' ') val_start++;
            auto val_end = val_start;
            while (val_end < line.size() && line[val_end] != ' ' && line[val_end] != ',') val_end++;
            if (val_start < line.size()) {
                return std::string(line.substr(val_start, val_end - val_start));
            }
        }

        return std::nullopt;
    }

    static std::optional<TWSOperation> parse_operation(std::string_view line) {
        TWSOperation op;

        auto jobname = try_extract_value(line, "JOBNAME");
        if (!jobname) jobname = try_extract_value(line, "JOB");
        if (jobname) op.jobname = *jobname;
        else return std::nullopt;

        auto jobnr = try_extract_value(line, "JOBNR");
        if (!jobnr) jobnr = try_extract_value(line, "NR");
        if (jobnr) op.jobnr = std::stoi(*jobnr);

        auto dur = try_extract_value(line, "DURATION");
        if (!dur) dur = try_extract_value(line, "DUR");
        if (dur) op.duration = std::stoi(*dur);

        auto info = try_extract_value(line, "OPINFO");
        if (!info) info = try_extract_value(line, "INFO");
        if (info) op.opinfo = *info;

        auto intv = try_extract_value(line, "INTERVAL");
        if (intv) op.interval = std::stoi(*intv);

        auto ws = try_extract_value(line, "WORKSTATION");
        if (!ws) ws = try_extract_value(line, "WS");
        if (ws) op.workstation = *ws;

        auto earliest = try_extract_value(line, "EARLIEST");
        if (earliest) op.time_restriction.earliest_start = *earliest;
        auto latest = try_extract_value(line, "LATEST");
        if (latest) op.time_restriction.latest_start = *latest;
        auto deadline = try_extract_value(line, "DEADLINE");
        if (deadline) op.time_restriction.deadline = *deadline;

        return op;
    }

    static std::optional<TWSDependency> parse_dependency(std::string_view line,
                                                          const std::string& current_adid) {
        TWSDependency dep;
        dep.succ_adid = current_adid;

        // Format: PRED(ADID.OPNR) SUCC(ADID.OPNR) TYPE(S)
        // Or:     PRED_ADID.PRED_OPNR -> SUCC_ADID.SUCC_OPNR TYPE=S
        // Or:     ADID.OPNR S   (implicit successor = current app first op)

        auto pred = try_extract_value(line, "PRED");
        auto succ = try_extract_value(line, "SUCC");

        if (pred) {
            auto dot = pred->find('.');
            if (dot != std::string::npos) {
                dep.pred_adid = pred->substr(0, dot);
                dep.pred_opnr = std::stoi(pred->substr(dot + 1));
            } else {
                dep.pred_adid = *pred;
                dep.pred_opnr = 1;
            }
        }

        if (succ) {
            auto dot = succ->find('.');
            if (dot != std::string::npos) {
                dep.succ_adid = succ->substr(0, dot);
                dep.succ_opnr = std::stoi(succ->substr(dot + 1));
            } else {
                dep.succ_adid = *succ;
                dep.succ_opnr = 1;
            }
        }

        // Arrow format: ADID.OPNR -> ADID.OPNR TYPE
        if (!pred) {
            auto arrow = line.find("->");
            if (arrow != std::string_view::npos) {
                auto lhs = trim(line.substr(0, arrow));
                auto rhs_str = trim(line.substr(arrow + 2));

                auto dot = lhs.find('.');
                if (dot != std::string_view::npos) {
                    dep.pred_adid = std::string(lhs.substr(0, dot));
                    dep.pred_opnr = std::stoi(std::string(lhs.substr(dot + 1)));
                }

                // RHS: ADID.OPNR TYPE
                auto sp = rhs_str.find(' ');
                std::string_view rhs_job = (sp != std::string_view::npos) ? rhs_str.substr(0, sp) : rhs_str;
                auto rdot = rhs_job.find('.');
                if (rdot != std::string_view::npos) {
                    dep.succ_adid = std::string(rhs_job.substr(0, rdot));
                    dep.succ_opnr = std::stoi(std::string(rhs_job.substr(rdot + 1)));
                }
                if (sp != std::string_view::npos) {
                    auto type_str = trim(rhs_str.substr(sp + 1));
                    if (!type_str.empty()) dep.type = type_str[0];
                }
                return dep;
            }
        }

        // Type
        auto type = try_extract_value(line, "TYPE");
        if (type && !type->empty()) {
            dep.type = (*type)[0];
        }

        if (dep.pred_adid.empty()) return std::nullopt;
        return dep;
    }

    static std::optional<TWSSpecialResource> parse_resource(std::string_view line) {
        TWSSpecialResource res;
        auto name = try_extract_value(line, "NAME");
        if (!name) name = try_extract_value(line, "RES");
        if (!name) return std::nullopt;
        res.name = *name;

        auto qty = try_extract_value(line, "QUANTITY");
        if (!qty) qty = try_extract_value(line, "QTY");
        if (qty) res.quantity = std::stoi(*qty);

        auto avail = try_extract_value(line, "AVAILABILITY");
        if (!avail) avail = try_extract_value(line, "AVAIL");
        if (avail) res.availability = std::stoi(*avail);

        return res;
    }

    static std::optional<TWSVariableEntry> parse_variable(std::string_view line) {
        // Format: &VAR=value or NAME=VAR VALUE=val
        TWSVariableEntry var;

        if (!line.empty() && line[0] == '&') {
            auto eq = line.find('=');
            if (eq != std::string_view::npos) {
                var.name = std::string(line.substr(1, eq - 1));
                var.value = std::string(line.substr(eq + 1));
                return var;
            }
        }

        auto name = try_extract_value(line, "NAME");
        auto value = try_extract_value(line, "VALUE");
        if (name && value) {
            var.name = *name;
            var.value = *value;
            return var;
        }

        return std::nullopt;
    }

    static void parse_calendar_line(std::string_view line, TWSCalendar& cal) {
        auto rundays = try_extract_list_value(line, "RUNDAYS");
        if (rundays) {
            auto days = split_csv(*rundays);
            for (const auto& d : days) {
                cal.run_days.push_back(d);
            }
        }

        auto freedays = try_extract_list_value(line, "FREEDAYS");
        if (freedays) {
            auto days = split_csv(*freedays);
            for (const auto& d : days) {
                cal.free_days.push_back(d);
            }
        }

        auto shift = try_extract_value(line, "SHIFT");
        if (shift) cal.shift = *shift;
    }

    static void parse_time_restriction(std::string_view line, TWSApplication& app) {
        // Apply to last operation or specific JOBNR
        auto jobnr = try_extract_value(line, "JOBNR");
        int target_nr = jobnr ? std::stoi(*jobnr) : -1;

        auto earliest = try_extract_value(line, "EARLIEST");
        auto latest = try_extract_value(line, "LATEST");
        auto deadline = try_extract_value(line, "DEADLINE");

        for (auto& op : app.operations) {
            if (target_nr < 0 || op.jobnr == target_nr) {
                if (earliest) op.time_restriction.earliest_start = *earliest;
                if (latest) op.time_restriction.latest_start = *latest;
                if (deadline) op.time_restriction.deadline = *deadline;
            }
        }
    }

    static std::vector<std::string> split_csv(const std::string& s) {
        std::vector<std::string> result;
        std::istringstream iss(s);
        std::string token;
        while (std::getline(iss, token, ',')) {
            auto sv = trim(std::string_view(token));
            if (!sv.empty()) result.push_back(std::string(sv));
        }
        return result;
    }

    static JobPriority map_priority(int p) {
        if (p <= 2) return JobPriority::LOW;
        if (p <= 6) return JobPriority::NORMAL;
        if (p <= 8) return JobPriority::HIGH;
        return JobPriority::CRITICAL;
    }

    static std::optional<DayOfWeek> parse_day(const std::string& d) {
        std::string lower = d;
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
        if (lower == "mon" || lower == "monday" || lower == "1") return DayOfWeek::MONDAY;
        if (lower == "tue" || lower == "tuesday" || lower == "2") return DayOfWeek::TUESDAY;
        if (lower == "wed" || lower == "wednesday" || lower == "3") return DayOfWeek::WEDNESDAY;
        if (lower == "thu" || lower == "thursday" || lower == "4") return DayOfWeek::THURSDAY;
        if (lower == "fri" || lower == "friday" || lower == "5") return DayOfWeek::FRIDAY;
        if (lower == "sat" || lower == "saturday" || lower == "6") return DayOfWeek::SATURDAY;
        if (lower == "sun" || lower == "sunday" || lower == "7") return DayOfWeek::SUNDAY;
        return std::nullopt;
    }
};

}} // namespace lazarus::scheduler

#endif // LAZARUS_SCHEDULER_TWS_PARSER_H
