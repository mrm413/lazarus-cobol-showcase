/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: before_reporting_preprocessed.cbl
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
std::string FINAL_FOOTING;
std::string FSM_1;
std::string FSM_2;
std::string FSM_3;
std::string PAGE_HEADING;
std::string REPORT_LINE;
int RETURN_CODE = 0;
std::string WS_BOOK_PAY;
std::string WS_STUDENT_NAME;
std::string WS_TRANSPORT_PAY;
std::string WS_TUTION_PAY;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_CALC_GRAND_SUM_AND_AVERAGE();
void P_BEFORE_FINAL1();

void P_CALC_GRAND_SUM_AND_AVERAGE() {
    std::cout << "BEFORE FINAL - SHOULD DISPLAY" << std::endl;
}

void P_BEFORE_FINAL1() {
    P_CALC_GRAND_SUM_AND_AVERAGE();
}

int main() {
    P_CALC_GRAND_SUM_AND_AVERAGE();
    return RETURN_CODE;
}