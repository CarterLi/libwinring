#include "common.hpp"

#include <string_view>

int main() {
    using namespace std::literals;

    win_ring_cpp ring(32);

    char str[512] = "";

    HANDLE hConIn = GetStdHandle(STD_INPUT_HANDLE);
    if (hConIn == INVALID_HANDLE_VALUE) panic();
    HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConOut == INVALID_HANDLE_VALUE) panic();

    ring.get_sqe()
        ->prep_read( hConIn, str, sizeof (str), 0, NT_IORING_OP_FLAG_NONE);
    ring.get_sqe()
        ->prep_write(hConOut, str, sizeof (str), 0, FILE_WRITE_FLAGS_NONE, NT_IORING_OP_FLAG_NONE)
        ->set_flags(NT_IORING_SQE_FLAG_DRAIN_PRECEDING_OPS);
    ring.submit_and_wait(2);

    clear_cqes(&ring, "console");
}