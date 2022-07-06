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
    abort();
}

DWORD Win32FromHResult(HRESULT hr) {
    if ((hr & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0)) {
        return HRESULT_CODE(hr);
    }

    if (hr == S_OK) {
        return ERROR_SUCCESS;
    }

    // Not a Win32 HRESULT so return a generic error code.
    return ERROR_CAN_NOT_COMPLETE;
}

inline void panic_on_error(HRESULT hRes) {
    if (SUCCEEDED(hRes)) return;
    SetLastError(Win32FromHResult(hRes));
    panic();
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
        panic_on_error(cqe->ResultCode);
        printf("%u %d %s\n", (unsigned)cqe->Information, (int)cqe->UserData, str);
    }

    win_ring_cq_clear(ring);
}
