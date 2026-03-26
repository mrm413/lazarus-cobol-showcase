/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: report_with_occurs_preprocessed.cbl
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
std::string HEADING_LINE;
int RETURN_CODE = 0;
std::string RP_DTL1;
std::string RP_DTL3;
std::string RP_DTL4;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string rp_dtl2;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    // OPEN RP-FILE (OUTPUT)
    if (file_status != "EXCEPTION_0") {
    }
    // UNHANDLED: goto rwexit_1;
    // UNHANDLED: rwmove_1: ;
    // UNHANDLED: rwfoot_1: ;
    // UNHANDLED: rwexit_1: ;
    // UNHANDLED: r_RP.code_is_present = 0;
    // UNHANDLED: cob_report_initiate (&r_RP);
    RP_DTL1 = "100";
    // UNHANDLED: memcpy (b_24 + 3 * 1, "100", 3);
    // UNHANDLED: memcpy (b_24 + 3 * 2, "100", 3);
    RP_DTL1.replace(12, 4, "<1>");
    // UNHANDLED: cob_report_generate (&r_RP, &rl_24, 0);
    rp_dtl2 = std::string(13, static_cast<char>(42));
    rp_dtl2 = "Tag1";
    // UNHANDLED: memcpy (b_27 + 13 * 1, "Tag1 ", 5);
    // UNHANDLED: memcpy (b_27 + 13 * 2, "Tag1 ", 5);
    rp_dtl2.replace(6, 5, "Tag2");
    // UNHANDLED: memcpy (b_27 + 19 * 1, "Tag2 ", 5);
    // UNHANDLED: memcpy (b_27 + 19 * 2, "Tag2 ", 5);
    // UNHANDLED: cob_report_generate (&r_RP, &rl_27, 0);
    RP_DTL3 = "200";
    // UNHANDLED: memcpy (b_33 + 3 * 1, "200", 3);
    // UNHANDLED: memcpy (b_33 + 3 * 2, "200", 3);
    // UNHANDLED: memcpy (b_33 + 3 * 3, "200", 3);
    RP_DTL3.replace(16, 4, "<3>");
    // UNHANDLED: cob_report_generate (&r_RP, &rl_33, 0);
    RP_DTL4 = "400";
    RP_DTL4.replace(11, 3, "401");
    // UNHANDLED: memcpy (b_36 + 14 * 1, "402", 3);
    // UNHANDLED: memcpy (b_36 + 14 * 2, "403", 3);
    RP_DTL4.replace(23, 4, "<4>");
    // UNHANDLED: cob_report_generate (&r_RP, &rl_36, 0);
    // UNHANDLED: cob_report_terminate (&r_RP, 0);
    // CLOSE RP-FILE
    if (file_status != "EXCEPTION_0") {
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}