/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: define_override_preprocessed.cbl
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
std::string MYHORSE;
std::string MYPONYENV;
int RETURN_CODE = 0;
std::string THEDOG;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    std::cout << "ENVPONY" << std::endl;
    // UNHANDLED: cob_accept_environment (&f_23);
    std::cout << "ENVPONY env var set to " << MYPONYENV << ";" << std::endl;
    std::cout << "1st Dog's name is " << "Pluto" << ";" << std::endl;
    std::cout << "2nd Dog's name is " << "Piper" << ";" << std::endl;
    std::cout << "ENVPONY is DEFINED as " << "No EnvPony" << ";" << std::endl;
    std::cout << "DPONY set to " << "No Dpony" << ";" << std::endl;
    std::cout << "My pony is " << "default Dirty" << ";" << std::endl;
    std::cout << "DPONY is DEFINED as " << "No Dpony" << ";" << std::endl;
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}