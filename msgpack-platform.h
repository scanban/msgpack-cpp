#ifndef MSGPACK_PLATFORM_H
#define MSGPACK_PLATFORM_H

//*****************************************************************************
// Intrinsics, derived from the boost intrinsic
//*****************************************************************************

#ifndef __has_builtin         // Optional of course
#define __has_builtin(x) 0  // Compatibility with non-clang compilers
#endif

//  GCC and Clang recent versions provide intrinsic byte swaps via builtins
#if (defined(__clang__) && __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap64)) \
 || (defined(__GNUC__ ) && \
  (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)))
# define PLATFORM_ENDIAN_INTRINSIC_MSG "__builtin_bswap16, etc."
// prior to 4.8, gcc did not provide __builtin_bswap16 on some platforms so we emulate it
// see http://gcc.gnu.org/bugzilla/show_bug.cgi?id=52624
// Clang has a similar problem, but their feature test macros make it easier to detect
# if (defined(__clang__) && __has_builtin(__builtin_bswap16)) \
 || (defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)))
#   define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2(x) __builtin_bswap16(x)
# else
#   define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2(x) __builtin_bswap32((x) << 16)
# endif
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4(x) __builtin_bswap32(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8(x) __builtin_bswap64(x)

//  Linux systems provide the byteswap.h header, with 
#elif defined(__linux__)
//  don't check for obsolete forms defined(linux) and defined(__linux) on the theory that
//  compilers that predefine only these are so old that byteswap.h probably isn't present.
# define PLATFORM_ENDIAN_INTRINSIC_MSG "byteswap.h bswap_16, etc."
# include <byteswap.h>
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2(x) bswap_16(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4(x) bswap_32(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8(x) bswap_64(x)

#elif defined(_MSC_VER)
//  Microsoft documents these as being compatible since Windows 95 and specificly
//  lists runtime library support since Visual Studio 2003 (aka 7.1).
# define PLATFORM_ENDIAN_INTRINSIC_MSG "cstdlib _byteswap_ushort, etc."
# include <cstdlib>
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2(x) _byteswap_ushort(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4(x) _byteswap_ulong(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8(x) _byteswap_uint64(x)
#else
// No intrinsics: use the portable shift based implementations from platform::detail (see below)
# define PLATFORM_ENDIAN_NO_INTRINSICS
# define PLATFORM_ENDIAN_INTRINSIC_MSG "no byte swap intrinsics"
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2(x) ::platform::detail::portable_byte_swap_2(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4(x) ::platform::detail::portable_byte_swap_4(x)
# define PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8(x) ::platform::detail::portable_byte_swap_8(x)
#endif

// Prefer the compiler provided byte order, fall back to OS specific headers
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && defined(__ORDER_LITTLE_ENDIAN__)
#   if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#       define  PLATFORM_BIG_ENDIAN    1
#       define  PLATFORM_LITTLE_ENDIAN 0
#   elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#       define  PLATFORM_BIG_ENDIAN    0
#       define  PLATFORM_LITTLE_ENDIAN 1
#   else
#       error "Unsupported OS endianness"
#   endif
#elif defined(__linux__)
#   include <endian.h>
#   if __BYTE_ORDER == __BIG_ENDIAN
#       define  PLATFORM_BIG_ENDIAN    1
#       define  PLATFORM_LITTLE_ENDIAN 0
#   else
#       define  PLATFORM_BIG_ENDIAN    0
#       define  PLATFORM_LITTLE_ENDIAN 1
#endif
#elif defined(__MACH__)
#   include <machine/endian.h>
#   if BYTE_ORDER == BIG_ENDIAN
#       define  PLATFORM_BIG_ENDIAN    1
#       define  PLATFORM_LITTLE_ENDIAN 0
#else
#       define  PLATFORM_BIG_ENDIAN    0
#       define  PLATFORM_LITTLE_ENDIAN 1
#endif
#elif defined(_WIN32)
#   define  PLATFORM_BIG_ENDIAN    0
#   define  PLATFORM_LITTLE_ENDIAN 1
#else
#error "Unable to determine OS endianness"
#endif

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <string>
#include <cstdio>
#include <cstring>
#include <stdexcept>

namespace msgpack {

class output_conversion_error : public std::logic_error {
public:
    output_conversion_error() : logic_error("unknown conversion error") {}
    explicit output_conversion_error(const char* s) : logic_error(s) {}
    explicit output_conversion_error(const uint8_t i) : logic_error(std::string("invalid conversion: ") + std::to_string(i)) {}
};

class output_underflow_error : public std::logic_error {
public:
    output_underflow_error() : std::logic_error("underflow error") {}
    explicit output_underflow_error(const char* s) : std::logic_error(s) {}
};

}

namespace platform {
namespace detail {
constexpr uint16_t portable_byte_swap_2(const uint16_t x) {
    return static_cast<uint16_t>(((x & 0x00ffu) << 8) | ((x & 0xff00u) >> 8));
}
constexpr uint32_t portable_byte_swap_4(const uint32_t x) {
    return ((x & 0x000000ffu) << 24) | ((x & 0x0000ff00u) << 8) | ((x & 0x00ff0000u) >> 8) | ((x & 0xff000000u) >> 24);
}
constexpr uint64_t portable_byte_swap_8(const uint64_t x) {
    return ((x & 0x00000000000000ffull) << 56) | ((x & 0x000000000000ff00ull) << 40) |
           ((x & 0x0000000000ff0000ull) << 24) | ((x & 0x00000000ff000000ull) << 8) |
           ((x & 0x000000ff00000000ull) >> 8) | ((x & 0x0000ff0000000000ull) >> 24) |
           ((x & 0x00ff000000000000ull) >> 40) | ((x & 0xff00000000000000ull) >> 56);
}
}

template<typename T> constexpr typename std::enable_if<sizeof(T) == 1, T>::type byte_swap(const T t) {
    return t;
}
template<typename T> constexpr typename std::enable_if<sizeof(T) == 2, T>::type byte_swap(const T t) {
    return (T) (PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_2((uint16_t) t));
}
template<typename T> constexpr typename std::enable_if<sizeof(T) == 4, T>::type byte_swap(const T t) {
    return (T) (PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4((uint32_t) t));
}
template<typename T> constexpr typename std::enable_if<sizeof(T) == 8, T>::type byte_swap(const T t) {
    return (T) (PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8((uint64_t) t));
}
inline float byte_swap(const float t) {
    uint32_t tmp;
    std::memcpy(&tmp, &t, sizeof(tmp));
    tmp = PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_4(tmp);
    float ret;
    std::memcpy(&ret, &tmp, sizeof(ret));
    return ret;
}
inline double byte_swap(const double t) {
    uint64_t tmp;
    std::memcpy(&tmp, &t, sizeof(tmp));
    tmp = PLATFORM_ENDIAN_INTRINSIC_BYTE_SWAP_8(tmp);
    double ret;
    std::memcpy(&ret, &tmp, sizeof(ret));
    return ret;
}

#if PLATFORM_BIG_ENDIAN
constexpr bool big_endian() { return true; }
constexpr bool little_endian() { return false; }
#else
constexpr bool big_endian() { return false; }
constexpr bool little_endian() { return true; }
#endif

template<typename T> constexpr T hton(T t) {
    return little_endian() ? byte_swap(t) : t;
}

template<typename T> constexpr T ntoh(T t) {
    return hton(t);
}

constexpr uint16_t hton_s(const uint16_t t) { return hton(t); }
constexpr uint32_t hton_l(const uint32_t t) { return hton(t); }
constexpr uint64_t hton_q(const uint64_t t) { return hton(t); }

constexpr uint16_t ntoh_s(const uint16_t t) { return ntoh(t); }
constexpr uint32_t ntoh_l(const uint32_t t) { return ntoh(t); }
constexpr uint64_t ntoh_q(const uint64_t t) { return ntoh(t); }

template<std::string::size_type N = 128, typename ... Args> std::string str_printf(const char* fmt, Args ...args) {
    std::string out;
    int size_used;

    // N is only the initial buffer size (may be 0); snprintf reports the exact size needed for the retry
    for (std::string::size_type size_needed = N;;) {
        out.resize(size_needed);
        if ((size_used = std::snprintf(&out[0], size_needed, fmt, args...)) < 0) { return std::string("FMT ERROR"); }
        if (static_cast<std::string::size_type>(size_used) < size_needed) { break; }
        size_needed = static_cast<std::string::size_type>(size_used) + 1;
    }
    out.resize(static_cast<std::string::size_type>(size_used));
    return out;
}
}

#endif //MSGPACK_PLATFORM_H
