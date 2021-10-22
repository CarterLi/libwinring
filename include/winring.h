#pragma once

#include "ioringnt.h"

struct win_ring {
    NT_IORING_INFO info;
    HANDLE handle;
};

typedef NT_IORING_SQE win_ring_sqe;
typedef IORING_CQE win_ring_cqe;

inline int win_ring_check_kernel_error(_In_ NTSTATUS status) {
    if (NT_SUCCESS(status)) return 0;
    ULONG error = RtlNtStatusToDosError(status);
    SetLastError(error);
    return -1;
}

// See https://github.com/axboe/liburing/blob/master/src/include/liburing.h to find some documents

inline int win_ring_queue_init(_In_ unsigned entries, _Out_ struct win_ring* ring) {
    IO_RING_STRUCTV1 ioringStruct = {
        .IoRingVersion = 1,
        .SubmissionQueueSize = entries,
        .CompletionQueueSize = entries * 2,
        .RequiredFlags = IORING_CREATE_REQUIRED_FLAGS_NONE,
        .AdvisoryFlags = IORING_CREATE_ADVISORY_FLAGS_NONE,
    };
    NTSTATUS status = NtCreateIoRing(&ring->handle, sizeof ioringStruct, &ioringStruct, sizeof ring->info, &ring->info);
    return win_ring_check_kernel_error(status);
}

inline int win_ring_queue_exit(_In_ struct win_ring* ring) {
    NTSTATUS status = NtClose(ring->handle);
    return win_ring_check_kernel_error(status);
}

inline void win_ring_prep_nop(_Inout_ win_ring_sqe* sqe) {
    memset(sqe, 0, sizeof(*sqe));
    sqe->Opcode = IORING_OP_NOP;
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

inline void win_ring_prep_cancel(_Inout_ win_ring_sqe* sqe, _In_ void* userDataToCancel) {
    memset(sqe, 0, sizeof(*sqe));
    sqe->Opcode = IORING_OP_CANCEL;
    // FIXME: which field is used to store the cancel token?
    sqe->Buffer = userDataToCancel;
}

// Doesn't work yet
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

// TODO: What is key used for?
inline void win_ring_sqe_set_key(_Inout_ win_ring_sqe* sqe, _In_ ULONG key) {
    sqe->Key = key;
}

inline void win_ring_sqe_set_data(_Inout_ win_ring_sqe* sqe, _In_ PVOID userData) {
    sqe->UserData = (UINT_PTR)userData;
}

inline win_ring_sqe* win_ring_get_sqe(_Inout_ struct win_ring* ring) {
    if (ring->info.SubQueueBase->QueueTail - ring->info.SubQueueBase->QueueHead == ring->info.SubmissionQueueSize) {
        return NULL;
    }

    win_ring_sqe* sqe = (win_ring_sqe*)(
        (ULONG64)ring->info.SubQueueBase +
        sizeof(IORING_SUB_QUEUE_HEAD) +
        (ring->info.SubQueueBase->QueueTail & ring->info.SubQueueSizeMask) * sizeof(NT_IORING_SQE)
    );
    ++ring->info.SubQueueBase->QueueTail;
    return sqe;
}

inline int win_ring_submit_and_wait_timeout(_Inout_ struct win_ring* ring, _In_ ULONG numberOfEntries, _In_ LONGLONG timeout) {
    LARGE_INTEGER integer = { .QuadPart = timeout };
    NTSTATUS status = NtSubmitIoRing(ring->handle, IORING_CREATE_REQUIRED_FLAGS_NONE, numberOfEntries, &integer);
    return win_ring_check_kernel_error(status);
}

inline int win_ring_submit_and_wait(_Inout_ struct win_ring* ring, _In_ ULONG numberOfEntries) {
    return win_ring_submit_and_wait_timeout(ring, numberOfEntries, INFINITE);
}

inline int win_ring_submit(_Inout_ struct win_ring* ring) {
    return win_ring_submit_and_wait_timeout(ring, 0, 0);
}

#define win_ring_for_each_cqe(ring, head, cqe) for ( \
    head = (ring)->info.CompQueueBase->QueueHead; \
    cqe = head < (ring)->info.CompQueueBase->QueueTail \
        ? (win_ring_cqe*)((ULONG64)(ring)->info.CompQueueBase + sizeof(IORING_COMP_QUEUE_HEAD) + (head & (ring)->info.CompQueueSizeMask) * sizeof(IORING_CQE)) \
        : NULL; \
    ++head \
)

inline int win_ring_peek_cqe(_In_ struct win_ring* ring, _Inout_ win_ring_cqe** cqePtr) {
    if (ring->info.CompQueueBase->QueueHead >= ring->info.CompQueueBase->QueueTail) return -1;
    *cqePtr = (win_ring_cqe*)(
        (ULONG64)ring->info.CompQueueBase +
        sizeof(IORING_COMP_QUEUE_HEAD) +
        (ring->info.CompQueueBase->QueueHead & ring->info.CompQueueSizeMask) * sizeof(IORING_CQE)
    );
    return 0;
}

inline void* win_ring_cqe_get_data(_In_ win_ring_cqe* cqe) {
    return (void*)cqe->UserData;
}

inline void win_ring_cq_clear(_Inout_ struct win_ring* ring) {
    ring->info.CompQueueBase->QueueHead = ring->info.CompQueueBase->QueueTail;
}

inline void win_ring_cq_advance(_Inout_ struct win_ring* ring, _In_ unsigned count) {
    ring->info.CompQueueBase->QueueHead += count;
}

inline void win_ring_cqe_seen(_Inout_ struct win_ring* ring, _In_ win_ring_cqe* cqe) {
    if (cqe) win_ring_cq_advance(ring, 1);
}
