/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: sort_table_3_1_preprocessed.cbl
 * Program: prog2
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
std::string CNT1;
std::string K;
int RETURN_CODE = 0;
std::string TAB1_NR;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_A();

void P_A() {
    std::cout << std::endl;
    K = "01";
    while (true) {
        if (to_int(K) > 4) break;
        K = to_string(to_int(K) + 1);
    }
    // UNHANDLED: cob_table_sort_init (1, 0);
    // UNHANDLED: cob_table_sort_init_key (&f_25, 1, 0);
    // UNHANDLED: cob_table_sort (COB_SET_FLD (f0, 7, b_23, &a_2), (*(unsigned int *)(b_18)));
    K = "01";
    while (true) {
        if (to_int(K) > 4) break;
        std::cout << std::endl;
        K = to_string(to_int(K) + 1);
    }
    return;
}

int main() {
    P_A();
    return RETURN_CODE;
}