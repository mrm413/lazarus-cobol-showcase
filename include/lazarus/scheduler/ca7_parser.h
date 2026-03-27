#ifndef LAZARUS_SCHEDULER_CA7_PARSER_H
#define LAZARUS_SCHEDULER_CA7_PARSER_H

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

// --- CA-7 Data Structures ---

struct CA7Prose {
    std::vector<std::string> lines;

    std::string full_text() const {
        std::string result;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i > 0) result += "\n";
            result += lines[i];
        }
        return result;
    }
};

struct CA7Schedule {
    std::string calendar;      // SCAL=
    int schedule_id = 0;       // ID=
    bool dotrig = false;       // DOTRIG=Y/N
    std::string start_time;    // STTM=
    std::string deadline;      // DLNT=
    int lead_time = 0;         // LDTM=
    std::string cycle_type;    // DAILY, WEEKLY, MONTHLY
    std::vector<int> run_days; // for WEEKLY: 1-7
    int monthly_day = 0;       // for MONTHLY: 1-31
};

struct CA7Trigger {
    std::string predecessor_job;
    std::string condition;     // SUCC, FAIL, ABEND
    int schid = 0;             // schedule ID filter
    bool resolve_late = false;
};

struct CA7Requirement {
    enum class Type { DATASET, TIME, USER, REPORT };
    Type type = Type::DATASET;
    std::string dsn;           // for dataset triggers
    std::string time;          // for time triggers
    std::string user_field;    // for user requirements
    int lead_time = 0;
    bool permanent = false;
};

struct CA7Network {
    std::string name;
    std::vector<std::string> job_sequence;
    std::string description;
};

struct CA7ARF {
    std::string condition;     // ABEND, Sxxx, Uxxx, CC GT n
    std::string action;        // RESTART, CANCEL, FORCE, HOLD
    int max_restarts = 1;
    std::string notify;
};

struct CA7XCC {
    std::string source_calendar;
    std::string target_calendar;
    std::string connection_type; // LINK, EXCL
    int offset_days = 0;
};

struct CA7Job {
    std::string name;
    std::string system;
    std::string jclid;
    std::string type;          // CPU, XPJOB, STC
    CA7Prose prose;
    std::vector<CA7Schedule> schedules;
    std::vector<CA7Trigger> triggers;
    std::vector<CA7Requirement> requirements;
    std::vector<CA7Network> networks;
    std::vector<CA7ARF> arf_rules;
    std::vector<CA7XCC> xcc_entries;
    std::map<std::string, std::string> extra_fields;
    bool valid = false;
};

// --- CA-7 Parser ---

class CA7Parser {
public:
    struct ParseResult {
        std::vector<CA7Job> jobs;
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

        CA7Job current_job;
        bool in_job = false;
        std::string section;

        for (size_t i = 0; i < lines.size(); ++i) {
            auto line = trim(lines[i]);
            if (line.empty()) continue;

            // Comments
            if (starts_with(line, "//*") || starts_with(line, "**")) {
                continue;
            }

            // DBJOB definition
            if (starts_with(line, "DBJOB")) {
                if (in_job && !current_job.name.empty()) {
                    current_job.valid = true;
                    result.jobs.push_back(current_job);
                }
                current_job = CA7Job{};
                in_job = true;
                section = "DBJOB";
                parse_dbjob_line(line, current_job);
                continue;
            }

            // Batch command format: /CMD verb operands
            if (starts_with(line, "/") && !starts_with(line, "//*")) {
                auto parsed = parse_batch_command(line);
                if (parsed) {
                    if (parsed->verb == "DBJOB" || parsed->verb == "DBJ") {
                        if (in_job && !current_job.name.empty()) {
                            current_job.valid = true;
                            result.jobs.push_back(current_job);
                        }
                        current_job = CA7Job{};
                        in_job = true;
                        section = "DBJOB";
                        for (const auto& [k, v] : parsed->operands) {
                            apply_dbjob_field(current_job, k, v);
                        }
                    } else if (in_job) {
                        apply_batch_command(current_job, *parsed, section);
                    }
                    continue;
                }
            }

            if (!in_job) continue;

            // Section detection
            if (starts_with(line, "PROSE")) {
                section = "PROSE";
                // Inline prose text after PROSE keyword
                if (line.size() > 6) {
                    auto text = trim(line.substr(5));
                    if (!text.empty()) current_job.prose.lines.push_back(std::string(text));
                }
                continue;
            }
            if (starts_with(line, "SCHD")) {
                section = "SCHD";
                parse_schd_line(line, current_job);
                continue;
            }
            if (starts_with(line, "TRIG")) {
                section = "TRIG";
                parse_trig_line(line, current_job);
                continue;
            }
            if (starts_with(line, "RQMT")) {
                section = "RQMT";
                parse_rqmt_line(line, current_job);
                continue;
            }
            if (starts_with(line, "NWK")) {
                section = "NWK";
                parse_nwk_line(line, current_job);
                continue;
            }
            if (starts_with(line, "#ARF")) {
                section = "ARF";
                parse_arf_line(line, current_job);
                continue;
            }
            if (starts_with(line, "XCC")) {
                section = "XCC";
                parse_xcc_line(line, current_job);
                continue;
            }
            if (starts_with(line, "ENDJOB") || starts_with(line, "ENDDB")) {
                if (in_job && !current_job.name.empty()) {
                    current_job.valid = true;
                    result.jobs.push_back(current_job);
                }
                current_job = CA7Job{};
                in_job = false;
                section.clear();
                continue;
            }

            // Continue in current section
            if (section == "PROSE") {
                current_job.prose.lines.push_back(std::string(line));
            } else if (section == "SCHD") {
                parse_schd_line(line, current_job);
            } else if (section == "TRIG") {
                parse_trig_line(line, current_job);
            } else if (section == "RQMT") {
                parse_rqmt_line(line, current_job);
            } else if (section == "NWK") {
                parse_nwk_line(line, current_job);
            } else if (section == "ARF") {
                parse_arf_line(line, current_job);
            } else if (section == "XCC") {
                parse_xcc_line(line, current_job);
            } else if (section == "DBJOB") {
                // Continuation of DBJOB fields
                parse_dbjob_line(line, current_job);
            }
        }

        // Flush last job
        if (in_job && !current_job.name.empty()) {
            current_job.valid = true;
            result.jobs.push_back(current_job);
        }

        return result;
    }

    // Convert parsed CA-7 jobs to a Schedule
    Schedule to_schedule(const ParseResult& pr) const {
        Schedule sched;
        sched.name = "CA7_Schedule";

        for (const auto& cj : pr.jobs) {
            ScheduledJob job;
            job.name = cj.name;
            job.description = cj.prose.full_text();
            job.jcl_member = cj.jclid;
            job.source_type = ScheduleSource::CA7;
            job.source_info = CA7Source{cj.system, cj.jclid, 0};

            // Apply first schedule
            if (!cj.schedules.empty()) {
                const auto& s = cj.schedules[0];
                job.calendar.name = s.calendar;
                job.time_window.earliest_start = s.start_time;
                job.time_window.deadline = s.deadline;

                if (s.cycle_type == "DAILY") {
                    job.calendar.cycle = RunCycle::DAILY;
                } else if (s.cycle_type == "WEEKLY") {
                    job.calendar.cycle = RunCycle::WEEKLY;
                    for (int d : s.run_days) {
                        if (d >= 1 && d <= 7) {
                            job.calendar.run_days.insert(static_cast<DayOfWeek>(d));
                        }
                    }
                } else if (s.cycle_type == "MONTHLY") {
                    job.calendar.cycle = RunCycle::MONTHLY;
                    job.calendar.monthly_day = s.monthly_day;
                }

                auto* ca7src = std::get_if<CA7Source>(&(*job.source_info));
                if (ca7src) ca7src->schedule_id = s.schedule_id;
            }

            // ARF -> error handling
            if (!cj.arf_rules.empty()) {
                std::string handling;
                for (const auto& arf : cj.arf_rules) {
                    if (!handling.empty()) handling += "; ";
                    handling += "ON " + arf.condition + " " + arf.action;
                    if (arf.max_restarts > 1) handling += " (max=" + std::to_string(arf.max_restarts) + ")";
                }
                job.error_handling = handling;
            }

            sched.add_job(job);

            // Triggers become dependencies
            for (const auto& trig : cj.triggers) {
                Dependency dep;
                dep.predecessor = trig.predecessor_job;
                dep.successor = cj.name;
                if (trig.condition == "SUCC") dep.condition = DependencyCondition::RC_ZERO;
                else if (trig.condition == "FAIL") dep.condition = DependencyCondition::ONLY_FAIL;
                else dep.condition = DependencyCondition::ALWAYS;
                sched.add_dependency(dep);
            }
        }

        return sched;
    }

private:
    struct BatchCommand {
        std::string verb;
        std::map<std::string, std::string> operands;
    };

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

    static std::optional<std::string> extract_kv(std::string_view line, std::string_view key) {
        size_t search_pos = 0;
        while (search_pos < line.size()) {
            auto pos = line.find(key, search_pos);
            if (pos == std::string_view::npos) return std::nullopt;

            // Check word boundary: key must be at start or preceded by non-alnum
            if (pos > 0 && std::isalnum(static_cast<unsigned char>(line[pos - 1]))) {
                search_pos = pos + 1;
                continue;
            }

            auto after = pos + key.size();
            if (after >= line.size()) return std::nullopt;

            if (line[after] == '=') {
                after++;
                if (after >= line.size()) return std::string("");
                if (line[after] == '\'') {
                    auto end = line.find('\'', after + 1);
                    if (end != std::string_view::npos) {
                        return std::string(line.substr(after + 1, end - after - 1));
                    }
                }
                auto end = after;
                while (end < line.size() && line[end] != ',' && line[end] != ' ' && line[end] != ')') {
                    end++;
                }
                return std::string(line.substr(after, end - after));
            }
            if (line[after] == '(') {
                auto end = line.find(')', after);
                if (end != std::string_view::npos) {
                    return std::string(line.substr(after + 1, end - after - 1));
                }
            }
            search_pos = pos + 1;
        }
        return std::nullopt;
    }

    static void parse_dbjob_line(std::string_view line, CA7Job& job) {
        auto name = extract_kv(line, "NAME");
        if (name) job.name = *name;

        auto sys = extract_kv(line, "SYSTEM");
        if (!sys) sys = extract_kv(line, "SYS");
        if (sys) job.system = *sys;

        auto jclid = extract_kv(line, "JCLID");
        if (jclid) job.jclid = *jclid;

        auto type = extract_kv(line, "TYPE");
        if (type) job.type = *type;

        // Collect any extra KEY=VALUE pairs
        auto owner = extract_kv(line, "OWNER");
        if (owner) job.extra_fields["OWNER"] = *owner;

        auto acct = extract_kv(line, "ACCT");
        if (acct) job.extra_fields["ACCT"] = *acct;

        auto msgid = extract_kv(line, "MSGID");
        if (msgid) job.extra_fields["MSGID"] = *msgid;
    }

    static void apply_dbjob_field(CA7Job& job, const std::string& key, const std::string& value) {
        if (key == "NAME") job.name = value;
        else if (key == "SYSTEM" || key == "SYS") job.system = value;
        else if (key == "JCLID") job.jclid = value;
        else if (key == "TYPE") job.type = value;
        else job.extra_fields[key] = value;
    }

    static void parse_schd_line(std::string_view line, CA7Job& job) {
        // May start with SCHD keyword — skip it
        if (starts_with(line, "SCHD")) {
            line = trim(line.substr(4));
        }
        if (line.empty()) return;

        CA7Schedule sched;

        auto scal = extract_kv(line, "SCAL");
        if (scal) sched.calendar = *scal;

        auto id = extract_kv(line, "ID");
        if (id) sched.schedule_id = std::stoi(*id);

        auto dotrig = extract_kv(line, "DOTRIG");
        if (dotrig) sched.dotrig = (*dotrig == "Y" || *dotrig == "YES");

        auto sttm = extract_kv(line, "STTM");
        if (sttm) sched.start_time = *sttm;

        auto dlnt = extract_kv(line, "DLNT");
        if (dlnt) sched.deadline = *dlnt;

        auto ldtm = extract_kv(line, "LDTM");
        if (ldtm) sched.lead_time = std::stoi(*ldtm);

        auto cycle = extract_kv(line, "CYCLE");
        if (cycle) {
            std::string c = *cycle;
            std::transform(c.begin(), c.end(), c.begin(),
                           [](unsigned char ch){ return static_cast<char>(std::toupper(ch)); });
            sched.cycle_type = c;
        }

        auto days = extract_kv(line, "DAYS");
        if (days) {
            std::istringstream iss(*days);
            std::string token;
            while (std::getline(iss, token, ',')) {
                auto t = trim(std::string_view(token));
                if (!t.empty()) sched.run_days.push_back(std::stoi(std::string(t)));
            }
        }

        auto mday = extract_kv(line, "MDAY");
        if (mday) sched.monthly_day = std::stoi(*mday);

        job.schedules.push_back(sched);
    }

    static void parse_trig_line(std::string_view line, CA7Job& job) {
        if (starts_with(line, "TRIG")) {
            line = trim(line.substr(4));
        }
        if (line.empty()) return;

        CA7Trigger trig;

        auto pred_job = extract_kv(line, "JOB");
        if (!pred_job) pred_job = extract_kv(line, "PRED");
        if (!pred_job) {
            // Bare jobname on the line
            auto sp = line.find(' ');
            if (sp != std::string_view::npos) {
                pred_job = std::string(trim(line.substr(0, sp)));
            } else {
                pred_job = std::string(line);
            }
        }
        if (pred_job) trig.predecessor_job = *pred_job;

        auto cond = extract_kv(line, "COND");
        if (!cond) cond = extract_kv(line, "TYPE");
        if (cond) trig.condition = *cond;
        else trig.condition = "SUCC"; // default

        auto schid = extract_kv(line, "SCHID");
        if (schid) trig.schid = std::stoi(*schid);

        auto rlate = extract_kv(line, "RLATE");
        if (rlate) trig.resolve_late = (*rlate == "Y");

        job.triggers.push_back(trig);
    }

    static void parse_rqmt_line(std::string_view line, CA7Job& job) {
        if (starts_with(line, "RQMT")) {
            line = trim(line.substr(4));
        }
        if (line.empty()) return;

        CA7Requirement rqmt;

        auto dsn = extract_kv(line, "DSN");
        auto type = extract_kv(line, "TYPE");

        if (dsn) {
            rqmt.type = CA7Requirement::Type::DATASET;
            rqmt.dsn = *dsn;
        } else if (type) {
            std::string t = *type;
            std::transform(t.begin(), t.end(), t.begin(),
                           [](unsigned char c){ return static_cast<char>(std::toupper(c)); });
            if (t == "TIME") {
                rqmt.type = CA7Requirement::Type::TIME;
                auto tm = extract_kv(line, "TIME");
                if (tm) rqmt.time = *tm;
            } else if (t == "USER") {
                rqmt.type = CA7Requirement::Type::USER;
                auto uf = extract_kv(line, "USER");
                if (uf) rqmt.user_field = *uf;
            } else if (t == "REPORT") {
                rqmt.type = CA7Requirement::Type::REPORT;
                auto rf = extract_kv(line, "REPORT");
                if (rf) rqmt.dsn = *rf;
            } else if (t == "DSN" || t == "DATASET") {
                rqmt.type = CA7Requirement::Type::DATASET;
                auto d = extract_kv(line, "NAME");
                if (d) rqmt.dsn = *d;
            }
        }

        auto ldtm = extract_kv(line, "LDTM");
        if (ldtm) rqmt.lead_time = std::stoi(*ldtm);

        auto perm = extract_kv(line, "PERM");
        if (perm) rqmt.permanent = (*perm == "Y" || *perm == "YES");

        job.requirements.push_back(rqmt);
    }

    static void parse_nwk_line(std::string_view line, CA7Job& job) {
        if (starts_with(line, "NWK")) {
            line = trim(line.substr(3));
        }
        if (line.empty()) return;

        CA7Network nwk;

        auto name = extract_kv(line, "NAME");
        if (name) nwk.name = *name;

        auto jobs = extract_kv(line, "JOBS");
        if (jobs) {
            std::istringstream iss(*jobs);
            std::string token;
            while (std::getline(iss, token, ',')) {
                auto t = trim(std::string_view(token));
                if (!t.empty()) nwk.job_sequence.push_back(std::string(t));
            }
        }

        auto desc = extract_kv(line, "DESC");
        if (desc) nwk.description = *desc;

        job.networks.push_back(nwk);
    }

    static void parse_arf_line(std::string_view line, CA7Job& job) {
        if (starts_with(line, "#ARF")) {
            line = trim(line.substr(4));
        }
        if (line.empty()) return;

        CA7ARF arf;

        auto cond = extract_kv(line, "COND");
        if (!cond) cond = extract_kv(line, "ON");
        if (cond) arf.condition = *cond;

        auto action = extract_kv(line, "ACTION");
        if (!action) action = extract_kv(line, "DO");
        if (action) arf.action = *action;

        auto maxr = extract_kv(line, "MAXRC");
        if (!maxr) maxr = extract_kv(line, "MAX");
        if (maxr) arf.max_restarts = std::stoi(*maxr);

        auto notify = extract_kv(line, "NOTIFY");
        if (notify) arf.notify = *notify;

        job.arf_rules.push_back(arf);
    }

    static void parse_xcc_line(std::string_view line, CA7Job& job) {
        if (starts_with(line, "XCC")) {
            line = trim(line.substr(3));
        }
        if (line.empty()) return;

        CA7XCC xcc;

        auto src = extract_kv(line, "SRC");
        if (!src) src = extract_kv(line, "SOURCE");
        if (src) xcc.source_calendar = *src;

        auto tgt = extract_kv(line, "TGT");
        if (!tgt) tgt = extract_kv(line, "TARGET");
        if (tgt) xcc.target_calendar = *tgt;

        auto ctype = extract_kv(line, "TYPE");
        if (ctype) xcc.connection_type = *ctype;

        auto offset = extract_kv(line, "OFFSET");
        if (offset) xcc.offset_days = std::stoi(*offset);

        job.xcc_entries.push_back(xcc);
    }

    static std::optional<BatchCommand> parse_batch_command(std::string_view line) {
        // Format: /CMD VERB,OPERAND1=val,OPERAND2=val
        // Or:     /VERB OPERAND1=val,OPERAND2=val
        if (line.empty() || line[0] != '/') return std::nullopt;

        BatchCommand cmd;
        line.remove_prefix(1); // skip /
        line = trim(line);

        // Extract verb (first token)
        auto sp = line.find(' ');
        std::string_view verb;
        std::string_view rest;
        if (sp != std::string_view::npos) {
            verb = line.substr(0, sp);
            rest = trim(line.substr(sp + 1));
        } else {
            auto comma = line.find(',');
            if (comma != std::string_view::npos) {
                verb = line.substr(0, comma);
                rest = line.substr(comma + 1);
            } else {
                verb = line;
            }
        }

        // Handle /CMD VERB format
        if (verb == "CMD") {
            sp = rest.find(' ');
            if (sp != std::string_view::npos) {
                verb = rest.substr(0, sp);
                rest = trim(rest.substr(sp + 1));
            } else {
                auto comma = rest.find(',');
                if (comma != std::string_view::npos) {
                    verb = rest.substr(0, comma);
                    rest = rest.substr(comma + 1);
                } else {
                    verb = rest;
                    rest = std::string_view{};
                }
            }
        }

        cmd.verb = std::string(verb);
        std::transform(cmd.verb.begin(), cmd.verb.end(), cmd.verb.begin(),
                       [](unsigned char c){ return static_cast<char>(std::toupper(c)); });

        // Parse KEY=VALUE pairs separated by commas or spaces
        while (!rest.empty()) {
            auto eq = rest.find('=');
            if (eq == std::string_view::npos) break;

            std::string key(trim(rest.substr(0, eq)));
            std::transform(key.begin(), key.end(), key.begin(),
                           [](unsigned char c){ return static_cast<char>(std::toupper(c)); });

            rest = rest.substr(eq + 1);
            std::string value;

            if (!rest.empty() && rest[0] == '\'') {
                auto end_quote = rest.find('\'', 1);
                if (end_quote != std::string_view::npos) {
                    value = std::string(rest.substr(1, end_quote - 1));
                    rest = rest.substr(end_quote + 1);
                }
            } else {
                auto delim = rest.find_first_of(", ");
                if (delim != std::string_view::npos) {
                    value = std::string(rest.substr(0, delim));
                    rest = rest.substr(delim + 1);
                } else {
                    value = std::string(rest);
                    rest = std::string_view{};
                }
            }

            rest = trim(rest);
            // skip leading comma
            if (!rest.empty() && rest[0] == ',') {
                rest.remove_prefix(1);
                rest = trim(rest);
            }

            cmd.operands[key] = value;
        }

        return cmd;
    }

    static void apply_batch_command(CA7Job& job, const BatchCommand& cmd,
                                    std::string& section) {
        if (cmd.verb == "PROSE") {
            section = "PROSE";
            for (const auto& [k, v] : cmd.operands) {
                job.prose.lines.push_back(k + "=" + v);
            }
        } else if (cmd.verb == "SCHD") {
            section = "SCHD";
            std::string rebuilt;
            for (const auto& [k, v] : cmd.operands) {
                if (!rebuilt.empty()) rebuilt += ",";
                rebuilt += k + "=" + v;
            }
            parse_schd_line(std::string_view(rebuilt), job);
        } else if (cmd.verb == "TRIG") {
            section = "TRIG";
            std::string rebuilt;
            for (const auto& [k, v] : cmd.operands) {
                if (!rebuilt.empty()) rebuilt += ",";
                rebuilt += k + "=" + v;
            }
            parse_trig_line(std::string_view(rebuilt), job);
        } else if (cmd.verb == "ARF" || cmd.verb == "#ARF") {
            section = "ARF";
            std::string rebuilt;
            for (const auto& [k, v] : cmd.operands) {
                if (!rebuilt.empty()) rebuilt += ",";
                rebuilt += k + "=" + v;
            }
            parse_arf_line(std::string_view(rebuilt), job);
        }
    }
};

}} // namespace lazarus::scheduler

#endif // LAZARUS_SCHEDULER_CA7_PARSER_H
