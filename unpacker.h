#ifndef MSGPACK_UNPACKER_H
#define MSGPACK_UNPACKER_H

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
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
#include "msgpack-platform.h"

namespace msgpack {

// output_conversion_error and output_underflow_error are defined in msgpack-platform.h

struct unpacker_skip {};
constexpr unpacker_skip const skip{};

class unpacker {
public:
    using buffer_type = std::vector<uint8_t>;

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

    unpacker() = default;

    explicit unpacker(const buffer_type& buf)
            : _buffer(std::make_shared<buffer_type>(buf)), _it{ _buffer->cbegin() }, _it_end{ _buffer->cend() } {}
    explicit unpacker(buffer_type&& buf)
            : _buffer(std::make_shared<buffer_type>(std::move(buf))), _it{ _buffer->cbegin() }, _it_end{ _buffer->cend() } {}

    inline unpacker& operator>>(bool& value);
    inline unpacker& operator>>(int8_t& value);
    inline unpacker& operator>>(int16_t& value);
    inline unpacker& operator>>(int32_t& value);
    inline unpacker& operator>>(int64_t& value);
    inline unpacker& operator>>(uint8_t& value);
    inline unpacker& operator>>(uint16_t& value);
    inline unpacker& operator>>(uint32_t& value);
    inline unpacker& operator>>(uint64_t& value);
    inline unpacker& operator>>(float& value);
    inline unpacker& operator>>(double& value);
    inline unpacker& operator>>(std::string& value);
    inline unpacker& operator>>(std::wstring& value);
    inline unpacker& operator>>(unpacker& value);

    inline unpacker& operator>>(const unpacker_skip) {
        return skip();
    }

    template<typename T, typename F> unpacker& for_each(F f);
    template<typename T> unpacker& operator>>(std::vector<T>& vec);

    template<typename K, typename V, typename F> unpacker& for_each(F f);
    template<typename K, typename V> unpacker& operator>>(std::map<K, V>& map);

    template <typename T> T get_value() {
        T val;
        *this >> val;
        return val;
    }

    bool empty() const { return _it == _it_end; }
    inline data_type_t type() const;
    inline unpacker& skip();

    // Maximum nesting depth of arrays/maps accepted by skip() and to_string(); deeper input is rejected
    // with output_conversion_error instead of exhausting the stack by unbounded recursion.
    static constexpr size_t max_nesting_depth() { return 512; }

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

    std::shared_ptr<buffer_type> _buffer;
    buffer_type::const_iterator _it;
    buffer_type::const_iterator _it_end;

    uint8_t peek_byte() const {
        if (_it != _it_end) { return *_it; }
        throw output_underflow_error{};
    }

    uint8_t get_byte() {
        if (_it != _it_end) { return *_it++; }
        throw output_underflow_error();
    }

    void skip_bytes(size_t count) {
        if (count > static_cast<size_t>(_it_end - _it)) { throw output_underflow_error(); }
        _it += static_cast<buffer_type::difference_type>(count);
    }

    inline void skip_value(size_t depth);

    inline size_t get_string_length();

    inline size_t get_array_length();

    inline size_t get_map_length();

    template<typename T> T get_numeric() {
        union {
            T data;
            uint8_t bytes[sizeof(T)];
        } cvt;
        for (uint8_t& b : cvt.bytes) { b = get_byte(); }
        return platform::ntoh(cvt.data);
    }

    inline static storage_type_t storage_type(uint8_t b);

    // Well defined conversion of a raw byte to a two's complement int8_t
    static int8_t to_int8(uint8_t b) {
        return static_cast<int8_t>(b < 0x80 ? b : int(b) - 256);
    }
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
        value = to_int8(get_byte());
    } else if (st == SINT8) {
        get_byte();
        value = to_int8(get_byte());
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
        value = get_numeric<int16_t>();
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
        value = get_numeric<int32_t>();
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
        value = get_numeric<int64_t>();
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
    } else if (st == SUINT16) {
        get_byte();
        value = get_numeric<uint16_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return *this;
}

unpacker& unpacker::operator>>(uint32_t& value) {
    const storage_type_t st = storage_type(peek_byte());

    if (type() == T_UINT8 || st == SFIXINT) {
        value = get_value<uint8_t>();
    } else if (st == SUINT16) {
        value = get_value<uint16_t>();
    } else if (st == SUINT32) {
        get_byte();
        value = get_numeric<uint32_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return *this;
}

unpacker& unpacker::operator>>(uint64_t& value) {
    const storage_type_t st = storage_type(peek_byte());

    if (type() == T_UINT8 || st == SFIXINT) {
        value = get_value<uint8_t>();
    } else if (st == SUINT16) {
        value = get_value<uint16_t>();
    } else if (st == SUINT32) {
        value = get_value<uint32_t>();
    } else if (st == SUINT64) {
        get_byte();
        value = get_numeric<uint64_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return *this;
}

unpacker& unpacker::operator>>(float& value) {
    const storage_type_t st = storage_type(peek_byte());

    if(st == SFLT32) {
        get_byte();
        value = get_numeric<float>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }
    return *this;
}

unpacker& unpacker::operator>>(double& value) {
    const storage_type_t st = storage_type(peek_byte());

    if(type() == T_FLOAT) {
        value = get_value<float>();
    } else if(st == SFLT64) {
        get_byte();
        value = get_numeric<double>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }
    return *this;
}


unpacker& unpacker::operator>>(std::string& value) {
    const size_t len = get_string_length();

    if (len > static_cast<size_t>(_it_end - _it)) {
        throw output_underflow_error{};
    }
    const buffer_type::const_iterator str_end = _it + static_cast<buffer_type::difference_type>(len);
    value.assign(_it, str_end);
    _it = str_end;

    return *this;
}

unpacker& unpacker::operator >>(std::wstring& value) {
    const buffer_type::const_iterator start = _it;
    std::string buf;
    *this >> buf;

    // strict UTF-8 decoder (RFC 3629), mirrors the encoder in packer
    std::wstring out;
    out.reserve(buf.length());
    const size_t n = buf.length();
    size_t i = 0;
    while (i < n) {
        const uint32_t b0 = static_cast<unsigned char>(buf[i]);
        uint32_t cp;
        size_t extra;
        uint32_t min_cp;
        if (b0 < 0x80u) {
            cp = b0;
            extra = 0;
            min_cp = 0;
        } else if (b0 >= 0xc2u && b0 <= 0xdfu) {
            cp = b0 & 0x1fu;
            extra = 1;
            min_cp = 0x80u;
        } else if (b0 >= 0xe0u && b0 <= 0xefu) {
            cp = b0 & 0x0fu;
            extra = 2;
            min_cp = 0x800u;
        } else if (b0 >= 0xf0u && b0 <= 0xf4u) {
            cp = b0 & 0x07u;
            extra = 3;
            min_cp = 0x10000u;
        } else {
            // stray continuation byte, 0xc0, 0xc1 or 0xf5..0xff
            _it = start;
            throw output_conversion_error("invalid utf-8 string");
        }
        if (extra > n - i - 1) {
            // truncated sequence
            _it = start;
            throw output_conversion_error("invalid utf-8 string");
        }
        for (size_t k = 1; k <= extra; ++k) {
            const uint32_t b = static_cast<unsigned char>(buf[i + k]);
            if ((b & 0xc0u) != 0x80u) {
                _it = start;
                throw output_conversion_error("invalid utf-8 string");
            }
            cp = (cp << 6) | (b & 0x3fu);
        }
        if (cp < min_cp || (cp >= 0xd800u && cp <= 0xdfffu) || cp > 0x10ffffu) {
            // overlong form, surrogate or out of range
            _it = start;
            throw output_conversion_error("invalid utf-8 string");
        }
        i += extra + 1;
        if (sizeof(wchar_t) == 2 && cp >= 0x10000u) {
            // 16-bit wchar_t (UTF-16): emit a surrogate pair
            cp -= 0x10000u;
            out.push_back(static_cast<wchar_t>(0xd800u + (cp >> 10)));
            out.push_back(static_cast<wchar_t>(0xdc00u + (cp & 0x3ffu)));
        } else {
            out.push_back(static_cast<wchar_t>(cp));
        }
    }
    value.swap(out);

    return *this;
}


unpacker& unpacker::operator>>(unpacker& value) {
    // skip() is atomic and may throw, so modify the target only after it succeeded
    const buffer_type::const_iterator start = _it;
    skip();
    const buffer_type::const_iterator end = _it;
    value._buffer = _buffer;
    value._it = start;
    value._it_end = end;
    return *this;
}

template<typename T, typename F> unpacker& unpacker::for_each(F f) {
    if(type() != T_ARRAY) { throw output_conversion_error("type is not an array"); }

    const size_t len = get_array_length();

    for (size_t i = 0; i < len; ++i) {
        T val;
        *this >> val;
        f(val);
    }

    return *this;
}
template<typename T> unpacker& unpacker::operator>>(std::vector<T>& vec) {
    return for_each<T>([&vec](T v) {
        vec.emplace_back(std::move(v));
    });
}

template<typename K, typename V, typename F> unpacker& unpacker::for_each(F f) {
    if(type() != T_MAP) { throw output_conversion_error("type is not a map"); }

    const size_t len = get_map_length();

    for (size_t i = 0; i < len; ++i) {
        K key;
        V value;
        *this >> key;
        *this >> value;
        f(key, value);
    }

    return *this;
}

template<typename K, typename V> unpacker& unpacker::operator>>(std::map<K, V>& map) {
    return for_each<K, V>([&map](K k, V v){
        map.emplace(std::make_pair(std::move(k), std::move(v)));
    });
}

unpacker::data_type_t unpacker::type() const {
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
        len = get_numeric<uint16_t>();
    } else if (st == SSTR32) {
        get_byte();
        len = get_numeric<uint32_t>();
    } else {
        throw output_conversion_error{ peek_byte() };
    }

    return len;
}

size_t unpacker::get_array_length() {
    const storage_type_t st = storage_type(peek_byte());

    if (st == SFIXARR) {
        return get_byte() & 0xfu;
    }
    if (st == SARR16) {
        get_byte();
        return get_numeric<uint16_t>();
    }
    if (st == SARR32) {
        get_byte();
        return get_numeric<uint32_t>();
    }

    throw output_conversion_error{};
}

size_t unpacker::get_map_length() {
    const storage_type_t st = storage_type(peek_byte());

    if (st == SFIXMAP) {
        return get_byte() & 0xfu;
    }
    if (st == SMAP16) {
        get_byte();
        return get_numeric<uint16_t>();
    }
    if (st == SMAP32) {
        get_byte();
        return get_numeric<uint32_t>();
    }

    throw output_conversion_error{};
}

unpacker& unpacker::skip() {
    // On failure the position is restored, so a failed skip() does not consume any input
    const buffer_type::const_iterator start = _it;
    try {
        skip_value(0);
    } catch (...) {
        _it = start;
        throw;
    }
    return *this;
}

void unpacker::skip_value(const size_t depth) {
    if (depth > max_nesting_depth()) { throw output_conversion_error("nesting is too deep"); }

    switch (storage_type(peek_byte())) {
        case STRUE:
        case SFALSE:
        case SFIXINT:
        case SFIXNINT:
        case SNIL:
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

        case SBIN8:
            get_byte();
            skip_bytes(get_byte());
            break;

        case SBIN16:
            get_byte();
            skip_bytes(get_numeric<uint16_t>());
            break;

        case SBIN32:
            get_byte();
            skip_bytes(get_numeric<uint32_t>());
            break;

        // ext8/16/32: length, 1 type byte, data
        case SEXT8: {
            get_byte();
            const size_t len = get_byte();
            skip_bytes(1);
            skip_bytes(len);
        }
            break;

        case SEXT16: {
            get_byte();
            const size_t len = get_numeric<uint16_t>();
            skip_bytes(1);
            skip_bytes(len);
        }
            break;

        case SEXT32: {
            get_byte();
            const size_t len = get_numeric<uint32_t>();
            skip_bytes(1);
            skip_bytes(len);
        }
            break;

        // fixext1/2/4/8/16: 1 format byte, 1 type byte, 1/2/4/8/16 data bytes
        case SFEXT1:
            skip_bytes(3);
            break;

        case SFEXT2:
            skip_bytes(4);
            break;

        case SFEXT4:
            skip_bytes(6);
            break;

        case SFEXT8:
            skip_bytes(10);
            break;

        case SFEXT16:
            skip_bytes(18);
            break;

        case SFIXARR:
        case SARR16:
        case SARR32: {
            const size_t len = get_array_length();
            for (size_t i = 0; i < len; ++i) {
                skip_value(depth + 1);
            }
        }
            break;

        case SFIXMAP:
        case SMAP16:
        case SMAP32: {
            const size_t len = get_map_length();
            for (size_t i = 0; i < len; ++i) {
                skip_value(depth + 1);
                skip_value(depth + 1);
            }
        }
            break;

        default:
            throw output_conversion_error{ peek_byte() };
    }
}


unpacker::storage_type_t unpacker::storage_type(uint8_t b) {
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

    return static_cast<storage_type_t>((b <= 0x7f) ? SFIXINT : map_table[b - 0x80]);
}

inline std::string to_string(const unpacker& value, size_t level = 0) {
    if (level > unpacker::max_nesting_depth()) { throw output_conversion_error("nesting is too deep"); }

    unpacker u { value };
    std::string ret;

    if(level == 0) {
        ret = '{';
    }

    while (!u.empty()) {
        switch (u.type()) {
            case unpacker::T_BOOLEAN:
                ret += u.get_value<bool>() ? "true" : "false";
                break;
            case unpacker::T_INT8:
                ret += std::to_string(u.get_value<int8_t>());
                break;
            case unpacker::T_INT16:
                ret += std::to_string(u.get_value<int16_t>());
                break;
            case unpacker::T_INT32:
                ret += std::to_string(u.get_value<int32_t>());
                break;
            case unpacker::T_INT64:
                ret += std::to_string(u.get_value<int64_t>());
                break;

            case unpacker::T_UINT8:
                ret += std::to_string(u.get_value<uint8_t>());
                break;
            case unpacker::T_UINT16:
                ret += std::to_string(u.get_value<uint16_t>());
                break;
            case unpacker::T_UINT32:
                ret += std::to_string(u.get_value<uint32_t>());
                break;
            case unpacker::T_UINT64:
                ret += std::to_string(u.get_value<uint64_t>());
                break;

            case unpacker::T_FLOAT:
                ret += std::to_string(u.get_value<float>());
                break;
            case unpacker::T_DOUBLE:
                ret += std::to_string(u.get_value<double>());
                break;

            case unpacker::T_STRING:
                ret += '"' + u.get_value<std::string>() + '"';
                break;

            case unpacker::T_ARRAY: {
                std::vector<unpacker> v;

                u >> v;
                ret += '[';

                for(const auto& e: v) {
                    ret += to_string(e, level + 1) + ',';
                }

                if(v.empty()) {
                    ret += ']';
                } else {
                    ret.back() = ']';
                }
            }
                break;

            case unpacker::T_MAP: {
                // map keys may be of any type, keys are rendered the same way as values
                std::vector<std::pair<unpacker, unpacker>> m;

                u.for_each<unpacker, unpacker>([&m](unpacker k, unpacker v) {
                    m.emplace_back(std::move(k), std::move(v));
                });
                ret += '{';

                for (const auto& e: m) {
                    ret += to_string(e.first, level + 1) + ':' + to_string(e.second, level + 1) + ',';
                }

                if(m.empty()) {
                    ret += '}';
                } else {
                    ret.back() = '}';
                }
            }
                break;

            case unpacker::T_NULL:
                ret += "null";
                u >> skip;
                break;

            default:
                throw output_conversion_error{};
        }
        ret += ',';
    }
    if (!ret.empty() && ret.back() == ',') {
        ret.pop_back();
    }

    if(level == 0) {
        ret += '}';
    }

    return ret;
}

}


#endif //MSGPACK_UNPACKER_H
