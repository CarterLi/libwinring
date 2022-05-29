#include "common.hpp"

int main() {
    HANDLE hFile = CreateFileW(L"test.txt",
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) panic();

    win_ring ring;
    if (win_ring_queue_init(32, &ring) < 0) panic();

    char buf[] = "1234567890-=";
    win_ring_sqe* sqe;

    sqe = win_ring_get_sqe(&ring);
    win_ring_prep_write(
        sqe,
        hFile,
        buf,
        sizeof(buf),
        0,
        NT_WRITE_FLAG_NONE,
        NT_IORING_OP_FLAG_NONE
    );
    win_ring_sqe_set_data64(sqe, 0x12345678DEADBEEF);
    if (win_ring_submit(&ring) < 0) panic();

    clear_cqes(&ring, "write");

    CloseHandle(hFile);
    win_ring_queue_exit(&ring);
    return 0;
}