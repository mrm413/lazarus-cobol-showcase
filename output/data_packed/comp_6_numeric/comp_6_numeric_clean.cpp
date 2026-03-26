/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: comp_6_numeric_preprocessed.cbl
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
std::string N_3;
std::string N_4;
int RETURN_CODE = 0;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string X_2;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    N_4 = std::string(2, static_cast<char>(0));
    if (false /* TODO: !cob_is_numeric (N_3) */) {
        std::cout << "3 0000 NG" << std::endl;
    }
    if (false /* TODO: !cob_is_numeric (N_4) */) {
        std::cout << "4 0000 NG" << std::endl;
    }
    N_4 = "\000\014";
    if (false /* TODO: cob_is_numeric (N_3) */) {
        std::cout << "3 000c NG" << std::endl;
    }
    if (false /* TODO: cob_is_numeric (N_4) */) {
        std::cout << "4 000c NG" << std::endl;
    }
    N_4 = "\0224";
    if (false /* TODO: !cob_is_numeric (N_3) */) {
        std::cout << "3 1234 NG" << std::endl;
    }
    if (false /* TODO: !cob_is_numeric (N_4) */) {
        std::cout << "4 1234 NG" << std::endl;
    }
    N_4 = std::string(2, static_cast<char>(255));
    if (false /* TODO: cob_is_numeric (N_3) */) {
        std::cout << "3 ffff NG" << std::endl;
    }
    if (false /* TODO: cob_is_numeric (N_4) */) {
        std::cout << "4 ffff NG" << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}