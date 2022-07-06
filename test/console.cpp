#include "common.hpp"

int main() {
    win_ring ring;
    panic_on_error(win_ring_queue_init(32, &ring));

    char str[128] = "";

    auto* sqe = win_ring_get_sqe(&ring);
    win_ring_prep_read(sqe, CONSOLE_REAL_INPUT_HANDLE, str, sizeof str, 0, NT_IORING_OP_FLAG_NONE);
    win_ring_submit_and_wait(&ring, 1);

    clear_cqes(&ring, "read");

    win_ring_queue_exit(&ring);

    return 0;
}