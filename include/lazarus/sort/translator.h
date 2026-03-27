// Lazarus Systems — DFSORT control card to C++17 code translator
// Tier 2: Code generation from sort control cards

#ifndef LAZARUS_SORT_TRANSLATOR_H
#define LAZARUS_SORT_TRANSLATOR_H

#include "lazarus/sort/parser.h"
#include "lazarus/sort/engine.h"

#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace lazarus { namespace sort { namespace translator {

using parser::FieldFormat;
using parser::SortOrder;
using parser::SortField;
using parser::SortSpec;
using parser::SortParser;
using parser::SimpleCondition;
using parser::CompoundCondition;
using parser::Condition;
using parser::CondOp;
using parser::LogicOp;
using parser::RecordReformat;
using parser::FieldRef;
using parser::SumField;
using parser::SortOptions;

// ================================================================
// Code generator: produces C++17 source from SortSpec
// ================================================================
class CodeGenerator {
public:
    // Generate complete C++ sort invocation from a control card string
    std::string generate(std::string_view control_card) {
        SortParser p;
        auto spec = p.parse(control_card);
        return generate(spec);
    }

    // Generate from a pre-parsed SortSpec
    std::string generate(const SortSpec& spec) {
        std::ostringstream out;

        emit_includes(out);
        out << "\n";
        emit_function_open(out);

        // Emit comparator
        if (!spec.sort_fields.empty() && !spec.options.copy) {
            emit_sort_fields(out, spec.sort_fields);
        }

        // Emit filter
        if (spec.include_cond.has_value()) {
            emit_filter(out, "include", *spec.include_cond);
        }
        if (spec.omit_cond.has_value()) {
            emit_filter(out, "omit", *spec.omit_cond);
        }

        // Emit INREC
        if (spec.inrec.has_value()) {
            emit_reformat(out, "inrec", *spec.inrec);
        }

        // Emit sort call
        if (!spec.sort_fields.empty() && !spec.options.copy) {
            emit_sort_call(out, spec.options.equals);
        } else if (spec.options.copy) {
            out << "    // OPTION COPY: no sort, pass through\n";
        }

        // Emit SUM
        if (!spec.sum_fields.empty()) {
            emit_sum(out, spec.sum_fields);
        }

        // Emit OUTREC
        if (spec.outrec.has_value()) {
            emit_reformat(out, "outrec", *spec.outrec);
        }

        emit_function_close(out);
        return out.str();
    }

    // Generate a JCL EXEC PGM=SORT wrapper
    std::string generate_jcl_wrapper(std::string_view control_card,
                                     const std::string& sortin = "SORTIN",
                                     const std::string& sortout = "SORTOUT") {
        std::ostringstream out;
        out << "//STEP1    EXEC PGM=SORT\n";
        out << "//SORTIN   DD DSN=" << sortin << ",DISP=SHR\n";
        out << "//SORTOUT  DD DSN=" << sortout << ",DISP=(NEW,CATLG)\n";
        out << "//SYSIN    DD *\n";
        out << control_card << "\n";
        out << "/*\n";
        out << "//\n";
        return out.str();
    }

    // Generate standalone sort function for a specific format type
    template <FieldFormat Fmt>
    std::string generate_typed_comparator(int pos, int len) {
        std::ostringstream out;
        out << "auto comparator = [](const Record& a, const Record& b) -> bool {\n";
        out << "    auto fa = extract_field(a, " << pos << ", " << len << ");\n";
        out << "    auto fb = extract_field(b, " << pos << ", " << len << ");\n";
        if constexpr (Fmt == FieldFormat::CH) {
            out << "    return compare_ch(fa, fb) < 0;\n";
        } else if constexpr (Fmt == FieldFormat::ZD) {
            out << "    return decode_zd(fa) < decode_zd(fb);\n";
        } else if constexpr (Fmt == FieldFormat::PD) {
            out << "    return decode_pd(fa) < decode_pd(fb);\n";
        } else if constexpr (Fmt == FieldFormat::BI) {
            out << "    return decode_bi(fa) < decode_bi(fb);\n";
        } else if constexpr (Fmt == FieldFormat::FI) {
            out << "    return decode_fi(fa) < decode_fi(fb);\n";
        } else if constexpr (Fmt == FieldFormat::FL) {
            out << "    return decode_fl(fa) < decode_fl(fb);\n";
        } else {
            out << "    return compare_ch(fa, fb) < 0;\n";
        }
        out << "};\n";
        return out.str();
    }

    // Emit format-specific decoder call
    static std::string format_decoder(FieldFormat fmt) {
        switch (fmt) {
            case FieldFormat::CH:  return "compare_ch";
            case FieldFormat::AC:  return "compare_ac";
            case FieldFormat::ZD:  return "compare_zd";
            case FieldFormat::PD:  return "compare_pd";
            case FieldFormat::BI:  return "compare_bi";
            case FieldFormat::FI:  return "compare_fi";
            case FieldFormat::FL:  return "compare_fl";
            case FieldFormat::CSF: return "compare_csf";
            case FieldFormat::CSL: return "compare_csl";
            case FieldFormat::CST: return "compare_cst";
        }
        return "compare_ch";
    }

private:
    void emit_includes(std::ostringstream& out) {
        out << "#include \"lazarus/sort/engine.h\"\n";
        out << "#include <vector>\n";
        out << "#include <string>\n";
        out << "#include <algorithm>\n";
        out << "using namespace lazarus::sort::engine;\n";
    }

    void emit_function_open(std::ostringstream& out) {
        out << "\nstd::vector<Record> sort_pipeline(std::vector<Record> records) {\n";
    }

    void emit_function_close(std::ostringstream& out) {
        out << "    return records;\n";
        out << "}\n";
    }

    void emit_sort_fields(std::ostringstream& out, const std::vector<SortField>& fields) {
        out << "    // Sort fields\n";
        out << "    std::vector<SortField> sort_fields = {\n";
        for (size_t i = 0; i < fields.size(); ++i) {
            const auto& f = fields[i];
            out << "        {" << f.position << ", " << f.length
                << ", FieldFormat::" << parser::format_to_string(f.format)
                << ", SortOrder::" << (f.order == SortOrder::ASC ? "ASC" : "DESC")
                << "}";
            if (i + 1 < fields.size()) out << ",";
            out << "\n";
        }
        out << "    };\n";
        out << "    RecordComparator cmp(sort_fields);\n";
    }

    void emit_filter(std::ostringstream& out, const std::string& type,
                     const Condition& cond) {
        out << "    // " << type << " filter\n";
        out << "    records.erase(std::remove_if(records.begin(), records.end(),\n";
        out << "        [](const Record& rec) {\n";
        out << "            return ";
        if (type == "omit") {
            // OMIT removes matching records
            emit_condition(out, cond);
        } else {
            // INCLUDE keeps matching, removes non-matching
            out << "!";
            emit_condition_parens(out, cond);
        }
        out << ";\n";
        out << "        }), records.end());\n";
    }

    void emit_condition(std::ostringstream& out, const Condition& cond) {
        std::visit([&out, this](const auto& c) {
            using T = std::decay_t<decltype(c)>;
            if constexpr (std::is_same_v<T, SimpleCondition>) {
                emit_simple_cond(out, c);
            } else {
                emit_compound_cond(out, c);
            }
        }, cond);
    }

    void emit_condition_parens(std::ostringstream& out, const Condition& cond) {
        out << "(";
        emit_condition(out, cond);
        out << ")";
    }

    void emit_simple_cond(std::ostringstream& out, const SimpleCondition& sc) {
        out << format_decoder(sc.format) << "(extract_field(rec, "
            << sc.position << ", " << sc.length << "), \""
            << sc.value << "\") " << condop_to_cpp(sc.op) << " 0";
    }

    void emit_compound_cond(std::ostringstream& out, const CompoundCondition& cc) {
        for (size_t i = 0; i < cc.children.size(); ++i) {
            if (i > 0) {
                out << (cc.logic == LogicOp::AND ? " && " : " || ");
            }
            emit_condition_parens(out, cc.children[i]);
        }
    }

    static std::string condop_to_cpp(CondOp op) {
        switch (op) {
            case CondOp::EQ: return "==";
            case CondOp::NE: return "!=";
            case CondOp::GT: return ">";
            case CondOp::GE: return ">=";
            case CondOp::LT: return "<";
            case CondOp::LE: return "<=";
            default: return "==";
        }
    }

    void emit_sort_call(std::ostringstream& out, bool stable) {
        if (stable) {
            out << "    std::stable_sort(records.begin(), records.end(), cmp);\n";
        } else {
            out << "    std::sort(records.begin(), records.end(), cmp);\n";
        }
    }

    void emit_sum(std::ostringstream& out, const std::vector<SumField>& fields) {
        out << "    // SUM: summarize duplicate key records\n";
        out << "    std::vector<SumField> sum_fields = {\n";
        for (size_t i = 0; i < fields.size(); ++i) {
            const auto& f = fields[i];
            out << "        {" << f.position << ", " << f.length
                << ", FieldFormat::" << parser::format_to_string(f.format) << "}";
            if (i + 1 < fields.size()) out << ",";
            out << "\n";
        }
        out << "    };\n";
        out << "    SumAggregator::summarize(records, sort_fields, sum_fields);\n";
    }

    void emit_reformat(std::ostringstream& out, const std::string& label,
                       const RecordReformat& rf) {
        out << "    // " << label << ": "
            << (rf.type == RecordReformat::Type::BUILD ? "BUILD" : "OVERLAY") << "\n";
        out << "    for (auto& rec : records) {\n";
        if (rf.type == RecordReformat::Type::BUILD) {
            out << "        std::string new_rec;\n";
            for (const auto& fr : rf.fields) {
                if (!fr.literal.empty()) {
                    out << "        new_rec += \"" << fr.literal << "\";\n";
                } else {
                    out << "        new_rec += std::string(extract_field(rec, "
                        << fr.position << ", " << fr.length << "));\n";
                }
            }
            out << "        rec = new_rec;\n";
        } else {
            out << "        // OVERLAY in-place modifications\n";
            for (const auto& fr : rf.fields) {
                if (!fr.literal.empty() && fr.position > 0) {
                    out << "        rec.replace(" << (fr.position - 1)
                        << ", " << fr.literal.size() << ", \"" << fr.literal << "\");\n";
                }
            }
        }
        out << "    }\n";
    }
};

// ================================================================
// Convenience: translate control card to C++ source string
// ================================================================
inline std::string translate(std::string_view control_card) {
    CodeGenerator gen;
    return gen.generate(control_card);
}

// Generate typed comparator for a specific format (compile-time dispatch)
template <FieldFormat Fmt>
inline std::string typed_comparator(int pos, int len) {
    CodeGenerator gen;
    return gen.generate_typed_comparator<Fmt>(pos, len);
}

}}} // namespace lazarus::sort::translator

#endif // LAZARUS_SORT_TRANSLATOR_H
