#include <cwchar>
#include <locale>
#include <gtest/gtest.h>
#include <cstdlib>
#include "../packer.h"
#include "../unpacker.h"

using namespace msgpack;
using V = std::vector<uint8_t>;

// F2: skip_bytes computes _it + count past end() (UB, C++11 [expr.add]/5).
// Only observable with checked iterators:
//   g++ -std=c++11 -D_GLIBCXX_DEBUG -I.. probe (see report); in a normal build this passes.
TEST(Review_F2, skip_truncated_str8_throws_underflow) {
    unpacker u{ V{ 0xd9, 0x05, 'a' } };
    EXPECT_THROW(u.skip(), output_underflow_error);
}
TEST(Review_F2, skip_truncated_str32_huge_throws_underflow) {
    unpacker u{ V{ 0xdb, 0xff, 0xff, 0xff, 0xff } };
    EXPECT_THROW(u.skip(), output_underflow_error);
}

// F3: ILP32-only (str32 length >= 0x80000000 becomes negative ptrdiff_t). Not testable on LP64.

// F4: failed unpacker >> unpacker must leave target consistent.
// Spec of contract: C++11 [iterator.requirements.general] (iterators from the same range).
// Run under -fsanitize=address to see heap-buffer-overflow.
TEST(Review_F4, failed_unpacker_extract_leaves_target_consistent) {
    unpacker dst{ V{ 1, 2, 3 } };
    unpacker src{ V{ 0xd2, 0x00 } };
    EXPECT_ANY_THROW(src >> dst);
    int32_t x = -1;
    try {
        dst >> x;
        EXPECT_EQ(1, x) << "target must be unchanged (reads 1) or throw underflow";
    } catch (const output_underflow_error&) {
    }
}

// Guard: strict signedness on decode is the intended contract (F5 rejected).
TEST(Review_Guard, negative_into_unsigned_throws) {
    unpacker u{ V{ 0xd0, 0xfb } };
    uint32_t v = 0;
    EXPECT_ANY_THROW(u >> v);
}

// F13: invalid UTF-8 into std::wstring must throw the library exception.
TEST(Review_F13, invalid_utf8_into_wstring_throws_library_error) {
    unpacker u{ V{ 0xa1, 0xff } };
    std::wstring w;
    EXPECT_THROW(u >> w, output_conversion_error);
}

// F6: skip() must handle self-describing bin/ext formats (spec).
TEST(Review_F6, skip_bin8) {
    unpacker u{ V{ 0xc4, 0x01, 0x00 } };
    EXPECT_NO_THROW(u.skip());
    EXPECT_TRUE(u.empty());
}
TEST(Review_F6, skip_bin16) {
    unpacker u{ V{ 0xc5, 0x00, 0x01, 0x00 } };
    EXPECT_NO_THROW(u.skip());
    EXPECT_TRUE(u.empty());
}
TEST(Review_F6, skip_bin32) {
    unpacker u{ V{ 0xc6, 0x00, 0x00, 0x00, 0x01, 0x00 } };
    EXPECT_NO_THROW(u.skip());
    EXPECT_TRUE(u.empty());
}
TEST(Review_F6, skip_ext8) {
    unpacker u{ V{ 0xc7, 0x01, 0x05, 0x00 } };  // len, type, data
    EXPECT_NO_THROW(u.skip());
    EXPECT_TRUE(u.empty());
}
TEST(Review_F6, skip_ext16) {
    unpacker u{ V{ 0xc8, 0x00, 0x01, 0x05, 0x00 } };
    EXPECT_NO_THROW(u.skip());
    EXPECT_TRUE(u.empty());
}
TEST(Review_F6, skip_ext32) {
    unpacker u{ V{ 0xc9, 0x00, 0x00, 0x00, 0x01, 0x05, 0x00 } };
    EXPECT_NO_THROW(u.skip());
    EXPECT_TRUE(u.empty());
}
TEST(Review_F6, skip_fixext1) {
    unpacker u{ V{ 0xd4, 0x01, 0x00 } };
    EXPECT_NO_THROW(u.skip());
    EXPECT_TRUE(u.empty());
}
TEST(Review_F6, skip_fixext16) {
    V v{ 0xd8, 0x01 };
    v.resize(2 + 16, 0);
    unpacker u{ v };
    EXPECT_NO_THROW(u.skip());
    EXPECT_TRUE(u.empty());
}
TEST(Review_F6, vector_of_unpackers_with_bin) {
    unpacker u{ V{ 0x91, 0xc4, 0x00 } };
    std::vector<unpacker> v;
    EXPECT_NO_THROW(u >> v);
    EXPECT_EQ(1u, v.size());
}

// F7: deep nesting must not crash; an exception is acceptable.
TEST(Review_F7, deeply_nested_skip_does_not_crash) {
    EXPECT_EXIT({
        V v(2000000, 0x91);
        v.push_back(0xc0);
        unpacker u{ v };
        try { u.skip(); } catch (const std::exception&) {}
        std::exit(0);
    }, ::testing::ExitedWithCode(0), "");
}

// F8: existing test msgpack_pack_unpacker_to_string wraps level-0 output in {...}.
TEST(Review_F8, to_string_empty) {
    unpacker u{ V{} };
    EXPECT_EQ("{}", to_string(u));
}

// F9: map keys may be any type (spec); to_string must not throw.
TEST(Review_F9, to_string_int_key_map) {
    unpacker u{ V{ 0x81, 0x01, 0x02 } };
    std::string s;
    ASSERT_NO_THROW(s = to_string(u));
    EXPECT_NE(std::string::npos, s.find('1'));
    EXPECT_NE(std::string::npos, s.find('2'));
}

// N2: invalid UTF-8 into std::wstring must throw output_conversion_error (RFC 3629 s3, msgpack str = UTF-8).
TEST(Review_N2, truncated_multibyte_throws) {
    unpacker u{ V{ 0xa2, 0xe2, 0x82 } };
    std::wstring w;
    EXPECT_THROW(u >> w, output_conversion_error);
    std::string s;
    ASSERT_NO_THROW(u >> s);
    EXPECT_EQ(std::string("\xe2\x82"), s);
}
TEST(Review_N2, encoded_surrogate_throws) {
    unpacker u{ V{ 0xa3, 0xed, 0xa0, 0x80 } };
    std::wstring w;
    EXPECT_THROW(u >> w, output_conversion_error);
    std::string s;
    ASSERT_NO_THROW(u >> s);
    EXPECT_EQ(std::string("\xed\xa0\x80"), s);
}

// N1 guard: non-BMP wstring must round-trip (fails on 16-bit wchar_t if decoder is UCS-2).
TEST(Review_N1, non_bmp_roundtrip) {
    std::wstring in = L"a\u00e9\u20ac";
#if WCHAR_MAX > 0xFFFF
    in += static_cast<wchar_t>(0x1f600);
#else
    in += std::wstring{ static_cast<wchar_t>(0xd83d), static_cast<wchar_t>(0xde00) };  // surrogate pair
#endif
    packer p;
    p << in;
    unpacker u{ p.get_buffer() };
    std::wstring out;
    ASSERT_NO_THROW(u >> out);
    EXPECT_EQ(in, out);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
