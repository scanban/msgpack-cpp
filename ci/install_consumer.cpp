// Built by CI against the *installed* headers only (via pkg-config), to check
// that `cmake --install` ships everything packer.h / unpacker.h depend on.
#include <packer.h>
#include <unpacker.h>
#include <cstdio>
#include <string>

int main() {
    msgpack::packer p;
    p << 42 << std::string("hi") << std::wstring(L"é");

    msgpack::unpacker u{ p.get_buffer() };
    int i = 0;
    std::string s;
    std::wstring w;
    u >> i >> s >> w;

    const bool ok = i == 42 && s == "hi" && w == L"é" && u.empty();
    std::printf("%s: %d %s U+%04X\n", ok ? "ok" : "MISMATCH", i, s.c_str(), w.empty() ? 0u : unsigned(w[0]));
    return ok ? 0 : 1;
}
