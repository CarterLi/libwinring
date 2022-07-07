#include "common.hpp"

#include <thread>

void createClient(HANDLE hWritePipe) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    puts("Writing pipe!");
    WriteFile(hWritePipe, "Test Pipe", sizeof("Test Pipe"), nullptr, nullptr);
    CloseHandle(hWritePipe);
}

int main() {
    win_ring ring;
    if (win_ring_queue_init(32, &ring) < 0) panic();

    HANDLE hReadPipe, hWritePipe;
    CreatePipe(&hReadPipe, &hWritePipe, nullptr, 512);

    std::thread t(createClient, hWritePipe);

    char str[128] = "";

    auto* sqe = win_ring_get_sqe(&ring);
    win_ring_prep_read(sqe, hReadPipe, str, sizeof str, 0, NT_IORING_OP_FLAG_NONE);
    win_ring_sqe_set_data64(sqe, 10);
    panic_on_error(win_ring_submit(&ring));

    sqe = win_ring_get_sqe(&ring);
    win_ring_prep_read(sqe, hReadPipe, str, sizeof str, 0, NT_IORING_OP_FLAG_NONE);
    win_ring_sqe_set_data64(sqe, 20);
    panic_on_error(win_ring_submit(&ring));

    sqe = win_ring_get_sqe(&ring);
    win_ring_prep_cancel(sqe, hReadPipe, 0, NT_IORING_OP_FLAG_NONE);
    win_ring_sqe_set_data64(sqe, 100);
    panic_on_error(win_ring_submit(&ring));

    puts("Reading pipe!");

    for (auto* cqe : &ring) {
        if (cqe->UserData == 100) {
            assert(SUCCEEDED(cqe->ResultCode));
        }
        else {
            assert(FAILED(cqe->ResultCode));
        }
    }
    win_ring_cq_clear(&ring);

    win_ring_queue_exit(&ring);

    CloseHandle(hReadPipe);

    t.join();

    return 0;
}