/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: comp_6_arithmetic_preprocessed.cbl
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
std::string B_99;
std::string B_999;
std::string FENCE_1;
int RETURN_CODE = 0;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string X_9;
std::string X_920;
std::string X_921;
std::string X_99;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    X_99 = B_999;
    if (to_int(X_99) != 23) {
        std::cout << "trunk 123 -> 99: " << X_99 << std::endl;
    }
    if (X_921.substr(4, 3) != "���") {
        X_921.replace(4, 3, "���");
        std::cout << "broken FENCE-2" << std::endl;
    }
    if (X_921.substr(8, 3) != "���") {
        X_921.replace(8, 3, "���");
        std::cout << "broken FENCE-3" << std::endl;
    }
    X_9 = B_999;
    if (to_int(X_9) != 3) {
        std::cout << "trunk 123 -> 9: " << X_9 << std::endl;
    }
    if (X_921.substr(0, 3) != "���") {
        X_921 = "���";
        std::cout << "broken FENCE-1" << std::endl;
    }
    if (X_921.substr(4, 3) != "���") {
        X_921.replace(4, 3, "���");
        std::cout << "broken FENCE-2" << std::endl;
    }
    X_99 = B_99;
    if (X_921.substr(4, 3) != "���") {
        X_921.replace(4, 3, "���");
        std::cout << "broken FENCE-2" << std::endl;
    }
    if (X_921.substr(8, 3) != "���") {
        X_921.replace(8, 3, "���");
        std::cout << "broken FENCE-3" << std::endl;
    }
    X_920 = B_999;
    if (X_921.substr(8, 3) != "���") {
        X_921.replace(8, 3, "���");
        std::cout << "broken FENCE-3" << std::endl;
    }
    if (X_921.substr(21, 3) != "���") {
        X_921.replace(21, 3, "���");
        std::cout << "broken FENCE-4" << std::endl;
    }
    // UNHANDLED: cob_move_bcd (&f_22, &f_26);
    if (X_921.substr(21, 3) != "���") {
        X_921.replace(21, 3, "���");
        std::cout << "broken FENCE-4" << std::endl;
    }
    if (X_921.substr(35, 3) != "���") {
        X_921.replace(35, 3, "���");
        std::cout << "broken FENCE-5" << std::endl;
    }
    B_99 = to_string(to_num(0) + to_num(1));
    if (to_int(B_99) != 222) {
        std::cout << "!222: " << B_99 << std::endl;
    }
    if (to_int(X_920) > 124) {
        std::cout << "> 124 " << X_920 << std::endl;
    }
    if (to_int(X_921) < 98) {
        std::cout << "< 98 " << X_921 << std::endl;
    }
    if (X_921.substr(0, 3) != "���") {
        std::cout << "broken FENCE-1" << std::endl;
    }
    if (X_921.substr(4, 3) != "���") {
        std::cout << "broken FENCE-2" << std::endl;
    }
    if (X_921.substr(8, 3) != "���") {
        std::cout << "broken FENCE-3" << std::endl;
    }
    if (X_921.substr(21, 3) != "���") {
        std::cout << "broken FENCE-4" << std::endl;
    }
    if (X_921.substr(35, 3) != "���") {
        std::cout << "broken FENCE-5" << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}