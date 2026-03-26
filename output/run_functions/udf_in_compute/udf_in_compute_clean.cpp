/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: udf_in_compute_preprocessed.cbl
 * Program: prog
 */

#include <iostream>
#include <string>
#include <cstdlib>
#include <cmath>

// Helper functions
inline int to_int(const std::string& s) {
    try { return std::stoi(s); }
    catch (...) { return 0; }
}

inline double to_num(const std::string& s) {
    try { return std::stod(s); }
    catch (...) { return 0.0; }
}
inline double to_num(int n) { return static_cast<double>(n); }
inline double to_num(long long n) { return static_cast<double>(n); }

inline std::string to_string(int n) {
    return std::to_string(n);
}

inline std::string to_string(double n) {
    return std::to_string(static_cast<long long>(n));
}

std::string current_date() { return "20260317"; }
std::string current_time() { return "120000"; }

bool file_exception = false;
std::string file_status = "00";
int n0 = 0, n1 = 0, n2 = 0, n3 = 0, n4 = 0;  // loop counters

// Working Storage variables
int RETURN_CODE = 0;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string num;
std::string x;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    num = "100";
    if (false /* TODO: cob_reference_count */) {
        // UNHANDLED: cob_reference_count--;
    }
    // UNHANDLED: cob_free (frame_stack);
    // UNHANDLED: return COB_SET_DATA (f_17, b_17);
    RETURN_CODE = 0;
    }
    // UNHANDLED: return prog_ (0);
    }
    if (false /* TODO: initialized == 0 */) {
        // UNHANDLED: goto P_initialize;
    }
    // UNHANDLED: cob_reference_count++;
    x = to_string(to_num(0) + to_num(1));
    std::cout << x << std::endl;
    if (false /* TODO: module->module_active */) {
    }
    if (false /* TODO: cob_reference_count */) {
        // UNHANDLED: cob_reference_count--;
    }
    if (false /* TODO: cob_dyn_0 */) {
        if (false /* TODO: cob_dyn_0->data */) {
            // UNHANDLED: cob_free (cob_dyn_0->data);
        }
        // UNHANDLED: cob_free (cob_dyn_0);
        // UNHANDLED: cob_dyn_0 = NULL;
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}