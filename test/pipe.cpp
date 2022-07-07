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
    panic_on_error(win_ring_submit_and_wait(&ring, 1));
    puts("Reading pipe!");


    for (auto* cqe : &ring) {
        panic_on_error(cqe->ResultCode);
        printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->UserData, str);
    }

    clear_cqes(&ring, "read");

    win_ring_queue_exit(&ring);

    CloseHandle(hReadPipe);

    t.join();

    return 0;
}