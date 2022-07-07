#include <chrono>
#include <string_view>
#include <thread>
#include <cstdio>
#include <intrin.h>

#ifdef _WIN32
#include "libwinring.h"
#else
#include <liburing.h>
#endif

#if defined(__GNUC__) || defined(__clang__)
template <class Tp>
inline __attribute__((always_inline)) void DoNotOptimize(Tp const& value) {
  asm volatile("" : : "r,m"(value) : "memory");
}

template <class Tp>
inline __attribute__((always_inline)) void DoNotOptimize(Tp& value) {
#if defined(__clang__)
  asm volatile("" : "+r,m"(value) : : "memory");
#else
  asm volatile("" : "+m,r"(value) : : "memory");
#endif
}
#endif

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
    const auto iteration = 10'000'000;

#ifdef _WIN32
    {
        win_ring ring;
        win_ring_queue_init(8, &ring);
        stopwatch sw("plain IORING_OP_NOP:");
        for (int i = 0; i < iteration; ++i) {
            auto* sqe = win_ring_get_sqe(&ring);
            win_ring_prep_nop(sqe);
            win_ring_submit_and_wait(&ring, 1);

            win_ring_cqe* cqe = win_ring_peek_cqe(&ring);
#if defined(__GNUC__) || defined(__clang__)
            DoNotOptimize(cqe->ResultCode);
#else
            (void)cqe->ResultCode;
#endif
            win_ring_cqe_seen(&ring, cqe);
        }
        win_ring_queue_exit(&ring);
    }
#else
    {
        io_uring ring;
        io_uring_queue_init(8, &ring, 0);
        stopwatch sw("plain IORING_OP_NOP:");
        for (int i = 0; i < iteration; ++i) {
            auto* sqe = io_uring_get_sqe(&ring);
            io_uring_prep_nop(sqe);
            io_uring_submit_and_wait(&ring, 1);

            io_uring_cqe* cqe;
            io_uring_peek_cqe(&ring, &cqe);
            DoNotOptimize(cqe->res);
            io_uring_cqe_seen(&ring, cqe);
        }
        io_uring_queue_exit(&ring);
    }
#endif
    {
        stopwatch sw("this_thread::yield:");
        for (int i = 0; i < iteration; ++i) {
            std::this_thread::yield();
        }
    }
    {
        stopwatch sw("pause:");
        for (int i = 0; i < iteration; ++i) {
#ifdef _M_ARM64
            __yield();
#else
            _mm_pause();
#endif
        }
    }
}
