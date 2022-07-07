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
        ->prep_read(hReadPipe, str, sizeof str, 0, NT_IORING_OP_FLAG_NONE);
    ring.submit_and_wait(1);
    puts("Reading pipe!");


    for (auto* cqe : ring) {
        throw_on_error(cqe->ResultCode);
        printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->UserData, str);
    }

    clear_cqes(&ring, "read");

    CloseHandle(hReadPipe);

    t.join();

    return 0;
}