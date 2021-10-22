#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <cstddef>

#include "winring.h"

[[noreturn]]
void panic() {
    char buf[256] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, (sizeof(buf) / sizeof(*buf)), NULL);
    printf("ERROR: %s", buf);
    exit(1);
}

int main() {
    HANDLE hFile = CreateFileW(LR"(C:\Users\Carter\Downloads\Read me.txt)",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) panic();

    win_ring ring;
    if (win_ring_queue_init(1, &ring) < 0) panic();
    char buf[128] = "";

    for (int x = 0; x < 8; ++x) {
        win_ring_sqe* sqe;
        for (unsigned i = 0; i < 4; ++i) {
            sqe = win_ring_get_sqe(&ring);
            win_ring_prep_read(sqe, hFile, buf + 8 * i, 8, 0);
            sqe->UserData = i + x * 4;
        }

        if (win_ring_submit_and_wait(&ring, 4) < 0) panic();

        unsigned head;
        win_ring_cqe* cqe;
        win_ring_for_each_cqe(&ring, head, cqe) {
            if (cqe->ResultCode != STATUS_SUCCESS) return 1;
            printf("%u %d %s\n", (unsigned)cqe->Information, (int)cqe->UserData, buf);
        }
        win_ring_cq_clear(&ring);
    }

    CloseHandle(hFile);
    win_ring_queue_exit(&ring);
}
