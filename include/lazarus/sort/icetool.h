// Lazarus Systems — ICETOOL operations
// Tier 2: DFSORT ICETOOL operator emulation

#ifndef LAZARUS_SORT_ICETOOL_H
#define LAZARUS_SORT_ICETOOL_H

#include "lazarus/sort/parser.h"
#include "lazarus/sort/engine.h"

#include <algorithm>
#include <functional>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <cmath>
#include <cstdint>

namespace lazarus { namespace sort { namespace icetool {

using engine::Record;
using engine::extract_field;
using engine::compare_field;
using engine::RecordComparator;
using parser::FieldFormat;
using parser::SortField;
using parser::SortOrder;
using parser::CondOp;
using parser::Condition;
using parser::SimpleCondition;

// ================================================================
// ICETOOL operation types
// ================================================================

struct Copy {
    // Copy input to output
};

struct Count {
    // Count records, store count
    int64_t result = 0;
};

struct Display {
    // Display field statistics
    int position;
    int length;
    FieldFormat format;
    struct Result {
        std::string min_val;
        std::string max_val;
        int64_t count = 0;
    };
    Result result;
};

struct Mode {
    // Find mode (most frequent value) of a field
    int position;
    int length;
    FieldFormat format;
    struct Result {
        std::string mode_val;
        int64_t frequency = 0;
    };
    Result result;
};

struct Occur {
    // Count occurrences of each unique value
    int position;
    int length;
    FieldFormat format;
    struct Entry {
        std::string value;
        int64_t count;
    };
    std::vector<Entry> result;
};

struct Range {
    // Select records where field is within range
    int position;
    int length;
    FieldFormat format;
    std::string low;
    std::string high;
};

struct Select {
    // Select records meeting criteria
    int position;
    int length;
    FieldFormat format;
    CondOp op;
    std::string value;
    int64_t num_value = 0;  // For HIGHER/LOWER/EQUAL/FIRST/LAST
};

struct Sort {
    // Sort with specified fields
    std::vector<SortField> fields;
};

struct Stats {
    // Compute statistics on a field
    int position;
    int length;
    FieldFormat format;
    struct Result {
        double mean = 0.0;
        double std_dev = 0.0;
        int64_t min_val = 0;
        int64_t max_val = 0;
        int64_t sum = 0;
        int64_t count = 0;
    };
    Result result;
};

struct Unique {
    // Remove duplicate records based on specified field
    int position;
    int length;
    FieldFormat format;
};

struct Verify {
    // Verify field contents match expected format
    int position;
    int length;
    FieldFormat format;
    struct Result {
        bool valid = true;
        int64_t error_count = 0;
        std::vector<size_t> error_records;
    };
    Result result;
};

using Operation = std::variant<Copy, Count, Display, Mode, Occur, Range,
                                Select, Sort, Stats, Unique, Verify>;

// ================================================================
// OUTFIL — conditional output routing
// ================================================================
struct OutfilCondition {
    Condition predicate;
    std::string output_name;
};

struct Outfil {
    std::vector<OutfilCondition> routes;

    // Route records to named outputs based on conditions
    std::map<std::string, std::vector<Record>> route(const std::vector<Record>& records) const {
        std::map<std::string, std::vector<Record>> result;
        for (const auto& r : routes) {
            result[r.output_name] = {};
        }
        for (const auto& rec : records) {
            for (const auto& r : routes) {
                if (engine::ConditionEvaluator::evaluate(r.predicate, rec)) {
                    result[r.output_name].push_back(rec);
                }
            }
        }
        return result;
    }
};

// ================================================================
// SYMNAMES — constexpr symbol table for named fields
// ================================================================
struct SymbolDef {
    std::string name;
    int position;
    int length;
    FieldFormat format;
};

class SymNames {
public:
    void define(const std::string& name, int pos, int len, FieldFormat fmt = FieldFormat::CH) {
        symbols_[name] = {name, pos, len, fmt};
    }

    bool has(const std::string& name) const {
        return symbols_.find(name) != symbols_.end();
    }

    const SymbolDef& get(const std::string& name) const {
        auto it = symbols_.find(name);
        if (it == symbols_.end()) {
            throw std::runtime_error("Undefined symbol: " + name);
        }
        return it->second;
    }

    SortField to_sort_field(const std::string& name, SortOrder order = SortOrder::ASC) const {
        const auto& sym = get(name);
        return SortField{sym.position, sym.length, sym.format, order};
    }

    std::string_view extract(const std::string& name, const Record& rec) const {
        const auto& sym = get(name);
        return extract_field(rec, sym.position, sym.length);
    }

    size_t size() const { return symbols_.size(); }

    std::vector<std::string> names() const {
        std::vector<std::string> result;
        result.reserve(symbols_.size());
        for (const auto& p : symbols_) {
            result.push_back(p.first);
        }
        return result;
    }

private:
    std::map<std::string, SymbolDef> symbols_;
};

// ================================================================
// IFTHEN — conditional field reformatting
// ================================================================
struct IfthenClause {
    enum class Type { WHEN, OVERLAY, BUILD };
    Type type;
    Condition when_cond;
    std::vector<parser::FieldRef> fields;
};

class IfthenProcessor {
public:
    void add_clause(IfthenClause clause) {
        clauses_.push_back(std::move(clause));
    }

    Record apply(const Record& rec) const {
        for (const auto& clause : clauses_) {
            if (engine::ConditionEvaluator::evaluate(clause.when_cond, rec)) {
                parser::RecordReformat rf;
                rf.type = (clause.type == IfthenClause::Type::OVERLAY)
                    ? parser::RecordReformat::Type::OVERLAY
                    : parser::RecordReformat::Type::BUILD;
                rf.fields = clause.fields;
                return engine::RecordReformatter::apply(rf, rec);
            }
        }
        return rec; // no clause matched — return unchanged
    }

private:
    std::vector<IfthenClause> clauses_;
};

// ================================================================
// ICETOOL executor
// ================================================================
class IcetoolExecutor {
public:
    // Execute an operation on a set of records
    std::vector<Record> execute(Operation& op, std::vector<Record> records) {
        return std::visit([&records](auto& o) -> std::vector<Record> {
            return dispatch(o, std::move(records));
        }, op);
    }

private:
    // --- Copy ---
    static std::vector<Record> dispatch(Copy& /*op*/, std::vector<Record> records) {
        return records;
    }

    // --- Count ---
    static std::vector<Record> dispatch(Count& op, std::vector<Record> records) {
        op.result = static_cast<int64_t>(records.size());
        return records;
    }

    // --- Display ---
    static std::vector<Record> dispatch(Display& op, std::vector<Record> records) {
        op.result.count = static_cast<int64_t>(records.size());
        if (!records.empty()) {
            op.result.min_val = std::string(extract_field(records[0], op.position, op.length));
            op.result.max_val = op.result.min_val;
            for (size_t i = 1; i < records.size(); ++i) {
                auto val = extract_field(records[i], op.position, op.length);
                std::string vs(val);
                if (compare_field(vs, op.result.min_val, op.format) < 0)
                    op.result.min_val = vs;
                if (compare_field(vs, op.result.max_val, op.format) > 0)
                    op.result.max_val = vs;
            }
        }
        return records;
    }

    // --- Mode ---
    static std::vector<Record> dispatch(Mode& op, std::vector<Record> records) {
        std::map<std::string, int64_t> freq;
        for (const auto& rec : records) {
            auto val = std::string(extract_field(rec, op.position, op.length));
            freq[val]++;
        }
        int64_t max_freq = 0;
        for (const auto& p : freq) {
            if (p.second > max_freq) {
                max_freq = p.second;
                op.result.mode_val = p.first;
                op.result.frequency = p.second;
            }
        }
        return records;
    }

    // --- Occur ---
    static std::vector<Record> dispatch(Occur& op, std::vector<Record> records) {
        std::map<std::string, int64_t> freq;
        for (const auto& rec : records) {
            auto val = std::string(extract_field(rec, op.position, op.length));
            freq[val]++;
        }
        op.result.clear();
        for (const auto& p : freq) {
            op.result.push_back({p.first, p.second});
        }
        return records;
    }

    // --- Range ---
    static std::vector<Record> dispatch(Range& op, std::vector<Record> records) {
        std::vector<Record> result;
        for (const auto& rec : records) {
            auto val = std::string(extract_field(rec, op.position, op.length));
            if (compare_field(val, op.low, op.format) >= 0 &&
                compare_field(val, op.high, op.format) <= 0) {
                result.push_back(rec);
            }
        }
        return result;
    }

    // --- Select ---
    static std::vector<Record> dispatch(Select& op, std::vector<Record> records) {
        std::vector<Record> result;
        for (const auto& rec : records) {
            auto val = std::string(extract_field(rec, op.position, op.length));
            bool match = false;
            switch (op.op) {
                case CondOp::EQ: match = compare_field(val, op.value, op.format) == 0; break;
                case CondOp::NE: match = compare_field(val, op.value, op.format) != 0; break;
                case CondOp::GT: match = compare_field(val, op.value, op.format) > 0; break;
                case CondOp::GE: match = compare_field(val, op.value, op.format) >= 0; break;
                case CondOp::LT: match = compare_field(val, op.value, op.format) < 0; break;
                case CondOp::LE: match = compare_field(val, op.value, op.format) <= 0; break;
                case CondOp::SS: match = val.find(op.value) != std::string::npos; break;
                case CondOp::NUM: match = true; break;
            }
            if (match) result.push_back(rec);
        }
        return result;
    }

    // --- Sort ---
    static std::vector<Record> dispatch(Sort& op, std::vector<Record> records) {
        RecordComparator cmp(op.fields);
        std::stable_sort(records.begin(), records.end(), cmp);
        return records;
    }

    // --- Stats ---
    static std::vector<Record> dispatch(Stats& op, std::vector<Record> records) {
        op.result = {};
        if (records.empty()) return records;

        std::vector<int64_t> values;
        values.reserve(records.size());
        for (const auto& rec : records) {
            auto field = extract_field(rec, op.position, op.length);
            int64_t v = 0;
            switch (op.format) {
                case FieldFormat::ZD: v = engine::decode_zd(field); break;
                case FieldFormat::PD: v = engine::decode_pd(field); break;
                case FieldFormat::BI: v = static_cast<int64_t>(engine::decode_bi(field)); break;
                case FieldFormat::FI: v = engine::decode_fi(field); break;
                default: {
                    for (auto c : field)
                        if (std::isdigit(static_cast<unsigned char>(c)))
                            v = v * 10 + (c - '0');
                    break;
                }
            }
            values.push_back(v);
        }

        op.result.count = static_cast<int64_t>(values.size());
        op.result.sum = std::accumulate(values.begin(), values.end(), int64_t(0));
        op.result.mean = static_cast<double>(op.result.sum) / static_cast<double>(op.result.count);
        op.result.min_val = *std::min_element(values.begin(), values.end());
        op.result.max_val = *std::max_element(values.begin(), values.end());

        double var = 0.0;
        for (auto v : values) {
            double diff = static_cast<double>(v) - op.result.mean;
            var += diff * diff;
        }
        if (op.result.count > 1)
            op.result.std_dev = std::sqrt(var / static_cast<double>(op.result.count - 1));

        return records;
    }

    // --- Unique ---
    static std::vector<Record> dispatch(Unique& op, std::vector<Record> records) {
        if (records.empty()) return records;
        std::set<std::string> seen;
        std::vector<Record> result;
        for (const auto& rec : records) {
            auto val = std::string(extract_field(rec, op.position, op.length));
            if (seen.insert(val).second) {
                result.push_back(rec);
            }
        }
        (void)op;
        return result;
    }

    // --- Verify ---
    static std::vector<Record> dispatch(Verify& op, std::vector<Record> records) {
        op.result = {};
        op.result.valid = true;
        for (size_t i = 0; i < records.size(); ++i) {
            auto field = extract_field(records[i], op.position, op.length);
            bool valid = verify_format(field, op.format);
            if (!valid) {
                op.result.valid = false;
                op.result.error_count++;
                op.result.error_records.push_back(i);
            }
        }
        return records;
    }

    static bool verify_format(std::string_view field, FieldFormat fmt) {
        switch (fmt) {
            case FieldFormat::ZD: {
                for (size_t i = 0; i < field.size(); ++i) {
                    uint8_t b = static_cast<uint8_t>(field[i]);
                    int digit = b & 0x0F;
                    if (digit > 9) return false;
                    uint8_t zone = (b >> 4) & 0x0F;
                    if (i < field.size() - 1) {
                        if (zone != 0x0F) return false;
                    } else {
                        if (zone != 0x0C && zone != 0x0D && zone != 0x0F) return false;
                    }
                }
                return true;
            }
            case FieldFormat::PD: {
                if (field.empty()) return false;
                for (size_t i = 0; i < field.size(); ++i) {
                    uint8_t b = static_cast<uint8_t>(field[i]);
                    int high = (b >> 4) & 0x0F;
                    int low = b & 0x0F;
                    if (i < field.size() - 1) {
                        if (high > 9 || low > 9) return false;
                    } else {
                        if (high > 9) return false;
                        if (low != 0x0C && low != 0x0D && low != 0x0F) return false;
                    }
                }
                return true;
            }
            case FieldFormat::BI:
            case FieldFormat::FI:
            case FieldFormat::FL:
                return true; // Any binary pattern is valid
            case FieldFormat::CH:
            case FieldFormat::AC:
                return true; // Any character data is valid
            case FieldFormat::CSF:
            case FieldFormat::CSL: {
                if (field.empty()) return false;
                size_t start = 0;
                if (field[0] == '+' || field[0] == '-') start = 1;
                for (size_t i = start; i < field.size(); ++i) {
                    if (!std::isdigit(static_cast<unsigned char>(field[i])) &&
                        field[i] != '.' && field[i] != ' ') return false;
                }
                return true;
            }
            case FieldFormat::CST: {
                if (field.empty()) return false;
                for (size_t i = 0; i + 1 < field.size(); ++i) {
                    if (!std::isdigit(static_cast<unsigned char>(field[i])) &&
                        field[i] != '.' && field[i] != ' ') return false;
                }
                char last = field.back();
                return std::isdigit(static_cast<unsigned char>(last)) ||
                       last == '+' || last == '-';
            }
        }
        return true;
    }
};

}}} // namespace lazarus::sort::icetool

#endif // LAZARUS_SORT_ICETOOL_H
