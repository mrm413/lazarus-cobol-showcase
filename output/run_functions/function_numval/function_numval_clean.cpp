/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: function_numval_preprocessed.cbl
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
std::string BAD1;
std::string BAD2;
std::string CSZE;
std::string EMPT;
std::string N;
std::string VSIZED;
std::string X1;
std::string X2;
std::string X2b;
std::string X2c;
std::string X2d;
std::string X3;
std::string X4;
std::string X5;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    // UNHANDLED: cob_move (cob_intr_numval (&f_1), &f_15);
    if (to_int(N) != to_int(c_1)) {
        std::cout << "X1 '" << X1 << "' : " << N << std::endl;
    }
    // UNHANDLED: cob_move (cob_intr_numval (&f_2), &f_15);
    if (to_int(N) != to_int(c_4)) {
        std::cout << "X2 '" << X2 << "' : " << N << std::endl;
    } else {
        if (false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */) {
            std::cout << "NUMVAL not case-insensitive!" << std::endl;
        }
    }
    // UNHANDLED: cob_move (cob_intr_numval (&f_6), &f_15);
    if (to_int(N) != to_int(c_7)) {
        std::cout << "X3 '" << X3 << "' : " << N << std::endl;
    }
    // UNHANDLED: cob_move (cob_intr_numval (&f_7), &f_15);
    if (to_int(N) != 4232400) {
        std::cout << "X4 '" << X4 << "' : " << N << std::endl;
    }
    // UNHANDLED: cob_move (cob_intr_numval (&f_8), &f_15);
    if (to_int(N) != to_int(c_10)) {
        std::cout << "X5 '" << X5 << "' : " << N << std::endl;
    }
    // UNHANDLED: cob_move (cob_intr_numval (&f_11), &f_15);
    if (to_int(N) != 0) {
        std::cout << "EMPT '" << EMPT << "' :  " << N << std::endl;
    }
    // MOVE to computed field
    if (to_int(N) != 0) {
        std::cout << "VSIZED '" << "' :  " << N << std::endl;
    }
    // UNHANDLED: cob_move (cob_intr_numval (&f_9), &f_15);
    if (to_int(N) != 0 && to_int(N) != to_int(c_15)) {
        std::cout << "BAD1 '" << BAD1 << "' : " << N << std::endl;
    }
    // UNHANDLED: cob_move (cob_intr_numval (&f_10), &f_15);
    if (to_int(N) != 0 && to_int(N) != 10) {
        std::cout << "BAD2 '" << BAD2 << "' : " << N << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}