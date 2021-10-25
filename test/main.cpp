#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>
#include <cstddef>

#include "libwinring.h"
#pragma comment(lib, "onecoreuap")

[[noreturn]]
void panic() {
    char buf[256] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, (sizeof(buf) / sizeof(*buf)), NULL);
    printf("ERROR: %s", buf);
    exit(1);
}

typedef struct _REAL_HIORING {
    ULONG SqePending;
    ULONG SqeCount;
    HANDLE handle;
    IORING_INFO Info;
    ULONG IoRingKernelAcceptedVersion;
} REAL_HIORING;

int kernelbase() {
    HANDLE hFile = CreateFileW(LR"(C:\Users\Carter\Downloads\Read me.txt)",
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) panic();

    HIORING hIoRing;
    CreateIoRing(IORING_VERSION_1, {
        .Required = IORING_CREATE_REQUIRED_FLAGS_NONE,
        .Advisory = IORING_CREATE_ADVISORY_FLAGS_NONE,
        }, 8, 16, &hIoRing);
    BuildIoRingCancelRequest(hIoRing, IORING_HANDLE_REF(hFile), IORING_OP_READ, 0xDEADBEEF);
    BuildIoRingCancelRequest(hIoRing, IORING_HANDLE_REF(hFile), IORING_OP_READ, 0xDEADBEEF);
    SubmitIoRing(hIoRing, 1, INFINITE, nullptr);
    IORING_CQE cqe;
    PopIoRingCompletion(hIoRing, &cqe);

    return 0;
}

int printDebugInfo() {
    if (win_ring_capabilities capabilities; win_ring_query_capabilities(&capabilities) < 0) {
        panic();
    } else {
        printf("Max opcode: %d; Supported flags: %x\n\n", capabilities.MaxOpcode, capabilities.FlagsSupported);
    }

    printf("offsetof(NT_IORING_SQE):\nOpcode: %zx\nFlags: %zx\nFile: %zx\nFileOffset: %zx\nBuffer: %zx\nBufferSize: %zx\nBufferOffset: %zx\nKey: %zx\nUserData: %zx\n\n",
        offsetof(NT_IORING_SQE, Opcode),
        offsetof(NT_IORING_SQE, Flags),
        offsetof(NT_IORING_SQE, File),
        offsetof(NT_IORING_SQE, FileOffset),
        offsetof(NT_IORING_SQE, Buffer),
        offsetof(NT_IORING_SQE, BufferSize),
        offsetof(NT_IORING_SQE, BufferOffset),
        offsetof(NT_IORING_SQE, Key),
        offsetof(NT_IORING_SQE, UserData)
    );

    printf("sizeof(NT_IORING_SQE): %zx\nsizeof(IORING_SUB_QUEUE_HEAD): %zx\n\n",
        sizeof(NT_IORING_SQE),
        sizeof(IORING_SUB_QUEUE_HEAD)
    );

    printf("offsetof(REAL_HIORING, Info): %zx\n\n", offsetof(REAL_HIORING, Info));

    printf("offsetof(NT_IORING_INFO):\nVersion: %zx\nFlags: %zx\nSubmissionQueueSize: %zx\nSubQueueSizeMask: %zx\nSubQueueBase: %zx\n",
        offsetof(NT_IORING_INFO, Version),
        offsetof(NT_IORING_INFO, Flags),
        offsetof(NT_IORING_INFO, SubmissionQueueSize),
        offsetof(NT_IORING_INFO, SubQueueSizeMask),
        offsetof(NT_IORING_INFO, SubQueueBase)
    );

    printf("\noffsetof(IORING_SUB_QUEUE_HEAD):\nQueueHead: %zx\nQueueTail: %zx\nAlignment: %zx\n",
        offsetof(IORING_SUB_QUEUE_HEAD, QueueHead),
        offsetof(IORING_SUB_QUEUE_HEAD, QueueTail),
        offsetof(IORING_SUB_QUEUE_HEAD, Alignment)
    );
}

int main() {
    HANDLE hFile = CreateFileW(LR"(C:\Users\Carter\Downloads\Read me.txt)",
        GENERIC_READ,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (hFile == INVALID_HANDLE_VALUE) panic();

    win_ring ring;
    if (win_ring_queue_init(1, &ring) < 0) panic();

    win_ring_sqe* sqe;

    char buf[32] = "";

    sqe = win_ring_get_sqe(&ring);
    win_ring_prep_register_files(sqe, &hFile, 1);
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
    IORING_BUFFER_INFO bufferInfo = { .Address = buf, .Length = 32 };
    win_ring_prep_register_buffers(sqe, &bufferInfo, 1);
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

    for (int x = 0; x < 16; ++x) {
        sqe = win_ring_get_sqe(&ring);
        if (x & 1) {
            win_ring_prep_read(sqe, 0u, IORING_REGISTERED_BUFFER{ .BufferIndex = 0, .Offset = 0 }, 8, 0);
            win_ring_sqe_set_flags(sqe, IORING_SQE_PREREGISTERED_FILE | IORING_SQE_PREREGISTERED_BUFFER);
        } else {
            win_ring_prep_read(sqe, hFile, buf, 16, 0);
        }
        sqe->UserData = x * 100;

        if (win_ring_submit_and_wait(&ring, 1) < 0) panic();

        unsigned head;
        win_ring_cqe* cqe;
        win_ring_for_each_cqe(&ring, head, cqe) {
            if (!SUCCEEDED(cqe->ResultCode)) return 1;
            printf("%u %llu %s\n", (unsigned)cqe->Information, (ULONG64)cqe->UserData, buf);
        }
        win_ring_cq_clear(&ring);
    }

    CloseHandle(hFile);
    win_ring_queue_exit(&ring);
}
