/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: turn_ec_i_o_preprocessed.cbl
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
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string f_rec;
std::string f_status;
std::string g_rec;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    // OPEN F (OUTPUT)
    if (file_status != "EXCEPTION_0") {
    }
    f_rec[0] = static_cast<char>(97);
    // WRITE F
    if (file_status != "EXCEPTION_0") {
    }
    // CLOSE F
    if (file_status != "EXCEPTION_0") {
    }
    // UNHANDLED: cob_set_exception (0);
    // OPEN F (INPUT)
    if (file_status != "EXCEPTION_0") {
    }
    for (int n0 = 2; n0 > 0; n0--) {
        // READ NEXT F
        if (file_status != "EXCEPTION_0") {
        }
        std::cout << f_rec << std::endl;
    }
    std::cout << f_status << std::endl;
    std::cout << std::endl;
    // UNHANDLED: cob_set_exception (0);
    // CLOSE F
    if (file_status != "EXCEPTION_0") {
    }
    // OPEN F (INPUT)
    if (file_status != "EXCEPTION_0") {
    }
    for (int n1 = 2; n1 > 0; n1--) {
        // READ NEXT F
        if (file_status != "EXCEPTION_0") {
        }
        std::cout << f_rec << std::endl;
    }
    std::cout << f_status << std::endl;
    std::cout << std::endl;
    // UNHANDLED: cob_set_exception (0);
    // CLOSE F
    if (file_status != "EXCEPTION_0") {
    }
    // OPEN G (INPUT)
    if (file_status != "EXCEPTION_0") {
    }
    for (int n2 = 2; n2 > 0; n2--) {
        // READ NEXT G
        if (file_status != "EXCEPTION_0") {
        }
        std::cout << g_rec << std::endl;
    }
    // CLOSE G
    if (file_status != "EXCEPTION_0") {
    }
    if (false /* TODO: module->module_active */) {
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}