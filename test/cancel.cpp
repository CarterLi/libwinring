#include "common.hpp"

#include <thread>

void createClient(HANDLE hWritePipe) {
    std::this_thread::sleep_for(std::chrono::seconds(2));
    puts("Writing pipe!");
    WriteFile(hWritePipe, "Test Pipe", sizeof("Test Pipe"), nullptr, nullptr);
    CloseHandle(hWritePipe);
}

int main() {
    win_ring_cpp ring(32);

    HANDLE hReadPipe, hWritePipe;
    CreatePipe(&hReadPipe, &hWritePipe, nullptr, 512);

    std::thread t(createClient, hWritePipe);

    char str[128] = "";

    ring.get_sqe()
        ->prep_read(hReadPipe, str, sizeof str, 0, NT_IORING_OP_FLAG_NONE)
        ->set_data64(10);
    ring.submit();

    ring.get_sqe()
        ->prep_read(hReadPipe, str, sizeof str, 0, NT_IORING_OP_FLAG_NONE)
        ->set_data64(20);
    ring.submit();

    ring.get_sqe()
        ->prep_cancel(hReadPipe, 0, NT_IORING_OP_FLAG_NONE)
        ->set_data64(100);
    ring.submit();

    puts("Reading pipe!");

    for (auto* cqe : ring) {
        if (cqe->get_data64() == 100) {
            throw_on_error(cqe->ResultCode);
        }
        else {
            if (!FAILED(cqe->ResultCode)) panic();
        }
    }
    ring.cq_clear();

    CloseHandle(hReadPipe);

    t.join();
}