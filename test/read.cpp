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

    win_ring_cpp ring(32);

    char buf4normal[32] = "", buf4fixed[32] = "";

    ring.get_sqe()
        ->prep_register_files(&hFile, 1, {}, NT_IORING_OP_FLAG_NONE)
        ->set_data64(140);
    ring.submit_and_wait(1);

    clear_cqes(&ring, "register");

    IORING_BUFFER_INFO bufferInfo = { .Address = buf4fixed, .Length = 32 };
    ring.get_sqe()
        ->prep_register_buffers(&bufferInfo, 1, {}, NT_IORING_OP_FLAG_NONE);
    ring.submit_and_wait(1);

    clear_cqes(&ring, "register");

    for (int x = 0; x < 2; ++x) {
        auto sqe = ring.get_sqe();
        if (x & 1) {
            sqe->prep_read(
                0u,
                IORING_REGISTERED_BUFFER{ .BufferIndex = 0, .Offset = 0 },
                8,
                0,
                NT_IORING_OP_FLAG_REGISTERED_FILE | NT_IORING_OP_FLAG_REGISTERED_BUFFER
            );
        } else {
            sqe->prep_read(hFile, buf4normal, 16, 0, NT_IORING_OP_FLAG_NONE);
        }
        sqe->set_data64(x * 100);
    }

    ring.submit_and_wait(1);

    for (auto* cqe : ring) {
        throw_on_error(cqe->ResultCode);
        if (cqe->Information == 8) {
            printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->UserData, buf4normal);
        }
        else {
            printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->UserData,
                buf4fixed);
        }
    }

    CloseHandle(hFile);
}
