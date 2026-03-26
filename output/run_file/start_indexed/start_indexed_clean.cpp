/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: start_indexed_preprocessed.cbl
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
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;
std::string file1_key;
std::string file1_rec;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    // OPEN FILE1 (OUTPUT)
    if (file_status != "EXCEPTION_0") {
    }
    // CLOSE FILE1
    if (file_status != "EXCEPTION_0") {
    }
    // OPEN FILE1 (I_O)
    if (file_status != "EXCEPTION_0") {
    }
    file1_key = "010";
    file1_key.replace(3, 3, "010");
    // WRITE FILE1
    if (file_status != "EXCEPTION_0") {
    }
    file1_key = "011";
    file1_key.replace(3, 3, "011");
    // WRITE FILE1
    if (file_status != "EXCEPTION_0") {
    }
    file1_key = "012";
    file1_key.replace(3, 3, "012");
    // WRITE FILE1
    if (file_status != "EXCEPTION_0") {
    }
    file1_key = "013";
    file1_key.replace(3, 3, "013");
    // WRITE FILE1
    if (file_status != "EXCEPTION_0") {
    }
    file1_key = std::string(3, '0');
    // UNHANDLED: cob_start (h_FILE1, 4, &f_18, NULL, NULL);
    if (file_status != "EXCEPTION_0") {
    }
    // READ NEXT FILE1
    if (file_status != "EXCEPTION_0") {
    }
    if (to_int(file1_key) != 10) {
        std::cout << "FAILED: START key > 0" << std::endl;
    }
    file1_key = "099";
    // UNHANDLED: cob_start (h_FILE1, 2, &f_18, NULL, NULL);
    if (file_status != "EXCEPTION_0") {
    }
    // READ NEXT FILE1
    if (file_status != "EXCEPTION_0") {
    }
    if (to_int(file1_key) != 13) {
        std::cout << "FAILED: START key < 99" << std::endl;
    }
    file1_key = std::string(3, static_cast<char>(57));
    // UNHANDLED: cob_start (h_FILE1, 7, &f_18, NULL, NULL);
    if (file_status != "EXCEPTION_0") {
    }
    // READ NEXT FILE1
    if (file_status != "EXCEPTION_0") {
    }
    if (to_int(file1_key) != 10) {
        std::cout << "FAILED: START key FIRST" << std::endl;
    }
    file1_key = std::string(3, '0');
    // UNHANDLED: cob_start (h_FILE1, 8, &f_18, NULL, NULL);
    if (file_status != "EXCEPTION_0") {
    }
    // READ NEXT FILE1
    if (file_status != "EXCEPTION_0") {
    }
    if (to_int(file1_key) != 13) {
        std::cout << "FAILED: START key LAST" << std::endl;
    }
    file1_key = std::string(3, '0');
    // UNHANDLED: cob_start (h_FILE1, 5, &f_18, NULL, NULL);
    if (file_status != "EXCEPTION_0") {
    }
    // READ NEXT FILE1
    if (file_status != "EXCEPTION_0") {
    }
    if (to_int(file1_key) != 10) {
        std::cout << "FAILED: START key >= 0" << std::endl;
    }
    file1_key = "099";
    // UNHANDLED: cob_start (h_FILE1, 3, &f_18, NULL, NULL);
    if (file_status != "EXCEPTION_0") {
    }
    // READ NEXT FILE1
    if (file_status != "EXCEPTION_0") {
    }
    if (to_int(file1_key) != 13) {
        std::cout << "FAILED: START key <= 99" << std::endl;
    }
    // CLOSE FILE1
    if (file_status != "EXCEPTION_0") {
    }
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}