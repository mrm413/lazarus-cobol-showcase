/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: 78_value_preprocessed.cbl
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
std::string FLD1;
std::string PICX;
int RETURN_CODE = 0;
std::string THEDOG;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string XMYREC;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    std::cout << "DIV1 is " << "33" << std::endl;
    std::cout << "HUN  is " << "143" << std::endl;
    std::cout << "HUN2 is " << "1855" << std::endl;
    FLD1 = "09";
    if (to_int(FLD1) == 9) {
        std::cout << "NUM2 is " << "9" << " left to right precedence." << std::endl;
    } else {
        std::cout << "NUM2 is " << "9" << " normal precedence." << std::endl;
    }
    std::cout << "XFLD3 starts at " << "9" << std::endl;
    std::cout << "XFLD4 starts at " << "19" << std::endl;
    std::cout << "XFLD4 starts at " << "11" << std::endl;
    std::cout << "Your Dog's name is " << "Barky" << ";" << std::endl;
    std::cout << "The Dog's name is " << THEDOG << ";" << std::endl;
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}