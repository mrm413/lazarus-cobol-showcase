/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: duplicate_initiate_preprocessed.cbl
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
std::string foo;
std::string hedpos;
std::string rp_detail;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    // OPEN REPORT-FILE (OUTPUT)
    if (file_status != "EXCEPTION_0") {
    }
    // UNHANDLED: goto rwexit_1;
    // UNHANDLED: rwmove_1: ;
    // UNHANDLED: rwfoot_1: ;
    // UNHANDLED: rwexit_1: ;
    // UNHANDLED: r_RP.code_is_present = 0;
    // UNHANDLED: cob_report_initiate (&r_RP);
    foo = "hello";
    // UNHANDLED: cob_report_generate (&r_RP, &rl_21, 0);
    // UNHANDLED: r_RP.code_is_present = 0;
    // UNHANDLED: cob_report_initiate (&r_RP);
    foo = "goodbye";
    // UNHANDLED: cob_report_generate (&r_RP, &rl_21, 0);
    // UNHANDLED: cob_report_terminate (&r_RP, 0);
    // CLOSE REPORT-FILE
    if (file_status != "EXCEPTION_0") {
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}