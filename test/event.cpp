#include "common.hpp"

int main() {
    win_ring ring;
    panic_on_error(win_ring_queue_init(32, &ring));

    HANDLE event = CreateEventA(nullptr, false, false, nullptr);
    if (!event) panic();
    if (win_ring_register_event(&ring, event) < 0) panic();

    auto* sqe = win_ring_get_sqe(&ring);
    win_ring_prep_nop(sqe);

    win_ring_submit(&ring);
    if (WaitForSingleObject(event, INFINITE) != 0) panic();

    if (win_ring_peek_cqe(&ring) == nullptr) panic();

    win_ring_queue_exit(&ring);

    return 0;
}