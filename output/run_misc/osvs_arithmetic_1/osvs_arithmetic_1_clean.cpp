/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: osvs_arithmetic_1_preprocessed.cbl
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
std::string NUMV1;
std::string NUM_A;
std::string NUM_B;
std::string NUM_C;
std::string PICX;
std::string RSLT;
std::string RSLTV1;
std::string RSLTV2;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    RSLT = to_string(to_num(0) + to_num(1));
    std::cout << "Simple Compute  RSLT IS " << RSLT << std::endl;
    RSLT = to_string(to_num(0) * to_num(1));
    std::cout << "Single Variable RSLT IS " << RSLT << std::endl;
    RSLTV2 = to_string(to_num(0) * to_num(1));
    RSLT = to_string(to_num(0) + to_num(1));
    std::cout << "Compute  RSLT    IS " << RSLT << std::endl;
    std::cout << "Compute  RSLTv99 IS " << RSLTV2 << std::endl;
    RSLTV1 = to_string(to_num(0) * to_num(1));
    RSLT = to_string(to_num(0) + to_num(1));
    std::cout << "Compute  RSLT    IS " << RSLT << std::endl;
    std::cout << "Compute  RSLTv9  IS " << RSLTV1 << std::endl;
    RSLT = std::string(3, '0');
    // ADD to RSLT
    std::cout << "Add      RSLT    IS " << RSLT << std::endl;
    RSLT = std::string(3, '0');
    RSLT = to_string(to_num(0) + to_num(1));
    std::cout << "Add      RSLT    IS " << RSLT << std::endl;
    // UNHANDLED: cob_sub (&f_6, &f_3, 0);
    std::cout << "Subtract RSLT    IS " << RSLT << std::endl;
    RSLT = to_string(to_num(0) - to_num(1));
    std::cout << "Subtract RSLT    IS " << RSLT << std::endl;
    RSLT = std::string(3, '0');
    RSLTV1 = to_string(to_num(0) + to_num(1));
    std::cout << "Add      RSLTv9  IS " << RSLTV1 << std::endl;
    RSLT = to_string(to_num(0) * to_num(1));
    std::cout << "Multiply RSLT    IS " << RSLT << std::endl;
    NUM_C = to_string(to_num(NUM_C) * to_num(RSLT));
    std::cout << "Multiply RSLT    IS " << RSLT << std::endl;
    RSLT = to_string(to_num(0) / to_num(1));
    std::cout << "Divide   RSLT    IS " << RSLT << std::endl;
    RSLTV1 = to_string(to_num(0) / to_num(1));
    std::cout << "Divide   RSLTv9  IS " << RSLTV1 << std::endl;
    RSLT = to_string(to_num(0) / to_num(1));
    std::cout << "Divide   RSLT    IS " << RSLT << std::endl;
    RSLTV1 = to_string(to_num(0) * to_num(1));
    RSLT = to_string(to_num(0) + to_num(1));
    std::cout << "Simple   RSLT    IS " << RSLT << " RSLTv9  IS " << RSLTV1 << std::endl;
    RSLTV1 = to_string(to_num(0) * to_num(1));
    RSLT = to_string(to_num(0) + to_num(1));
    std::cout << "Complex  RSLT    IS " << RSLT << " RSLTv9  IS " << RSLTV1 << std::endl;
    RSLTV1 = to_string(to_num(0) * to_num(1));
    RSLT = to_string(to_num(0) + to_num(1));
    std::cout << "Reduced  RSLT    IS " << RSLT << " RSLTv9  IS " << RSLTV1 << std::endl;
    NUMV1 = NUM_A;
    if (false /* decimal_cmp == 0 */) {
        std::cout << "Not Using ARITHMETIC-OSVS" << std::endl;
    } else {
        std::cout << "Using ARITHMETIC-OSVS" << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}