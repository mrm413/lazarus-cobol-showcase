/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: ppp_comp_3_preprocessed.cbl
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
std::string AE_0002;
std::string D_1;
std::string D_2;
std::string FENCE_1;
std::string FENCE_2;
std::string FENCE_A;
std::string GRP_AE_0002;
std::string MOVE72;
std::string MOVE73;
int RETURN_CODE = 0;
std::string WRK_AE_3;
std::string WRK_DS_LS_1P17_1;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string X_1;
std::string X_2;
std::string X_4;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    MOVE72 = "3344556677";
    MOVE73 = MOVE72;
    if (to_int(MOVE73) != to_int(c_1)) {
        std::cout << "MOVE X-EDIT failed: " << MOVE73 << "." << std::endl;
    }
    AE_0002 = "019823";
    if (AE_0002.substr(0, 9) != "01098 23") {
        std::cout << "MOVE AE-EDIT failed: " << GRP_AE_0002 << "." << std::endl;
    }
    WRK_AE_3 = WRK_DS_LS_1P17_1;
    std::cout << "MOVE 1P17: " << WRK_DS_LS_1P17_1 << " : " << "2" << "." << std::endl;
    std::cout << "MOVE A-E : " << WRK_AE_3 << "." << std::endl;
    if (to_int(WRK_AE_3) != to_int(c_10)) {
        std::cout << "MOVE 1P17 TO A-E failed" << std::endl;
    }
    std::cout << "INIT X-1 : " << X_1 << " ." << std::endl;
    std::cout << "INIT X-2 : " << X_2 << " ." << std::endl;
    std::cout << "INIT X-4 : " << X_4 << " ." << std::endl;
    std::cout << "INIT D-1 : " << D_1 << " ." << std::endl;
    X_4.replace(8, 2, D_1.substr(0, 2));
    X_4.replace(3, 2, D_1.substr(0, 2));
    D_1 = X_4.substr(8, 2);
    std::cout << "MOVE X-1 : " << X_1 << " ." << std::endl;
    std::cout << "MOVE X-2 : " << X_2 << " ." << std::endl;
    X_4 = "0000256";
    std::cout << "MOVE X-4 : " << X_4 << " ." << std::endl;
    std::cout << "MOVE D-1 : " << D_1 << " ." << std::endl;
    X_2 = D_2;
    X_1 = D_2;
    std::cout << "MOVE X-1 : " << X_1 << ":" << D_2 << " ." << std::endl;
    std::cout << "MOVE X-2 : " << X_2 << ":" << D_2 << " ." << std::endl;
    X_1 = "98000";
    if (to_int(X_1) != 98000) {
        std::cout << "MOVE 98000 failed: " << X_1 << std::endl;
    }
    D_1 = "98000";
    if (to_int(D_1) != 98000) {
        std::cout << "MOVE 98000 failed: " << D_1 << std::endl;
    }
    X_1 = to_string(to_int(X_1) + 1000);
    if (to_int(X_1) != 99000) {
        std::cout << "+ 1000 failed: " << X_1 << std::endl;
    }
    X_1 = to_string(to_int(X_1) - 4000);
    if (to_int(X_1) != 95000) {
        std::cout << "- 4000 failed: " << X_1 << std::endl;
    }
    // UNHANDLED: cob_div (&f_20, (cob_field *)&c_27, 0);
    if (to_int(X_1) != 31000) {
        std::cout << "/ 3 failed: " << X_1 << std::endl;
    }
    X_1 = to_string(to_num(0) * to_num(1));
    if (to_int(X_1) != 62000) {
        std::cout << "* 2 failed: " << X_1 << std::endl;
    }
    if (FENCE_1.substr(0, 3) != "���") {
        std::cout << "broken FENCE-1" << std::endl;
    }
    if (FENCE_2.substr(0, 3) != "���") {
        std::cout << "broken FENCE-2" << std::endl;
    }
    if (X_4.substr(0, 3) != "���") {
        std::cout << "broken FENCE-A" << std::endl;
    }
    if (X_4.substr(5, 3) != "���") {
        std::cout << "broken FENCE-B" << std::endl;
    }
    if (X_4.substr(10, 3) != "���") {
        std::cout << "broken FENCE-C" << std::endl;
    }
    if (X_4.substr(15, 3) != "���") {
        std::cout << "broken FENCE-D" << std::endl;
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}