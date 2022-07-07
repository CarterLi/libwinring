#include "common.hpp"

int main() {
    win_ring_cpp ring(32);

    HANDLE event = CreateEventA(nullptr, false, false, nullptr);
    if (!event) panic();
    ring.register_event(event);

    ring.get_sqe()->prep_nop();
    ring.submit();

    if (WaitForSingleObject(event, INFINITE) != 0) panic();

    if (ring.peek_cqe() == nullptr) panic();
}