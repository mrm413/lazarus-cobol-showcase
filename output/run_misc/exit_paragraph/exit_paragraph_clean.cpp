/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: exit_paragraph_preprocessed.cbl
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
std::string INDVAL;
int RETURN_CODE = 0;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_A01();
void P_A02();

void P_A01() {
    INDVAL = "0001";
    while (true) {
        if (to_int(INDVAL) > 10) break;
        if (to_int(INDVAL) > 2) {
        }
        INDVAL = to_string(to_int(INDVAL) + 1);
    }
}

void P_A02() {
    if (to_int(INDVAL) != 3) {
        std::cout << INDVAL << std::endl;
    }
    return;
}

int main() {
    P_A01();
    return RETURN_CODE;
}