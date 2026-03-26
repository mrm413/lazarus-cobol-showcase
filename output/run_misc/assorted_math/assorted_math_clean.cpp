/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: assorted_math_preprocessed.cbl
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
std::string m;
std::string n;
std::string variable;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    std::cout << std::endl;
    std::cout << "-" << std::endl;
    std::cout << std::endl;
    if (to_int(variable) == 1 && to_int(variable) == 2) {
        std::cout << "succeeded" << std::endl;
    } else {
        std::cout << "failed" << std::endl;
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}