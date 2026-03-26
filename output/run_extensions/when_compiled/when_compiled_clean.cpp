/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: when_compiled_preprocessed.cbl
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
int RETURN_CODE = 0;
std::string X;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    X = "26/03/2609.59.24";
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    // INSPECT operation
    if (X.substr(0, 20) != "99/99/9999.99.99") {
        if (false /* TODO: call_CBL_OC_DUMP.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
            // CALL subroutine
        }
        if (false /* TODO: call_CBL_OC_DUMP.funcvoid == NULL */) {
            std::cout << X << std::endl;
        } else {
            // RETURN_CODE = ((int (*)(void *))call_CBL_OC_DUMP.funcint) (b_17);
        }
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}