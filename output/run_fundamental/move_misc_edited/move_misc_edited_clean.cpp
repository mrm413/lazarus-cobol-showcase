/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: move_misc_edited_preprocessed.cbl
 * Program: MoveOtherCases
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
std::string DST_BIN;
std::string DST_DISP;
std::string DST_FIELD_1;
int RETURN_CODE = 0;
std::string SRC_BIN;
std::string SRC_EDIT;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    SRC_BIN = "-12345678";
    DST_BIN = SRC_BIN;
    if (to_int(RETURN_CODE) != 12345678) {
        std::cout << "1: DST-BIN <" << DST_BIN << ">!= <12345678>" << std::endl;
    }
    SRC_EDIT = "-3";
    DST_DISP = SRC_EDIT;
    if (to_int(SRC_EDIT) != to_int(c_5)) {
        std::cout << "2: SRC-EDIT <" << SRC_EDIT << "> != <      $300->" << std::endl;
    }
    if (to_int(DST_DISP) != 3) {
        std::cout << "2.1: DST-DISP <" << DST_DISP << "> != <3>" << std::endl;
    }
    SRC_EDIT = "3";
    DST_FIELD_1 = SRC_EDIT;
    if (to_int(DST_FIELD_1) != to_int(c_11)) {
        std::cout << "3: DST-FIELD-1 <" << DST_FIELD_1 << "> != <  0  0  />" << std::endl;
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}