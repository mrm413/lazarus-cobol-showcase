/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: length_of_preprocessed.cbl
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
std::string I;
std::string L;
int RETURN_CODE = 0;
std::string TSTDISP;
std::string X;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string Y;

// Forward declarations
void P_VARME();

void P_VARME() {
    L = "5";
    if (to_int(L) != 5) {
        std::cout << "Length '00128'" << L << std::endl;
    }
    L = "1";
    if (to_int(L) != 1) {
        std::cout << "Length x'a0'" << L << std::endl;
    }
    L = "3";
    if (to_int(L) != 3) {
        std::cout << "Length z'a0'" << L << std::endl;
    }
    L = "2";
    if (to_int(L) != 2) {
        std::cout << "Length 1 " << L << std::endl;
    }
    L = "2";
    if (to_int(L) != 2) {
        std::cout << "Length 1a " << L << "2" << std::endl;
    }
    L = "2";
    if (to_int(L) != 2) {
        std::cout << "Length 2 " << L << std::endl;
    }
    if (to_int(L) != 2) {
        std::cout << "Length 2a " << L << "2" << std::endl;
    }
    L = "2";
    if (to_int(L) != 2) {
        std::cout << "Length 3 " << L << std::endl;
    }
    L = "2";
    if (to_int(L) != 2) {
        std::cout << "Length 3a " << L << "2" << std::endl;
    }
    L = "0";
    for (int n0 = 6; n0 > 0; n0--) {
        L = to_string(to_int(L) + 1);
    }
    for (int n1 = 6; n1 > 0; n1--) {
        L = to_string(to_int(L) + 1);
    }
    if (to_int(L) != 12) {
        std::cout << "Length 6 " << L << std::endl;
    }
    L = "0";
    I = "02";
    while (true) {
        if (to_int(I) > 6) break;
        P_VARME();
        I = to_string(to_int(I) + 2);
    }
    if (to_int(L) != 3 || to_int(I) != 8) {
        std::cout << "Length 7 " << L << " - " << I << std::endl;
    }
    L = "0";
    I = "02";
    while (true) {
        if (to_int(I) > 6) break;
        P_VARME();
        I = to_string(to_int(I) + 2);
    }
    if (to_int(L) != 3 || to_int(I) != 8) {
        std::cout << "Length 7a " << L << " - " << I << std::endl;
    }
    L = "33";
    if (to_int(L) != 33) {
        std::cout << "Length 8a " << L << std::endl;
    }
    L = "43";
    if (to_int(L) != 43) {
        std::cout << "Length 8b " << L << std::endl;
    }
    L = "33";
    if (to_int(L) != 33) {
        std::cout << "Length 8c " << L << std::endl;
    }
    L = "63";
    if (to_int(L) != 63) {
        std::cout << "Length 8d " << L << std::endl;
    }
    std::cout << "165000" << std::endl;
    return;
    L = to_string(to_int(L) + 1);
}

int main() {
    P_VARME();
    return RETURN_CODE;
}