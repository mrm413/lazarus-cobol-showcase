/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: picture_l_basic_preprocessed.cbl
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
std::string LGX;
std::string LGY;
int RETURN_CODE = 0;
std::string W_DATA;
std::string W_LEN;
std::string W_NESTED;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string X_GRP;
std::string Z_GRP;
std::string Z_V;

// Forward declarations
void P_MAIN();
void P_SHOW_VARS();
void P_CHECK_LATE_LVAR();

void P_MAIN() {
    W_DATA = std::string(38, ' ');
    if (false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */) {
        std::cout << "GROUP INITIALIZATION WITH SPACES FAILED" << std::endl;
        P_SHOW_VARS();
    }
    LGX = "5";
    LGX[4] = '1';
    if (W_DATA.substr(0, 38) != "") {
        std::cout << "GROUP COMPARISON FAILED (W-DATA)" << std::endl;
        P_SHOW_VARS();
    }
    if (X_GRP.substr(0, 12) != "0          1" || X_GRP.substr(0, 12) != X_GRP.substr(0, 12) || false /* cob_cmp != 0 */) {
        std::cout << "GROUP COMPARISON FAILED (W-NESTED)" << std::endl;
        P_SHOW_VARS();
    }
    // MOVE to computed field
    // MOVE to computed field
    // MOVE to computed field
    // MOVE to computed field
    if (false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */) {
        std::cout << "MOVES FROM LITERALS FAILED" << std::endl;
        P_SHOW_VARS();
    }
    // MOVE to computed field
    // MOVE to computed field
    if (false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */ || false /* cob_cmp != 0 */) {
        std::cout << "MOVES FROM/TO VARS FAILED" << std::endl;
        P_SHOW_VARS();
    }
    // MOVE to computed field
    LGX = "1";
    LGX = "10";
    if (false /* cob_cmp != 0 */) {
        std::cout << "DIRECT VAR INITIALIZATION FAILED" << std::endl;
        P_SHOW_VARS();
    }
    LGX = "1";
    W_LEN = to_string(to_num(0) + to_num(1));
    if (to_int(W_LEN) != 10) {
        std::cout << "UNEXPECTED RESULT FOR 'LENGTH OF X-DAT': " << W_LEN << std::endl;
        P_SHOW_VARS();
    }
    W_LEN = to_string(to_num(0) + to_num(1));
    if (to_int(W_LEN) != 1) {
        std::cout << "UNEXPECTED RESULT FOR 'FUNCTION LENGTH " << "(X-ALPHA)': " << W_LEN << std::endl;
        P_SHOW_VARS();
    }
    LGX[4] = static_cast<char>(57);
    LGX[4] = static_cast<char>(53);
    LGX[4] = static_cast<char>(57);
    if (false /* cob_cmp != 0 */) {
        std::cout << "UNEXPECTED RESULT AFTER 'MOVE ALL/INITIALIZE': " << std::endl;
        P_SHOW_VARS();
    }
    P_CHECK_LATE_LVAR();
    return;
}

void P_SHOW_VARS() {
    std::cout << W_DATA << "*" << std::endl;
    std::cout << LGX << "*" << LGX << "*" << "*" << "*" << std::endl;
    std::cout << W_NESTED << "*" << std::endl;
    std::cout << X_GRP << "*" << std::endl;
    std::cout << LGX << "*" << std::endl;
}

void P_CHECK_LATE_LVAR() {
    Z_GRP.replace(0, 2, "aa");
    Z_GRP[3] = '1';
    if (Z_GRP.substr(0, 3) != " a1") {
        std::cout << "UNEXPECTED Z-GRP WITH NESTED LATE L-VAR: " << "*" << Z_GRP << "*" << std::endl;
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}