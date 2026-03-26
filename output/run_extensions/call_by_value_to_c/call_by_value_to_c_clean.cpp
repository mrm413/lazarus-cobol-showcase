/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: call_by_value_to_c_preprocessed.cbl
 * Program: caller
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
std::string ADR_BUFFER;
std::string ADR_PTR;
std::string BIG_BUFF;
std::string CALL_NAME;
int RETURN_CODE = 0;
std::string SOME_FILL;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    CALL_NAME = "callee32";
    ADR_BUFFER = "12345678";
    ADR_BUFFER = "-42";
    if (false /* TODO: call_callee32.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(cob_u32_t))call_callee32.funcint) ((cob_u32_t)((*(int *)(b_18 + 33))));
    if (false /* TODO: call_callee64.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(cob_u64_t))call_callee64.funcint) ((cob_u64_t)((*(cob_s64_ptr)(b_22))));
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}