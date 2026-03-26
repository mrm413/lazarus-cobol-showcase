/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: json_generate_exceptions_preprocessed.cbl
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
std::string a;
std::string b;
std::string json_len;
std::string normal_str;
std::string short_str;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    // UNHANDLED: cob_json_generate_new (&f_21, &mt_1, &f_22, 46);
    if (false /* TODO: memcmp (short_str, (cob_u8_ptr)"{\"val", 5) != 0 */ || to_int(json_len) != 33 || to_int(RETURN_CODE) != 1) {
        std::cout << "Failed 1: " << short_str << " " << json_len << " " << VAR_15 << std::endl;
    }
    // UNHANDLED: cob_json_generate_new (&f_17, &mt_4, NULL, 46);
    if (false /* EXCEPTION 0x1700 */) {
        std::cout << "Failed 2: EXCEPTION " << normal_str << " " << VAR_15 << std::endl;
    } else {
        if (to_int(RETURN_CODE) != 0) {
            std::cout << "Failed 2: NOT EXCEPTION " << normal_str << " " << VAR_15 << std::endl;
        }
    }
    // UNHANDLED: cob_json_generate_new (&f_21, &mt_7, NULL, 46);
    if (false /* EXCEPTION 0x1700 */) {
        if (to_int(RETURN_CODE) != 1) {
            std::cout << "Failed 3: ON EXCEPTION " << normal_str << " " << VAR_15 << std::endl;
        }
    } else {
        std::cout << "Failed 3: NOT EXCEPTION " << normal_str << " " << VAR_15 << std::endl;
    }
    if (false /* TODO: module->module_active */) {
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}