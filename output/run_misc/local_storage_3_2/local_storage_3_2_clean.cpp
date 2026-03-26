/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: local_storage_3_2_preprocessed.cbl
 * Program: callee
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
std::string DISP_IDX;
std::string DISP_VALS;
int RETURN_CODE = 0;
std::string WRK_IDX;
std::string WRK_VAR;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    std::cout << std::endl;
    // ADD operation
    // WRK_IDX = (b_22 + 1);
    // UNHANDLED: cob_set_int (&f_19, b_22);
    DISP_IDX[0] = WRK_VAR[0];
    std::cout << DISP_VALS << std::endl;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}