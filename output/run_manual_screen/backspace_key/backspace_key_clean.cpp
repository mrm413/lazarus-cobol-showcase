/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: backspace_key_preprocessed.cbl
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
std::string COB_CRT_STATUS;
int RETURN_CODE = 0;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string cur_pos;
std::string success_flag;
std::string ws_x_20;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    std::cout << "Enter 'y' if each press of the BACKSPACE key" << "1" << "1" << std::endl;
    std::cout << "deletes the character to the left and moves the" << "2" << "1" << std::endl;
    std::cout << "cursor and remaining characters one space to the" << "3" << "1" << std::endl;
    std::cout << "left." << "4" << "1" << std::endl;
    ws_x_20 = "ABCD";
    cur_pos = "006002";
    // UNHANDLED: cob_accept_field (&f_20, 1048592, "lcS", (cob_field *)&c_9, (cob_field *)&c_2, (cob_field *)&c_10);
    // UNHANDLED: cob_accept_field (&f_18, 1048576, "lc", (cob_field *)&c_11, (cob_field *)&c_2);
    if (success_flag[0] == 'Y' || (success_flag[0] == 'y' && to_int(COB_CRT_STATUS) == 0)) {
        RETURN_CODE = 0;
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}