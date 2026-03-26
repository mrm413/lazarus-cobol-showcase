/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: function_length_preprocessed.cbl
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
std::string N;
int RETURN_CODE = 0;
std::string TEST_FLD;
std::string T_ENTRIES;
std::string X;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    TEST_FLD = "8";
    if (to_int(TEST_FLD) != 8) {
        std::cout << "LENGTH \\" << TEST_FLD << std::endl;
    }
    TEST_FLD = "9";
    if (to_int(TEST_FLD) != 9) {
        std::cout << "LENGTH N(9) wrong: " << TEST_FLD << std::endl;
    }
    TEST_FLD = "5";
    if (to_int(TEST_FLD) != 5) {
        std::cout << "LENGTH \\" << TEST_FLD << std::endl;
    }
    TEST_FLD = "1";
    if (to_int(TEST_FLD) != 1) {
        std::cout << "LENGTH x\\" << TEST_FLD << std::endl;
    }
    TEST_FLD = "3";
    if (to_int(TEST_FLD) != 3) {
        std::cout << "LENGTH z\\" << TEST_FLD << std::endl;
    }
    TEST_FLD = "2";
    if (to_int(TEST_FLD) != 2) {
        std::cout << "LENGTH n\\" << TEST_FLD << std::endl;
    }
    T_ENTRIES = "10";
    // MOVE to computed field
    if (to_int(TEST_FLD) != 12) {
        std::cout << "LENGTH TEST-TAB (10 entries): " << TEST_FLD << std::endl;
    }
    T_ENTRIES = "01";
    // MOVE to computed field
    if (to_int(TEST_FLD) != 3) {
        std::cout << "LENGTH TEST-TAB (1 entry): " << TEST_FLD << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}