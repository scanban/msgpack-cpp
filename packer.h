#ifndef MSGPACK_PACKER_H
#define MSGPACK_PACKER_H

#include <limits>
#include <string>
#include <iterator>
#include <algorithm>
#include <cstring>
#include "platform.h"

namespace msgpack {

using namespace std;

class packer {
public:
    using buffer_type = vector<uint8_t>;

    template <typename T> struct is_pair : false_type {};
    template <typename K, typename V> struct is_pair<pair<K, V>> : true_type {};

    inline packer& operator<<(nullptr_t);
    template<typename T> typename enable_if<is_same<bool, T>::value, packer&>::type operator<<(const T value);
    inline packer& operator<<(const int32_t value);
    inline packer& operator<<(const int64_t value);
    inline packer& operator<<(const uint32_t value);
    inline packer& operator<<(const uint64_t value);
    inline packer& operator<<(const float value);
    inline packer& operator<<(const double value);
    inline packer& operator<<(const string& str);
    inline packer& operator<<(const char* str);
    inline packer& operator<<(const packer& value);

    template <typename T> typename enable_if<not is_fundamental<T>::value, packer&>::type operator <<(const T& val) {
        return put<T>(begin(val), end(val));
    }

    template<typename T, size_t N> packer& operator<<(const T (& array)[N]);

    vector<uint8_t> get_buffer() const {
        return _buffer;
    }

private:
    buffer_type _buffer;

    void put_byte(const uint8_t b) {
        _buffer.emplace_back(b);
    }

    template<typename T> void put_numeric(const T t);

    inline void put_string_length(size_t length);
    inline void put_array_length(size_t length);
    inline void put_map_length(size_t length);

    template<typename T, typename U = typename iterator_traits<typename T::const_iterator>::value_type>
    typename enable_if<is_pair<U>::value, packer&>::type
    put(typename T::const_iterator begin, typename T::const_iterator end) {
        put_map_length(static_cast<size_t>(distance(begin, end)));
        for_each(begin, end, [this](const pair<typename U::first_type, typename U::second_type>& e) {
            *this << e.first;
            *this << e.second;
        });
        return *this;
    }

    template<typename T, typename U = typename iterator_traits<typename T::const_iterator>::value_type>
    typename enable_if<not is_pair<U>::value, packer&>::type
    put(typename T::const_iterator begin, typename T::const_iterator end) {
        put_array_length(static_cast<size_t>(distance(begin, end)));
        for_each(begin, end, [this](const U& e) {
            *this << e;
        });
        return *this;
    }
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
        put_numeric(static_cast<const int16_t>(value));
    } else {
        put_byte(0xd2);
        put_numeric(value);
    }
    return *this;
}

packer& packer::operator<<(const int64_t value) {
    if (value >= numeric_limits<int32_t>::min() && value <= numeric_limits<int32_t>::max()) {
        *this << static_cast<int32_t>(value);
    } else {
        put_byte(0xd3);
        put_numeric(value);
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
        put_numeric(static_cast<const int16_t>(value));
    } else {
        put_byte(0xce);
        put_numeric(value);
    }
    return *this;
}

packer& packer::operator<<(const uint64_t value) {
    if (value <= numeric_limits<uint32_t>::max()) {
        *this << static_cast<uint32_t>(value);
    } else {
        put_byte(0xcf);
        put_numeric(value);
    }

    return *this;
}

packer& packer::operator<<(const float value) {
    put_byte(0xca);
    put_numeric(value);

    return *this;
}

packer& packer::operator<<(const double value) {
    put_byte(0xcb);
    put_numeric(value);

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

packer& packer::operator<<(const packer& value) {
    _buffer.reserve(_buffer.size() + value._buffer.size());
    _buffer.insert(_buffer.end(), value._buffer.cbegin(), value._buffer.cend());
    return *this;
}

template<typename T> void packer::put_numeric(const T t) {
    union {
        T data;
        uint8_t bytes[sizeof(T)];
    } cvt = { t };
    cvt.data = platform::hton(cvt.data);
    for (uint8_t b : cvt.bytes) { put_byte(b); }
}

template<typename T, size_t N> packer& packer::operator<<(const T (& array)[N]) {
    put_array_length(N);
    std::for_each(array, array + N, [this] (const T& e) {
        *this << e;
    });
    return *this;
}

void packer::put_string_length(size_t length) {
    if (length < 32) {
        put_byte(uint8_t { 0xa0u } + static_cast<uint8_t>(length));
    } else if (length <= numeric_limits<uint8_t>::max()) {
        put_byte(static_cast<uint8_t>(length));
    } else if (length <= numeric_limits<uint16_t>::max()) {
        put_numeric(static_cast<uint16_t>(length));
    } else if (length <= numeric_limits<uint32_t>::max()) {
        put_numeric(static_cast<uint32_t>(length));
    }
}

void packer::put_array_length(size_t length) {
    if (length < 16) {
        put_byte(uint8_t { 0x90u } + static_cast<uint8_t>(length));
    } else if (length <= numeric_limits<uint16_t>::max()) {
        put_numeric(static_cast<uint16_t>(length));
    } else if (length <= numeric_limits<uint32_t>::max()) {
        put_numeric(static_cast<uint32_t>(length));
    }
}

void packer::put_map_length(size_t length) {
    if (length < 16) {
        put_byte(uint8_t { 0x80u } + static_cast<uint8_t>(length));
    } else if (length <= numeric_limits<uint16_t>::max()) {
        put_numeric(static_cast<uint16_t>(length));
    } else if (length <= numeric_limits<uint32_t>::max()) {
        put_numeric(static_cast<uint32_t>(length));
    }
}

}

#endif //MSGPACK_PACKER_H
