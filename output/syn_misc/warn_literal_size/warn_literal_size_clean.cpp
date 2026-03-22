/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: warn_literal_size_preprocessed.cbl
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
std::string COMPUTE_1;
std::string IF_D16;
std::string PIC_9_DECIMAL;
std::string PIC_9_NOT_DECIMAL;
std::string PIC_9_NOT_SIGNED;
std::string PIC_9_SIGNED;
std::string PIC_9_SIGNED_DECIMAL;
int RETURN_CODE = 0;
std::string WS_LINE_NUMBER;
std::string WS_NUMBER;
std::string WS_TEXT;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string XX;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    if (to_int(PIC_9_NOT_DECIMAL) == to_int(c_1)) {
    }
    if (to_int(PIC_9_DECIMAL) == to_int(c_2)) {
    }
    if (to_int(PIC_9_SIGNED) != to_int(c_3)) {
    }
    if (to_int(PIC_9_NOT_SIGNED) < 0) {
    }
    if (to_int(PIC_9_NOT_SIGNED) < 0) {
    }
    if (to_int(PIC_9_NOT_SIGNED) < -25) {
    }
    if (to_int(PIC_9_NOT_DECIMAL) == to_int(c_4)) {
    }
    if (to_int(PIC_9_NOT_DECIMAL) == to_int(c_5)) {
    }
    if (PIC_9_NOT_DECIMAL.substr(0, 3) == "1B0") {
    }
    if (true) {
    }
    if (false /* cob_cmp != 0 */) {
    }
    if (to_int(WS_LINE_NUMBER) > to_int(c_4)) {
    }
    if (WS_TEXT.substr(0, 5) > "DOGGY") {
    }
    if (to_int(WS_NUMBER) > to_int(c_7)) {
    }
    if (true) {
    }
    if (true) {
    }
    if (true) {
    }
    if (to_int(WS_TEXT) > to_int(c_8)) {
    }
    if (to_int(WS_TEXT) > to_int(c_9)) {
    }
    if (to_int(WS_TEXT) > to_int(c_10)) {
    }
    if (true) {
    }
    if (true) {
    } else if (true) {
    } else if (to_int(WS_LINE_NUMBER) > 234) {
    }
    if (to_int(WS_TEXT) == to_int(c_11)) {
    }
    if (to_int(COMPUTE_1) < to_int(c_12) && to_int(COMPUTE_1) > to_int(c_13)) {
    }
    if (true) {
    }
    if (true) {
    }
    if (true) {
    }
    if (true) {
    }
    if (to_int(IF_D16) > 0) {
    }
    if (to_int(IF_D16) > 0) {
    }
    if (to_int(IF_D16) < 0) {
    }
    if (to_int(IF_D16) < 0) {
    }
    if (to_int(PIC_9_NOT_SIGNED) > -25) {
    }
    if (to_int(PIC_9_NOT_SIGNED) >= -1) {
    }
    if (to_int(PIC_9_NOT_SIGNED) >= to_int(c_14)) {
    }
    if (to_int(PIC_9_NOT_SIGNED) > to_int(c_15)) {
    }
    if (to_int(PIC_9_NOT_SIGNED) > to_int(c_16)) {
    }
    if (to_int(PIC_9_NOT_SIGNED) > 0) {
    }
    if (to_int(PIC_9_NOT_SIGNED) >= to_int(c_15)) {
    }
    if (to_int(PIC_9_NOT_SIGNED) >= to_int(c_16)) {
    }
    if (to_int(PIC_9_NOT_SIGNED) >= 0) {
    }
    if (VAR_27.substr(0, 6) == "123456") {
    }
    if (false /* cob_cmp == 0 */) {
    }
    if (false /* cob_cmp == 0 */) {
    }
    if (to_int(PIC_9_NOT_DECIMAL) > 9) {
    }
    if (to_int(PIC_9_NOT_DECIMAL) > 9) {
    }
    if (to_int(PIC_9_NOT_DECIMAL) > 900) {
    }
    if (to_int(PIC_9_NOT_DECIMAL) > 909) {
    }
    if (to_int(PIC_9_NOT_DECIMAL) > 999) {
    }
    if (to_int(PIC_9_NOT_DECIMAL) > 999) {
    }
    if (to_int(PIC_9_DECIMAL) > 999) {
    }
    if (to_int(PIC_9_DECIMAL) > to_int(c_18)) {
    }
    if (to_int(PIC_9_DECIMAL) > to_int(c_19)) {
    }
    if (to_int(PIC_9_DECIMAL) > to_int(c_20)) {
    }
    if (to_int(PIC_9_DECIMAL) > to_int(c_21)) {
    }
    if (to_int(PIC_9_SIGNED_DECIMAL) > to_int(c_22)) {
    }
    if (to_int(PIC_9_SIGNED_DECIMAL) >= to_int(c_22)) {
    }
    if (to_int(PIC_9_SIGNED_DECIMAL) < to_int(c_23)) {
    }
    if (to_int(PIC_9_SIGNED_DECIMAL) <= to_int(c_24)) {
    }
    if (to_int(PIC_9_SIGNED_DECIMAL) <= to_int(c_25)) {
    }
    if (to_int(XX) < 99) {
    }
    if (to_int(XX) >= 99) {
    }
    if (to_int(XX) < 99) {
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}