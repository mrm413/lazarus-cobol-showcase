/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: function_acos_preprocessed.cbl
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
std::string P;
int RETURN_CODE = 0;
std::string S;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string Z;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    // UNHANDLED: cob_move (cob_intr_acos ((cob_field *)&c_1), &f_17);
    if (to_int(Z) != to_int(c_2)) {
        std::cout << "DISPLAY: " << Z << std::endl;
    }
    // UNHANDLED: cob_move (cob_intr_acos ((cob_field *)&c_1), &f_18);
    if (to_int(P) != to_int(c_2)) {
        std::cout << "PACKED: " << P << std::endl;
    }
    // UNHANDLED: cob_move (cob_intr_acos ((cob_field *)&c_1), &f_19);
    if (to_int(S) != to_int(c_2)) {
        std::cout << "COMP-6: " << S << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}