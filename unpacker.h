#ifndef MSGPACK_UNPACKER_H
#define MSGPACK_UNPACKER_H

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
#include <memory>

namespace msgpack {

using namespace std;

class output_conversion_error : public logic_error {
public:
    output_conversion_error() : logic_error("unknown conversion error") {}
    output_conversion_error(const char* s) : logic_error(s) {}
    output_conversion_error(const uint8_t i) : logic_error(string("invalid conversion: ") + to_string(i)) {}
};

class output_underflow_error : public logic_error {
public:
    output_underflow_error() : logic_error("underflow error") {}
    output_underflow_error(const char* s) : logic_error(s) {}
};

struct unpacker_skip {};
constexpr unpacker_skip const skip{};

class unpacker {
public:
    using buffer_type = vector<uint8_t>;

    enum data_type_t {
        T_UNKNOWN,
        T_BOOLEAN,
        T_NULL,
        T_INT8,
        T_INT16,
        T_INT32,
        T_INT64,
        T_UINT8,
        T_UINT16,
        T_UINT32,
        T_UINT64,
        T_FLOAT,
        T_DOUBLE,
        T_STRING,
        T_BINARY,
        T_EXTERNAL,
        T_ARRAY,
        T_MAP,
    };

    unpacker() : _it{ nullptr }, _it_end{ nullptr } {}

    unpacker(const buffer_type& buf)
            : _buffer(make_shared<buffer_type>(buf)), _it{ _buffer->cbegin() }, _it_end{ _buffer->cend() } {}
    unpacker(buffer_type&& buf)
            : _buffer(make_shared<buffer_type>(move(buf))), _it{ _buffer->cbegin() }, _it_end{ _buffer->cend() } {}

    unpacker& operator>>(bool& value);
    unpacker& operator>>(int8_t& value);
    unpacker& operator>>(int16_t& value);
    unpacker& operator>>(int32_t& value);
    unpacker& operator>>(int64_t& value);
    unpacker& operator>>(uint8_t& value);
    unpacker& operator>>(uint16_t& value);
    unpacker& operator>>(uint32_t& value);
    unpacker& operator>>(uint64_t& value);
    unpacker& operator>>(string& value);
    unpacker& operator>>(unpacker& value);

    unpacker& operator>>(const unpacker_skip) {
        return skip();
    }

    template<typename T> unpacker& operator>>(vector<T>& vec);
    template<typename K, typename V> unpacker& operator>>(map<K, V>& map);

    bool empty() const { return _it == _it_end; }
    const data_type_t type() const;
    unpacker& skip();

private:
    enum storage_type_t : uint8_t {
        SFIXINT = 1,
        SFIXARR = 2,
        SFIXMAP = 3,
        SFIXSTR = 4,
        SNIL = 5,
        SUNUSED = 0,
        SFALSE = 6,
        STRUE = 7,
        SBIN8 = 8,
        SBIN16 = 9,
        SBIN32 = 10,
        SEXT8 = 11,
        SEXT16 = 12,
        SEXT32 = 13,
        SFLT32 = 14,
        SFLT64 = 15,
        SUINT8 = 16,
        SUINT16 = 17,
        SUINT32 = 18,
        SUINT64 = 19,
        SINT8 = 20,
        SINT16 = 21,
        SINT32 = 22,
        SINT64 = 23,

        SFEXT1 = 24,
        SFEXT2 = 25,
        SFEXT4 = 26,
        SFEXT8 = 27,
        SFEXT16 = 28,
        SSTR8 = 29,
        SSTR16 = 30,
        SSTR32 = 31,
        SARR16 = 32,
        SARR32 = 33,
        SMAP16 = 34,
        SMAP32 = 35,
        SFIXNINT = 36,
    };

    shared_ptr<buffer_type> _buffer;
    buffer_type::const_iterator _it;
    buffer_type::const_iterator _it_end;

    uint8_t peek_byte() const {
        if (_it != _it_end) { return *_it; }
        else { throw output_underflow_error{}; }
    }

    uint8_t get_byte() {
        if (_it != _it_end) { return *_it++; }
        else { throw output_underflow_error(); }
    }

    void skip_bytes(size_t count) {
        if (_it + count > _it_end) { throw output_underflow_error(); }
        _it += count;
    }

    size_t get_string_length();

    size_t get_array_length();

    size_t get_map_length();

    template<typename T> T get_int() {
        union {
            T data;
            uint8_t bytes[sizeof(T)];
        } cvt;
        for (uint8_t& b : cvt.bytes) { b = get_byte(); }
        return platform::ntoh(cvt.data);
    };

    template<typename T> T get_value() {
        T val;
        *this >> val;
        return val;
    }

    static const storage_type_t storage_type(uint8_t b);
};

unpacker& unpacker::operator>>(bool& value) {
    const storage_type_t st = storage_type(peek_byte());
    if (st == STRUE) {
        get_byte();
        value = true;
    } else if (st == SFALSE) {
        get_byte();
        value = false;
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return *this;
}

unpacker& unpacker::operator>>(int8_t& value) {
    const storage_type_t st = storage_type(peek_byte());

    if (st == SFIXINT || st == SFIXNINT) {
        value = get_byte();
    } else if (st == SINT8) {
        get_byte();
        value = get_byte();
    } else {
        throw output_conversion_error{ peek_byte() };
    }
    return *this;
}

unpacker& unpacker::operator>>(int16_t& value) {
    const storage_type_t st = storage_type(peek_byte());

    if (type() == T_INT8) {
        value = get_value<int8_t>();
    } else if (st == SINT16) {
        get_byte();
        value = get_int<int16_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return *this;
}

unpacker& unpacker::operator>>(int32_t& value) {
    const storage_type_t st = storage_type(peek_byte());

    if (type() == T_INT8) {
        value = get_value<int8_t>();
    } else if (st == SINT16) {
        value = get_value<int16_t>();
    } else if (st == SINT32) {
        get_byte();
        value = get_int<int32_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return *this;
}

unpacker& unpacker::operator>>(int64_t& value) {
    const storage_type_t st = storage_type(peek_byte());

    if (type() == T_INT8) {
        value = get_value<int8_t>();
    } else if (st == SINT16) {
        value = get_value<int16_t>();
    } else if (st == SINT32) {
        value = get_value<int32_t>();
    } else if (st == SINT64) {
        get_byte();
        value = get_int<int64_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return *this;
}

unpacker& unpacker::operator>>(uint8_t& value) {
    const storage_type_t st = storage_type(peek_byte());

    if (st == SFIXINT) {
        value = get_byte();
    } else if (st == SUINT8) {
        get_byte();
        value = get_byte();
    } else {
        throw output_conversion_error{ peek_byte() };
    }
    return *this;
}

unpacker& unpacker::operator>>(uint16_t& value) {
    const storage_type_t st = storage_type(peek_byte());

    if (type() == T_UINT8 || st == SFIXINT) {
        value = get_value<uint8_t>();
    } else if (st == SINT16) {
        get_byte();
        value = get_int<uint16_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return *this;
}

unpacker& unpacker::operator>>(uint32_t& value) {
    const storage_type_t st = storage_type(peek_byte());

    if (type() == T_UINT8) {
        value = get_value<uint8_t>();
    } else if (st == SUINT16) {
        value = get_value<uint16_t>();
    } else if (st == SUINT32) {
        get_byte();
        value = get_int<uint32_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return *this;
}

unpacker& unpacker::operator>>(uint64_t& value) {
    const storage_type_t st = storage_type(peek_byte());

    if (type() == T_UINT8) {
        value = get_value<uint8_t>();
    } else if (st == SUINT16) {
        value = get_value<uint16_t>();
    } else if (st == SUINT32) {
        value = get_value<uint32_t>();
    } else if (st == SUINT64) {
        get_byte();
        value = get_int<uint64_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return *this;
}

unpacker& unpacker::operator>>(string& value) {
    iterator_traits<decltype(_it)>::difference_type len = get_string_length();

    if (len > distance(_it, _it_end)) {
        output_underflow_error{};
    }
    value.clear();
    value.append(_it, _it_end);
    _it += len;

    return *this;
}

unpacker& unpacker::operator>>(unpacker& value) {
    value._buffer = _buffer;
    value._it = _it;
    skip();
    value._it_end = _it;
    return *this;
}

template<typename T> unpacker& unpacker::operator>>(vector<T>& vec) {
    const size_t len = get_array_length();

    for (size_t i = 0; i < len; ++i) {
        T val;
        *this >> val;
        vec.emplace_back(std::move(val));
    }

    return *this;
}

template<typename K, typename V> unpacker& unpacker::operator>>(map<K, V>& map) {
    const size_t len = get_map_length();

    for (size_t i = 0; i < len; ++i) {
        K key;
        V value;
        *this >> key;
        *this >> value;
        map.emplace(std::make_pair(std::move(key), std::move(value)));
    }

    return *this;
}

const unpacker::data_type_t unpacker::type() const {
    // @formatter:off
        static const data_type_t map_table[] {
                /*  0 */ T_UNKNOWN, T_INT8, T_ARRAY, T_MAP, T_STRING, T_NULL, T_BOOLEAN, T_BOOLEAN,
                /*  8 */ T_BINARY, T_BINARY, T_BINARY, T_EXTERNAL, T_EXTERNAL, T_EXTERNAL, T_FLOAT, T_DOUBLE,
                /* 16 */ T_UINT8, T_UINT16, T_UINT32, T_UINT64, T_INT8, T_INT16, T_INT32, T_INT64,
                /* 24 */ T_EXTERNAL, T_EXTERNAL, T_EXTERNAL, T_EXTERNAL, T_EXTERNAL, T_STRING, T_STRING, T_STRING,
                /* 32 */ T_ARRAY, T_ARRAY, T_MAP, T_MAP, T_INT8
        };
        // @formatter:on

    if (empty()) {
        throw output_underflow_error();
    } else {
        return map_table[storage_type(peek_byte())];
    }
}

size_t unpacker::get_string_length() {
    const storage_type_t st = storage_type(peek_byte());
    size_t len;

    if (st == SFIXSTR) {
        len = get_byte() & 0x1fu;
    } else if (st == SSTR8) {
        get_byte();
        len = get_byte();
    } else if (st == SSTR16) {
        get_byte();
        len = get_int<uint16_t>();
    } else if (st == SSTR32) {
        get_byte();
        len = get_int<uint32_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return len;
}

size_t unpacker::get_array_length() {
    const storage_type_t st = storage_type(peek_byte());

    if (st == SFIXARR) {
        return get_byte() & 0xfu;
    } else if (st == SARR16) {
        return get_int<uint16_t>();
    } else if (st == SARR32) {
        return get_int<uint32_t>();
    } else {
        throw output_conversion_error{};
    }
}

size_t unpacker::get_map_length() {
    const storage_type_t st = storage_type(peek_byte());

    if (st == SFIXMAP) {
        return get_byte() & 0xfu;
    } else if (st == SMAP16) {
        return get_int<uint16_t>();
    } else if (st == SMAP32) {
        return get_int<uint32_t>();
    } else {
        throw output_conversion_error{};
    }
}

unpacker& unpacker::skip() {
    switch (storage_type(peek_byte())) {
        case STRUE:
        case SFALSE:
        case SFIXINT:
        case SFIXNINT:
            skip_bytes(1);
            break;

        case SINT8:
        case SUINT8:
            skip_bytes(2);
            break;

        case SINT16:
        case SUINT16:
            skip_bytes(3);
            break;

        case SINT32:
        case SUINT32:
        case SFLT32:
            skip_bytes(5);
            break;

        case SINT64:
        case SUINT64:
        case SFLT64:
            skip_bytes(9);
            break;

        case SFIXSTR:
        case SSTR8:
        case SSTR16:
        case SSTR32:
            skip_bytes(get_string_length());
            break;

        case SFIXARR:
        case SARR16:
        case SARR32: {
            const size_t len = get_array_length();
            for (size_t i = 0; i < len; ++i) {
                skip();
            }
        }
            break;

        case SFIXMAP:
        case SMAP16:
        case SMAP32: {
            const size_t len = get_map_length();
            for (size_t i = 0; i < len; ++i) {
                skip();
                skip();
            }
        }
            break;

        default:
            throw output_conversion_error{ peek_byte() };
    }
    return *this;
}


const unpacker::storage_type_t unpacker::storage_type(uint8_t b) {
    // @formatter:off
    static const storage_type_t map_table[128] = {
            /* 0x80 */  SFIXMAP,  SFIXMAP,  SFIXMAP,  SFIXMAP,  SFIXMAP,  SFIXMAP,  SFIXMAP,  SFIXMAP,
            /* 0x88 */  SFIXMAP,  SFIXMAP,  SFIXMAP,  SFIXMAP,  SFIXMAP,  SFIXMAP,  SFIXMAP,  SFIXMAP,
            /* 0x90 */  SFIXARR,  SFIXARR,  SFIXARR,  SFIXARR,  SFIXARR,  SFIXARR,  SFIXARR,  SFIXARR,
            /* 0x98 */  SFIXARR,  SFIXARR,  SFIXARR,  SFIXARR,  SFIXARR,  SFIXARR,  SFIXARR,  SFIXARR,
            /* 0xa0 */  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,
            /* 0xa8 */  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,
            /* 0xb0 */  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,
            /* 0xb8 */  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,  SFIXSTR,
            /* 0xc0 */  SNIL,     SUNUSED,  SFALSE,   STRUE,    SBIN8,    SBIN16,   SBIN32,   SEXT8,
            /* 0xc8 */  SEXT16,   SEXT32,   SFLT32,   SFLT64,   SUINT8,   SUINT16,  SUINT32,  SUINT64,
            /* 0xd0 */  SINT8,    SINT16,   SINT32,   SINT64,   SFEXT1,   SFEXT2,   SFEXT4,   SFEXT8,
            /* 0xd8 */  SFEXT16,  SSTR8,    SSTR16,   SSTR32,   SARR16,   SARR32,   SMAP16,   SMAP32,
            /* 0xe0 */  SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT,
            /* 0xe8 */  SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT,
            /* 0xf0 */  SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT,
            /* 0xf8 */  SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT, SFIXNINT
    };
    // @formatter:on

    if (b <= 0x7f) { return SFIXINT; }
    else { return map_table[b - 0x80]; }
}

}


#endif //MSGPACK_UNPACKER_H
