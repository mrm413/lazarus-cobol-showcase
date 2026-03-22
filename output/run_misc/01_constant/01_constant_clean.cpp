/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: 01_constant_preprocessed.cbl
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
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    std::cout << "CAT  is '" << "Cat " << "'" << std::endl;
    std::cout << "Yard is '" << "Cat & Dog " << "'" << std::endl;
    std::cout << "DIV1 is " << "33" << std::endl;
    std::cout << "HUN  is " << "143" << std::endl;
    std::cout << "HUN2 is " << "1855" << std::endl;
    FLD1 = "09";
    if (to_int(FLD1) == 9) {
        std::cout << "78 VALUE has simple left to right precedence." << std::endl;
    } else {
        std::cout << "78 VALUE is " << "9" << " normal precedence." << std::endl;
    }
    FLD1 = "07";
    if (to_int(FLD1) == 7) {
        std::cout << "01 CONSTANT has normal operator precedence." << std::endl;
    } else {
        std::cout << "01 CONSTANT is " << "7" << " left to right precedence." << std::endl;
    }
    std::cout << "CON3 is " << "23" << std::endl;
    std::cout << "CON4 is " << "26" << " vs " << "3141596" << " & " << "-2189" << " & " << "+12" << std::endl;
    std::cout << "CON6 is " << "18" << "." << std::endl;
    std::cout << "My Dog's name is " << "Piper" << ";" << std::endl;
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}