#ifndef MSGPACK_PACKER_H
#define MSGPACK_PACKER_H

#include <limits>
#include <string>
#include <iterator>
#include <algorithm>
#include <cstring>
#include <tuple>
#include <array>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include "platform.h"

namespace msgpack {

using namespace std;

class packer {
public:
    using buffer_type = vector<uint8_t>;

    packer& operator<<(nullptr_t);
    template<typename T> typename enable_if<is_same<bool, T>::value, packer&>::type operator<<(const T value);
    packer& operator<<(const int32_t value);
    packer& operator<<(const int64_t value);
    packer& operator<<(const uint32_t value);
    packer& operator<<(const uint64_t value);
    packer& operator<<(const float value);
    packer& operator<<(const double value);
    packer& operator<<(const string& str);
    packer& operator<<(const char* str);

    template<typename T> packer& operator<<(pair<const T*, size_t> array_tuple);

    template<typename T, size_t N> packer& operator<<(const T (& array)[N]) {
        return *this << make_pair(array, N);
    }

    template<typename T, size_t N> packer& operator<<(const array <T, N> array) {
        return *this << make_pair(array.data(), N);
    }

    template<typename T> packer& operator<<(const vector<T>& v) {
        return *this << make_pair(v.data(), v.size());
    }

    template<typename T> packer& operator<<(const set<T>& s) {
        for (const T& e : s) {
            *this << e;
        }
        return *this;
    }

    template<typename T> packer& operator<<(const unordered_set<T>& s) {
        for (const T& e : s) {
            *this << e;
        }
        return *this;
    }

    template<typename K, typename V> packer& operator<<(const map<K, V>& m) {
        put_map_length(m.size());
        for (const auto& e: m) {
            *this << e.first;
            *this << e.second;
        }
        return *this;
    }

    template<typename K, typename V> packer& operator<<(const unordered_map<K, V>& m) {
        put_map_length(m.size());
        for (const auto& e: m) {
            *this << e.first;
            *this << e.second;
        }
        return *this;
    }

    const vector<uint8_t>& get_buffer() const {
        return _buffer;
    }

private:
    buffer_type _buffer;

    void put_byte(const uint8_t b) {
        _buffer.emplace_back(b);
    }

    template<typename T> void put_int(const T t);;

    void put_string_length(size_t length);
    void put_array_length(size_t length);
    void put_map_length(size_t length);
};

packer& packer::operator<<(nullptr_t) {
    put_byte(0xc0);
    return *this;
}

template<typename T> typename enable_if<is_same<bool, T>::value, packer&>::type packer::operator<<(const T value) {
    if (value) {
        put_byte(0xc3);
    } else {
        put_byte(0xc2);
    }
    return *this;
}

packer& packer::operator<<(const int32_t value) {
    if ((value >= 0 && value <= 0x7f) || (value < 0 && value >= -32)) {
        put_byte(static_cast<uint8_t>(value));
    } else if (value >= numeric_limits<int8_t>::min() && value <= numeric_limits<int8_t>::max()) {
        put_byte(0xd0);
        put_byte(static_cast<uint8_t>(value));
    } else if (value >= numeric_limits<int16_t>::min() && value <= numeric_limits<int16_t>::max()) {
        put_byte(0xd1);
        put_int(static_cast<const int16_t>(value));
    } else {
        put_byte(0xd2);
        put_int(value);
    }
    return *this;
}

packer& packer::operator<<(const int64_t value) {
    if (value >= numeric_limits<int32_t>::min() && value <= numeric_limits<int32_t>::max()) {
        *this << static_cast<int32_t>(value);
    } else {
        put_byte(0xd3);
        put_int(value);
    }

    return *this;
}

packer& packer::operator<<(const uint32_t value) {
    if (value <= 0x7f) {
        put_byte(static_cast<uint8_t>(value));
    } else if (value <= numeric_limits<uint8_t>::max()) {
        put_byte(0xcc);
        put_byte(static_cast<uint8_t>(value));
    } else if (value <= numeric_limits<uint16_t>::max()) {
        put_byte(0xcd);
        put_int(static_cast<const int16_t>(value));
    } else {
        put_byte(0xce);
        put_int(value);
    }
    return *this;
}

packer& packer::operator<<(const uint64_t value) {
    if (value <= numeric_limits<uint32_t>::max()) {
        *this << static_cast<uint32_t>(value);
    } else {
        put_byte(0xcf);
        put_int(value);
    }

    return *this;
}

packer& packer::operator<<(const float value) {
    put_byte(0xca);
    put_int(value);

    return *this;
}

packer& packer::operator<<(const double value) {
    put_byte(0xcb);
    put_int(value);

    return *this;
}

packer& packer::operator<<(const string& str) {
    put_string_length(str.length());
    std::copy(str.data(), str.data() + str.length(), back_inserter(_buffer));

    return *this;
}

packer& packer::operator<<(const char* str) {
    const size_t len = strlen(str);
    put_string_length(len);
    std::copy(str, str + len, back_inserter(_buffer));

    return *this;
}

template<typename T> packer& packer::operator<<(pair<const T*, size_t> array_tuple) {
    put_array_length(get<1>(array_tuple));
    for (size_t i = 0; i < get<1>(array_tuple); ++i) {
        *this << get<0>(array_tuple)[i];
    }

    return *this;
}

template<typename T> void packer::put_int(const T t) {
    union {
        T data;
        uint8_t bytes[sizeof(T)];
    } cvt = { t };
    cvt.data = platform::hton(cvt.data);
    for (uint8_t b : cvt.bytes) { put_byte(b); }
}

void packer::put_string_length(size_t length) {
    if (length < 32) {
        put_byte(uint8_t { 0xa0u } + static_cast<uint8_t>(length));
    } else if (length <= numeric_limits<uint8_t>::max()) {
        put_byte(static_cast<uint8_t>(length));
    } else if (length <= numeric_limits<uint16_t>::max()) {
        put_int(static_cast<uint16_t>(length));
    } else if (length <= numeric_limits<uint32_t>::max()) {
        put_int(static_cast<uint32_t>(length));
    }
}

void packer::put_array_length(size_t length) {
    if (length < 16) {
        put_byte(uint8_t { 0x90u } + static_cast<uint8_t>(length));
    } else if (length <= numeric_limits<uint16_t>::max()) {
        put_int(static_cast<uint16_t>(length));
    } else if (length <= numeric_limits<uint32_t>::max()) {
        put_int(static_cast<uint32_t>(length));
    }
}

void packer::put_map_length(size_t length) {
    if (length < 16) {
        put_byte(uint8_t { 0x80u } + static_cast<uint8_t>(length));
    } else if (length <= numeric_limits<uint16_t>::max()) {
        put_int(static_cast<uint16_t>(length));
    } else if (length <= numeric_limits<uint32_t>::max()) {
        put_int(static_cast<uint32_t>(length));
    }
}

}

#endif //MSGPACK_PACKER_H
