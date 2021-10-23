#pragma once

#include "ioringnt.h"

struct win_ring {
    NT_IORING_INFO info;
    HANDLE handle;
};

typedef NT_IORING_SQE win_ring_sqe;
typedef NT_IORING_CQE win_ring_cqe;
typedef NT_IORING_CAPABILITIES win_ring_capabilities;

enum IORING_SQE_FLAG {
    IORING_SQE_PREREGISTERED_FILE = 0b01,
    IORING_SQE_PREREGISTERED_BUFFER = 0b10,
};

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

inline int win_ring_query_capabilities(_Out_ win_ring_capabilities* capabilities) {
    NTSTATUS status = NtQueryIoRingCapabilities(sizeof (*capabilities), capabilities);
    return win_ring_check_kernel_error(status);
}

inline void win_ring_prep_nop(_Inout_ win_ring_sqe* sqe) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->Opcode = IORING_OP_NOP;
}

inline void win_ring_prep_read(
    _Inout_ win_ring_sqe* sqe,
    _In_ IORING_HANDLE_UNION file,
    IORING_BUFFER_UNION buffer,
    _In_ UINT32 sizeToRead,
    _In_ UINT64 fileOffset
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->Opcode = IORING_OP_READ;
    sqe->File = file;
    sqe->FileOffset = fileOffset;
    sqe->Buffer = buffer;
    sqe->BufferOffset = 0;
    sqe->BufferSize = sizeToRead;

    // To read fixed file, IORING_SQE_PREREGISTERED_FILE flag must be set manually
    // To write fixed buffer, IORING_SQE_PREREGISTERED_BUFFER flag must be set manually
}

inline void win_ring_prep_register_files(
    _Inout_ win_ring_sqe* sqe,
    _In_reads_(count) HANDLE const handles[],
    _In_ unsigned count
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->Opcode = IORING_OP_REGISTER_FILES;
    sqe->HandlesToRegister = handles;
    sqe->BufferSize = count * sizeof (*handles);
}

inline void win_ring_prep_register_buffers(
    _Inout_ win_ring_sqe* sqe,
    _In_reads_(count) IORING_BUFFER_INFO const buffers[],
    _In_ unsigned count
) {
    memset(sqe, 0, sizeof(*sqe));
    sqe->Opcode = IORING_OP_REGISTER_BUFFERS;
    sqe->BuffersToRegister = buffers;
    sqe->BufferSize = count * sizeof (*buffers);
}

inline void win_ring_prep_cancel(
    _Inout_ win_ring_sqe* sqe,
    _In_ IORING_HANDLE_UNION file,
    IORING_OP_CODE opcodeToCancel,
    _In_ void* userDataToCancel
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->Opcode = IORING_OP_CANCEL;
    sqe->OpcodeToCancel = opcodeToCancel;
    sqe->File = file;
    // FIXME: Where to store user data for this sqe?
    sqe->UserData = (UINT_PTR)userDataToCancel;

    // To cancel fixed file, IORING_SQE_PREREGISTERED_FILE flag must be set manually
}

// Doesn't work yet. The opcode is accepted, but won't run
inline void win_ring_prep_write(
    _Inout_ win_ring_sqe* sqe,
    _In_ IORING_HANDLE_UNION file,
    _In_ IORING_BUFFER_UNION buffer,
    _In_ UINT32 sizeToRead,
    _In_ UINT64 fileOffset
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->Opcode = 0x5;
    sqe->File = file;
    sqe->FileOffset = fileOffset;
    sqe->Buffer = buffer;
    sqe->BufferOffset = 0;
    sqe->BufferSize = sizeToRead;

    // See win_ring_prep_read for fixed file / buffer
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
    // Do we need atomic operations?
    if (ring->info.SubQueueBase->QueueTail - ring->info.SubQueueBase->QueueHead == ring->info.SubmissionQueueSize) {
        return NULL;
    }

    win_ring_sqe* sqe = (win_ring_sqe*)(
        (ULONG64)ring->info.SubQueueBase +
        sizeof (IORING_SUB_QUEUE_HEAD) +
        (ring->info.SubQueueBase->QueueTail & ring->info.SubQueueSizeMask) * sizeof (NT_IORING_SQE)
        );
    ++ring->info.SubQueueBase->QueueTail;
    return sqe;
}

inline int win_ring_submit_and_wait_timeout(_Inout_ struct win_ring* ring, _In_ ULONG numberOfEntries, _In_ LONGLONG timeout) {
    NTSTATUS status = NtSubmitIoRing(ring->handle, IORING_CREATE_REQUIRED_FLAGS_NONE, numberOfEntries, &timeout);
    return win_ring_check_kernel_error(status);
}

inline int win_ring_submit_and_wait(_Inout_ struct win_ring* ring, _In_ ULONG numberOfEntries) {
    return win_ring_submit_and_wait_timeout(ring, numberOfEntries, INFINITE);
}

inline int win_ring_submit(_Inout_ struct win_ring* ring) {
    NTSTATUS status = NtSubmitIoRing(ring->handle, IORING_CREATE_REQUIRED_FLAGS_NONE, 0, NULL);
    return win_ring_check_kernel_error(status);
}

// Do we need atomic operations?
#define win_ring_for_each_cqe(ring, head, cqe) for ( \
    head = (ring)->info.CompQueueBase->QueueHead; \
    cqe = head < (ring)->info.CompQueueBase->QueueTail \
        ? (win_ring_cqe*)((ULONG64)(ring)->info.CompQueueBase + sizeof (IORING_COMP_QUEUE_HEAD) + (head & (ring)->info.CompQueueSizeMask) * sizeof (NT_IORING_CQE)) \
        : NULL; \
    ++head \
)

inline int win_ring_peek_cqe(_In_ struct win_ring* ring, _Inout_ win_ring_cqe** cqePtr) {
    if (ring->info.CompQueueBase->QueueHead >= ring->info.CompQueueBase->QueueTail) return -1;
    *cqePtr = (win_ring_cqe*)(
        (ULONG64)ring->info.CompQueueBase +
        sizeof (IORING_COMP_QUEUE_HEAD) +
        (ring->info.CompQueueBase->QueueHead & ring->info.CompQueueSizeMask) * sizeof (NT_IORING_CQE)
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
