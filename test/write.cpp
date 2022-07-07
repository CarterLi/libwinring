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

    win_ring_cpp ring(32);

    char buf[] = "1234567890-=";

    ring.get_sqe()
        ->prep_write(
            hFile,
            buf,
            sizeof(buf),
            0,
            FILE_WRITE_FLAGS_NONE,
            NT_IORING_OP_FLAG_NONE
        )
        ->set_data64(0x12345678DEADBEEF);

    ring.submit();

    clear_cqes(&ring, "write");

    CloseHandle(hFile);
}