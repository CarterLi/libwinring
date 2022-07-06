#include "common.hpp"

int main() {
    HANDLE hFile = CreateFileW(WFILE,
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) panic();

    win_ring ring;
    panic_on_error(win_ring_queue_init(32, &ring));

    win_ring_sqe* sqe;

    char buf4normal[32] = "", buf4fixed[32] = "";

    sqe = win_ring_get_sqe(&ring);
    win_ring_prep_register_files(sqe, &hFile, 1, {}, NT_IORING_OP_FLAG_NONE);
    sqe->UserData = 140;
    panic_on_error(win_ring_submit_and_wait(&ring, 1));

    clear_cqes(&ring, "register");

    sqe = win_ring_get_sqe(&ring);
    IORING_BUFFER_INFO bufferInfo = { .Address = buf4fixed, .Length = 32 };
    win_ring_prep_register_buffers(sqe, &bufferInfo, 1, {}, NT_IORING_OP_FLAG_NONE);
    panic_on_error(win_ring_submit(&ring));

    clear_cqes(&ring, "register");

    for (int x = 0; x < 2; ++x) {
        sqe = win_ring_get_sqe(&ring);
        if (x & 1) {
            win_ring_prep_read(
                sqe,
                0u,
                IORING_REGISTERED_BUFFER{ .BufferIndex = 0, .Offset = 0 },
                8,
                0,
                NT_IORING_OP_FLAG_REGISTERED_FILE | NT_IORING_OP_FLAG_REGISTERED_BUFFER
            );
        } else {
            win_ring_prep_read(sqe, hFile, buf4normal, 16, 0, NT_IORING_OP_FLAG_NONE);
        }
        sqe->UserData = x * 100;
    }

    if (win_ring_submit_and_wait(&ring, 1) < 0) panic();

    unsigned head;
    win_ring_cqe* cqe;
    win_ring_for_each_cqe(&ring, head, cqe) {
        panic_on_error(cqe->ResultCode);
        if (cqe->Information == 8) {
            printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->UserData, buf4normal);
        } else {
            printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->UserData,
                buf4fixed);
        }
    }
    win_ring_cq_clear(&ring);

    CloseHandle(hFile);
    win_ring_queue_exit(&ring);
    return 0;
}
