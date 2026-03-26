/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: occurs_on_level_01_preprocessed.cbl
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
std::string X;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string X_ALL;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    X[0] = ' ';
    X[2] = ' ';
    X[4] = ' ';
    X[6] = ' ';
    X[1] = '0';
    X[3] = '0';
    X[5] = '0';
    X[7] = '0';
    if (X_ALL.substr(0, 10) != " 0 0 0 0AA") {
        std::cout << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}