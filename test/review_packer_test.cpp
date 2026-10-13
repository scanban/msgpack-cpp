#include <locale>
#include <gtest/gtest.h>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cstring>
#include <type_traits>
#include <cwchar>
#include <stdexcept>
#include <utility>
#ifndef _WIN32
#include <unistd.h>
#endif

// Pull in every standard header packer.h uses before macroizing `private`,
// so that only packer.h itself is affected (MSVC's STL rejects keyword macros).
#include "../msgpack-platform.h"

// F16 needs access to the private put_*_length helpers (no huge allocation).
#define private public
#include "../packer.h"
#undef private

using msgpack::packer;
using bytes = std::vector<uint8_t>;

// ---- F16: length > 0xFFFFFFFF must not silently emit no header.
TEST(Review_F16, string_length_over_4GiB_writes_header_or_throws) {
    packer p;
    bool threw = false;
    try { p.put_string_length(size_t(0x100000000ull)); } catch (...) { threw = true; }
    EXPECT_TRUE(threw || !p.get_buffer().empty()) << "no header written and no error";
}
TEST(Review_F16, array_length_over_4GiB_writes_header_or_throws) {
    packer p;
    bool threw = false;
    try { p.put_array_length(size_t(0x100000000ull)); } catch (...) { threw = true; }
    EXPECT_TRUE(threw || !p.get_buffer().empty()) << "no header written and no error";
}
TEST(Review_F16, map_length_over_4GiB_writes_header_or_throws) {
    packer p;
    bool threw = false;
    try { p.put_map_length(size_t(0x100000000ull)); } catch (...) { threw = true; }
    EXPECT_TRUE(threw || !p.get_buffer().empty()) << "no header written and no error";
}

// ---- F17: non-const char[] must encode as str like const char[].
TEST(Review_F17, nonconst_char_array_encodes_as_str) {
    char buf[4] = "abc";
    packer p; p << buf;
    EXPECT_EQ(p.get_buffer(), (bytes{0xa3, 0x61, 0x62, 0x63}));
}
TEST(Review_F17, const_char_array_encodes_as_str_baseline) {
    const char buf[4] = "abc";
    packer p; p << buf;
    EXPECT_EQ(p.get_buffer(), (bytes{0xa3, 0x61, 0x62, 0x63}));
}
// char* (non-const) and map key char* do not compile: see .review/scratch-B/F17_*.cpp

// ---- F13: wstring -> UTF-8 must yield valid UTF-8 (RFC 3629) or fail.
TEST(Review_F13, valid_non_bmp_encodes_4_bytes_baseline) {
#if WCHAR_MAX > 0xFFFF
    const std::wstring w(1, static_cast<wchar_t>(0x1F600));
#else
    const std::wstring w{ static_cast<wchar_t>(0xD83D), static_cast<wchar_t>(0xDE00) };  // surrogate pair
#endif
    packer p; p << w;
    EXPECT_EQ(p.get_buffer(), (bytes{0xa4, 0xf0, 0x9f, 0x98, 0x80}));
}
TEST(Review_F13, lone_surrogate_throws_library_exception) {
    EXPECT_THROW({ packer p; p << std::wstring(1, static_cast<wchar_t>(0xD800)); }, msgpack::output_conversion_error);
}
#if WCHAR_MAX > 0xFFFF
TEST(Review_F13, out_of_range_0x110000_throws_library_exception) {
    EXPECT_THROW({ packer p; p << std::wstring(1, static_cast<wchar_t>(0x110000)); }, msgpack::output_conversion_error);
}
#else
TEST(Review_F13, lone_low_surrogate_throws_library_exception) {
    EXPECT_THROW({ packer p; p << std::wstring(1, static_cast<wchar_t>(0xDC00)); }, msgpack::output_conversion_error);
}
#endif

// ---- F18: README promises integers / arrays of integers; compile failures are in scratch-B probes.

// ---- F23: appending packers must grow geometrically.
TEST(Review_F23, append_packer_reallocations_are_logarithmic) {
    packer small; small << 1;
    packer big;
    // observe capacity via a copy of the buffer is impossible; use the (test-visible) member.
    size_t changes = 0, last = big._buffer.capacity();
    const int K = 5000;
    for (int i = 0; i < K; ++i) {
        big << small;
        if (big._buffer.capacity() != last) { ++changes; last = big._buffer.capacity(); }
    }
    EXPECT_LT(changes, 64u) << changes << " reallocations for " << K << " appends";
}
// Self-append (p << p) passes iterators of the same vector to insert(): violates
// [sequence.reqmts] Table 100. Proof via _GLIBCXX_DEBUG: .review/scratch-B/F23_self.cpp
TEST(Review_F23, self_append_doubles_content) {
    packer p; p << 1 << 2;
    p << p;
    EXPECT_EQ(p.get_buffer(), (bytes{1, 2, 1, 2}));
}

// ---- F24: nullptr const char* encodes nil (0xc0).
TEST(Review_F24, null_cstr_encodes_nil) {
    EXPECT_EXIT({
        const char* volatile s = nullptr;
        packer p;
        p << (const char*)s;
        std::exit(p.get_buffer() == bytes{0xc0} ? 0 : 3);
    }, ::testing::ExitedWithCode(0), "");
}

// ---- F25: str_printf<0> must terminate.
// The body lives in a function: preprocessor directives inside a macro argument
// (EXPECT_EXIT) are undefined behaviour and rejected by MSVC.
static void str_printf_N0_then_exit() {
#ifndef _WIN32
    alarm(2);  // watchdog: a regression would hang instead of failing
#endif
    std::string s = platform::str_printf<0>("%d", 1);
    std::exit(s == "1" ? 0 : 3);
}
TEST(Review_F25, str_printf_N0_terminates) {
    EXPECT_EXIT(str_printf_N0_then_exit(), ::testing::ExitedWithCode(0), "");
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
