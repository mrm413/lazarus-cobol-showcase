/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: function_variance_preprocessed.cbl
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
std::string Z;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    // UNHANDLED: cob_move (cob_intr_variance (5, (cob_field *)&c_1, (cob_field *)&c_2, (cob_field *)&c_3, (cob_field *)&c_4, (cob_field *)&c_5), &f_1);
    if (to_int(Z) != to_int(c_6)) {
        std::cout << "EXP 54.16 GOT " << Z << std::endl;
    }
    Z = to_string(to_num(0) + to_num(1));
    if (to_int(Z) != to_int(c_10)) {
        std::cout << "EXP 4.66666666 GOT " << Z << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}