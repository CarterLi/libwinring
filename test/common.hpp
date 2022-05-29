#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstddef>
#include <memory>

#include "libwinring.h"

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
    exit(1);
}

struct win_ring_cqe_iterator {
    win_ring_cqe_iterator& operator++() {
        ++head;
        return *this;
    }

    const win_ring_cqe* operator *() {
        return &ring->info.CompletionQueue->Entries[head & ring->info.CompletionQueueRingMask];
    }

    bool operator !=(const win_ring_cqe_iterator& right) {
        return head != right.head;
    }

    win_ring* ring;
    uint32_t head;
};

win_ring_cqe_iterator begin(win_ring* ring) {
    return { ring, ring->info.CompletionQueue->Head };
}

win_ring_cqe_iterator end(win_ring* ring) {
    return { ring, ring->info.CompletionQueue->Tail };
}

static void clear_cqes(win_ring* ring, const char* str) {
    win_ring_submit_and_wait(ring, -1);

    for (auto* cqe : ring) {
        if (!SUCCEEDED(cqe->ResultCode)) exit(1);
        printf("%u %d %s\n", (unsigned)cqe->Information, (int)cqe->UserData, str);
    }

    win_ring_cq_clear(ring);
}
