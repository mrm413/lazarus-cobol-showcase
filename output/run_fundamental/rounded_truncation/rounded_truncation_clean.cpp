/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: rounded_truncation_preprocessed.cbl
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
std::string M;
std::string N;
std::string O;
std::string P;
std::string Q;
std::string R;
int RETURN_CODE = 0;
std::string S;
std::string T;
std::string U;
std::string V;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    M = to_string(to_num(0) + to_num(1));
    N = to_string(to_num(0) + to_num(1));
    O = to_string(to_num(0) + to_num(1));
    P = to_string(to_num(0) + to_num(1));
    Q = to_string(to_num(0) + to_num(1));
    R = to_string(to_num(0) + to_num(1));
    S = to_string(to_num(0) + to_num(1));
    T = to_string(to_num(0) + to_num(1));
    U = to_string(to_num(0) + to_num(1));
    V = to_string(to_num(0) + to_num(1));
    std::cout << M << " " << N << " " << O << " " << P << " " << Q << " " << R << " " << S << " " << T << " " << U << " " << V << std::endl;
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}