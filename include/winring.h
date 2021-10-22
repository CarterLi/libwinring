#pragma once

#include "ioringnt.h"

struct win_ring {
    NT_IORING_INFO info;
    HANDLE handle;
};

typedef NT_IORING_SQE win_ring_sqe;
typedef IORING_CQE win_ring_cqe;

inline int win_ring_queue_init(_In_ unsigned entries, _Out_ struct win_ring* ring) {
    IO_RING_STRUCTV1 ioringStruct = {
        .IoRingVersion = 1,
        .SubmissionQueueSize = entries,
        .CompletionQueueSize = entries * 2,
        .RequiredFlags = IORING_CREATE_REQUIRED_FLAGS_NONE,
        .AdvisoryFlags = IORING_CREATE_ADVISORY_FLAGS_NONE,
    };
    return NT_SUCCESS(NtCreateIoRing(&ring->handle, sizeof ioringStruct, &ioringStruct, sizeof ring->info, &ring->info)) ? 0 : -1;
}

inline void win_ring_queue_exit(_In_ struct win_ring* ring) {
    NtClose(ring->handle);
}

inline void win_ring_prep_read(_Inout_ win_ring_sqe* sqe, _In_ HANDLE file, _In_ PVOID buffer, _In_ ULONG sizeToRead, _In_ LONGLONG fileOffset) {
    memset(sqe, 0, sizeof(*sqe));
    sqe->Opcode = IORING_OP_READ;
    sqe->FileRef = file;
    sqe->FileOffset.QuadPart = fileOffset;
    sqe->Buffer = buffer;
    sqe->BufferOffset = 0;
    sqe->BufferSize = sizeToRead;
}

inline void win_ring_prep_write(_Inout_ win_ring_sqe* sqe, _In_ HANDLE file, _In_ PVOID buffer, _In_ ULONG sizeToRead, _In_ LONGLONG fileOffset) {
    memset(sqe, 0, sizeof(*sqe));
    sqe->Opcode = 0x5;
    sqe->FileRef = file;
    sqe->FileOffset.QuadPart = fileOffset;
    sqe->Buffer = buffer;
    sqe->BufferOffset = 0;
    sqe->BufferSize = sizeToRead;
}

inline void win_ring_sqe_set_flags(_Inout_ win_ring_sqe* sqe, _In_ ULONG flags) {
    sqe->Flags = flags;
}

inline void win_ring_sqe_set_key(_Inout_ win_ring_sqe* sqe, _In_ ULONG key) {
    sqe->Key = key;
}

inline void win_ring_sqe_set_data(_Inout_ win_ring_sqe* sqe, _In_ PVOID userData) {
    sqe->UserData = (UINT_PTR)userData;
}

inline win_ring_sqe* win_ring_get_sqe(_Inout_ struct win_ring* ring) {
    win_ring_sqe* sqe = (win_ring_sqe*)((ULONG64)ring->info.SubQueueBase + sizeof(IORING_QUEUE_HEAD) + ring->info.SubQueueBase->QueueCount * sizeof(NT_IORING_SQE));
    ++ring->info.SubQueueBase->QueueCount; // ring buffer?
    return sqe;
}

inline int win_ring_submit_and_wait_timeout(_Inout_ struct win_ring* ring, _In_ ULONG numberOfEntries, _In_ LONGLONG timeout) {
    LARGE_INTEGER integer = { .QuadPart = timeout };
    int res = NT_SUCCESS(NtSubmitIoRing(ring->handle, IORING_CREATE_REQUIRED_FLAGS_NONE, numberOfEntries, &integer)) ? 0 : -1;
    return res;
}

inline int win_ring_submit_and_wait(_Inout_ struct win_ring* ring, _In_ ULONG numberOfEntries) {
    return win_ring_submit_and_wait_timeout(ring, numberOfEntries, 0);
}

inline int win_ring_submit(_Inout_ struct win_ring* ring) {
    return win_ring_submit_and_wait(ring, 0);
}

inline int win_ring_get_cqe(_In_ struct win_ring* ring, _Inout_ win_ring_cqe** cqePtr) {
    if (ring->info.CompQueueBase->QueueIndex >= ring->info.CompQueueBase->QueueCount) return -1;
    *cqePtr = (win_ring_cqe*)((ULONG64)ring->info.CompQueueBase + sizeof(IORING_COMP_QUEUE_HEAD) + ring->info.CompQueueBase->QueueIndex * sizeof(IORING_CQE));
    return 0;
}

inline void* win_ring_cqe_get_data(_In_ win_ring_cqe* cqe) {
    return (void*)cqe->UserData;
}

inline void win_ring_cq_advance(_Inout_ struct win_ring* ring, _In_ unsigned count) {
    ring->info.CompQueueBase->QueueIndex += count;
}

inline int win_ring_cqe_seen(_Inout_ struct win_ring* ring, _In_ win_ring_cqe* cqe) {
    if (cqe) win_ring_cq_advance(ring, 1);
}

#define win_ring_for_each_cqe(ring, head, cqe) for ( \
    head = 0; \
    cqe = head < (ring)->info.CompQueueBase->QueueCount \
        ? (win_ring_cqe*)((ULONG64)(ring)->info.CompQueueBase + sizeof(IORING_COMP_QUEUE_HEAD) + (head + (ring)->info.CompQueueBase->QueueIndex) * sizeof(IORING_CQE)) \
        : NULL; \
    ++head \
)
