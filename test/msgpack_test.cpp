#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <packer.h>
#include <unpacker.h>

using namespace msgpack;

template<typename T> T get_value(unpacker& u) {
    T val;
    u >> val;
    return val;
}

TEST(MSGPACK_PACKER_BASE, msgpack_bool) {
    packer p;

    p << true;
    p << false;

    EXPECT_EQ(p.get_buffer().size(), 2u);
    EXPECT_EQ(p.get_buffer()[0], 0xc3u);
    EXPECT_EQ(p.get_buffer()[1], 0xc2u);

    unpacker u{ p.get_buffer() };

    EXPECT_EQ(u.type(), unpacker::T_BOOLEAN);
    EXPECT_TRUE(get_value<bool>(u));
    EXPECT_EQ(u.type(), unpacker::T_BOOLEAN);
    EXPECT_FALSE(get_value<bool>(u));
    EXPECT_TRUE(u.empty());
}

TEST(MSGPACK_PACKER_BASE, msgpack_int8) {
    packer p;
    p << 1;
    p << -31;
    p << -63;

    EXPECT_EQ(p.get_buffer()[0], 0x01);
    EXPECT_EQ(p.get_buffer()[1], 0xe1);
    EXPECT_EQ(p.get_buffer()[2], 0xd0);
    EXPECT_EQ(p.get_buffer()[3], 0xc1);

    unpacker u{ p.get_buffer() };

    EXPECT_EQ(u.type(), unpacker::T_INT8);
    EXPECT_EQ(get_value<int8_t>(u), 1);
    EXPECT_EQ(u.type(), unpacker::T_INT8);
    EXPECT_EQ(get_value<int8_t>(u), -31);
    EXPECT_EQ(u.type(), unpacker::T_INT8);
    EXPECT_EQ(get_value<int8_t>(u), -63);

    EXPECT_TRUE(u.empty());
}

TEST(MSGPACK_PACKER_BASE, msgpack_int16) {
    packer p;
    p << (1 << 8);
    p << (127 << 8);
    p << (-128 << 8);

    unpacker u{ p.get_buffer() };

    EXPECT_EQ(u.type(), unpacker::T_INT16);
    EXPECT_EQ(get_value<int16_t>(u), 1 << 8);
    EXPECT_EQ(u.type(), unpacker::T_INT16);
    EXPECT_EQ(get_value<int16_t>(u), 127 << 8);
    EXPECT_EQ(u.type(), unpacker::T_INT16);
    EXPECT_EQ(get_value<int16_t>(u), -128 << 8);

    EXPECT_TRUE(u.empty());
}

TEST(MSGPACK_PACKER_BASE, msgpack_int32) {
    packer p;
    p << (int32_t{ 1 } << 24);
    p << (int32_t{ 127 } << 24);
    p << (int32_t{ -128 } << 24);

    unpacker u{ p.get_buffer() };

    EXPECT_EQ(u.type(), unpacker::T_INT32);
    EXPECT_EQ(get_value<int32_t>(u), int32_t{ 1 } << 24);
    EXPECT_EQ(u.type(), unpacker::T_INT32);
    EXPECT_EQ(get_value<int32_t>(u), int32_t{ 127 } << 24);
    EXPECT_EQ(u.type(), unpacker::T_INT32);
    EXPECT_EQ(get_value<int32_t>(u), int32_t{ -128 } << 24);

    EXPECT_TRUE(u.empty());
}

TEST(MSGPACK_PACKER_BASE, msgpack_int64) {
    packer p;
    p << (int64_t{ 1 } << 48);
    p << (int64_t{ 127 } << 48);
    p << (int64_t{ -128 } << 48);

    unpacker u{ p.get_buffer() };

    EXPECT_EQ(u.type(), unpacker::T_INT64);
    EXPECT_EQ(get_value<int64_t>(u), int64_t{ 1 } << 48);
    EXPECT_EQ(u.type(), unpacker::T_INT64);
    EXPECT_EQ(get_value<int64_t>(u), int64_t{ 127 } << 48);
    EXPECT_EQ(u.type(), unpacker::T_INT64);
    EXPECT_EQ(get_value<int64_t>(u), int64_t{ -128 } << 48);

    EXPECT_TRUE(u.empty());
}

TEST(MSGPACK_PACKER_BASE, msgpack_uint8) {
    packer p;
    p << 1u;
    p << 127u;
    p << 255u;

    EXPECT_EQ(p.get_buffer()[0], 0x01);
    EXPECT_EQ(p.get_buffer()[1], 0x7f);
    EXPECT_EQ(p.get_buffer()[2], 0xcc);
    EXPECT_EQ(p.get_buffer()[3], 0xff);

    unpacker u{ p.get_buffer() };

    EXPECT_EQ(u.type(), unpacker::T_INT8);
    EXPECT_EQ(get_value<uint8_t>(u), 1);
    EXPECT_EQ(u.type(), unpacker::T_INT8);
    EXPECT_EQ(get_value<uint8_t>(u), 127);
    EXPECT_EQ(u.type(), unpacker::T_UINT8);
    EXPECT_EQ(get_value<uint8_t>(u), 255);

    EXPECT_TRUE(u.empty());
}

TEST(MSGPACK_PACKER_BASE, msgpack_str) {
    packer p;
    p << "test";

    unpacker u{ p.get_buffer() };
    EXPECT_EQ(u.type(), unpacker::T_STRING);
    EXPECT_EQ(get_value<string>(u), "test");

    EXPECT_TRUE(u.empty());
}

TEST(MSGPACK_PACKER_BASE, msgpack_array) {
    packer p;
    vector<int8_t> v_in{ 1, 2, 3, 4, -5 };
    p << v_in;

    unpacker u{ p.get_buffer() };
    vector<int8_t> v_out;
    u >> v_out;

    EXPECT_THAT(v_in, ::testing::ContainerEq(v_out));
    EXPECT_TRUE(u.empty());
}

TEST(MSGPACK_PACKER_BASE, msgpack_map) {
    packer p;
    map<int, int> m = {{ 1, 10 },
                       { 2, 20 },
                       { 3, 30 }};

    p << m;

    unpacker u{ p.get_buffer() };
    map<int, int> m_out;
    u >> m_out;

    EXPECT_THAT(m, ::testing::ContainerEq(m_out));
    EXPECT_TRUE(u.empty());
}

#define TEST_SKIP(_P) \
    do { \
        packer p; \
        p << (_P); \
        unpacker u { p.get_buffer() }; \
        u >> skip; \
        EXPECT_TRUE(u.empty()); \
    } while(0)


TEST(MSGPACK_PACKER_BASE, msgpack_unpack_skip) {
    TEST_SKIP(true);
    TEST_SKIP(int8_t(1));
    TEST_SKIP(uint8_t(1));
    TEST_SKIP(numeric_limits<int16_t>::max());
    TEST_SKIP(numeric_limits<uint16_t>::max());
    TEST_SKIP(numeric_limits<int32_t>::max());
    TEST_SKIP(numeric_limits<uint32_t>::max());
    TEST_SKIP(numeric_limits<int64_t>::max());
    TEST_SKIP(numeric_limits<uint64_t>::max());
    TEST_SKIP(float{ 1.0 });
    TEST_SKIP(double{ 1.0 });

    vector<int8_t> v{ 1, 2, 3, 4, -5 };
    TEST_SKIP(v);

    map<int, int> m = {{ 1, 10 },
                       { 2, 20 },
                       { 3, 30 }};

    TEST_SKIP(m);
}

TEST(MSGPACK_PACKER_BASE, msgpack_unpack_unpacker_simple) {
    packer p;
    p << 1;

    unpacker u(p.get_buffer());
    unpacker t;
    EXPECT_TRUE(t.empty());

    u >> t;

    EXPECT_TRUE(u.empty());
    EXPECT_EQ(t.type(), unpacker::T_INT8);
    EXPECT_EQ(get_value<int8_t>(t), 1);
}

TEST(MSGPACK_PACKER_BASE, msgpack_pack_packer_simple) {
    packer p;
    p << 1 << (packer{} << 2) << 3;

    unpacker u(p.get_buffer());
    EXPECT_EQ(get_value<int8_t>(u), 1);
    EXPECT_EQ(get_value<int8_t>(u), 2);
    EXPECT_EQ(get_value<int8_t>(u), 3);
}

TEST(MSGPACK_PACKER_BASE, msgpack_pack_packer_array) {
    packer p;
    vector<packer> vp{ packer{} << 1, packer{} << "test", packer{} << 100 };
    p << vp;

    unpacker u{ p.get_buffer() };
    EXPECT_EQ(u.type(), unpacker::T_ARRAY);

    vector<unpacker> vu;
    u >> vu;
    EXPECT_EQ(get_value<int8_t>(vu[0]), 1);
    EXPECT_EQ(get_value<string>(vu[1]), "test");
    EXPECT_EQ(get_value<int8_t>(vu[2]), 100);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}