/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: sort_table_preprocessed.cbl
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
std::string G;
std::string TBL;
std::string X;
std::string Y;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    // UNHANDLED: cob_table_sort_init (1, 0);
    // UNHANDLED: cob_table_sort_init_key (&f_3, 0, 0);
    // UNHANDLED: cob_table_sort (COB_SET_FLD (f0, 2, b_1, &a_2), 5);
    if (Y.substr(0, 10) != "a3b2c5d4e1") {
        std::cout << G << std::endl;
    }
    // UNHANDLED: cob_table_sort_init (1, 0);
    // UNHANDLED: cob_table_sort_init_key (&f_4, 1, 1);
    // UNHANDLED: cob_table_sort (COB_SET_FLD (f0, 2, b_1, &a_2), 5);
    if (Y.substr(0, 10) != "c5d4a3b2e1") {
        std::cout << G << std::endl;
    }
    // UNHANDLED: cob_table_sort_init (1, 0);
    // UNHANDLED: cob_table_sort_init_key (&f_2, 0, 0);
    // UNHANDLED: cob_table_sort (COB_SET_FLD (f0, 2, b_1, &a_2), 5);
    if (Y.substr(0, 10) != "a3b2c5d4e1") {
        std::cout << G << std::endl;
    }
    // UNHANDLED: cob_table_sort_init (1, 0);
    // UNHANDLED: cob_table_sort_init_key (&f_2, 1, 0);
    // UNHANDLED: cob_table_sort (COB_SET_FLD (f0, 2, b_1, &a_2), 5);
    if (Y.substr(0, 10) != "e1d4c5b2a3") {
        std::cout << G << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}