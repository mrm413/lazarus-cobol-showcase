/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: any_length_4_preprocessed.cbl
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
std::string str;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    if (false /* TODO: call_subprog.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // UNHANDLED: ((int (*)(void *))call_subprog.funcint) (b_1);
    str = "   45";
    if (false /* TODO: call_subprog.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // UNHANDLED: ((int (*)(void *))call_subprog.funcint) (b_1);
    if (false /* TODO: module->module_active */) {
    }
    if (false /* TODO: cob_reference_count */) {
        // UNHANDLED: cob_reference_count--;
    }
    str = std::string(20, static_cast<char>(88));
    // CALL subroutine
    }
    if (false /* TODO: cob_call_params > 0 */) {
        // UNHANDLED: cob_parm_3 = b_3;
    }
    // UNHANDLED: return subprog_ (0, cob_parm_3);
    }
    if (false /* TODO: initialized == 0 */) {
        // UNHANDLED: goto P_initialize;
    }
    // UNHANDLED: cob_reference_count++;
    // UNHANDLED: last_b_3 = b_3;
    // UNHANDLED: f_3.data = b_3;
    if (false /* cob_cmp == 0 */) {
        std::cout << "X is X" << std::endl;
    }
    if (false /* cob_cmp == 0 */) {
        std::cout << "X is space" << std::endl;
    }
    if (false /* TODO: module->module_active */) {
    }
    if (false /* TODO: cob_reference_count */) {
        // UNHANDLED: cob_reference_count--;
    }
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}