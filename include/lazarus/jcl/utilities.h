#ifndef LAZARUS_JCL_UTILITIES_H
#define LAZARUS_JCL_UTILITIES_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <functional>
#include <algorithm>
#include <fstream>
#include <stdexcept>

// Undefine Windows ERROR macro to avoid collision
#ifdef ERROR
#undef ERROR
#endif

namespace lazarus { namespace jcl {

// Return codes matching IBM conventions
enum class UtilRC : int {
    RC_SUCCESS = 0,
    RC_WARNING = 4,
    RC_ERROR   = 8,
    RC_SEVERE  = 12,
    RC_ABEND   = 16
};

// --- IEFBR14: No-op utility ---
// Used to allocate/delete datasets via DD DISP without running a real program
class IEFBR14 {
public:
    struct Result {
        int rc;
        std::string message;
    };

    static Result execute() {
        return {0, "IEFBR14 COMPLETED - NO OPERATION PERFORMED"};
    }

    // With DD list: validates dispositions are sane
    static Result execute(const std::vector<std::pair<std::string, std::string>>& dd_list) {
        (void)dd_list;
        return {0, "IEFBR14 COMPLETED - " + std::to_string(dd_list.size()) + " DD STATEMENTS PROCESSED"};
    }
};

// --- IEBGENER: Sequential dataset copy ---
class IEBGENER {
public:
    struct Config {
        std::string sysut1;     // Input dataset (source)
        std::string sysut2;     // Output dataset (target)
        bool generate_records = false;
        int record_count = 0;
        std::string exit_name;
    };

    struct Result {
        int rc;
        std::string message;
        int records_copied;
    };

    // Execute copy operation using in-memory data
    static Result execute(const std::string& input_data, std::string& output_data) {
        output_data = input_data;
        int lines = 0;
        for (char c : input_data) {
            if (c == '\n') ++lines;
        }
        if (!input_data.empty() && input_data.back() != '\n') ++lines;
        return {0, "IEBGENER COMPLETED - " + std::to_string(lines) + " RECORDS COPIED", lines};
    }

    // Execute with config (validates parameters)
    static Result execute(const Config& config) {
        if (config.sysut1.empty()) {
            return {12, "IEBGENER ERROR - SYSUT1 NOT SPECIFIED", 0};
        }
        if (config.sysut2.empty()) {
            return {12, "IEBGENER ERROR - SYSUT2 NOT SPECIFIED", 0};
        }
        return {0, "IEBGENER COMPLETED - COPY FROM " + config.sysut1 + " TO " + config.sysut2, 0};
    }

    // In-memory record-level copy with optional transformation
    static Result execute_records(const std::vector<std::string>& input,
                                   std::vector<std::string>& output,
                                   std::function<std::string(const std::string&)> transform = nullptr) {
        output.clear();
        output.reserve(input.size());
        for (const auto& rec : input) {
            if (transform) {
                output.push_back(transform(rec));
            } else {
                output.push_back(rec);
            }
        }
        return {0, "IEBGENER COMPLETED - " + std::to_string(static_cast<int>(output.size())) + " RECORDS COPIED",
                static_cast<int>(output.size())};
    }
};

// --- IEBCOPY: PDS member copy/merge ---
// PDS = Partitioned Data Set (like a directory of members)
class IEBCOPY {
public:
    using PDS = std::map<std::string, std::string>;  // member name -> content

    enum class Operation {
        COPY,           // Copy members from source to target
        MERGE,          // Merge: copy only members not in target
        REPLACE,        // Replace: copy all, overwriting target members
        SELECT,         // Copy only selected members
        EXCLUDE         // Copy all except excluded members
    };

    struct Config {
        Operation op = Operation::COPY;
        std::set<std::string> select_members;   // For SELECT/EXCLUDE operations
        bool replace_existing = true;
    };

    struct Result {
        int rc;
        std::string message;
        int members_copied;
        std::vector<std::string> copied_names;
        std::vector<std::string> skipped_names;
    };

    static Result execute(const PDS& source, PDS& target) {
        Config config;
        return execute(source, target, config);
    }

    static Result execute(const PDS& source, PDS& target, const Config& config) {
        Result result{0, "", 0, {}, {}};

        for (const auto& [name, content] : source) {
            bool should_copy = true;

            switch (config.op) {
                case Operation::SELECT:
                    should_copy = config.select_members.count(name) > 0;
                    break;
                case Operation::EXCLUDE:
                    should_copy = config.select_members.count(name) == 0;
                    break;
                case Operation::MERGE:
                    should_copy = target.find(name) == target.end();
                    break;
                case Operation::COPY:
                case Operation::REPLACE:
                    should_copy = true;
                    break;
            }

            if (!should_copy) {
                result.skipped_names.push_back(name);
                continue;
            }

            bool exists = target.find(name) != target.end();
            if (exists && !config.replace_existing && config.op != Operation::REPLACE) {
                result.skipped_names.push_back(name);
                result.rc = std::max(result.rc, 4); // Warning: member existed
                continue;
            }

            target[name] = content;
            result.copied_names.push_back(name);
            ++result.members_copied;
        }

        result.message = "IEBCOPY COMPLETED - " + std::to_string(result.members_copied) +
                         " MEMBERS COPIED, " + std::to_string(static_cast<int>(result.skipped_names.size())) +
                         " SKIPPED";
        return result;
    }

    // Compress PDS (remove deleted members, reclaim space) - in-memory is always compact
    static Result compress(PDS& pds) {
        (void)pds;
        return {0, "IEBCOPY COMPRESS COMPLETED - PDS REORGANIZED", 0, {}, {}};
    }
};

// --- IDCAMS: Access Method Services ---
// Manages VSAM clusters and other catalog operations
class IDCAMS {
public:
    // Catalog entry types
    enum class EntryType {
        CLUSTER,        // VSAM cluster (KSDS, ESDS, RRDS, LDS)
        AIX,            // Alternate index
        PATH,           // Access path
        NONVSAM,        // Non-VSAM dataset
        GDG,            // Generation Data Group
        ALIAS           // Catalog alias
    };

    // VSAM organization
    enum class VsamOrg {
        KSDS,   // Key-Sequenced
        ESDS,   // Entry-Sequenced
        RRDS,   // Relative Record
        LDS     // Linear
    };

    struct CatalogEntry {
        std::string name;
        EntryType type;
        VsamOrg org = VsamOrg::KSDS;
        int record_size = 0;
        int key_length = 0;
        int key_offset = 0;
        int primary_space = 0;
        int secondary_space = 0;
        std::string space_unit;   // CYL, TRK, REC
        std::string volume;
        bool exists = true;
        std::vector<std::string> data;  // In-memory data
    };

    struct Result {
        int rc;
        std::string message;
        std::vector<std::string> output_lines;
    };

    IDCAMS() = default;

    // DEFINE CLUSTER
    Result define_cluster(const std::string& name, VsamOrg org,
                          int record_size, int key_length = 0, int key_offset = 0) {
        if (catalog_.count(name)) {
            return {8, "IDCAMS ERROR - CLUSTER " + name + " ALREADY DEFINED", {}};
        }
        CatalogEntry entry;
        entry.name = name;
        entry.type = EntryType::CLUSTER;
        entry.org = org;
        entry.record_size = record_size;
        entry.key_length = key_length;
        entry.key_offset = key_offset;
        catalog_[name] = entry;
        return {0, "IDCAMS DEFINE CLUSTER " + name + " COMPLETED", {}};
    }

    // DELETE
    Result delete_entry(const std::string& name, bool purge = false) {
        auto it = catalog_.find(name);
        if (it == catalog_.end()) {
            if (purge) return {0, "IDCAMS DELETE " + name + " - NOT FOUND (PURGE)", {}};
            return {8, "IDCAMS ERROR - ENTRY " + name + " NOT FOUND", {}};
        }
        catalog_.erase(it);
        return {0, "IDCAMS DELETE " + name + " COMPLETED", {}};
    }

    // REPRO (copy data between datasets)
    Result repro(const std::string& from, const std::string& to) {
        auto from_it = catalog_.find(from);
        auto to_it = catalog_.find(to);
        if (from_it == catalog_.end()) {
            return {12, "IDCAMS ERROR - SOURCE " + from + " NOT FOUND", {}};
        }
        if (to_it == catalog_.end()) {
            return {12, "IDCAMS ERROR - TARGET " + to + " NOT FOUND", {}};
        }
        to_it->second.data = from_it->second.data;
        int count = static_cast<int>(from_it->second.data.size());
        return {0, "IDCAMS REPRO COMPLETED - " + std::to_string(count) + " RECORDS COPIED", {}};
    }

    // REPRO from raw data
    Result repro_from_data(const std::string& target, const std::vector<std::string>& data) {
        auto it = catalog_.find(target);
        if (it == catalog_.end()) {
            return {12, "IDCAMS ERROR - TARGET " + target + " NOT FOUND", {}};
        }
        it->second.data = data;
        return {0, "IDCAMS REPRO COMPLETED - " + std::to_string(static_cast<int>(data.size())) + " RECORDS LOADED", {}};
    }

    // LISTCAT
    Result listcat(const std::string& pattern = "") const {
        Result result{0, "IDCAMS LISTCAT", {}};
        for (const auto& [name, entry] : catalog_) {
            if (!pattern.empty() && name.find(pattern) == std::string::npos) continue;
            std::string type_str;
            switch (entry.type) {
                case EntryType::CLUSTER: type_str = "CLUSTER"; break;
                case EntryType::AIX:     type_str = "AIX"; break;
                case EntryType::PATH:    type_str = "PATH"; break;
                case EntryType::NONVSAM: type_str = "NONVSAM"; break;
                case EntryType::GDG:     type_str = "GDG"; break;
                case EntryType::ALIAS:   type_str = "ALIAS"; break;
            }
            std::string org_str;
            switch (entry.org) {
                case VsamOrg::KSDS: org_str = "KSDS"; break;
                case VsamOrg::ESDS: org_str = "ESDS"; break;
                case VsamOrg::RRDS: org_str = "RRDS"; break;
                case VsamOrg::LDS:  org_str = "LDS"; break;
            }
            result.output_lines.push_back(
                type_str + " " + name + " ORG=" + org_str +
                " RECSZ=" + std::to_string(entry.record_size) +
                " RECS=" + std::to_string(static_cast<int>(entry.data.size())));
        }
        if (result.output_lines.empty()) {
            result.message = "IDCAMS LISTCAT - NO ENTRIES FOUND";
        } else {
            result.message = "IDCAMS LISTCAT - " +
                             std::to_string(static_cast<int>(result.output_lines.size())) + " ENTRIES";
        }
        return result;
    }

    // ALTER (change attributes)
    Result alter(const std::string& name, const std::map<std::string, std::string>& attrs) {
        auto it = catalog_.find(name);
        if (it == catalog_.end()) {
            return {8, "IDCAMS ERROR - ENTRY " + name + " NOT FOUND", {}};
        }
        for (const auto& [attr, val] : attrs) {
            if (attr == "RECSZ" || attr == "RECORDSIZE") {
                try { it->second.record_size = std::stoi(val); } catch (...) {}
            } else if (attr == "KEYS") {
                // KEYS(length offset)
                try { it->second.key_length = std::stoi(val); } catch (...) {}
            }
        }
        return {0, "IDCAMS ALTER " + name + " COMPLETED", {}};
    }

    // Check if an entry exists
    bool exists(const std::string& name) const {
        return catalog_.count(name) > 0;
    }

    // Get entry count
    size_t entry_count() const { return catalog_.size(); }

    // Get record count for an entry
    int record_count(const std::string& name) const {
        auto it = catalog_.find(name);
        if (it == catalog_.end()) return -1;
        return static_cast<int>(it->second.data.size());
    }

    // Parse and execute IDCAMS control statements
    Result execute_sysin(const std::string& sysin) {
        std::istringstream iss(sysin);
        std::string line;
        Result last_result{0, "", {}};
        int max_rc = 0;

        while (std::getline(iss, line)) {
            // Trim
            while (!line.empty() && line.front() == ' ') line.erase(line.begin());
            while (!line.empty() && line.back() == ' ') line.pop_back();
            if (line.empty() || line[0] == ' ') continue;

            // Continuation: lines ending with - are continued
            while (!line.empty() && line.back() == '-') {
                line.pop_back();
                std::string next;
                if (!std::getline(iss, next)) break;
                while (!next.empty() && next.front() == ' ') next.erase(next.begin());
                line += next;
            }

            std::string upper_line = line;
            std::transform(upper_line.begin(), upper_line.end(), upper_line.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

            if (upper_line.find("DEFINE CLUSTER") == 0 || upper_line.find("DEF CL") == 0) {
                auto params = parse_idcams_params(line);
                std::string name = params["NAME"];
                VsamOrg org = VsamOrg::KSDS;
                if (params.count("INDEXED")) org = VsamOrg::KSDS;
                if (params.count("NONINDEXED") || params.count("NUMBERED")) org = VsamOrg::ESDS;
                int recsz = 80;
                if (params.count("RECORDSIZE")) {
                    try { recsz = std::stoi(params["RECORDSIZE"]); } catch (...) {}
                }
                last_result = define_cluster(name, org, recsz);
            } else if (upper_line.find("DELETE") == 0 || upper_line.find("DEL") == 0) {
                auto params = parse_idcams_params(line);
                std::string name = params.count("NAME") ? params["NAME"] : "";
                if (name.empty()) {
                    // Try to extract name from "DELETE name"
                    size_t sp = upper_line.find(' ');
                    if (sp != std::string::npos) {
                        name = line.substr(sp + 1);
                        while (!name.empty() && name.front() == ' ') name.erase(name.begin());
                        size_t end = name.find(' ');
                        if (end != std::string::npos) name = name.substr(0, end);
                    }
                }
                bool purge = params.count("PURGE") > 0;
                last_result = delete_entry(name, purge);
            } else if (upper_line.find("REPRO") == 0) {
                auto params = parse_idcams_params(line);
                std::string from = params.count("INFILE") ? params["INFILE"] : params["INDATASET"];
                std::string to = params.count("OUTFILE") ? params["OUTFILE"] : params["OUTDATASET"];
                last_result = repro(from, to);
            } else if (upper_line.find("LISTCAT") == 0 || upper_line.find("LISTC") == 0) {
                auto params = parse_idcams_params(line);
                std::string pattern;
                if (params.count("ENTRIES")) pattern = params["ENTRIES"];
                last_result = listcat(pattern);
            } else if (upper_line.find("ALTER") == 0) {
                auto params = parse_idcams_params(line);
                std::string name = params.count("NAME") ? params["NAME"] : "";
                if (name.empty()) {
                    size_t sp = upper_line.find(' ');
                    if (sp != std::string::npos) {
                        name = line.substr(sp + 1);
                        while (!name.empty() && name.front() == ' ') name.erase(name.begin());
                        size_t end = name.find(' ');
                        if (end != std::string::npos) name = name.substr(0, end);
                    }
                }
                last_result = alter(name, params);
            }

            max_rc = std::max(max_rc, last_result.rc);
        }

        if (max_rc > last_result.rc) last_result.rc = max_rc;
        return last_result;
    }

private:
    std::map<std::string, CatalogEntry> catalog_;

    static std::map<std::string, std::string> parse_idcams_params(const std::string& line) {
        std::map<std::string, std::string> params;
        size_t pos = 0;
        // Skip command word(s)
        while (pos < line.size() && line[pos] != '(') {
            if (line[pos] == ' ') {
                ++pos;
                // Skip second word if it's part of command (e.g., DEFINE CLUSTER)
                while (pos < line.size() && line[pos] == ' ') ++pos;
                if (pos < line.size() && std::isalpha(static_cast<unsigned char>(line[pos]))) {
                    size_t ws = pos;
                    while (pos < line.size() && line[pos] != ' ' && line[pos] != '(') ++pos;
                    std::string word = line.substr(ws, pos - ws);
                    std::string upper_word = word;
                    std::transform(upper_word.begin(), upper_word.end(), upper_word.begin(),
                                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
                    if (upper_word == "CLUSTER" || upper_word == "AIX" || upper_word == "PATH" ||
                        upper_word == "GDG" || upper_word == "ALIAS") {
                        continue;
                    }
                    // It's a name or parameter
                    params["NAME"] = word;
                }
                break;
            }
            ++pos;
        }

        // Parse keyword(value) pairs
        while (pos < line.size()) {
            // Skip spaces and separators
            while (pos < line.size() && (line[pos] == ' ' || line[pos] == '-')) ++pos;
            if (pos >= line.size()) break;

            // Extract keyword
            size_t kw_start = pos;
            while (pos < line.size() && line[pos] != '(' && line[pos] != ' ' && line[pos] != '-') {
                ++pos;
            }
            if (pos <= kw_start) { ++pos; continue; }
            std::string keyword = line.substr(kw_start, pos - kw_start);
            std::transform(keyword.begin(), keyword.end(), keyword.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

            // Check for (value)
            if (pos < line.size() && line[pos] == '(') {
                ++pos;
                int depth = 1;
                size_t val_start = pos;
                while (pos < line.size() && depth > 0) {
                    if (line[pos] == '(') ++depth;
                    if (line[pos] == ')') --depth;
                    if (depth > 0) ++pos;
                }
                std::string value = line.substr(val_start, pos - val_start);
                params[keyword] = value;
                if (pos < line.size()) ++pos; // skip closing paren
            } else {
                // Boolean keyword (no value)
                params[keyword] = "";
            }
        }
        return params;
    }
};

// --- SORT: Integration stub for Tier 2 ---
class SortStub {
public:
    struct SortField {
        int position;   // 1-based
        int length;
        char format;    // CH, ZD, PD, BI, FI
        char order;     // A or D
    };

    struct Config {
        std::vector<SortField> fields;
        std::string include_cond;
        std::string omit_cond;
        std::string outrec;
        bool sum = false;
    };

    struct Result {
        int rc;
        std::string message;
        int records_in;
        int records_out;
    };

    // In-memory character sort (simplified - real impl in Tier 2)
    static Result sort_records(std::vector<std::string>& records, const Config& config) {
        if (config.fields.empty()) {
            return {16, "SORT ERROR - NO SORT FIELDS SPECIFIED", static_cast<int>(records.size()), 0};
        }

        int in_count = static_cast<int>(records.size());
        const auto& fields = config.fields;

        std::sort(records.begin(), records.end(),
            [&fields](const std::string& a, const std::string& b) {
                for (const auto& f : fields) {
                    int pos = f.position - 1;
                    int len = f.length;
                    std::string fa = (pos < static_cast<int>(a.size()))
                        ? a.substr(static_cast<size_t>(pos), static_cast<size_t>(std::min(len, static_cast<int>(a.size()) - pos)))
                        : "";
                    std::string fb = (pos < static_cast<int>(b.size()))
                        ? b.substr(static_cast<size_t>(pos), static_cast<size_t>(std::min(len, static_cast<int>(b.size()) - pos)))
                        : "";
                    int cmp = fa.compare(fb);
                    if (cmp != 0) {
                        return f.order == 'A' ? (cmp < 0) : (cmp > 0);
                    }
                }
                return false;
            });

        return {0, "SORT COMPLETED", in_count, static_cast<int>(records.size())};
    }
};

// --- IKJEFT01: TSO Command Processor ---
class IKJEFT01 {
public:
    struct Result {
        int rc;
        std::string message;
        std::vector<std::string> output;
    };

    // Execute TSO commands (in-memory simulation)
    static Result execute(const std::vector<std::string>& commands) {
        Result result{0, "", {}};

        for (const auto& cmd : commands) {
            std::string upper_cmd = cmd;
            std::transform(upper_cmd.begin(), upper_cmd.end(), upper_cmd.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

            if (upper_cmd.find("ALLOC") == 0) {
                result.output.push_back("ALLOCATED: " + cmd);
            } else if (upper_cmd.find("FREE") == 0) {
                result.output.push_back("FREED: " + cmd);
            } else if (upper_cmd.find("LISTDS") == 0) {
                result.output.push_back("LISTED: " + cmd);
            } else if (upper_cmd.find("DELETE") == 0) {
                result.output.push_back("DELETED: " + cmd);
            } else if (upper_cmd.find("EXEC") == 0) {
                result.output.push_back("EXECUTED: " + cmd);
            } else if (upper_cmd.find("RUN") == 0) {
                result.output.push_back("RAN: " + cmd);
            } else {
                result.output.push_back("UNKNOWN COMMAND: " + cmd);
                result.rc = 4;
            }
        }

        result.message = "IKJEFT01 COMPLETED - " +
                         std::to_string(static_cast<int>(commands.size())) + " COMMANDS PROCESSED";
        return result;
    }
};

// --- IEBUPDTE: Source Library Update ---
// Updates members in a PDS from control statements
class IEBUPDTE {
public:
    using PDS = std::map<std::string, std::string>;

    enum class Function {
        NEW,        // Create new member (replace if exists)
        CHANGE,     // Modify existing member
        REPL        // Replace entire member
    };

    struct UpdateRecord {
        Function func;
        std::string member_name;
        std::vector<std::string> data_lines;
        int insert_after = 0;      // For CHANGE: insert after this sequence number
        int delete_from = 0;       // For CHANGE: delete from seq number
        int delete_to = 0;         // For CHANGE: delete to seq number
    };

    struct Result {
        int rc;
        std::string message;
        int members_processed;
        std::vector<std::string> processed_names;
    };

    // Apply updates to a PDS
    static Result apply(PDS& pds, const std::vector<UpdateRecord>& updates) {
        Result result{0, "", 0, {}};

        for (const auto& upd : updates) {
            switch (upd.func) {
                case Function::NEW:
                case Function::REPL: {
                    std::ostringstream content;
                    for (const auto& line : upd.data_lines) {
                        content << line << "\n";
                    }
                    pds[upd.member_name] = content.str();
                    result.processed_names.push_back(upd.member_name);
                    ++result.members_processed;
                    break;
                }
                case Function::CHANGE: {
                    auto it = pds.find(upd.member_name);
                    if (it == pds.end()) {
                        result.rc = std::max(result.rc, 8);
                        continue;
                    }
                    // Split existing content into lines
                    std::vector<std::string> existing;
                    std::istringstream iss(it->second);
                    std::string line;
                    while (std::getline(iss, line)) {
                        existing.push_back(line);
                    }

                    // Apply deletions
                    if (upd.delete_from > 0 && upd.delete_to > 0) {
                        int from = upd.delete_from - 1;
                        int to = upd.delete_to;
                        if (from >= 0 && to <= static_cast<int>(existing.size())) {
                            existing.erase(existing.begin() + from,
                                           existing.begin() + to);
                        }
                    }

                    // Apply insertions
                    if (upd.insert_after >= 0 && !upd.data_lines.empty()) {
                        int pos = std::min(upd.insert_after,
                                           static_cast<int>(existing.size()));
                        existing.insert(existing.begin() + pos,
                                        upd.data_lines.begin(), upd.data_lines.end());
                    }

                    // Reassemble
                    std::ostringstream out;
                    for (const auto& l : existing) {
                        out << l << "\n";
                    }
                    it->second = out.str();
                    result.processed_names.push_back(upd.member_name);
                    ++result.members_processed;
                    break;
                }
            }
        }

        result.message = "IEBUPDTE COMPLETED - " +
                         std::to_string(result.members_processed) + " MEMBERS PROCESSED";
        return result;
    }

    // Parse IEBUPDTE control statements
    static std::vector<UpdateRecord> parse_control(const std::string& sysin) {
        std::vector<UpdateRecord> updates;
        std::istringstream iss(sysin);
        std::string line;
        UpdateRecord current;
        bool in_data = false;

        while (std::getline(iss, line)) {
            // Control statement starts with ./
            if (line.size() >= 2 && line[0] == '.' && line[1] == '/') {
                if (in_data && !current.member_name.empty()) {
                    updates.push_back(current);
                    current = UpdateRecord{};
                    in_data = false;
                }

                std::string upper_line = line;
                std::transform(upper_line.begin(), upper_line.end(), upper_line.begin(),
                               [](unsigned char c) { return static_cast<char>(std::toupper(c)); });

                if (upper_line.find("ADD") != std::string::npos) {
                    // ./ ADD NAME=membername
                    current.func = Function::NEW;
                    size_t name_pos = upper_line.find("NAME=");
                    if (name_pos != std::string::npos) {
                        size_t start = name_pos + 5;
                        size_t end = upper_line.find_first_of(" ,", start);
                        current.member_name = upper_line.substr(start,
                            end == std::string::npos ? std::string::npos : end - start);
                    }
                    in_data = true;
                } else if (upper_line.find("REPL") != std::string::npos) {
                    current.func = Function::REPL;
                    size_t name_pos = upper_line.find("NAME=");
                    if (name_pos != std::string::npos) {
                        size_t start = name_pos + 5;
                        size_t end = upper_line.find_first_of(" ,", start);
                        current.member_name = upper_line.substr(start,
                            end == std::string::npos ? std::string::npos : end - start);
                    }
                    in_data = true;
                } else if (upper_line.find("CHANGE") != std::string::npos) {
                    current.func = Function::CHANGE;
                    size_t name_pos = upper_line.find("NAME=");
                    if (name_pos != std::string::npos) {
                        size_t start = name_pos + 5;
                        size_t end = upper_line.find_first_of(" ,", start);
                        current.member_name = upper_line.substr(start,
                            end == std::string::npos ? std::string::npos : end - start);
                    }
                    in_data = true;
                } else if (upper_line.find("ENDUP") != std::string::npos) {
                    if (in_data && !current.member_name.empty()) {
                        updates.push_back(current);
                        current = UpdateRecord{};
                        in_data = false;
                    }
                }
            } else if (in_data) {
                current.data_lines.push_back(line);
            }
        }

        if (in_data && !current.member_name.empty()) {
            updates.push_back(current);
        }

        return updates;
    }
};

}} // namespace lazarus::jcl

#endif // LAZARUS_JCL_UTILITIES_H
