/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: udf_with_recursion_preprocessed.cbl
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
std::string arg;
std::string num;
std::string ret;
std::string ttl;

// Forward declarations
void P_MAIN();

void P_MAIN() {
    if (to_int(arg) < 5) {
    } else {
        ret[0] = arg[0];
    }
    std::cout << "Step: " << ttl << ", Arg: " << ", Return: " << std::endl;
    ttl = to_string(to_int(ttl) + 1);
    // UNHANDLED: goto exit_function;
    // UNHANDLED: exit_function:
    if (false /* TODO: cob_reference_count */) {
        // UNHANDLED: cob_reference_count--;
    }
    if (false /* TODO: cob_dyn_0 */) {
        if (false /* TODO: cob_dyn_0->data */) {
            // UNHANDLED: cob_free (cob_dyn_0->data);
        }
        // UNHANDLED: cob_free (cob_dyn_0);
        // UNHANDLED: cob_dyn_0 = NULL;
    }
    if (false /* TODO: cob_local_ptr */) {
    }
    // UNHANDLED: cob_free (frame_stack);
    // UNHANDLED: return COB_SET_DATA (f_20, b_20);
    RETURN_CODE = 0;
    ttl[0] = '1';
    }
    // UNHANDLED: return prog_ (0);
    }
    if (false /* TODO: initialized == 0 */) {
        // UNHANDLED: goto P_initialize;
    }
    // UNHANDLED: cob_reference_count++;
    std::cout << "Return value '" << num << "'" << std::endl;
}

int main() {
    P_MAIN();
    return RETURN_CODE;
}