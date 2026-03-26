/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: inspect_trailing_preprocessed.cbl
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
std::string W01_INDEX;
std::string W01_STRING;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    W01_INDEX = "0";
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    if (to_int(RETURN_CODE) != 10) {
        std::cout << "Bad Result for Inspect Trailing Case 1" << std::endl;
    }
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    if (W01_STRING.substr(0, 20) != "0123456789AAAAAAAAAA") {
        std::cout << "Bad Result for Inspect Trailing Case 2" << std::endl;
    }
    W01_INDEX = "0";
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    if (W01_STRING.substr(0, 20) != "0123456789BBBBBBBBBB") {
        std::cout << "Bad Result for Inspect Trailing Case 3" << std::endl;
    }
    W01_INDEX = "0";
    W01_STRING = std::string(20, ' ');
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    if (to_int(RETURN_CODE) != 0) {
        std::cout << "Bad Result for Inspect Trailing Case 4" << W01_INDEX << std::endl;
    }
    W01_INDEX = "0";
    W01_STRING = std::string(20, ' ');
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    // INSPECT operation
    // INSPECT (TALLYING/REPLACING)
    if (to_int(RETURN_CODE) != 20) {
        std::cout << "Bad Result for Inspect Trailing Case 5" << W01_INDEX << std::endl;
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}