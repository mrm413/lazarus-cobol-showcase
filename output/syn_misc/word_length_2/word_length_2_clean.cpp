/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: word_length_2_preprocessed.cbl
 * Program: prog3
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
std::string COUNTER;
int RETURN_CODE = 0;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_000000000000000000000000000000000000000000000000000000000000();
void P_20000000000000000000000000000000000000000000000000000000002();
void P_00000000000000000000000000000000000000000000000000000000000();
void P_100000000000000000000000000000000000000000000000000000000001();

void P_000000000000000000000000000000000000000000000000000000000000() {
    COUNTER = to_string(to_int(COUNTER) + 1);
    if (to_int(COUNTER) == 1) {
        P_00000000000000000000000000000000000000000000000000000000000();
    } else if (to_int(COUNTER) == 2) {
        P_000000000000000000000000000000000000000000000000000000000000();
    } else if (to_int(COUNTER) == 3) {
        COUNTER[0] = '0';
    }
}

void P_20000000000000000000000000000000000000000000000000000000002() {
    COUNTER = to_string(to_int(COUNTER) + 1);
    if (to_int(COUNTER) == 1) {
        P_100000000000000000000000000000000000000000000000000000000001();
    } else if (to_int(COUNTER) == 2) {
        P_20000000000000000000000000000000000000000000000000000000002();
    } else if (to_int(COUNTER) == 3) {
        COUNTER[0] = '0';
    }
    return;
}

void P_00000000000000000000000000000000000000000000000000000000000() {
    P_000000000000000000000000000000000000000000000000000000000000();
}

void P_100000000000000000000000000000000000000000000000000000000001() {
    P_20000000000000000000000000000000000000000000000000000000002();
}

int main() {
    P_000000000000000000000000000000000000000000000000000000000000();
    return RETURN_CODE;
}