/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: occurs_unbounded_1_preprocessed.cbl
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
std::string A_TABLE;
std::string N;
std::string P;
int RETURN_CODE = 0;
std::string SAV;
std::string WCOL1;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    if (false /* TODO: cob_cmp_llint (cob_intr_length (COB_SET_FLD (f0, 3 * (*(unsigned short *)(N)), WCOL1, &a_1)), 369LL) != 0 */) {
        std::cout << "WRONG WS LENGTH: " << std::endl;
    }
    if (false /* TODO: cob_cmp_llint (cob_intr_length (COB_SET_FLD (f0, 3 * (*(unsigned short *)(N)), A_TABLE, &a_1)), 369LL) != 0 */) {
        std::cout << "WRONG LS LENGTH: " << std::endl;
    }
    // UNHANDLED: cob_allocate (NULL, &f_18, cob_intr_length (COB_SET_FLD (f0, 3 * (*(unsigned short *)(b_17)), b_24, &a_1)), (cob_field *)&c_3);
    if (A_TABLE.substr(1, 2) != "BC") {
        std::cout << "col2(1) wrong: " << std::endl;
    }
    if (false /* TODO: memcmp (A_TABLE + 3LL * 1LL, (cob_u8_ptr)"DEA", 3) != 0 */) {
        std::cout << "rows(2) wrong: " << std::endl;
    }
    // i_len = (*(int *) (b_29));
    WCOL1[0] = '0';
    WCOL1.replace(1, 2, std::string(2, ' '));
    // UNHANDLED: cob_init_table (b_20, 3, (*(unsigned short *)(b_17)));
    // MOVE to computed field
    // UNHANDLED: cob_free_alloc (NULL, b_18);
    if (false /* TODO: module->module_active */) {
    }
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}