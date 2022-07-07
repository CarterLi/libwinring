#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <memory>

#include "libwinring.hpp"

#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)
#define WFILE WIDE1(__FILE__)

[[noreturn]]
static void panic() {
    char buf[256] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, (sizeof(buf) / sizeof(*buf)), NULL);
    printf("ERROR: %s", buf);
    abort();
}

static void clear_cqes(win_ring_cpp* ring, const char* str) {
    ring->submit_and_wait(-1);

    for (auto* cqe : *ring) {
        throw_on_error(cqe->ResultCode);
        printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->get_data64(), str);
    }

    ring->cq_clear();
}
