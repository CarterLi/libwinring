#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <cstddef>

#include "libwinring.h"

#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)
#define WFILE WIDE1(__FILE__)

[[noreturn]]
void panic() {
    char buf[256] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, (sizeof(buf) / sizeof(*buf)), NULL);
    printf("ERROR: %s", buf);
    exit(1);
}

void printDebugInfo() {
    if (win_ring_capabilities capabilities; win_ring_query_capabilities(&capabilities) < 0) {
        panic();
    } else {
        printf("IoRing Version: %d\n", (int)capabilities.IoRingVersion);
        printf("Max opcode: %d\n", capabilities.MaxOpCode);
        printf("Supported flags: %x\n\n", capabilities.FlagsSupported);
    }
}

int testRead() {
    HANDLE hFile = CreateFileW(WFILE,
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) panic();

    win_ring ring;
    if (win_ring_queue_init(32, &ring) < 0) panic();

    win_ring_sqe* sqe;

    char buf4normal[32] = "", buf4fixed[32] = "";

    sqe = win_ring_get_sqe(&ring);
    win_ring_prep_register_files(sqe, &hFile, 1, {}, NT_IORING_OP_FLAG_NONE);
    sqe->UserData = 140;
    if (win_ring_submit_and_wait(&ring, 1) < 0) panic();

    {
        unsigned head;
        win_ring_cqe* cqe;
        win_ring_for_each_cqe(&ring, head, cqe) {
            if (!SUCCEEDED(cqe->ResultCode)) return 1;
            printf("%u %d %s\n", (unsigned)cqe->Information, (int)cqe->UserData, "register");
        }
        win_ring_cq_clear(&ring);
    }

    sqe = win_ring_get_sqe(&ring);
    IORING_BUFFER_INFO bufferInfo = { .Address = buf4fixed, .Length = 32 };
    win_ring_prep_register_buffers(sqe, &bufferInfo, 1, {}, NT_IORING_OP_FLAG_NONE);
    if (win_ring_submit(&ring) < 0) panic();

    {
        unsigned head;
        win_ring_cqe* cqe;
        win_ring_for_each_cqe(&ring, head, cqe) {
            if (!SUCCEEDED(cqe->ResultCode)) return 1;
            printf("%u %d %s\n", (unsigned)cqe->Information, (int)cqe->UserData, "register");
        }
        win_ring_cq_clear(&ring);
    }

    for (int x = 0; x < 2; ++x) {
        sqe = win_ring_get_sqe(&ring);
        if (x & 1) {
            win_ring_prep_read(
                sqe,
                0u,
                IORING_REGISTERED_BUFFER{ .BufferIndex = 0, .Offset = 0 },
                8,
                0,
                NT_IORING_OP_FLAG_REGISTERED_FILE | NT_IORING_OP_FLAG_REGISTERED_BUFFER
            );
        } else {
            win_ring_prep_read(sqe, hFile, buf4normal, 16, 0, NT_IORING_OP_FLAG_NONE);
        }
        sqe->UserData = x * 100;
    }

    if (win_ring_submit_and_wait(&ring, 1) < 0) panic();

    unsigned head;
    win_ring_cqe* cqe;
    win_ring_for_each_cqe(&ring, head, cqe) {
        if (!SUCCEEDED(cqe->ResultCode)) return 1;
        if (cqe->Information == 8) {
            printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->UserData, buf4normal);
        } else {
            printf("%u %llu %s\n", (unsigned)cqe->Information, cqe->UserData,
                 buf4fixed);
        }
    }
    win_ring_cq_clear(&ring);

    CloseHandle(hFile);
    win_ring_queue_exit(&ring);
    return 0;
}

int testWrite() {
    HANDLE hFile = CreateFileW(L"test.txt",
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) panic();

    win_ring ring;
    if (win_ring_queue_init(32, &ring) < 0) panic();

    char buf[] = "1234567890-=";
    win_ring_sqe* sqe;

    sqe = win_ring_get_sqe(&ring);
    win_ring_prep_write(
        sqe,
        hFile,
        buf,
        sizeof(buf),
        0,
        NT_WRITE_FLAG_NONE,
        NT_IORING_OP_FLAG_NONE
    );
    win_ring_sqe_set_data(sqe, (void*)0x12345678DEADBEEF);
    if (win_ring_submit(&ring) < 0) panic();

    {
        unsigned head;
        win_ring_cqe* cqe;
        win_ring_for_each_cqe(&ring, head, cqe) {
            if (!SUCCEEDED(cqe->ResultCode)) return 1;
            printf("%u %llux %s\n", (unsigned)cqe->Information, cqe->UserData, "register");
        }
        win_ring_cq_clear(&ring);
    }

    CloseHandle(hFile);
    win_ring_queue_exit(&ring);
    return 0;
}

int testEvent() {
    win_ring ring;
    if (win_ring_queue_init(32, &ring) < 0) panic();

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

int main() {
    printDebugInfo();
    testRead();
    testWrite();
    testEvent();
}
