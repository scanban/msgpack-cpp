#include <packer.h>
#include <unpacker.h>
#include <hayai.hpp>

class packer_fixture: public ::hayai::Fixture {
public:
    virtual void SetUp() {
    }

    virtual void TearDown() {
    }

    void run() {
        int a[128];
        msgpack::packer p;
        p << 1 << 4 << "test" << a;
    }
};

BENCHMARK_F(packer_fixture, packer, 10, 1000000) {
    run();
}