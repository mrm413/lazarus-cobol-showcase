/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: perform_type_osvs_preprocessed.cbl
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
std::string MYOCC;

// Forward declarations
void P_A01();
void P_B01();
void P_B02();
void P_B99();
void P_ASTART();
void P_BTEST();

void P_A01() {
    P_BTEST();
    if (to_int(RETURN_CODE) != 2) {
        std::cout << MYOCC << std::endl;
    }
    return;
}

void P_B01() {
    MYOCC = "1";
    while (true) {
        if (to_int(RETURN_CODE) > 5) break;
        P_B02();
        // ADD operation
    }
}

void P_B02() {
    if (to_int(RETURN_CODE) > 1) {
    }
}

void P_B99() {
}

void P_ASTART() {
    P_A01();
}

void P_BTEST() {
    P_B01();
    P_B02();
    P_B99();
}

int main() {
    P_A01();
    return RETURN_CODE;
}