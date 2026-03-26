/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: comp_4_truncate_preprocessed.cbl
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
std::string RANDOM_ORIGIN_B;
std::string RANDOM_ORIGIN_D;
std::string RANDOM_TARGET_B;
std::string RANDOM_TARGET_D;
int RETURN_CODE = 0;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    RANDOM_ORIGIN_D = "012";
    RANDOM_TARGET_D = RANDOM_ORIGIN_D;
    RANDOM_ORIGIN_B = "012";
    RANDOM_TARGET_B = RANDOM_ORIGIN_B;
    if (to_int(RANDOM_ORIGIN_D) != to_int(RANDOM_ORIGIN_B)) {
        std::cout << "ORIGIN 0.12 WRONG" << std::endl;
        std::cout << "DISPLAY: " << RANDOM_ORIGIN_D << " !=  BINARY : " << RANDOM_ORIGIN_B << std::endl;
    }
    if (to_int(RANDOM_TARGET_D) != to_int(RANDOM_TARGET_B)) {
        std::cout << "TARGET  .12 WRONG" << std::endl;
        std::cout << "DISPLAY: " << RANDOM_TARGET_D << " !=  BINARY : " << RANDOM_TARGET_B << std::endl;
    } else {
        std::cout << "Ok with " << RANDOM_TARGET_D << " == " << RANDOM_TARGET_B << std::endl;
    }
    RANDOM_ORIGIN_D = "985";
    RANDOM_TARGET_D = RANDOM_ORIGIN_D;
    RANDOM_ORIGIN_B = "985";
    RANDOM_TARGET_B = RANDOM_ORIGIN_B;
    if (to_int(RANDOM_ORIGIN_D) != to_int(RANDOM_ORIGIN_B)) {
        std::cout << "ORIGIN 9.85 WRONG" << std::endl;
        std::cout << "DISPLAY: " << RANDOM_ORIGIN_D << " !=  BINARY : " << RANDOM_ORIGIN_B << std::endl;
    }
    if (to_int(RANDOM_TARGET_D) != to_int(RANDOM_TARGET_B)) {
        std::cout << "TARGET  .85 WRONG" << std::endl;
        std::cout << "DISPLAY: " << RANDOM_TARGET_D << " !=  BINARY : " << RANDOM_TARGET_B << std::endl;
    } else {
        std::cout << "Ok with " << RANDOM_TARGET_D << " == " << RANDOM_TARGET_B << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}