/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: divide_complex_preprocessed.cbl
 * Program: DivideExtent
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
std::string W01_DIVIDES;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_TEST_FORMAT_1();
void P_TEST_FORMAT_2();
void P_TEST_FORMAT_4();

void P_TEST_FORMAT_1() {
    // MOVE to computed field
    // MOVE to computed field
    // MOVE to computed field
    // DIVIDE 100 INTO array-element (COB_SET_FLD)
    // DIVIDE 100 INTO array-element (COB_SET_FLD)
    // DIVIDE 100 INTO array-element (COB_SET_FLD)
    if (false /* SIZE ERROR */) {
        std::cout << "1: Should not raised size error" << std::endl;
    }
    if (to_int(RETURN_CODE) != 1 || to_int(RETURN_CODE) != 2 || to_int(RETURN_CODE) != 3) {
        std::cout << "F1.1: result <" << "> <" << "> <" << "> should be 1 2 3" << std::endl;
    }
    // MOVE to computed field
    // MOVE to computed field
    // MOVE to computed field
    // DIVIDE 1 INTO array-element (COB_SET_FLD)
    // DIVIDE 1 INTO array-element (COB_SET_FLD)
    if (true /* NOT ON SIZE ERROR */) {
        std::cout << "F1.2: Should raise size error" << std::endl;
    }
    if (to_int(RETURN_CODE) != 0 || to_int(RETURN_CODE) != 100 || to_int(RETURN_CODE) != 200) {
        std::cout << "F1.2: result <" << "> <" << "> <" << "> should be 0 100 200" << std::endl;
    }
    // MOVE to computed field
    // DIVIDE 219 INTO array-element (COB_SET_FLD)
    if (false /* SIZE ERROR */) {
        std::cout << "F1.3: Should not raise size error" << std::endl;
    }
    if (to_int(RETURN_CODE) != 45) {
        std::cout << "F1.3: result <" << "> should be 45" << std::endl;
    }
    // MOVE to computed field
    // DIVIDE 219 INTO array-element (COB_SET_FLD)
    if (false /* SIZE ERROR */) {
        std::cout << "F1.4: Should not raise size error" << std::endl;
    }
    if (to_int(RETURN_CODE) != 46) {
        std::cout << "F1.4: result <" << "> should be 46" << std::endl;
    }
}

void P_TEST_FORMAT_2() {
    if (false /* SIZE ERROR */) {
        std::cout << "F2.1: Should not raised size error" << std::endl;
    }
    if (to_int(RETURN_CODE) != 9) {
        std::cout << "F2.1: result <" << "> should be 9" << std::endl;
    }
    if (false /* SIZE ERROR */) {
        std::cout << "F2.2: Should not raise size error" << std::endl;
    }
    if (to_int(RETURN_CODE) != 10) {
        std::cout << "F2.2: result <" << "> should be 10" << std::endl;
    }
    if (false /* SIZE ERROR */) {
        std::cout << "F2.3: Should not raise size error" << std::endl;
    }
    if (to_int(RETURN_CODE) != 9) {
        std::cout << "F2.3: result <" << "> should be 9" << std::endl;
    }
}

void P_TEST_FORMAT_4() {
    // DIVIDE 100 INTO 2 GIVING array-element
    // DIVIDE 1 INTO array-element (COB_SET_FLD)
    if (to_int(RETURN_CODE) != 50 || to_int(RETURN_CODE) != 0) {
        std::cout << "F4.1: result <" << "> <" << "> should be <50> <0>" << std::endl;
    }
    // DIVIDE 100 INTO 3 GIVING array-element
    // DIVIDE 1 INTO array-element (COB_SET_FLD)
    if (to_int(RETURN_CODE) != 33 || to_int(RETURN_CODE) != 1) {
        std::cout << "F4.1: result <" << "> <" << "> should be <33> <1>" << std::endl;
    }
    // MOVE to computed field
    // MOVE to computed field
    // MOVE to computed field
    // DIVIDE-QUOTIENT (COB_SET_FLD)
    // DIVIDE 1 INTO array-element (COB_SET_FLD)
    if (true /* NOT ON SIZE ERROR */) {
        std::cout << "F4.3: Should  raised size error" << std::endl;
    }
}

int main() {
    P_TEST_FORMAT_1();
    return RETURN_CODE;
}