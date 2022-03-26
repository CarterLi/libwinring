#pragma once

#include "ioringnt.h"
#include <winternl.h>

struct win_ring {
    NT_IORING_INFO info;
    HANDLE handle;
};

typedef NT_IORING_SQE win_ring_sqe;
typedef NT_IORING_CQE win_ring_cqe;
typedef NT_IORING_CAPABILITIES win_ring_capabilities;

static inline int win_ring_check_kernel_error(_In_ NTSTATUS status) {
    if (NT_SUCCESS(status)) return 0;
    SetLastError(RtlNtStatusToDosError(status));
    return -1;
}

// See https://github.com/axboe/liburing/blob/master/src/include/liburing.h to find some documents

static inline int win_ring_queue_init(
    _In_ uint32_t entries,
    _Out_ struct win_ring* ring
) {
    NT_IORING_STRUCTV1 ioringStruct = {
        .IoRingVersion = IORING_VERSION_3, // Requires Win11 22H2
        .SubmissionQueueSize = entries,
        .CompletionQueueSize = entries * 2,
        .Flags = {
            .Required = NT_IORING_CREATE_REQUIRED_FLAG_NONE,
            .Advisory = NT_IORING_CREATE_ADVISORY_FLAG_NONE,
        }
    };
    NTSTATUS status = NtCreateIoRing(&ring->handle, sizeof (ioringStruct), &ioringStruct, sizeof (ring->info), &ring->info);
    return win_ring_check_kernel_error(status);
}

static inline int win_ring_queue_exit(_In_ struct win_ring* ring) {
    NTSTATUS status = NtClose(ring->handle);
    return win_ring_check_kernel_error(status);
}

static inline int win_ring_query_capabilities(_Out_ win_ring_capabilities* capabilities) {
    NTSTATUS status = NtQueryIoRingCapabilities(sizeof (*capabilities), capabilities);
    return win_ring_check_kernel_error(status);
}

static inline void win_ring_prep_nop(_Inout_ win_ring_sqe* sqe) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->OpCode = IORING_OP_NOP;
}

static inline void win_ring_prep_read(
    _Inout_ win_ring_sqe* sqe,
    _In_ NT_IORING_HANDLEREF file,
    NT_IORING_BUFFERREF buffer,
    _In_ UINT32 sizeToRead,
    _In_ UINT64 fileOffset,
    _In_ NT_IORING_OP_FLAGS commonOpFlags
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->OpCode = IORING_OP_READ;
    sqe->Read = {
        .CommonOpFlags = commonOpFlags,
        .File = file,
        .Buffer = buffer,
        .Offset = fileOffset,
        .Length = sizeToRead,
    };
}

static inline void win_ring_prep_register_files(
    _Inout_ win_ring_sqe* sqe,
    _In_reads_(count) HANDLE const handles[],
    _In_ unsigned count,
    _In_ NT_IORING_REG_FILES_FLAGS flags,
    _In_ NT_IORING_OP_FLAGS commonOpFlags
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->OpCode = IORING_OP_REGISTER_FILES;
    sqe->RegisterFiles = {
        .CommonOpFlags = commonOpFlags,
        .Flags = flags,
        .Count = count,
        .Handles = handles,
    };
}

static inline void win_ring_prep_register_buffers(
    _Inout_ win_ring_sqe* sqe,
    _In_reads_(count) IORING_BUFFER_INFO const buffers[],
    _In_ unsigned count,
    _In_ NT_IORING_REG_BUFFERS_FLAGS flags,
    _In_ NT_IORING_OP_FLAGS commonOpFlags
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->OpCode = IORING_OP_REGISTER_BUFFERS;
    sqe->RegisterBuffers = {
        .CommonOpFlags = commonOpFlags,
        .Flags = flags,
        .Count = count,
        .Buffers = buffers,
    };
}

static inline void win_ring_prep_cancel(
    _Inout_ win_ring_sqe* sqe,
    _In_ NT_IORING_HANDLEREF file,
    _In_ uint64_t cancelId, // ???
    _In_ void* userDataToCancel,
    _In_ NT_IORING_OP_FLAGS commonOpFlags
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->OpCode = IORING_OP_CANCEL;
    sqe->Cancel = {
        .CommonOpFlags = commonOpFlags,
        .File = file,
        .CancelId = cancelId,
    };
}

static inline void win_ring_prep_write(
    _Inout_ win_ring_sqe* sqe,
    _In_ NT_IORING_HANDLEREF file,
    _In_ NT_IORING_BUFFERREF buffer,
    _In_ UINT32 sizeToWrite,
    _In_ UINT64 fileOffset,
    _In_ NT_WRITE_FLAGS flags,
    _In_ NT_IORING_OP_FLAGS commonOpFlags
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->OpCode = IORING_OP_WRITE;
    sqe->Write = {
        .CommonOpFlags = commonOpFlags,
        .Flags = flags,
        .File = file,
        .Buffer = buffer,
        .Offset = fileOffset,
        .Length = sizeToWrite,
    };
}

static inline void win_ring_prep_flush(
    _Inout_ win_ring_sqe* sqe,
    _In_ NT_IORING_HANDLEREF file,
    _In_ uint32_t flags,
    _In_ NT_IORING_OP_FLAGS commonOpFlags
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->OpCode = IORING_OP_FLUSH;
    sqe->Flush = {
        .CommonOpFlags = commonOpFlags,
        .Flags = flags,
        .File = file,
    };
}

static inline void win_ring_sqe_set_flags(_Inout_ win_ring_sqe* sqe, _In_ NT_IORING_SQE_FLAGS flags) {
    sqe->Flags = flags;
}

static inline void win_ring_sqe_set_data(_Inout_ win_ring_sqe* sqe, _In_ void* userData) {
    sqe->UserData = (uint64_t)(uintptr_t)userData;
}

static inline unsigned win_ring_sq_ready(_In_ struct win_ring* ring) {
    return ring->info.SubQueueBase->Tail - ring->info.SubQueueBase->Head;
}

static inline unsigned win_ring_sq_space_left(_In_ struct win_ring* ring) {
    return ring->info.SubQueueSize - win_ring_sq_ready(ring);
}

static inline win_ring_sqe* win_ring_get_sqe(_Inout_ struct win_ring* ring) {
    // Do we need atomic operations?
    if (!win_ring_sq_space_left(ring)) return NULL;
    win_ring_sqe* sqe = &ring->info.SubQueueBase->Entries[
        ring->info.SubQueueBase->Tail & ring->info.SubQueueSizeMask
    ];
    ++ring->info.SubQueueBase->Tail;
    return sqe;
}

static inline int win_ring_submit_and_wait_timeout(_Inout_ struct win_ring* ring, _In_ uint32_t numberOfEntries, _In_ uint64_t timeout) {
    NTSTATUS status = NtSubmitIoRing(ring->handle, NT_IORING_CREATE_REQUIRED_FLAG_NONE, numberOfEntries, &timeout);
    return win_ring_check_kernel_error(status);
}

static inline int win_ring_submit_and_wait(_Inout_ struct win_ring* ring, _In_ uint32_t numberOfEntries) {
    return win_ring_submit_and_wait_timeout(ring, numberOfEntries, INFINITE);
}

static inline int win_ring_submit(_Inout_ struct win_ring* ring) {
    NTSTATUS status = NtSubmitIoRing(ring->handle, NT_IORING_CREATE_REQUIRED_FLAG_NONE, 0, NULL);
    return win_ring_check_kernel_error(status);
}

// Do we need atomic operations?
#define win_ring_for_each_cqe(ring, head, cqe) for ( \
    head = (ring)->info.CompQueueBase->Head; \
    (cqe = head != (ring)->info.CompQueueBase->Tail \
        ? &(ring)->info.CompQueueBase->Entries[head & (ring)->info.CompQueueSizeMask] \
        : NULL); \
    ++head \
)

static inline unsigned win_ring_cq_ready(_In_ struct win_ring* ring) {
    return ring->info.CompQueueBase->Tail - ring->info.CompQueueBase->Head;
}

static inline unsigned win_ring_cq_space_left(_In_ struct win_ring* ring) {
    return ring->info.CompQueueSize - win_ring_cq_ready(ring);
}

static inline win_ring_cqe* win_ring_peek_cqe(_In_ struct win_ring* ring) {
    uint32_t head;
    win_ring_cqe* cqe;
    win_ring_for_each_cqe(ring, head, cqe) {
        return cqe;
    }
    return NULL;
}

static inline win_ring_cqe* win_ring_wait_cqe(_In_ struct win_ring* ring) {
    win_ring_cqe* cqe = win_ring_peek_cqe(ring);
    if (cqe) return cqe;

    if (win_ring_submit_and_wait(ring, 1) < 0) return NULL;
    return win_ring_peek_cqe(ring);
}

static inline void* win_ring_cqe_get_data(_In_ win_ring_cqe* cqe) {
    return (void*)(uintptr_t)cqe->UserData;
}

static inline void win_ring_cq_clear(_Inout_ struct win_ring* ring) {
    ring->info.CompQueueBase->Head = ring->info.CompQueueBase->Tail;
}

static inline void win_ring_cq_advance(_Inout_ struct win_ring* ring, _In_ unsigned count) {
    ring->info.CompQueueBase->Head += count;
}

static inline void win_ring_cqe_seen(_Inout_ struct win_ring* ring, _In_ win_ring_cqe* cqe) {
    if (cqe) win_ring_cq_advance(ring, 1);
}
