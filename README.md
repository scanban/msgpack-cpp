[![Build Status](https://travis-ci.org/scanban/msgpack-cpp.svg?branch=master)](https://travis-ci.org/scanban/msgpack-cpp)

What is msgpack-cpp ?
===============

msgpack-cpp is a tiny, headers-only MsgPack library for C++11, 
providing MsgPack parsing and serialization.

Main goals are simplicity and performance.

Installation
===============

    git clone https://github.com/scanban/msgpack-cpp
    mkdir build
    cd build
    cmake ..
    make && make install

Examples
===============
## simple
``` c++
msgpack::packer p;
p << 10 << 20 << 30 << "test"
msgpack::unpacker u { p.get_buffer() };
int i1, i2, i3;
string s;
u >> i1 >> i2 >> i3 >> s;
```

## array
``` c++
packer p;
vector<int8_t> v_in{ 1, 2, 3, 4, -5 };
p << v_in;

unpacker u{ p.get_buffer() };
vector<int8_t> v_out;
u >> v_out;
```

## map
``` c++
packer p;
map<int, int> m = {{ 1, 10 },
                   { 2, 20 },
                   { 3, 30 }};

p << m;

unpacker u{ p.get_buffer() };
map<int, int> m_out;
u >> m_out;
```

Supported features
===============
* serialization and deserialization of integers, floats, doubles and strings.
* serialization and deserialization of arrays of integers, floats, doubles and strings.
* serialization and deserialization of maps of integers, floats, doubles and strings.

Unsupported features
===============
* nested arrays and maps
* external data

License
===============
This software is released under the MIT License, see LICENSE
