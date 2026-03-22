/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: comp_6_comparison_preprocessed.cbl
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
std::string U1;
std::string U1_32;
std::string U1_32D;
std::string U2;
std::string U2_32;
std::string U2_32D;
std::string U3;
std::string U3_32D;
std::string U4;
std::string U4_32D;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();
void P_DO_CHECK();

void P_MAIN() {
    P_DO_CHECK();
}

void P_DO_CHECK() {
    if (U1.substr(0, 1) != U2.substr(0, 1)) {
        if (to_int(VAR_27) == 0) {
            std::cout << "U1 <> U2" << std::endl;
        }
    }
    if (U3.substr(0, 2) != U4.substr(0, 2)) {
        if (to_int(VAR_27) == 0) {
            std::cout << "U3 <> U4" << std::endl;
        }
    }
    if (false /* cob_cmp_packed >= 0 */) {
        if (to_int(VAR_27) == 0) {
            std::cout << "U1 >= U3" << std::endl;
        }
    }
    if (U1_32.substr(0, 16) != U2_32.substr(0, 16)) {
        if (to_int(VAR_27) == 0) {
            std::cout << "U1-32 <> U2-32" << std::endl;
        }
    }
    if (false /* bcd_cmp >= 0 */) {
        if (to_int(VAR_27) == 0) {
            std::cout << "U1-32 >= U1-32D" << std::endl;
        }
    }
    if (U1_32D.substr(0, 15) != U2_32D.substr(0, 15)) {
        if (to_int(VAR_27) == 0) {
            std::cout << "U1-32D <> U2-32D" << std::endl;
        }
    }
    if (false /* bcd_cmp <= 0 */) {
        if (to_int(VAR_27) == 0) {
            std::cout << "U1-32 <= U3" << std::endl;
        }
    }
    if (false /* bcd_cmp != 0 */) {
        if (to_int(VAR_27) == 0) {
            std::cout << "U2-32D <> U3-32D" << std::endl;
        }
    }
    if (false /* bcd_cmp <= 0 */) {
        if (to_int(VAR_27) == 0) {
            std::cout << "U2-32D <= U4-32D" << std::endl;
        }
    }
    if (false /* bcd_cmp >= 0 */) {
        if (to_int(VAR_27) == 0) {
            std::cout << "U4-32D >= U2-32D" << std::endl;
        }
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}