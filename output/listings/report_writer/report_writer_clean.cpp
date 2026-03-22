/**
 * LAZARUS-Generated C++ Code (V2)
 * Source: report_writer_preprocessed.cbl
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
std::string CHARGE_DETAIL;
std::string END_OF_FILE_SWITCH;
std::string PAGE_HEAD_GROUP;
int RETURN_CODE = 0;
std::string XML_NAMESPACE;
std::string XML_NAMESPACE_PREFIX;
std::string XML_NNAMESPACE;
std::string XML_NNAMESPACE_PREFIX;
std::string XML_NTEXT;
std::string XML_TEXT;

// Forward declarations
void P_000_INITIATE();
void P_000_TERMINATE();
void P_100_PROCESS_TRANSACTION_DATA();
void P_199_EXIT();

void P_000_INITIATE() {
    // OPEN TRANSACTION-DATA (INPUT)
    // OPEN REPORT-FILE (OUTPUT)
    if (file_status != "EXCEPTION_0") {
    }
    if (file_status != "EXCEPTION_0") {
    }
    // UNHANDLED: goto rwexit_1;
    // UNHANDLED: rwmove_1: ;
    // UNHANDLED: rwfoot_1: ;
    // UNHANDLED: rwexit_1: ;
    // UNHANDLED: r_CUSTOMER_REPORT.code_is_present = 0;
    // UNHANDLED: cob_report_initiate (&r_CUSTOMER_REPORT);
    // READ NEXT TRANSACTION-DATA
    if (file_status != "EXCEPTION_0") {
        if (file_status == "AT_END") {
            END_OF_FILE_SWITCH[0] = 'Y';
        } else {
        }
    }
    while (true) {
        if (END_OF_FILE_SWITCH[0] == 'Y') break;
        P_100_PROCESS_TRANSACTION_DATA();
    }
}

void P_000_TERMINATE() {
    // UNHANDLED: cob_report_terminate (&r_CUSTOMER_REPORT, 0);
    // CLOSE TRANSACTION-DATA
    if (file_status != "EXCEPTION_0") {
    }
    // CLOSE REPORT-FILE
    if (file_status != "EXCEPTION_0") {
    }
    return;
}

void P_100_PROCESS_TRANSACTION_DATA() {
    // UNHANDLED: cob_report_generate (&r_CUSTOMER_REPORT, &rl_43, 0);
    // READ NEXT TRANSACTION-DATA
    if (file_status != "EXCEPTION_0") {
        if (file_status == "AT_END") {
            END_OF_FILE_SWITCH[0] = 'Y';
        } else {
        }
    }
}

void P_199_EXIT() {
}

int main() {
    P_000_INITIATE();
    return RETURN_CODE;
}