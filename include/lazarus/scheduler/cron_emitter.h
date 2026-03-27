#ifndef LAZARUS_SCHEDULER_CRON_EMITTER_H
#define LAZARUS_SCHEDULER_CRON_EMITTER_H

#include "lazarus/scheduler/schedule_model.h"
#include <string>
#include <vector>
#include <set>
#include <sstream>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <cctype>

namespace lazarus { namespace scheduler {

// --- Cron Expression ---

struct CronExpression {
    std::string minute = "*";
    std::string hour = "*";
    std::string day_of_month = "*";
    std::string month = "*";
    std::string day_of_week = "*";
    std::string comment;

    std::string to_string() const {
        return minute + " " + hour + " " + day_of_month + " " + month + " " + day_of_week;
    }

    std::string to_crontab_line(const std::string& command) const {
        std::string line;
        if (!comment.empty()) {
            line += "# " + comment + "\n";
        }
        line += to_string() + " " + command;
        return line;
    }

    bool is_valid() const {
        return validate_field(minute, 0, 59) &&
               validate_field(hour, 0, 23) &&
               validate_field(day_of_month, 1, 31) &&
               validate_field(month, 1, 12) &&
               validate_field(day_of_week, 0, 7);
    }

private:
    static bool validate_field(const std::string& field, int min_val, int max_val) {
        if (field.empty()) return false;
        if (field == "*") return true;

        // Handle */N (step)
        if (field.size() > 2 && field[0] == '*' && field[1] == '/') {
            auto step_str = field.substr(2);
            for (char c : step_str) {
                if (!std::isdigit(static_cast<unsigned char>(c))) return false;
            }
            int step = std::stoi(step_str);
            return step > 0 && step <= max_val;
        }

        // Handle comma-separated values and ranges
        std::istringstream iss(field);
        std::string token;
        while (std::getline(iss, token, ',')) {
            auto dash = token.find('-');
            if (dash != std::string::npos) {
                auto lo_str = token.substr(0, dash);
                auto hi_str = token.substr(dash + 1);
                // Check step in range
                auto slash = hi_str.find('/');
                if (slash != std::string::npos) {
                    hi_str = hi_str.substr(0, slash);
                }
                for (char c : lo_str) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
                }
                for (char c : hi_str) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
                }
                int lo = std::stoi(lo_str);
                int hi = std::stoi(hi_str);
                if (lo < min_val || hi > max_val || lo > hi) return false;
            } else {
                for (char c : token) {
                    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
                }
                int val = std::stoi(token);
                if (val < min_val || val > max_val) return false;
            }
        }
        return true;
    }
};

// --- Cron Emitter ---

class CronEmitter {
public:
    // Build cron expression from a ScheduledJob
    CronExpression build(const ScheduledJob& job) const {
        CronExpression cron;
        cron.comment = job.name;

        // Extract time from earliest_start or use midnight
        int hour = 0, minute = 0;
        if (!job.time_window.earliest_start.empty()) {
            parse_time(job.time_window.earliest_start, hour, minute);
        }

        cron.minute = std::to_string(minute);
        cron.hour = std::to_string(hour);

        switch (job.calendar.cycle) {
            case RunCycle::DAILY:
                // "MM HH * * *"
                cron.day_of_month = "*";
                cron.month = "*";
                cron.day_of_week = "*";
                break;

            case RunCycle::WEEKLY:
                cron.day_of_month = "*";
                cron.month = "*";
                if (job.calendar.run_days.empty()) {
                    cron.day_of_week = "1"; // default Monday
                } else {
                    cron.day_of_week = build_dow_field(job.calendar.run_days);
                }
                break;

            case RunCycle::MONTHLY:
                if (job.calendar.monthly_day > 0) {
                    cron.day_of_month = std::to_string(job.calendar.monthly_day);
                } else {
                    cron.day_of_month = "1";
                }
                cron.month = "*";
                cron.day_of_week = "*";
                break;

            case RunCycle::YEARLY:
                if (job.calendar.yearly_day > 0) {
                    cron.day_of_month = std::to_string(job.calendar.yearly_day);
                } else {
                    cron.day_of_month = "1";
                }
                if (job.calendar.yearly_month > 0) {
                    cron.month = std::to_string(job.calendar.yearly_month);
                } else {
                    cron.month = "1";
                }
                cron.day_of_week = "*";
                break;

            case RunCycle::BUSINESS_DAYS:
                cron.day_of_month = "*";
                cron.month = "*";
                cron.day_of_week = "1-5";
                break;

            case RunCycle::CUSTOM:
                if (!job.calendar.custom_expression.empty()) {
                    return parse_cron_string(job.calendar.custom_expression, job.name);
                }
                break;
        }

        return cron;
    }

    // Build multiple cron entries for a schedule
    std::vector<CronExpression> build_all(const Schedule& schedule) const {
        std::vector<CronExpression> result;
        for (const auto& [name, job] : schedule.jobs()) {
            (void)name;
            if (job.enabled) {
                result.push_back(build(job));
            }
        }
        return result;
    }

    // Generate complete crontab content
    std::string emit_crontab(const Schedule& schedule,
                             const std::string& command_template = "run_job.sh {JOB}") const {
        std::ostringstream out;
        out << "# Crontab generated from schedule: " << schedule.name << "\n";
        out << "# " << schedule.description << "\n";
        out << "SHELL=/bin/bash\n";
        out << "PATH=/usr/local/bin:/usr/bin:/bin\n";
        out << "\n";

        auto entries = build_all(schedule);
        auto order = schedule.topological_order();

        // Emit in topological order if possible, else alphabetical
        std::vector<std::string> job_names;
        if (!order.empty()) {
            job_names = order;
        } else {
            for (const auto& [name, _] : schedule.jobs()) {
                (void)_;
                job_names.push_back(name);
            }
        }

        for (const auto& name : job_names) {
            auto job = schedule.get_job(name);
            if (!job || !job->enabled) continue;

            auto cron = build(*job);
            std::string cmd = command_template;
            auto pos = cmd.find("{JOB}");
            if (pos != std::string::npos) {
                cmd.replace(pos, 5, name);
            }

            // Holiday check wrapper
            if (!job->calendar.holidays.empty()) {
                out << "# " << name << " (with holiday exclusion)\n";
                std::string holiday_list;
                for (const auto& h : job->calendar.holidays) {
                    if (!holiday_list.empty()) holiday_list += " ";
                    holiday_list += h;
                }
                out << cron.to_string() << " "
                    << "test \"$(date +\\%Y-\\%m-\\%d)\" != \""
                    << job->calendar.holidays[0] << "\" && " << cmd << "\n";
            } else {
                out << cron.to_crontab_line(cmd) << "\n";
            }
        }

        return out.str();
    }

    // Human-readable schedule description
    static std::string describe(const CronExpression& cron) {
        std::ostringstream out;

        // Day of week description
        if (cron.day_of_week == "*" && cron.day_of_month == "*") {
            out << "Every day";
        } else if (cron.day_of_week == "1-5") {
            out << "Every business day (Mon-Fri)";
        } else if (cron.day_of_week != "*") {
            out << "On " << describe_dow(cron.day_of_week);
        } else if (cron.day_of_month != "*") {
            out << "On day " << cron.day_of_month;
            if (cron.month != "*") {
                out << " of month " << cron.month;
            } else {
                out << " of every month";
            }
        }

        // Time description
        if (cron.hour != "*" && cron.minute != "*") {
            out << " at " << pad2(cron.hour) << ":" << pad2(cron.minute);
        } else if (cron.hour != "*") {
            out << " at hour " << cron.hour;
        }

        return out.str();
    }

    // Validate a cron expression string
    static bool validate_cron_string(const std::string& expr) {
        auto cron = parse_cron_string(expr, "");
        return cron.is_valid();
    }

    // Parse "MM HH DOM MON DOW" string into CronExpression
    static CronExpression parse_cron_string(const std::string& expr,
                                            const std::string& comment = "") {
        CronExpression cron;
        cron.comment = comment;

        std::istringstream iss(expr);
        std::string fields[5];
        for (int i = 0; i < 5 && iss >> fields[i]; ++i) {}

        if (!fields[0].empty()) cron.minute = fields[0];
        if (!fields[1].empty()) cron.hour = fields[1];
        if (!fields[2].empty()) cron.day_of_month = fields[2];
        if (!fields[3].empty()) cron.month = fields[3];
        if (!fields[4].empty()) cron.day_of_week = fields[4];

        return cron;
    }

private:
    static void parse_time(const std::string& time_str, int& hour, int& minute) {
        auto colon = time_str.find(':');
        if (colon != std::string::npos) {
            hour = std::stoi(time_str.substr(0, colon));
            minute = std::stoi(time_str.substr(colon + 1));
        } else if (time_str.size() == 4) {
            hour = std::stoi(time_str.substr(0, 2));
            minute = std::stoi(time_str.substr(2, 2));
        } else if (time_str.size() <= 2) {
            hour = std::stoi(time_str);
            minute = 0;
        }
    }

    static std::string build_dow_field(const std::set<DayOfWeek>& days) {
        if (days.empty()) return "*";

        std::vector<int> day_nums;
        for (auto d : days) {
            // Convert to cron day-of-week (0=Sunday, 1=Monday, ... 6=Saturday)
            int n = static_cast<int>(d);  // 1=Monday...7=Sunday
            int cron_day = (n == 7) ? 0 : n; // Sunday=0
            day_nums.push_back(cron_day);
        }
        std::sort(day_nums.begin(), day_nums.end());

        // Check for consecutive range
        bool is_range = (day_nums.size() > 1);
        for (size_t i = 1; i < day_nums.size() && is_range; ++i) {
            if (day_nums[i] != day_nums[i-1] + 1) is_range = false;
        }

        if (is_range && day_nums.size() > 2) {
            return std::to_string(day_nums.front()) + "-" + std::to_string(day_nums.back());
        }

        std::string result;
        for (size_t i = 0; i < day_nums.size(); ++i) {
            if (i > 0) result += ",";
            result += std::to_string(day_nums[i]);
        }
        return result;
    }

    static std::string describe_dow(const std::string& dow_field) {
        static const std::array<const char*, 7> day_names = {
            "Sunday", "Monday", "Tuesday", "Wednesday",
            "Thursday", "Friday", "Saturday"
        };

        // Handle range
        auto dash = dow_field.find('-');
        if (dash != std::string::npos) {
            int lo = std::stoi(dow_field.substr(0, dash));
            int hi = std::stoi(dow_field.substr(dash + 1));
            if (lo >= 0 && lo <= 6 && hi >= 0 && hi <= 6) {
                return std::string(day_names[static_cast<size_t>(lo)]) + " through " +
                       day_names[static_cast<size_t>(hi)];
            }
        }

        // Handle comma-separated
        std::string result;
        std::istringstream iss(dow_field);
        std::string token;
        while (std::getline(iss, token, ',')) {
            int d = std::stoi(token);
            if (d >= 0 && d <= 6) {
                if (!result.empty()) result += ", ";
                result += day_names[static_cast<size_t>(d)];
            }
        }
        return result;
    }

    static std::string pad2(const std::string& s) {
        if (s.size() == 1) return "0" + s;
        return s;
    }
};

}} // namespace lazarus::scheduler

#endif // LAZARUS_SCHEDULER_CRON_EMITTER_H
