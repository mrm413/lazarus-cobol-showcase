/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: overlapping_move_preprocessed.cbl
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
std::string FIELD1_1;
std::string FIELD1_2;
std::string NUMVAR;
int RETURN_CODE = 0;
std::string STRUCTURE1;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    STRUCTURE1 = FIELD1_2;
    FIELD1_2 = FIELD1_2.substr(5, 5);
    FIELD1_2 = FIELD1_1;
    FIELD1_2.replace(10, 5, FIELD1_2.substr(0, 5));
    FIELD1_2 = FIELD1_2.substr(5, 10);
    FIELD1_2.replace(10, 5, FIELD1_2.substr(0, 5));
    // MOVE to computed field
    FIELD1_2.replace(4, 4, FIELD1_2.substr(1, 4));
    FIELD1_2.replace(5, 4, FIELD1_2.substr(1, 4));
    // MOVE to computed field
    // MOVE to computed field
    // MOVE to computed field
    FIELD1_2.replace(2, 2, FIELD1_2.substr(3, 2));
    FIELD1_2.replace(1, 4, FIELD1_2.substr(5, 4));
    // MOVE to computed field
    // MOVE (complex memory operation)
    // MOVE (complex memory operation)
    // MOVE (complex memory operation)
    VAR_26 = VAR_26.substr(0, 2);
    // MOVE (complex memory operation)
    // UNHANDLED: memcpy (b_26 + 2 * 1, b_26, 2);
    // MOVE (complex memory operation)
    // MOVE (complex memory operation)
    // MOVE to computed field
    // MOVE to computed field
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}