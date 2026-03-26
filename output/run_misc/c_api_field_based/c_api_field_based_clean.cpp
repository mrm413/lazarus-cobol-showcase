/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: c_api_field_based_preprocessed.cbl
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
std::string BINFLD5;
std::string BINFLD5S;
std::string BINFLD9;
std::string CHRX;
std::string COMP3;
std::string COMP3V99;
std::string GRPX;
std::string NE;
std::string PIC9;
int RETURN_CODE = 0;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    NE = "-51277";
    // UNHANDLED: cob_content	content_1;
    // UNHANDLED: content_fb_1.data = content_1.data;
    // UNHANDLED: content_1.dataint = 2560;
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *, cob_u32_t))call_CAPI.funcint) (content_1.data, (cob_u32_t)16);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *, void *))call_CAPI.funcint) (b_17, b_23);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *))call_CAPI.funcint) (b_18);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *))call_CAPI.funcint) (b_19);
    NE = "51277";
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *, void *))call_CAPI.funcint) (b_20, b_23);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *, void *, void *, void *))call_CAPI.funcint) (b_22, b_18, b_24, b_25);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *, void *, void *))call_CAPI.funcint) (b_20, b_23, b_24);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *, void *))call_CAPI.funcint) (b_17, b_23);
    CHRX = "Hello!";
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(cob_u32_t, void *))call_CAPI.funcint) ((cob_u32_t)((*(unsigned int *)(b_17))), content_2.data);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(cob_u32_t, void *))call_CAPI.funcint) ((cob_u32_t)((*(unsigned int *)(b_17))), content_2.data);
    // UNHANDLED: cob_content	content_1;
    // UNHANDLED: content_fb_1.data = content_1.data;
    // UNHANDLED: content_1.dataint = 18;
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *))call_CAPI.funcint) (content_1.data);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *, cob_u32_t))call_CAPI.funcint) (content_1.data, (cob_u32_t)18);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *, void *))call_CAPI.funcint) ((cob_u8_ptr)"Fred Fish", b_20);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void *))call_CAPI.funcint) (b_21);
    if (false /* TODO: call_CAPI.funcvoid == NULL */ || false /* TODO: cob_glob_ptr->cob_physical_cancel == 1 */) {
        // CALL subroutine
    }
    // RETURN_CODE = ((int (*)(void))call_CAPI.funcint) ();
    std::cout << "COMP3    is now " << COMP3 << ";" << std::endl;
    std::cout << "COMP4    is now " << BINFLD5 << ";" << std::endl;
    std::cout << "BINFLD5S is now " << BINFLD5S << ";" << std::endl;
    std::cout << "CHRX     is now " << CHRX << ";" << std::endl;
    std::cout << "NE       is now " << NE << ";" << std::endl;
    return;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}