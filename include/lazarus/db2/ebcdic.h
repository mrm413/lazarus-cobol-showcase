#ifndef LAZARUS_DB2_EBCDIC_H
#define LAZARUS_DB2_EBCDIC_H

#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>
#include <algorithm>
#include <cmath>

namespace lazarus { namespace db2 { namespace ebcdic {

// IBM Code Page 037 (US/Canada) EBCDIC-to-ASCII mapping
// Each index is an EBCDIC byte value, the value is the ASCII equivalent.
// Non-printable / unmapped positions map to 0x1A (SUB).
inline constexpr std::array<char, 256> make_ebcdic_to_ascii() {
    std::array<char, 256> t{};
    for (int i = 0; i < 256; ++i) t[static_cast<std::size_t>(i)] = '\x1a'; // SUB default

    // Control characters
    t[0x00] = '\x00'; // NUL
    t[0x05] = '\x09'; // HT
    t[0x0B] = '\x0B'; // VT
    t[0x0C] = '\x0C'; // FF
    t[0x0D] = '\x0D'; // CR
    t[0x15] = '\x0A'; // NL -> LF
    t[0x25] = '\x0A'; // LF
    t[0x40] = ' ';

    // Punctuation
    t[0x4B] = '.'; t[0x4C] = '<'; t[0x4D] = '('; t[0x4E] = '+'; t[0x4F] = '|';
    t[0x50] = '&'; t[0x5A] = '!'; t[0x5B] = '$'; t[0x5C] = '*'; t[0x5D] = ')';
    t[0x5E] = ';'; t[0x60] = '-'; t[0x61] = '/';
    t[0x6B] = ','; t[0x6C] = '%'; t[0x6D] = '_'; t[0x6E] = '>'; t[0x6F] = '?';
    t[0x7A] = ':'; t[0x7B] = '#'; t[0x7C] = '@'; t[0x7D] = '\''; t[0x7E] = '=';
    t[0x7F] = '"';

    // Lowercase letters
    t[0x81] = 'a'; t[0x82] = 'b'; t[0x83] = 'c'; t[0x84] = 'd';
    t[0x85] = 'e'; t[0x86] = 'f'; t[0x87] = 'g'; t[0x88] = 'h'; t[0x89] = 'i';
    t[0x91] = 'j'; t[0x92] = 'k'; t[0x93] = 'l'; t[0x94] = 'm';
    t[0x95] = 'n'; t[0x96] = 'o'; t[0x97] = 'p'; t[0x98] = 'q'; t[0x99] = 'r';
    t[0xA2] = 's'; t[0xA3] = 't'; t[0xA4] = 'u'; t[0xA5] = 'v';
    t[0xA6] = 'w'; t[0xA7] = 'x'; t[0xA8] = 'y'; t[0xA9] = 'z';

    // Uppercase letters
    t[0xC1] = 'A'; t[0xC2] = 'B'; t[0xC3] = 'C'; t[0xC4] = 'D';
    t[0xC5] = 'E'; t[0xC6] = 'F'; t[0xC7] = 'G'; t[0xC8] = 'H'; t[0xC9] = 'I';
    t[0xD1] = 'J'; t[0xD2] = 'K'; t[0xD3] = 'L'; t[0xD4] = 'M';
    t[0xD5] = 'N'; t[0xD6] = 'O'; t[0xD7] = 'P'; t[0xD8] = 'Q'; t[0xD9] = 'R';
    t[0xE2] = 'S'; t[0xE3] = 'T'; t[0xE4] = 'U'; t[0xE5] = 'V';
    t[0xE6] = 'W'; t[0xE7] = 'X'; t[0xE8] = 'Y'; t[0xE9] = 'Z';

    // Digits
    t[0xF0] = '0'; t[0xF1] = '1'; t[0xF2] = '2'; t[0xF3] = '3'; t[0xF4] = '4';
    t[0xF5] = '5'; t[0xF6] = '6'; t[0xF7] = '7'; t[0xF8] = '8'; t[0xF9] = '9';

    // Additional CP037 punctuation
    t[0x5F] = '\xAC'; // logical not (map to ~)
    t[0x5F] = '~';
    t[0x79] = '`';
    t[0xBA] = '[';  // not standard 037 but needed
    t[0xBB] = ']';
    t[0xAD] = '[';
    t[0xBD] = ']';
    t[0xC0] = '{';
    t[0xD0] = '}';
    t[0xE0] = '\\';
    t[0xB0] = '^';

    return t;
}

inline constexpr std::array<char, 256> ebcdic_to_ascii_table = make_ebcdic_to_ascii();

// Build reverse table from the forward table
inline constexpr std::array<uint8_t, 256> make_ascii_to_ebcdic() {
    std::array<uint8_t, 256> t{};
    for (int i = 0; i < 256; ++i) t[static_cast<std::size_t>(i)] = 0x3F; // default: EBCDIC '?'

    // Walk forward table and invert
    for (int i = 0; i < 256; ++i) {
        auto ch = static_cast<unsigned char>(ebcdic_to_ascii_table[static_cast<std::size_t>(i)]);
        if (ch != 0x1A) { // skip unmapped
            t[ch] = static_cast<uint8_t>(i);
        }
    }
    return t;
}

inline constexpr std::array<uint8_t, 256> ascii_to_ebcdic_table = make_ascii_to_ebcdic();

// ------------------------------------------------------------------
//  String conversion
// ------------------------------------------------------------------

inline char ebcdic_char_to_ascii(uint8_t eb) {
    return ebcdic_to_ascii_table[eb];
}

inline uint8_t ascii_char_to_ebcdic(char a) {
    return ascii_to_ebcdic_table[static_cast<unsigned char>(a)];
}

inline std::string ebcdic_to_ascii(const uint8_t* data, std::size_t len) {
    std::string out;
    out.reserve(len);
    for (std::size_t i = 0; i < len; ++i) {
        out.push_back(ebcdic_to_ascii_table[data[i]]);
    }
    return out;
}

inline std::string ebcdic_to_ascii(std::string_view sv) {
    return ebcdic_to_ascii(reinterpret_cast<const uint8_t*>(sv.data()), sv.size());
}

inline std::vector<uint8_t> ascii_to_ebcdic(std::string_view ascii) {
    std::vector<uint8_t> out;
    out.reserve(ascii.size());
    for (char c : ascii) {
        out.push_back(ascii_to_ebcdic_table[static_cast<unsigned char>(c)]);
    }
    return out;
}

// ------------------------------------------------------------------
//  Packed decimal (COMP-3)
// ------------------------------------------------------------------

// Decode a packed-decimal (COMP-3) field.
// Each byte has two BCD nibbles (high, low), last byte's low nibble = sign.
// Sign: 0xC/0xA/0xF = positive, 0xD/0xB = negative
// scale = number of implied decimal places
inline double packed_decimal_decode(const uint8_t* data, std::size_t len, int scale = 0) {
    if (len == 0) return 0.0;
    int64_t value = 0;
    for (std::size_t i = 0; i < len - 1; ++i) {
        int hi = (data[i] >> 4) & 0x0F;
        int lo = data[i] & 0x0F;
        value = value * 100 + hi * 10 + lo;
    }
    // Last byte: high nibble is digit, low nibble is sign
    int last_digit = (data[len - 1] >> 4) & 0x0F;
    int sign_nibble = data[len - 1] & 0x0F;
    value = value * 10 + last_digit;

    bool negative = (sign_nibble == 0x0D || sign_nibble == 0x0B);
    if (negative) value = -value;

    if (scale == 0) return static_cast<double>(value);
    double divisor = 1.0;
    for (int i = 0; i < scale; ++i) divisor *= 10.0;
    return static_cast<double>(value) / divisor;
}

inline int64_t packed_decimal_decode_int(const uint8_t* data, std::size_t len) {
    if (len == 0) return 0;
    int64_t value = 0;
    for (std::size_t i = 0; i < len - 1; ++i) {
        int hi = (data[i] >> 4) & 0x0F;
        int lo = data[i] & 0x0F;
        value = value * 100 + hi * 10 + lo;
    }
    int last_digit = (data[len - 1] >> 4) & 0x0F;
    int sign_nibble = data[len - 1] & 0x0F;
    value = value * 10 + last_digit;

    bool negative = (sign_nibble == 0x0D || sign_nibble == 0x0B);
    if (negative) value = -value;
    return value;
}

// Encode an integer value to packed decimal bytes.
// len = number of output bytes.  Digits capacity = len*2 - 1.
// sign: 0x0C positive, 0x0D negative (unsigned uses 0x0F)
inline std::vector<uint8_t> packed_decimal_encode(int64_t value, std::size_t len, bool unsigned_sign = false) {
    bool neg = value < 0;
    uint64_t abs_val = static_cast<uint64_t>(neg ? -value : value);

    int max_digits = static_cast<int>(len) * 2 - 1;
    // Extract BCD digits
    std::vector<int> digits(static_cast<std::size_t>(max_digits), 0);
    for (int i = max_digits - 1; i >= 0 && abs_val > 0; --i) {
        digits[static_cast<std::size_t>(i)] = static_cast<int>(abs_val % 10);
        abs_val /= 10;
    }

    std::vector<uint8_t> result(len, 0);
    int d = 0; // digit index
    for (std::size_t i = 0; i < len - 1; ++i) {
        int hi = digits[static_cast<std::size_t>(d++)];
        int lo = digits[static_cast<std::size_t>(d++)];
        result[i] = static_cast<uint8_t>((hi << 4) | lo);
    }
    // Last byte: digit + sign
    int last_digit = digits[static_cast<std::size_t>(d)];
    uint8_t sign_nib = unsigned_sign ? 0x0F : (neg ? 0x0D : 0x0C);
    result[len - 1] = static_cast<uint8_t>((last_digit << 4) | sign_nib);

    return result;
}

// ------------------------------------------------------------------
//  Binary integer (COMP / COMP-4 / COMP-5) — big-endian
// ------------------------------------------------------------------

inline int16_t binary_to_int16(const uint8_t* data) {
    return static_cast<int16_t>(
        (static_cast<uint16_t>(data[0]) << 8) |
         static_cast<uint16_t>(data[1]));
}

inline int32_t binary_to_int32(const uint8_t* data) {
    return static_cast<int32_t>(
        (static_cast<uint32_t>(data[0]) << 24) |
        (static_cast<uint32_t>(data[1]) << 16) |
        (static_cast<uint32_t>(data[2]) << 8) |
         static_cast<uint32_t>(data[3]));
}

inline int64_t binary_to_int64(const uint8_t* data) {
    return static_cast<int64_t>(
        (static_cast<uint64_t>(data[0]) << 56) |
        (static_cast<uint64_t>(data[1]) << 48) |
        (static_cast<uint64_t>(data[2]) << 40) |
        (static_cast<uint64_t>(data[3]) << 32) |
        (static_cast<uint64_t>(data[4]) << 24) |
        (static_cast<uint64_t>(data[5]) << 16) |
        (static_cast<uint64_t>(data[6]) << 8) |
         static_cast<uint64_t>(data[7]));
}

inline void int16_to_binary(int16_t val, uint8_t* out) {
    auto u = static_cast<uint16_t>(val);
    out[0] = static_cast<uint8_t>((u >> 8) & 0xFF);
    out[1] = static_cast<uint8_t>(u & 0xFF);
}

inline void int32_to_binary(int32_t val, uint8_t* out) {
    auto u = static_cast<uint32_t>(val);
    out[0] = static_cast<uint8_t>((u >> 24) & 0xFF);
    out[1] = static_cast<uint8_t>((u >> 16) & 0xFF);
    out[2] = static_cast<uint8_t>((u >> 8) & 0xFF);
    out[3] = static_cast<uint8_t>(u & 0xFF);
}

inline void int64_to_binary(int64_t val, uint8_t* out) {
    auto u = static_cast<uint64_t>(val);
    for (int i = 7; i >= 0; --i) {
        out[i] = static_cast<uint8_t>(u & 0xFF);
        u >>= 8;
    }
}

// ------------------------------------------------------------------
//  Zoned decimal
// ------------------------------------------------------------------

// Zoned decimal: each byte is zone nibble (high) + digit nibble (low).
// For EBCDIC, zone = 0xF for non-sign bytes. Last byte's zone = sign.
// 0xC/0xA/0xF = positive, 0xD/0xB = negative.
inline int64_t zoned_decimal_decode(const uint8_t* data, std::size_t len) {
    if (len == 0) return 0;
    int64_t value = 0;
    for (std::size_t i = 0; i < len; ++i) {
        int digit = data[i] & 0x0F;
        value = value * 10 + digit;
    }
    // Sign from last byte zone nibble
    int zone = (data[len - 1] >> 4) & 0x0F;
    if (zone == 0x0D || zone == 0x0B) value = -value;
    return value;
}

inline std::vector<uint8_t> zoned_decimal_encode(int64_t value, std::size_t len) {
    bool neg = value < 0;
    uint64_t abs_val = static_cast<uint64_t>(neg ? -value : value);

    std::vector<uint8_t> result(len, 0xF0); // zone = 0xF, digit = 0
    for (int i = static_cast<int>(len) - 1; i >= 0 && abs_val > 0; --i) {
        int digit = static_cast<int>(abs_val % 10);
        abs_val /= 10;
        result[static_cast<std::size_t>(i)] = static_cast<uint8_t>(0xF0 | digit);
    }
    // Set sign on last byte
    int last_digit = result[len - 1] & 0x0F;
    uint8_t sign_zone = neg ? 0xD0 : 0xC0;
    result[len - 1] = static_cast<uint8_t>(sign_zone | last_digit);

    return result;
}

}}} // namespace lazarus::db2::ebcdic

#endif // LAZARUS_DB2_EBCDIC_H
