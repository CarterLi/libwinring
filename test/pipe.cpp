#include "common.hpp"

#include <thread>

void createClient() {
    auto hPipe = CreateFileW(
        LR"(\\.\pipe\testpipe)",
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    puts("Writing pipe!");
    WriteFile(hPipe, "Test Pipe", sizeof("Test Pipe"), nullptr, nullptr);
    CloseHandle(hPipe);
}

int main() {
    win_ring ring;
    if (win_ring_queue_init(32, &ring) < 0) panic();

    auto hPipe = CreateNamedPipeW(
        LR"(\\.\pipe\testpipe)",
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
        1,
        512,
        512,
        0,
        nullptr);

    std::thread t(createClient);

    ConnectNamedPipe(hPipe, nullptr);

    char str[128] = "";

    auto* sqe = win_ring_get_sqe(&ring);
    win_ring_prep_read(sqe, hPipe, str, sizeof str, 0, NT_IORING_OP_FLAG_NONE);
    panic_on_error(win_ring_submit_and_wait(&ring, 1));
    puts("Reading pipe!");

    unsigned head;
    win_ring_cqe* cqe;
    win_ring_for_each_cqe(&ring, head, cqe) {
        panic_on_error(cqe->ResultCode);
        if (cqe->Information == 8) {
            printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->UserData, str);
        }
        else {
            printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->UserData,
                str);
        }
    }

    clear_cqes(&ring, "read");

    win_ring_queue_exit(&ring);

    CloseHandle(hPipe);

    t.join();

    return 0;
}