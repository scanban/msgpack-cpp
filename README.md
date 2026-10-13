# msgpack-cpp

**Tiny header-only [MessagePack](https://msgpack.org) (msgpack) serialization library for modern C++11.**

[![CI](https://github.com/scanban/msgpack-cpp/actions/workflows/ci.yml/badge.svg)](https://github.com/scanban/msgpack-cpp/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++11](https://img.shields.io/badge/C%2B%2B-11-blue.svg?logo=cplusplus)](https://en.cppreference.com/w/cpp/11)
[![Header-only](https://img.shields.io/badge/header--only-yes-brightgreen.svg)](#installation)
[![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](#building-and-testing)

msgpack-cpp packs C++ values into the compact [MessagePack](https://github.com/msgpack/msgpack/blob/master/spec.md)
binary format and unpacks them again, using familiar `<<` / `>>` stream operators. It is three
headers with no dependencies beyond the C++11 standard library. You can drop it into any project,
or install it with CMake and find it through pkg-config.

The main goals are **simplicity**, **performance** and **safe handling of untrusted input**.

## Contents

- [Features](#features)
- [Requirements](#requirements)
- [Installation](#installation)
- [Quick start](#quick-start)
- [Examples](#examples)
- [Type mapping](#type-mapping)
- [Error handling](#error-handling)
- [Limitations](#limitations)
- [Building and testing](#building-and-testing)
- [Contributing](#contributing)
- [License](#license)

## Features

- **Header-only, zero dependencies.** Include `packer.h` / `unpacker.h` and you are done.
- **C++11.** Works with GCC, Clang, AppleClang and MSVC.
- **Stream-style API.** `packer << value` and `unpacker >> value`, chainable.
- **STL support.** Any iterable container packs as a MessagePack array, `std::map` packs as a map,
  and `std::vector` / `std::map` unpack back.
- **Strings.** `std::string`, C strings, and `std::wstring` transferred as strictly validated UTF-8.
- **Heterogeneous data.** `packer::array(1, 2.5, "three")` and `packer::map("key", value, ...)`.
- **Lazy access.** Nested `unpacker` objects, `type()` inspection, `skip`, `for_each` and
  `to_string()` for debugging.
- **Safe decoding.** Bounds-checked reads, a nesting depth limit, and strict UTF-8 validation.
  A failed read throws and leaves the read position unchanged.
- **Tested in CI** on Linux, macOS and Windows, including AddressSanitizer and
  UndefinedBehaviorSanitizer builds.

## Requirements

- A C++11 compiler: GCC ≥ 9, Clang ≥ 13, AppleClang or MSVC (Visual Studio 2019+) are tested.
- CMake ≥ 3.14, only to install the headers or build the tests. GoogleTest is fetched automatically.

## Installation

### Option 1: copy the headers

Copy `packer.h`, `unpacker.h` and `msgpack-platform.h` into your include path.

### Option 2: install with CMake and use pkg-config

```sh
git clone https://github.com/scanban/msgpack-cpp.git
cd msgpack-cpp
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/usr/local
cmake --build build
cmake --install build          # may need sudo for system prefixes
```

This installs `packer.h`, `unpacker.h` and `msgpack-platform.h` into `<prefix>/include`, and
`msgpack-cpp.pc` into `<prefix>/lib/pkgconfig`. Compile against it with:

```sh
g++ -std=c++11 $(pkg-config --cflags msgpack-cpp) main.cpp -o main
```

> **Note:** the install prefix is written into `msgpack-cpp.pc` at configure time. Set it with
> `-DCMAKE_INSTALL_PREFIX`, not with `cmake --install --prefix`.

### Option 3: git submodule

```sh
git submodule add https://github.com/scanban/msgpack-cpp.git third_party/msgpack-cpp
```

Then add `third_party/msgpack-cpp` to your include directories.

## Quick start

```cpp
#include <packer.h>
#include <unpacker.h>
#include <iostream>
#include <string>

int main() {
    msgpack::packer p;
    p << 10 << 20 << 30 << "test";

    msgpack::unpacker u{ p.get_buffer() };   // p.get_buffer() is a std::vector<uint8_t>
    int i1, i2, i3;
    std::string s;
    u >> i1 >> i2 >> i3 >> s;

    std::cout << i1 << ' ' << i2 << ' ' << i3 << ' ' << s << '\n';   // 10 20 30 test
}
```

## Examples

The snippets below assume `#include <packer.h>`, `#include <unpacker.h>` and
`using namespace msgpack;`.

### Arrays and STL containers

```cpp
packer p;
std::vector<int8_t> v_in{ 1, 2, 3, 4, -5 };
p << v_in;                          // any iterable container (vector, list, set, ...) packs as an array

unpacker u{ p.get_buffer() };
std::vector<int8_t> v_out;
u >> v_out;                         // v_out == v_in
```

### Maps

```cpp
packer p;
std::map<int, int> m_in{ { 1, 10 }, { 2, 20 }, { 3, 30 } };
p << m_in;

unpacker u{ p.get_buffer() };
std::map<int, int> m_out;
u >> m_out;                         // m_out == m_in
```

### Heterogeneous arrays and maps

```cpp
packer p;
p.map("name", "msgpack-cpp", "version", 1);    // {"name": "msgpack-cpp", "version": 1}
p.array(1, 2.5, "three", true, nullptr);       // [1, 2.5, "three", true, nil]

std::cout << to_string(unpacker{ p.get_buffer() }) << '\n';
// {{"name":"msgpack-cpp","version":1},[1,2.500000,"three",true,null]}
```

### Unicode strings

```cpp
packer p;
p << std::wstring(L"café €");        // encoded as a UTF-8 MessagePack str

unpacker u{ p.get_buffer() };
std::wstring w;
u >> w;                                        // decoded with strict UTF-8 validation
```

### Inspecting, iterating and skipping

```cpp
packer p;
p.map("id", 7, "tags", std::vector<std::string>{ "a", "b" });
p << "ignored" << 42;

unpacker u{ p.get_buffer() };
if (u.type() == unpacker::T_MAP) {
    u.for_each<std::string, unpacker>([](std::string key, unpacker value) {
        std::cout << key << " = " << to_string(value) << '\n';   // nested values stay lazy
    });
}
u >> skip;                                     // skip one object of any type
int answer = u.get_value<int>();               // 42
```

## Type mapping

| C++ type (packing)                                       | MessagePack format                     | Unpack into                       |
|----------------------------------------------------------|----------------------------------------|-----------------------------------|
| `bool`                                                   | true / false                           | `bool`                            |
| `std::nullptr_t`, null `const char*`                     | nil                                    | (check `type() == T_NULL`, `skip`) |
| signed integers (`int8_t` … `int64_t`, `long long`)      | positive/negative fixint, int 8–64     | signed integer of sufficient width |
| unsigned integers (`uint8_t` … `uint64_t`)               | positive fixint, uint 8–64             | unsigned integer of sufficient width |
| `float` / `double`                                       | float 32 / float 64                    | `float` / `double`                |
| `std::string`, `const char*`, `char*`, `char[N]`         | fixstr, str 8/16/32                    | `std::string`                     |
| `std::wstring`                                           | str (UTF-8)                            | `std::wstring`                    |
| iterable containers, `T[N]`, `packer::array(...)`        | fixarray, array 16/32                  | `std::vector<T>`, `for_each<T>`, `unpacker` |
| `std::map<K,V>`, `packer::map(...)`                      | fixmap, map 16/32                      | `std::map<K,V>`, `for_each<K,V>`, `unpacker` |
| `packer`                                                 | its packed contents, appended as is    | `unpacker`                        |

Integers are encoded according to their C++ signedness: a positive `int` uses the int formats, not
uint. Decoding is strict about signedness. Values 0–127 (positive fixint) can be read into any
integer type, but larger values packed from a signed type must be read into a signed type, and
likewise for unsigned.

## Error handling

Malformed or mismatched input throws one of two exceptions. Both derive from `std::logic_error`:

| Exception                         | Thrown when                                                               |
|-----------------------------------|---------------------------------------------------------------------------|
| `msgpack::output_conversion_error` | the data does not match the requested type, is invalid UTF-8 / Unicode, a length exceeds 2³²−1, or nesting is deeper than `unpacker::max_nesting_depth()` (512) |
| `msgpack::output_underflow_error`  | the buffer ends before the object does (truncated input)                   |

```cpp
try {
    unpacker u{ std::vector<uint8_t>{ 0xd9, 0x05, 'a' } };   // str 8 of length 5, only 1 byte present
    std::string s;
    u >> s;
} catch (const output_underflow_error& e) {
    std::cerr << "truncated message: " << e.what() << '\n';
}
```

A failed read leaves the unpacker's position unchanged, so you can retry with another type or `skip`.

## Limitations

- `bin` and `ext` objects can be skipped, but not read into C++ values yet.
- `std::vector<uint8_t>` packs as an array of integers, not as `bin`.
- Integers are not packed in the smallest possible format (see [Type mapping](#type-mapping)).

## Building and testing

```sh
cmake -S . -B build -DENABLE_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Tests are built with `-Wall -Wextra -Werror` (`/W3 /WX` on MSVC). To run them under sanitizers:

```sh
cmake -S . -B build-asan -DENABLE_TESTING=ON -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-omit-frame-pointer" \
      -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined"
cmake --build build-asan && ctest --test-dir build-asan --output-on-failure
```

Benchmarks use [hayai](https://github.com/nickbruun/hayai). Enable them with `-DENABLE_BENCHMARKING=ON`
and run `build/benchmark/msgpack_benchmark`.

[GitHub Actions](https://github.com/scanban/msgpack-cpp/actions/workflows/ci.yml) builds and tests
every push and pull request on:

- GCC 9 and 14, and Clang 13 and 18 (Linux)
- AppleClang (macOS)
- MSVC (Windows)

It also runs a sanitizer build, an install/pkg-config check and a benchmark build.

## Contributing

Bug reports and pull requests are welcome. Commits follow
[Conventional Commits](https://www.conventionalcommits.org/). See [CONTRIBUTING.md](CONTRIBUTING.md)
for the workflow, code style and commit message conventions.

## License

msgpack-cpp is released under the [MIT License](LICENSE). Copyright © 2017 Stanislav Anokhin.
