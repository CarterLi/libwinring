#include "common.hpp"

int main() {
    win_ring_cpp ring(32);

    char str[128] = "";

    ring.get_sqe()
        ->prep_read(CONSOLE_REAL_INPUT_HANDLE, str, sizeof str, 0, NT_IORING_OP_FLAG_NONE);
    ring.submit_and_wait(1);

    clear_cqes(&ring, "read");
}