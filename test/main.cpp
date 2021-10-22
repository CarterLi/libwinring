#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <cstddef>

#include "winring.h"

int main() {
    HANDLE hFile = CreateFileW(LR"(.\main.cpp)",
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE) return 1;

    win_ring ring;
    if (win_ring_queue_init(2, &ring) < 0) return 1;
    char buf[32] = "";

    win_ring_sqe* sqe;
    for (unsigned i = 0; i < 2; ++i) {
        sqe = win_ring_get_sqe(&ring);
        win_ring_prep_read(sqe, hFile, buf + 8 * 0, 8, 0);
        sqe->UserData = i * 2 + 1;

        sqe = win_ring_get_sqe(&ring);
        win_ring_prep_read(sqe, hFile, buf + 8 * 1, 8, 0);
        sqe->UserData = i * 2 + 2;

        if (win_ring_submit_and_wait(&ring, 2) < 0) return 1;
    }

    win_ring_cqe* cqe;
    unsigned head;
    win_ring_for_each_cqe(&ring, head, cqe) {
        if (cqe->ResultCode != STATUS_SUCCESS) return 2;
        printf("%u %p\n", (unsigned)cqe->Information, win_ring_cqe_get_data(cqe));
        puts(buf);
    }
    win_ring_cq_advance(&ring, head);

    CloseHandle(hFile);
    win_ring_queue_exit(&ring);
}
