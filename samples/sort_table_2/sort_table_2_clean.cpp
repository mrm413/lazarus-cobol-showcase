/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: sort_table_2_preprocessed.cbl
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
std::string CNT1;
std::string CNT2;
std::string CNT3;
std::string K;
int RETURN_CODE = 0;
std::string ROW2;
std::string TAB1;
std::string TAB1_NR;
std::string TAB2_NR;
std::string TAB3_DATA;
std::string TAB3_NR;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_A();

void P_A() {
    K = "01";
    while (true) {
        if (to_int(K) > 4) break;
        K = to_string(to_int(K) + 1);
    }
    TAB3_DATA.replace(4, 2, "01");
    // UNHANDLED: memcpy (b_26 + 20 * 7, "01", 2);
    // UNHANDLED: memcpy (b_26 + 20 * 3, "01", 2);
    // UNHANDLED: memcpy (b_26 + 20 * 1, "06", 2);
    // UNHANDLED: memcpy (b_26 + 20 * 2, "05", 2);
    // UNHANDLED: memcpy (b_26 + 20 * 8, "05", 2);
    // UNHANDLED: memcpy (b_26 + 20 * 4, "02", 2);
    // UNHANDLED: memcpy (b_26 + 20 * 9, "02", 2);
    // UNHANDLED: memcpy (b_26 + 20 * 5, "04", 2);
    // UNHANDLED: memcpy (b_26 + 20 * 6, "03", 2);
    TAB3_DATA.replace(8, 5, "abcde");
    // UNHANDLED: memcpy (b_26 + 24 * 1, "AbCde", 5);
    // UNHANDLED: memcpy (b_26 + 24 * 2, "abcde", 5);
    // UNHANDLED: memcpy (b_26 + 24 * 3, "zyx  ", 5);
    // UNHANDLED: memcpy (b_26 + 24 * 4, "12345", 5);
    // UNHANDLED: memcpy (b_26 + 24 * 5, "zyx  ", 5);
    // UNHANDLED: memcpy (b_26 + 24 * 6, "abcde", 5);
    // UNHANDLED: memcpy (b_26 + 24 * 7, "AbCde", 5);
    // UNHANDLED: memcpy (b_26 + 24 * 8, "abc  ", 5);
    // UNHANDLED: memcpy (b_26 + 24 * 9, "12346", 5);
    TAB3_DATA.replace(15, 5, "day");
    // UNHANDLED: memcpy (b_26 + 31 * 1, "The  ", 5);
    // UNHANDLED: memcpy (b_26 + 31 * 2, "eats ", 5);
    // UNHANDLED: memcpy (b_26 + 31 * 3, ".    ", 5);
    // UNHANDLED: memcpy (b_26 + 31 * 4, "mooos", 5);
    // UNHANDLED: memcpy (b_26 + 31 * 5, "grass", 5);
    // UNHANDLED: memcpy (b_26 + 31 * 6, "and  ", 5);
    // UNHANDLED: memcpy (b_26 + 31 * 7, "whole", 5);
    // UNHANDLED: memcpy (b_26 + 31 * 8, "cow  ", 5);
    // UNHANDLED: memcpy (b_26 + 31 * 9, "the  ", 5);
    // UNHANDLED: cob_table_sort_init (1, 0);
    // UNHANDLED: cob_table_sort_init_key (&f_21, 1, 0);
    // UNHANDLED: cob_table_sort (COB_SET_FLD (f0, 2, b_19, &a_2), (*(unsigned int *)(b_18)));
    // UNHANDLED: cob_table_sort_init (1, 0);
    // UNHANDLED: cob_table_sort_init_key (&f_25, 1, 0);
    // UNHANDLED: cob_table_sort (COB_SET_FLD (f0, 2, b_22 + 4, &a_2), (*(unsigned int *)(b_22)));
    std::cout << "SINGLE TABLE" << std::endl;
    K = "01";
    while (true) {
        if (to_int(K) > 4) break;
        std::cout << std::endl;
        K = to_string(to_int(K) + 1);
    }
    std::cout << "LOWER LEVEL TABLE" << std::endl;
    K = "01";
    while (true) {
        if (to_int(K) > 4) break;
        std::cout << std::endl;
        K = to_string(to_int(K) + 1);
    }
    // UNHANDLED: cob_table_sort_init (2, 0);
    // UNHANDLED: cob_table_sort_init_key (&f_29, 1, 0);
    // UNHANDLED: cob_table_sort_init_key (&f_31, 0, 4);
    // UNHANDLED: cob_table_sort (COB_SET_FLD (f0, 16, b_26 + 4, &a_2), (*(unsigned int *)(b_26)));
    std::cout << "MULTY KEY SORT" << std::endl;
    K = "01";
    while (true) {
        if (to_int(K) > 10) break;
        std::cout << std::endl;
        K = to_string(to_int(K) + 1);
    }
    return;
}

int main() {
    P_A();
    return RETURN_CODE;
}