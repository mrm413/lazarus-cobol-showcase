/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: add_corresponding_preprocessed.cbl
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
std::string FIELD_A;
std::string FIELD_B;
std::string FIELD_C;
std::string FIELD_D;
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
    // ADD to FIELD_A
    // ADD to FIELD_C
    if (to_int(FIELD_D) != 1) {
        std::cout << "BAD FIELD-A " << FIELD_A << std::endl;
    }
    if (false /* TODO: cob_cmp_s32 (FIELD_D + 1, 2LL) != 0 */) {
        std::cout << "BAD FIELD-B " << FIELD_B << std::endl;
    }
    if (to_int(FIELD_D) != 3) {
        std::cout << "BAD FIELD-C " << FIELD_C << std::endl;
    }
    if (to_int(FIELD_D) != 0) {
        std::cout << "BAD FIELD-D " << FIELD_D << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}