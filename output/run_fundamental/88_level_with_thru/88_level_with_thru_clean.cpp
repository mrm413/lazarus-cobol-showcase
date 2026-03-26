/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: 88_level_with_thru_preprocessed.cbl
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
std::string VAR_9;
std::string VAR_X;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    if (VAR_X[0] == 'X') {
        std::cout << "NOT OK '" << VAR_X << "' IS X" << std::endl;
    }
    VAR_X[0] = static_cast<char>(88);
    if (VAR_X[0] == 'X') {
        std::cout << "NOT OK '" << VAR_X << "' IS NOT X" << std::endl;
    }
    if (false /* TODO: !(((*(VAR_X) - 'T') >= 0) && ((*(VAR_X) - 'Y') <= 0)) */) {
        std::cout << "NOT OK '" << VAR_X << "' IS NOT T-Y" << std::endl;
    }
    VAR_X[0] = static_cast<char>(84);
    if (false /* TODO: !(((*(VAR_X) - 'T') >= 0) && ((*(VAR_X) - 'Y') <= 0)) */) {
        std::cout << "NOT OK '" << VAR_X << "' IS NOT T-Y" << std::endl;
    }
    VAR_X[0] = 'Y';
    if (false /* TODO: !(((*(VAR_X) - 'T') >= 0) && ((*(VAR_X) - 'Y') <= 0)) */) {
        std::cout << "NOT OK '" << VAR_X << "' IS NOT T-Y" << std::endl;
    }
    VAR_X[0] = static_cast<char>(90);
    if (false /* TODO: (*(VAR_X) - 'T') >= 0 */ && false /* TODO: (*(VAR_X) - 'Y') <= 0 */) {
        std::cout << "NOT OK '" << VAR_X << "' IS T-Y" << std::endl;
    }
    VAR_X[0] = 'A';
    if (false /* TODO: (*(VAR_X) - 'T') >= 0 */ && false /* TODO: (*(VAR_X) - 'Y') <= 0 */) {
        std::cout << "NOT OK '" << VAR_X << "' IS T-Y" << std::endl;
    }
    if (to_int(VAR_9) == 9) {
        std::cout << "NOT OK '" << VAR_9 << "' IS V9" << std::endl;
    }
    VAR_9[0] = static_cast<char>(57);
    if (to_int(VAR_9) == 9) {
        std::cout << "NOT OK '" << VAR_9 << "' IS NOT V9" << std::endl;
    }
    VAR_9[0] = static_cast<char>(50);
    if (to_int(VAR_9) == 9) {
        std::cout << "NOT OK '" << VAR_9 << "' IS V9" << std::endl;
    }
    if (to_int(VAR_9) >= 2) {
        std::cout << "NOT OK '" << VAR_9 << "' IS NOT V2-4" << std::endl;
    }
    VAR_9[0] = static_cast<char>(51);
    if (to_int(VAR_9) >= 2) {
        std::cout << "NOT OK '" << VAR_9 << "' IS NOT V2-4" << std::endl;
    }
    VAR_9[0] = static_cast<char>(52);
    if (to_int(VAR_9) >= 2) {
        std::cout << "NOT OK '" << VAR_9 << "' IS NOT V2-4" << std::endl;
    }
    VAR_9[0] = static_cast<char>(53);
    if (to_int(VAR_9) >= 2 && to_int(VAR_9) <= 4) {
        std::cout << "NOT OK '" << VAR_9 << "' IS V2-4" << std::endl;
    }
    VAR_9[0] = '1';
    if (to_int(VAR_9) >= 2 && to_int(VAR_9) <= 4) {
        std::cout << "NOT OK '" << VAR_9 << "' IS V2-4" << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}