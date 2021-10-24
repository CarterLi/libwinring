#include <chrono>
#include <string_view>
#include <thread>
#include <cstdio>

#include "winring.h";

struct stopwatch {
    stopwatch(std::string_view str_) : str(str_) {}
    ~stopwatch() {
        std::printf("%*s\t%lld\n", (unsigned)str.length(), str.data(), (clock::now() - start).count());
    }

    using clock = std::chrono::high_resolution_clock;
    std::string_view str;
    clock::time_point start = clock::now();
};

int main() {
    const auto iteration = 10000000;

    {
        win_ring ring;
        win_ring_queue_init(8, &ring);
        stopwatch sw("plain IORING_OP_NOP:");
        for (int i = 0; i < iteration; ++i) {
            auto* sqe = win_ring_get_sqe(&ring);
            win_ring_prep_nop(sqe);
            win_ring_submit_and_wait(&ring, 1);

            win_ring_cqe* cqe = win_ring_peek_cqe(&ring);
            (void)cqe->ResultCode;
            win_ring_cqe_seen(&ring, cqe);
        }
        win_ring_queue_exit(&ring);
    }
    {
        stopwatch sw("this_thread::yield:");
        for (int i = 0; i < iteration; ++i) {
            std::this_thread::yield();
        }
    }
    {
        stopwatch sw("pause:");
        for (int i = 0; i < iteration; ++i) {
            _mm_pause();
        }
    }
}
