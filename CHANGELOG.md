# Changelog

All notable changes to msgpack-cpp are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.9.1] - 2026-10-13

First tagged release. It hardens decoding of untrusted input, fixes a number of portability and
standard-conformance problems, and adds CI on Linux, macOS and Windows.

### Added

- `std::wstring` is validated as UTF-8 in both directions. Lone surrogates, code points above
  U+10FFFF, truncated or overlong sequences throw `output_conversion_error`. Where `wchar_t` is
  16 bits, surrogate pairs are supported.
- `packer` accepts `long long` / `unsigned long long` (and `long` where it differs from the
  fixed-width types), non-const `char*` and `char[]` (packed as str), and a null `const char*`
  (packed as nil).
- `unpacker::skip()` handles bin 8/16/32, ext 8/16/32 and fixext 1–16 objects.
- `unpacker::max_nesting_depth()`: input nested deeper than 512 levels is rejected with
  `output_conversion_error` instead of overflowing the stack.
- `to_string()` renders maps with non-string keys.
- `make install` installs a working `msgpack-cpp.pc` for pkg-config.
- GitHub Actions CI covers GCC, Clang, AppleClang and MSVC, sanitizers, an install check and a
  benchmark build, and a tag-driven release workflow publishes releases.
- `CONTRIBUTING.md` (workflow, code rules, Conventional Commits) and a rewritten README.

### Changed

- **`platform.h` was renamed to `msgpack-platform.h`** to avoid clashing with other libraries'
  headers once installed. Code that includes only `packer.h` / `unpacker.h` is unaffected.
- `output_conversion_error` and `output_underflow_error` now live in `msgpack-platform.h`, still in
  namespace `msgpack`, so that `packer.h` can throw them too.
- A failed read (`>>`, `skip()`, reading into a nested `unpacker` or a `std::wstring`) no longer
  consumes input, so the caller can retry with another type.
- `to_string()` prints map entries in wire order; previously they were sorted and duplicate keys
  were dropped.
- `packer.h` and `unpacker.h` no longer use `<codecvt>` / `std::wstring_convert`, which are deprecated
  in C++17.
- Building the tests requires CMake 3.14. GoogleTest 1.12.1 is fetched with FetchContent, and the
  tests are built with `-Wall -Wextra -Werror` (`/W3 /WX` on MSVC).

### Fixed

- Truncated input could advance the read iterator past the end of the buffer (undefined behaviour)
  before the bounds check ran, including a signed-length bypass on 32-bit targets.
- If reading into a nested `unpacker` failed, the target was left with a dangling end iterator, and
  later reads could overrun freed memory.
- Deeply nested input crashed `skip()` and `to_string()` with a stack overflow.
- `to_string()` of an empty unpacker returned `"}"` instead of `"{}"`.
- str/array/map lengths above 2³²−1 were written without a header, producing a corrupt stream.
- Appending one packer to another reallocated on every append (quadratic growth), and `p << p`
  violated `vector::insert` preconditions.
- The headers did not compile on their own (missing `<locale>` and other standard includes).
- Strict-aliasing undefined behaviour in the float/double byte swap, an implementation-defined
  `uint8_t` → `int8_t` conversion, a zero-size array in `packer::array()`, ignored `const` qualifiers
  and stray semicolons.
- The no-intrinsics byte-swap fallback could not compile, and operating systems other than
  Linux/macOS/Windows failed with `#error` even when the compiler reports its endianness.
- `platform::str_printf<0>()` looped forever.
- `make install` did not install the platform header and failed on the pkg-config file.
- The test targets never received their intended warning flags, and the tests contained undefined
  left shifts of negative values.

### Removed

- Travis CI configuration and the unused `ENABLE_TRAVIS` CMake option.

[Unreleased]: https://github.com/scanban/msgpack-cpp/compare/v0.9.1...HEAD
[0.9.1]: https://github.com/scanban/msgpack-cpp/releases/tag/v0.9.1
