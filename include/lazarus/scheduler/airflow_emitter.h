#ifndef LAZARUS_SCHEDULER_AIRFLOW_EMITTER_H
#define LAZARUS_SCHEDULER_AIRFLOW_EMITTER_H

#include "lazarus/scheduler/schedule_model.h"
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <ctime>

namespace lazarus { namespace scheduler {

// --- Airflow DAG Emitter ---

struct AirflowEmitterConfig {
    std::string dag_id;
    std::string owner = "lazarus";
    int retries = 1;
    int retry_delay_minutes = 5;
    std::string start_date = "2024-01-01";
    std::string schedule_interval;           // cron or @daily etc.
    bool catchup = false;
    std::string default_queue = "default";
    int concurrency = 16;
    std::string tags;
    std::map<std::string, int> pools;        // resource -> pool slots
    std::string on_failure_callback_fn;
    bool emit_external_sensors = true;
};

class AirflowEmitter {
public:
    explicit AirflowEmitter(const AirflowEmitterConfig& config = {})
        : config_(config) {}

    // Emit a complete Airflow DAG Python file from a schedule
    std::string emit(const Schedule& schedule) const {
        std::ostringstream out;

        emit_imports(out, schedule);
        out << "\n";
        emit_default_args(out);
        out << "\n";
        emit_dag_definition(out, schedule);
        out << "\n";
        emit_tasks(out, schedule);
        out << "\n";
        emit_dependencies(out, schedule);
        out << "\n";

        // Pools (as comment block for airflow CLI setup)
        if (!config_.pools.empty()) {
            emit_pool_comments(out);
        }

        return out.str();
    }

    // Emit just the task definition for a single job
    std::string emit_task(const ScheduledJob& job) const {
        std::ostringstream out;
        emit_single_task(out, job, "    ");
        return out.str();
    }

    // Map dependency condition to Airflow trigger_rule
    static std::string map_trigger_rule(DependencyCondition cond) {
        switch (cond) {
            case DependencyCondition::RC_ZERO:   return "all_success";
            case DependencyCondition::RC_LT4:    return "all_success";
            case DependencyCondition::RC_LT8:    return "all_done";
            case DependencyCondition::ALWAYS:     return "all_done";
            case DependencyCondition::ONLY_FAIL:  return "all_failed";
            case DependencyCondition::CUSTOM:     return "one_success";
        }
        return "all_success";
    }

    // Map calendar to Airflow timetable expression
    static std::string map_timetable(const Calendar& cal) {
        switch (cal.cycle) {
            case RunCycle::DAILY:
                return "@daily";
            case RunCycle::WEEKLY: {
                // Build cron for specific days
                if (cal.run_days.empty()) return "@weekly";
                std::string days;
                for (auto d : cal.run_days) {
                    if (!days.empty()) days += ",";
                    days += std::to_string((static_cast<int>(d) % 7));
                }
                return "0 0 * * " + days;
            }
            case RunCycle::MONTHLY:
                if (cal.monthly_day > 0) {
                    return "0 0 " + std::to_string(cal.monthly_day) + " * *";
                }
                return "@monthly";
            case RunCycle::YEARLY:
                if (cal.yearly_month > 0 && cal.yearly_day > 0) {
                    return "0 0 " + std::to_string(cal.yearly_day) + " " +
                           std::to_string(cal.yearly_month) + " *";
                }
                return "@yearly";
            case RunCycle::BUSINESS_DAYS:
                return "0 0 * * 1-5";
            case RunCycle::CUSTOM:
                return cal.custom_expression.empty() ? "@daily" : cal.custom_expression;
        }
        return "@daily";
    }

    const AirflowEmitterConfig& config() const { return config_; }

private:
    AirflowEmitterConfig config_;

    void emit_imports(std::ostringstream& out, const Schedule& schedule) const {
        out << "from airflow import DAG\n";
        out << "from airflow.operators.bash import BashOperator\n";
        out << "from airflow.operators.python import PythonOperator\n";
        out << "from datetime import datetime, timedelta\n";

        // Check if we need ExternalTaskSensor
        bool needs_sensor = false;
        if (config_.emit_external_sensors) {
            for (const auto& dep : schedule.dependencies()) {
                auto pred = schedule.get_job(dep.predecessor);
                auto succ = schedule.get_job(dep.successor);
                if (pred && succ && pred->group != succ->group &&
                    !pred->group.empty() && !succ->group.empty()) {
                    needs_sensor = true;
                    break;
                }
            }
        }
        if (needs_sensor) {
            out << "from airflow.sensors.external_task import ExternalTaskSensor\n";
        }
    }

    void emit_default_args(std::ostringstream& out) const {
        out << "default_args = {\n";
        out << "    'owner': '" << escape_python(config_.owner) << "',\n";
        out << "    'depends_on_past': False,\n";
        out << "    'retries': " << config_.retries << ",\n";
        out << "    'retry_delay': timedelta(minutes=" << config_.retry_delay_minutes << "),\n";
        if (!config_.on_failure_callback_fn.empty()) {
            out << "    'on_failure_callback': " << config_.on_failure_callback_fn << ",\n";
        }
        out << "}\n";
    }

    void emit_dag_definition(std::ostringstream& out, const Schedule& schedule) const {
        std::string dag_id = config_.dag_id.empty() ? sanitize_id(schedule.name) : config_.dag_id;
        std::string timetable = config_.schedule_interval;

        // Auto-detect timetable from first job if not specified
        if (timetable.empty()) {
            for (const auto& [name, job] : schedule.jobs()) {
                (void)name;
                timetable = map_timetable(job.calendar);
                if (timetable != "@daily") break;
            }
            if (timetable.empty()) timetable = "@daily";
        }

        out << "with DAG(\n";
        out << "    dag_id='" << escape_python(dag_id) << "',\n";
        out << "    default_args=default_args,\n";
        out << "    description='" << escape_python(schedule.description) << "',\n";
        out << "    schedule_interval='" << escape_python(timetable) << "',\n";
        out << "    start_date=datetime(" << format_date(config_.start_date) << "),\n";
        out << "    catchup=" << (config_.catchup ? "True" : "False") << ",\n";
        out << "    concurrency=" << config_.concurrency << ",\n";
        if (!config_.tags.empty()) {
            out << "    tags=['" << escape_python(config_.tags) << "'],\n";
        }
        out << ") as dag:\n";
    }

    void emit_tasks(std::ostringstream& out, const Schedule& schedule) const {
        auto order = schedule.topological_order();
        if (order.empty()) {
            // Fallback: emit in alphabetical order
            for (const auto& [name, job] : schedule.jobs()) {
                (void)name;
                emit_single_task(out, job, "    ");
                out << "\n";
            }
        } else {
            for (const auto& name : order) {
                auto job = schedule.get_job(name);
                if (job) {
                    emit_single_task(out, *job, "    ");
                    out << "\n";
                }
            }
        }
    }

    void emit_single_task(std::ostringstream& out, const ScheduledJob& job,
                          const std::string& indent) const {
        std::string task_id = sanitize_id(job.name);

        // Determine trigger rule from incoming dependencies
        std::string trigger_rule;
        // This would need schedule context; for standalone, use default
        // The dependencies section handles trigger_rule if non-default

        out << indent << task_id << " = BashOperator(\n";
        out << indent << "    task_id='" << escape_python(task_id) << "',\n";

        // Build bash command
        std::string bash_cmd = "echo 'Running " + escape_python(job.name) + "'";
        if (!job.jcl_member.empty()) {
            bash_cmd = "bash -c 'echo Running " + escape_python(job.jcl_member) + "'";
        }
        out << indent << "    bash_command=\"" << escape_python_double(bash_cmd) << "\",\n";

        // Execution timeout from time window
        if (job.time_window.duration_minutes > 0) {
            out << indent << "    execution_timeout=timedelta(minutes="
                << job.time_window.duration_minutes << "),\n";
        }

        // SLA from deadline
        if (!job.time_window.deadline.empty()) {
            int deadline_mins = parse_time_minutes(job.time_window.deadline);
            if (deadline_mins > 0) {
                out << indent << "    sla=timedelta(minutes=" << deadline_mins << "),\n";
            }
        }

        // Pool from first resource
        if (!job.resources.empty()) {
            out << indent << "    pool='" << escape_python(job.resources[0].name) << "',\n";
        }

        // Priority weight
        out << indent << "    priority_weight=" << job.priority_value() << ",\n";

        out << indent << ")\n";
    }

    void emit_dependencies(std::ostringstream& out, const Schedule& schedule) const {
        out << "    # Dependencies\n";

        // Group deps by successor to set trigger_rule
        std::map<std::string, std::vector<std::pair<std::string, DependencyCondition>>> succ_map;
        for (const auto& dep : schedule.dependencies()) {
            if (schedule.get_job(dep.predecessor) && schedule.get_job(dep.successor)) {
                succ_map[dep.successor].push_back({dep.predecessor, dep.condition});
            }
        }

        // Emit trigger rules for non-default conditions
        for (const auto& [succ, preds] : succ_map) {
            // Check if any predecessor has non-default trigger rule
            bool has_non_default = false;
            std::string rule = "all_success";
            for (const auto& [pred_name, cond] : preds) {
                (void)pred_name;
                std::string r = map_trigger_rule(cond);
                if (r != "all_success") {
                    has_non_default = true;
                    rule = r;
                }
            }
            if (has_non_default) {
                out << "    " << sanitize_id(succ) << ".trigger_rule = '"
                    << rule << "'\n";
            }
        }

        // Emit set_upstream calls
        for (const auto& dep : schedule.dependencies()) {
            if (!schedule.get_job(dep.predecessor) || !schedule.get_job(dep.successor)) continue;

            std::string pred_id = sanitize_id(dep.predecessor);
            std::string succ_id = sanitize_id(dep.successor);

            // Check for cross-DAG dependency
            auto pred = schedule.get_job(dep.predecessor);
            auto succ = schedule.get_job(dep.successor);
            if (pred && succ && !pred->group.empty() && !succ->group.empty() &&
                pred->group != succ->group && config_.emit_external_sensors) {
                out << "    # Cross-DAG: " << dep.predecessor << " -> " << dep.successor << "\n";
                out << "    sensor_" << succ_id << " = ExternalTaskSensor(\n";
                out << "        task_id='wait_" << escape_python(pred_id) << "',\n";
                out << "        external_dag_id='" << escape_python(sanitize_id(pred->group)) << "',\n";
                out << "        external_task_id='" << escape_python(pred_id) << "',\n";
                out << "        timeout=600,\n";
                out << "    )\n";
                out << "    sensor_" << succ_id << " >> " << succ_id << "\n";
            } else {
                out << "    " << pred_id << " >> " << succ_id << "\n";
            }
        }
    }

    void emit_pool_comments(std::ostringstream& out) const {
        out << "\n# Pool setup (run via airflow CLI):\n";
        for (const auto& [name, slots] : config_.pools) {
            out << "# airflow pools set '" << name << "' " << slots
                << " 'Auto-generated from mainframe schedule'\n";
        }
    }

    // --- Utility ---

    static std::string sanitize_id(const std::string& name) {
        std::string result;
        result.reserve(name.size());
        for (char c : name) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
                result += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            } else {
                result += '_';
            }
        }
        // Ensure starts with letter
        if (!result.empty() && std::isdigit(static_cast<unsigned char>(result[0]))) {
            result = "t_" + result;
        }
        return result;
    }

    static std::string escape_python(const std::string& s) {
        std::string result;
        result.reserve(s.size() + 4);
        for (char c : s) {
            switch (c) {
                case '\'': result += "\\'"; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:   result += c; break;
            }
        }
        return result;
    }

    static std::string escape_python_double(const std::string& s) {
        std::string result;
        result.reserve(s.size() + 4);
        for (char c : s) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:   result += c; break;
            }
        }
        return result;
    }

    static std::string format_date(const std::string& date) {
        // "2024-01-01" -> "2024, 1, 1"
        if (date.size() < 10) return "2024, 1, 1";
        int y = std::stoi(date.substr(0, 4));
        int m = std::stoi(date.substr(5, 2));
        int d = std::stoi(date.substr(8, 2));
        return std::to_string(y) + ", " + std::to_string(m) + ", " + std::to_string(d);
    }

    static int parse_time_minutes(const std::string& time_str) {
        // "HH:MM" -> total minutes
        if (time_str.size() < 4) return 0;
        auto colon = time_str.find(':');
        if (colon != std::string::npos) {
            int h = std::stoi(time_str.substr(0, colon));
            int m = std::stoi(time_str.substr(colon + 1));
            return h * 60 + m;
        }
        // HHMM format
        if (time_str.size() == 4) {
            int h = std::stoi(time_str.substr(0, 2));
            int m = std::stoi(time_str.substr(2, 2));
            return h * 60 + m;
        }
        return 0;
    }
};

}} // namespace lazarus::scheduler

#endif // LAZARUS_SCHEDULER_AIRFLOW_EMITTER_H
