# Contributing to msgpack-cpp

Thanks for helping improve msgpack-cpp! This guide covers the workflow, the code rules, and how
commit messages are written.

## Workflow

1. Create a branch off `master`.
2. Make your change and add or update tests under `test/`. Every bug fix needs a regression test
   that fails without the fix.
3. Build and run the tests locally:

   ```sh
   cmake -S . -B build -DENABLE_TESTING=ON
   cmake --build build
   ctest --test-dir build --output-on-failure
   ```

4. Push and open a pull request. [CI](.github/workflows/ci.yml) builds and tests on GCC, Clang,
   AppleClang and MSVC, and also runs a sanitizer build, an install check and a benchmark build.
   It must be green before merging.

## Code rules

- **C++11 only.** The library targets C++11, so don't use C++14 or later features in the headers.
- **Header-only and self-contained.** `packer.h`, `unpacker.h` and `msgpack-platform.h` must each
  compile on their own: include every standard header you use.
- **Warning-free.** Tests build with `-Wall -Wextra -Werror` (GCC/Clang) and `/W3 /WX` (MSVC).
- **Portable.** Code must work on Linux, macOS and Windows, including where `wchar_t` is 16 bits.
  Qualify standard library calls (`std::move`, not `move`).
- **Match the surrounding style.** Use the same naming, formatting and comment density as the
  code around you.

## Commit messages

Commits follow [Conventional Commits 1.0.0](https://www.conventionalcommits.org/en/v1.0.0/):

```
<type>(<optional scope>): <subject>

<body>

<optional footer(s)>
```

### Subject line

- Use the imperative mood and lower case, with no trailing period: `fix(packer): reject lone surrogates`.
- Keep it to 72 characters or fewer.

### Types

| Type       | Use for                                                       |
|------------|---------------------------------------------------------------|
| `feat`     | a new user-visible capability                                 |
| `fix`      | a bug fix                                                     |
| `perf`     | a performance improvement without behaviour change            |
| `refactor` | restructuring that does not change behaviour                  |
| `test`     | adding or fixing tests only                                   |
| `docs`     | documentation only (README, comments, this file)              |
| `build`    | CMake, dependencies, install rules                            |
| `ci`       | GitHub Actions workflow and CI scripts                        |
| `chore`    | maintenance that fits nowhere else (e.g. `.gitignore`)        |

### Scopes

Use the part of the project the change affects: `packer`, `unpacker`, `platform`, `test` or
`cmake`. Omit the scope when the change is cross-cutting.

### Body

Write a **medium-length body**: a few lines that explain *why* the change is needed and *what* it
does, not how the diff looks.

- Wrap lines at 72 columns.
- Use a bullet list when one commit makes several related changes.
- Mention user-visible behaviour changes explicitly.

### Breaking changes

Add `!` after the type or scope, and describe the migration in a `BREAKING CHANGE:` footer:

```
refactor(unpacker)!: rename to_string() to dump()

BREAKING CHANGE: callers of msgpack::to_string(const unpacker&) must
use msgpack::dump() instead.
```

### Granularity

Make one logical change per commit, and make sure every commit builds and passes the tests. Split
unrelated changes into separate commits, even when they touch the same file.

### Examples from this repository

```
fix(unpacker): qualify std::move in the buffer constructor

The rvalue constructor called move(buf) unqualified and relied on
argument-dependent lookup through std::vector to find std::move.
Clang 15+ diagnoses this with -Wunqualified-std-cast-call, which fails
the -Werror test builds on clang-18 and AppleClang.
```

```
ci: install gcc-9 explicitly on ubuntu-22.04

The ubuntu-22.04 runner image no longer ships gcc-9, so the job failed
at configure time with CMAKE_CXX_COMPILER not set. Install the
compiler from the Ubuntu archive for matrix entries that request it.
```

## Releasing

Releases are tag-driven, and the version follows [Semantic Versioning](https://semver.org/).

1. Bump `project(msgpack VERSION x.y.z ...)` in `CMakeLists.txt`.
2. In [CHANGELOG.md](CHANGELOG.md), move the `[Unreleased]` entries into a new `## [x.y.z] - YYYY-MM-DD`
   section, and update the links at the bottom.
3. Commit as `chore(release): x.y.z`, push, and wait for CI to pass.
4. Tag and push:

   ```sh
   git tag -a vx.y.z -m "msgpack-cpp x.y.z"
   git push origin vx.y.z
   ```

The [release workflow](.github/workflows/release.yml) then:

- runs the full CI matrix;
- checks that the tag matches the CMake version;
- uses the changelog section as the release notes;
- publishes the release with `msgpack-cpp-x.y.z-headers.tar.gz` / `.zip` and `SHA256SUMS` attached.

To check a release locally before tagging, run `ci/package-release.sh vx.y.z`.

## Reporting bugs

Please include:

- the compiler and version, and the platform;
- a minimal code sample;
- for decoding problems, the input bytes as hex.
