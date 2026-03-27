// Lazarus Systems — DFSORT/SYNCSORT sort/merge engine
// Tier 2: Template-based sort engine with format-aware comparators

#ifndef LAZARUS_SORT_ENGINE_H
#define LAZARUS_SORT_ENGINE_H

#include "lazarus/sort/parser.h"
#include "lazarus/sort/ebcdic.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <cstdint>
#include <cassert>

namespace lazarus { namespace sort { namespace engine {

using parser::FieldFormat;
using parser::SortOrder;
using parser::SortField;
using parser::CondOp;
using parser::LogicOp;
using parser::SimpleCondition;
using parser::CompoundCondition;
using parser::Condition;
using parser::RecordReformat;
using parser::FieldRef;
using parser::SumField;
using parser::SortSpec;
using parser::SortOptions;

// ================================================================
// Fixed-length record type (byte buffer)
// ================================================================
using Record = std::string;

// Extract a field from a record (1-based position)
inline std::string_view extract_field(const Record& rec, int pos, int len) {
    if (pos < 1) pos = 1;
    size_t start = static_cast<size_t>(pos - 1);
    size_t field_len = static_cast<size_t>(len);
    if (start >= rec.size()) return {};
    if (start + field_len > rec.size()) field_len = rec.size() - start;
    return std::string_view(rec).substr(start, field_len);
}

// ================================================================
// Format-specific comparison functions
// ================================================================

// CH — character (lexicographic byte comparison)
inline int compare_ch(std::string_view a, std::string_view b) {
    size_t len = std::min(a.size(), b.size());
    for (size_t i = 0; i < len; ++i) {
        auto ca = static_cast<unsigned char>(a[i]);
        auto cb = static_cast<unsigned char>(b[i]);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
    }
    if (a.size() < b.size()) return -1;
    if (a.size() > b.size()) return 1;
    return 0;
}

// AC — ASCII character (same as CH for ASCII records)
inline int compare_ac(std::string_view a, std::string_view b) {
    return compare_ch(a, b);
}

// ZD — zoned decimal: each byte has low nibble = digit, high nibble = zone (0xF)
// Last byte sign: 0xC=positive, 0xD=negative, 0xF=unsigned positive
inline int64_t decode_zd(std::string_view field) {
    if (field.empty()) return 0;
    int64_t val = 0;
    for (size_t i = 0; i < field.size(); ++i) {
        uint8_t b = static_cast<uint8_t>(field[i]);
        int digit = b & 0x0F;
        if (digit > 9) digit = 0;
        val = val * 10 + digit;
    }
    // Check sign in last byte
    uint8_t last = static_cast<uint8_t>(field.back());
    uint8_t zone = (last >> 4) & 0x0F;
    if (zone == 0x0D || zone == 0x0B) val = -val; // D, B = negative
    return val;
}

inline int compare_zd(std::string_view a, std::string_view b) {
    auto va = decode_zd(a);
    auto vb = decode_zd(b);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

// PD — packed decimal: BCD, two digits per byte, last nibble = sign
// Sign: C=positive, D=negative, F=unsigned positive
inline int64_t decode_pd(std::string_view field) {
    if (field.empty()) return 0;
    int64_t val = 0;
    for (size_t i = 0; i < field.size(); ++i) {
        uint8_t b = static_cast<uint8_t>(field[i]);
        if (i < field.size() - 1) {
            // Both nibbles are digits
            int high = (b >> 4) & 0x0F;
            int low = b & 0x0F;
            val = val * 100 + high * 10 + low;
        } else {
            // Last byte: high nibble = digit, low nibble = sign
            int digit = (b >> 4) & 0x0F;
            val = val * 10 + digit;
            uint8_t sign = b & 0x0F;
            if (sign == 0x0D || sign == 0x0B) val = -val;
        }
    }
    return val;
}

inline int compare_pd(std::string_view a, std::string_view b) {
    auto va = decode_pd(a);
    auto vb = decode_pd(b);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

// BI — binary (unsigned, big-endian)
inline uint64_t decode_bi(std::string_view field) {
    uint64_t val = 0;
    for (size_t i = 0; i < field.size(); ++i) {
        val = (val << 8) | static_cast<uint8_t>(field[i]);
    }
    return val;
}

inline int compare_bi(std::string_view a, std::string_view b) {
    auto va = decode_bi(a);
    auto vb = decode_bi(b);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

// FI — fixed-point signed integer (big-endian, two's complement)
inline int64_t decode_fi(std::string_view field) {
    if (field.empty()) return 0;
    int64_t val = 0;
    bool negative = (static_cast<uint8_t>(field[0]) & 0x80) != 0;
    for (size_t i = 0; i < field.size(); ++i) {
        val = (val << 8) | static_cast<uint8_t>(field[i]);
    }
    if (negative) {
        // Sign extend
        int bits = static_cast<int>(field.size()) * 8;
        if (bits < 64) {
            int64_t mask = (static_cast<int64_t>(1) << bits) - 1;
            val = val | (~mask);
        }
    }
    return val;
}

inline int compare_fi(std::string_view a, std::string_view b) {
    auto va = decode_fi(a);
    auto vb = decode_fi(b);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

// FL — floating point (IEEE 754, big-endian)
inline double decode_fl(std::string_view field) {
    if (field.size() == 4) {
        uint32_t bits = 0;
        for (int i = 0; i < 4; ++i)
            bits = (bits << 8) | static_cast<uint8_t>(field[static_cast<size_t>(i)]);
        float f;
        std::memcpy(&f, &bits, sizeof(f));
        return static_cast<double>(f);
    } else if (field.size() == 8) {
        uint64_t bits = 0;
        for (int i = 0; i < 8; ++i)
            bits = (bits << 8) | static_cast<uint8_t>(field[static_cast<size_t>(i)]);
        double d;
        std::memcpy(&d, &bits, sizeof(d));
        return d;
    }
    return 0.0;
}

inline int compare_fl(std::string_view a, std::string_view b) {
    auto va = decode_fl(a);
    auto vb = decode_fl(b);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

// CSF — numeric with optional leading floating sign (+/-)
inline int64_t decode_csf(std::string_view field) {
    if (field.empty()) return 0;
    int64_t val = 0;
    bool neg = false;
    size_t i = 0;
    if (field[0] == '-') { neg = true; ++i; }
    else if (field[0] == '+') { ++i; }
    for (; i < field.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(field[i]))) {
            val = val * 10 + (field[i] - '0');
        }
    }
    return neg ? -val : val;
}

inline int compare_csf(std::string_view a, std::string_view b) {
    auto va = decode_csf(a);
    auto vb = decode_csf(b);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

// CSL — numeric with leading separate sign (first byte is sign char)
inline int64_t decode_csl(std::string_view field) {
    return decode_csf(field); // Same parsing logic
}

inline int compare_csl(std::string_view a, std::string_view b) {
    return compare_csf(a, b);
}

// CST — numeric with trailing separate sign (last byte is sign char)
inline int64_t decode_cst(std::string_view field) {
    if (field.empty()) return 0;
    int64_t val = 0;
    bool neg = false;
    for (size_t i = 0; i + 1 < field.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(field[i]))) {
            val = val * 10 + (field[i] - '0');
        }
    }
    char last = field.back();
    if (last == '-') neg = true;
    else if (std::isdigit(static_cast<unsigned char>(last))) {
        val = val * 10 + (last - '0');
    }
    return neg ? -val : val;
}

inline int compare_cst(std::string_view a, std::string_view b) {
    auto va = decode_cst(a);
    auto vb = decode_cst(b);
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}

// ================================================================
// Generic field comparison dispatcher
// ================================================================
inline int compare_field(std::string_view a, std::string_view b, FieldFormat fmt) {
    switch (fmt) {
        case FieldFormat::CH:  return compare_ch(a, b);
        case FieldFormat::AC:  return compare_ac(a, b);
        case FieldFormat::ZD:  return compare_zd(a, b);
        case FieldFormat::PD:  return compare_pd(a, b);
        case FieldFormat::BI:  return compare_bi(a, b);
        case FieldFormat::FI:  return compare_fi(a, b);
        case FieldFormat::FL:  return compare_fl(a, b);
        case FieldFormat::CSF: return compare_csf(a, b);
        case FieldFormat::CSL: return compare_csl(a, b);
        case FieldFormat::CST: return compare_cst(a, b);
    }
    return compare_ch(a, b);
}

// ================================================================
// Composite multi-key comparator
// ================================================================
class RecordComparator {
public:
    explicit RecordComparator(const std::vector<SortField>& fields) : fields_(fields) {}

    bool operator()(const Record& a, const Record& b) const {
        for (const auto& f : fields_) {
            auto fa = extract_field(a, f.position, f.length);
            auto fb = extract_field(b, f.position, f.length);
            int cmp = compare_field(fa, fb, f.format);
            if (cmp != 0) {
                return (f.order == SortOrder::ASC) ? (cmp < 0) : (cmp > 0);
            }
        }
        return false; // equal — stable sort preserves original order
    }

    // Raw comparison (negative/0/positive)
    int compare(const Record& a, const Record& b) const {
        for (const auto& f : fields_) {
            auto fa = extract_field(a, f.position, f.length);
            auto fb = extract_field(b, f.position, f.length);
            int cmp = compare_field(fa, fb, f.format);
            if (cmp != 0) {
                return (f.order == SortOrder::ASC) ? cmp : -cmp;
            }
        }
        return 0;
    }

private:
    std::vector<SortField> fields_;
};

// ================================================================
// Condition evaluator
// ================================================================
class ConditionEvaluator {
public:
    static bool evaluate(const Condition& cond, const Record& rec) {
        return std::visit([&rec](const auto& c) -> bool {
            using T = std::decay_t<decltype(c)>;
            if constexpr (std::is_same_v<T, SimpleCondition>) {
                return eval_simple(c, rec);
            } else {
                return eval_compound(c, rec);
            }
        }, cond);
    }

private:
    static bool eval_simple(const SimpleCondition& sc, const Record& rec) {
        auto field = extract_field(rec, sc.position, sc.length);
        std::string field_str(field);

        switch (sc.op) {
            case CondOp::EQ: return field_compare(field_str, sc.value, sc.format) == 0;
            case CondOp::NE: return field_compare(field_str, sc.value, sc.format) != 0;
            case CondOp::GT: return field_compare(field_str, sc.value, sc.format) > 0;
            case CondOp::GE: return field_compare(field_str, sc.value, sc.format) >= 0;
            case CondOp::LT: return field_compare(field_str, sc.value, sc.format) < 0;
            case CondOp::LE: return field_compare(field_str, sc.value, sc.format) <= 0;
            case CondOp::SS: return field_str.find(sc.value) != std::string::npos;
            case CondOp::NUM: {
                for (auto c : field_str) {
                    if (!std::isdigit(static_cast<unsigned char>(c)) &&
                        c != '+' && c != '-' && c != '.') return false;
                }
                return true;
            }
        }
        return false;
    }

    static int field_compare(const std::string& field_val, const std::string& cmp_val, FieldFormat fmt) {
        // For numeric formats, decode both sides
        switch (fmt) {
            case FieldFormat::CH:
            case FieldFormat::AC:
                return compare_ch(field_val, cmp_val);
            case FieldFormat::ZD: {
                auto a = decode_zd(field_val);
                int64_t b = 0;
                try { b = std::stoll(cmp_val); } catch (...) {}
                return (a < b) ? -1 : (a > b ? 1 : 0);
            }
            case FieldFormat::PD: {
                auto a = decode_pd(field_val);
                int64_t b = 0;
                try { b = std::stoll(cmp_val); } catch (...) {}
                return (a < b) ? -1 : (a > b ? 1 : 0);
            }
            case FieldFormat::BI: {
                auto a = decode_bi(field_val);
                uint64_t b = 0;
                try { b = std::stoull(cmp_val); } catch (...) {}
                return (a < b) ? -1 : (a > b ? 1 : 0);
            }
            case FieldFormat::FI: {
                auto a = decode_fi(field_val);
                int64_t b = 0;
                try { b = std::stoll(cmp_val); } catch (...) {}
                return (a < b) ? -1 : (a > b ? 1 : 0);
            }
            default:
                return compare_ch(field_val, cmp_val);
        }
    }

    static bool eval_compound(const CompoundCondition& cc, const Record& rec) {
        if (cc.children.empty()) return true;
        if (cc.logic == LogicOp::AND) {
            for (const auto& child : cc.children) {
                if (!evaluate(child, rec)) return false;
            }
            return true;
        } else { // OR
            for (const auto& child : cc.children) {
                if (evaluate(child, rec)) return true;
            }
            return false;
        }
    }
};

// ================================================================
// Record reformatter (INREC/OUTREC)
// ================================================================
class RecordReformatter {
public:
    static Record apply(const RecordReformat& rf, const Record& rec) {
        if (rf.type == RecordReformat::Type::BUILD) {
            return build_record(rf.fields, rec);
        } else {
            return overlay_record(rf.fields, rec);
        }
    }

private:
    static Record build_record(const std::vector<FieldRef>& fields, const Record& rec) {
        std::string result;
        for (const auto& fr : fields) {
            if (fr.position == 0 && !fr.literal.empty()) {
                result += fr.literal;
            } else {
                auto field = extract_field(rec, fr.position, fr.length);
                result += std::string(field);
            }
        }
        return result;
    }

    static Record overlay_record(const std::vector<FieldRef>& fields, const Record& rec) {
        std::string result(rec);
        size_t offset = 0;
        for (const auto& fr : fields) {
            if (fr.position > 0) {
                offset = static_cast<size_t>(fr.position - 1);
            }
            if (!fr.literal.empty()) {
                for (size_t i = 0; i < fr.literal.size() && offset + i < result.size(); ++i) {
                    result[offset + i] = fr.literal[i];
                }
                offset += fr.literal.size();
            } else if (fr.position > 0 && fr.length > 0) {
                offset += static_cast<size_t>(fr.length);
            }
        }
        return result;
    }
};

// ================================================================
// SUM aggregator — summarize duplicate key records
// ================================================================
class SumAggregator {
public:
    static void summarize(std::vector<Record>& records,
                          const std::vector<SortField>& sort_keys,
                          const std::vector<SumField>& sum_fields) {
        if (records.size() <= 1 || sort_keys.empty()) return;

        RecordComparator cmp(sort_keys);
        std::vector<Record> result;
        result.push_back(records[0]);

        for (size_t i = 1; i < records.size(); ++i) {
            if (cmp.compare(result.back(), records[i]) == 0) {
                // Same key: add sum fields
                add_sum_fields(result.back(), records[i], sum_fields);
            } else {
                result.push_back(records[i]);
            }
        }
        records = std::move(result);
    }

private:
    static void add_sum_fields(Record& target, const Record& source,
                               const std::vector<SumField>& sum_fields) {
        for (const auto& sf : sum_fields) {
            auto tf = extract_field(target, sf.position, sf.length);
            auto sf_v = extract_field(source, sf.position, sf.length);

            int64_t tv = decode_numeric(tf, sf.format);
            int64_t sv = decode_numeric(sf_v, sf.format);
            int64_t sum = tv + sv;

            encode_numeric(target, sf.position, sf.length, sf.format, sum);
        }
    }

    static int64_t decode_numeric(std::string_view field, FieldFormat fmt) {
        switch (fmt) {
            case FieldFormat::ZD: return decode_zd(field);
            case FieldFormat::PD: return decode_pd(field);
            case FieldFormat::BI: return static_cast<int64_t>(decode_bi(field));
            case FieldFormat::FI: return decode_fi(field);
            default: {
                int64_t v = 0;
                for (auto c : field) {
                    if (std::isdigit(static_cast<unsigned char>(c)))
                        v = v * 10 + (c - '0');
                }
                return v;
            }
        }
    }

    static void encode_numeric(Record& rec, int pos, int len, FieldFormat fmt, int64_t val) {
        size_t start = static_cast<size_t>(pos - 1);
        if (start + static_cast<size_t>(len) > rec.size()) return;

        switch (fmt) {
            case FieldFormat::BI: {
                uint64_t uv = static_cast<uint64_t>(val);
                for (int i = len - 1; i >= 0; --i) {
                    rec[start + static_cast<size_t>(i)] = static_cast<char>(uv & 0xFF);
                    uv >>= 8;
                }
                break;
            }
            case FieldFormat::FI: {
                uint64_t uv = static_cast<uint64_t>(val);
                for (int i = len - 1; i >= 0; --i) {
                    rec[start + static_cast<size_t>(i)] = static_cast<char>(uv & 0xFF);
                    uv >>= 8;
                }
                break;
            }
            case FieldFormat::ZD: {
                bool neg = val < 0;
                if (neg) val = -val;
                for (int i = len - 1; i >= 0; --i) {
                    int digit = static_cast<int>(val % 10);
                    val /= 10;
                    uint8_t zone = 0xF0;
                    if (i == len - 1) {
                        zone = neg ? 0xD0 : 0xC0;
                    }
                    rec[start + static_cast<size_t>(i)] = static_cast<char>(zone | static_cast<uint8_t>(digit));
                }
                break;
            }
            case FieldFormat::PD: {
                bool neg = val < 0;
                if (neg) val = -val;
                int total_digits = len * 2 - 1;
                std::vector<int> digits(static_cast<size_t>(total_digits), 0);
                for (int i = total_digits - 1; i >= 0 && val > 0; --i) {
                    digits[static_cast<size_t>(i)] = static_cast<int>(val % 10);
                    val /= 10;
                }
                size_t di = 0;
                for (int i = 0; i < len - 1; ++i) {
                    uint8_t b = static_cast<uint8_t>((digits[di] << 4) | digits[di + 1]);
                    rec[start + static_cast<size_t>(i)] = static_cast<char>(b);
                    di += 2;
                }
                // Last byte: digit + sign
                uint8_t sign = neg ? 0x0D : 0x0C;
                uint8_t lb = static_cast<uint8_t>((digits[di] << 4) | sign);
                rec[start + static_cast<size_t>(len - 1)] = static_cast<char>(lb);
                break;
            }
            default: {
                // Character numeric: right-justified with leading zeros
                bool neg = val < 0;
                if (neg) val = -val;
                for (int i = len - 1; i >= 0; --i) {
                    rec[start + static_cast<size_t>(i)] = static_cast<char>('0' + val % 10);
                    val /= 10;
                }
                if (neg && len > 0) rec[start] = '-';
                break;
            }
        }
    }
};

// ================================================================
// Merge engine — merges pre-sorted record sets
// ================================================================
class MergeEngine {
public:
    static std::vector<Record> merge(const std::vector<std::vector<Record>>& inputs,
                                     const std::vector<SortField>& fields) {
        RecordComparator cmp(fields);
        std::vector<Record> result;

        // Simple k-way merge using indices
        std::vector<size_t> indices(inputs.size(), 0);
        size_t total = 0;
        for (const auto& inp : inputs) total += inp.size();
        result.reserve(total);

        for (size_t count = 0; count < total; ++count) {
            int best = -1;
            for (size_t k = 0; k < inputs.size(); ++k) {
                if (indices[k] < inputs[k].size()) {
                    if (best < 0 || cmp(inputs[k][indices[k]],
                                        inputs[static_cast<size_t>(best)][indices[static_cast<size_t>(best)]])) {
                        best = static_cast<int>(k);
                    }
                }
            }
            if (best < 0) break;
            result.push_back(inputs[static_cast<size_t>(best)][indices[static_cast<size_t>(best)]]);
            ++indices[static_cast<size_t>(best)];
        }
        return result;
    }
};

// ================================================================
// Main sort engine — processes a SortSpec against records
// ================================================================
class SortEngine {
public:
    explicit SortEngine(const SortSpec& spec) : spec_(spec) {}

    std::vector<Record> execute(std::vector<Record> records) {
        // SKIPREC
        if (spec_.options.skip_rec > 0) {
            size_t skip = static_cast<size_t>(spec_.options.skip_rec);
            if (skip >= records.size()) return {};
            records.erase(records.begin(), records.begin() + static_cast<ptrdiff_t>(skip));
        }

        // STOPAFT
        if (spec_.options.stop_aft > 0) {
            size_t stop = static_cast<size_t>(spec_.options.stop_aft);
            if (stop < records.size()) {
                records.resize(stop);
            }
        }

        // INCLUDE filter
        if (spec_.include_cond.has_value()) {
            std::vector<Record> filtered;
            for (const auto& rec : records) {
                if (ConditionEvaluator::evaluate(*spec_.include_cond, rec)) {
                    filtered.push_back(rec);
                }
            }
            records = std::move(filtered);
        }

        // OMIT filter (inverse of include)
        if (spec_.omit_cond.has_value()) {
            std::vector<Record> filtered;
            for (const auto& rec : records) {
                if (!ConditionEvaluator::evaluate(*spec_.omit_cond, rec)) {
                    filtered.push_back(rec);
                }
            }
            records = std::move(filtered);
        }

        // INREC
        if (spec_.inrec.has_value()) {
            for (auto& rec : records) {
                rec = RecordReformatter::apply(*spec_.inrec, rec);
            }
        }

        // SORT (unless OPTION COPY)
        if (!spec_.options.copy && !spec_.sort_fields.empty()) {
            RecordComparator cmp(spec_.sort_fields);
            if (spec_.options.equals) {
                std::stable_sort(records.begin(), records.end(), cmp);
            } else {
                std::sort(records.begin(), records.end(), cmp);
            }
        }

        // SUM
        if (!spec_.sum_fields.empty()) {
            SumAggregator::summarize(records, spec_.sort_fields, spec_.sum_fields);
        }

        // OUTREC
        if (spec_.outrec.has_value()) {
            for (auto& rec : records) {
                rec = RecordReformatter::apply(*spec_.outrec, rec);
            }
        }

        return records;
    }

    // Merge variant: takes multiple pre-sorted inputs
    std::vector<Record> execute_merge(const std::vector<std::vector<Record>>& inputs) {
        auto merged = MergeEngine::merge(inputs, spec_.sort_fields);

        // Apply same post-processing
        if (spec_.outrec.has_value()) {
            for (auto& rec : merged) {
                rec = RecordReformatter::apply(*spec_.outrec, rec);
            }
        }
        return merged;
    }

private:
    SortSpec spec_;
};

// ================================================================
// Convenience: parse and execute sort in one call
// ================================================================
inline std::vector<Record> sort_records(std::string_view control_card,
                                        std::vector<Record> records) {
    parser::SortParser p;
    auto spec = p.parse(control_card);
    SortEngine eng(spec);
    return eng.execute(std::move(records));
}

}}} // namespace lazarus::sort::engine

#endif // LAZARUS_SORT_ENGINE_H
