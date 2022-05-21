#pragma once

#include "ioringnt.h"
#include <winternl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct win_ring {
    NT_IORING_INFO info;
    HANDLE handle;
} win_ring;

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

static inline int win_ring_queue_exit(_In_ win_ring* ring) {
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
    _In_ uint32_t sizeToRead,
    _In_ uint64_t fileOffset,
    _In_ NT_IORING_OP_FLAGS commonOpFlags
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->OpCode = IORING_OP_READ;
    sqe->Read.CommonOpFlags = commonOpFlags;
    sqe->Read.File = file;
    sqe->Read.Buffer = buffer;
    sqe->Read.Offset = fileOffset;
    sqe->Read.Length = sizeToRead;
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
    sqe->RegisterFiles.CommonOpFlags = commonOpFlags;
    sqe->RegisterFiles.Flags = flags;
    sqe->RegisterFiles.Count = count;
    sqe->RegisterFiles.Handles = handles;
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
    sqe->RegisterBuffers.CommonOpFlags = commonOpFlags;
    sqe->RegisterBuffers.Flags = flags;
    sqe->RegisterBuffers.Count = count;
    sqe->RegisterBuffers.Buffers = buffers;
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
    sqe->Cancel.CommonOpFlags = commonOpFlags;
    sqe->Cancel.File = file;
    sqe->Cancel.CancelId = cancelId;
}

static inline void win_ring_prep_write(
    _Inout_ win_ring_sqe* sqe,
    _In_ NT_IORING_HANDLEREF file,
    _In_ NT_IORING_BUFFERREF buffer,
    _In_ uint32_t sizeToWrite,
    _In_ uint64_t fileOffset,
    _In_ NT_WRITE_FLAGS flags,
    _In_ NT_IORING_OP_FLAGS commonOpFlags
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->OpCode = IORING_OP_WRITE;
    sqe->Write.CommonOpFlags = commonOpFlags;
    sqe->Write.Flags = flags;
    sqe->Write.File = file;
    sqe->Write.Buffer = buffer;
    sqe->Write.Offset = fileOffset;
    sqe->Write.Length = sizeToWrite;
}

static inline void win_ring_prep_flush(
    _Inout_ win_ring_sqe* sqe,
    _In_ NT_IORING_HANDLEREF file,
    _In_ uint32_t flags,
    _In_ NT_IORING_OP_FLAGS commonOpFlags
) {
    memset(sqe, 0, sizeof (*sqe));
    sqe->OpCode = IORING_OP_FLUSH;
    sqe->Flush.CommonOpFlags = commonOpFlags;
    sqe->Flush.Flags = flags;
    sqe->Flush.File = file;
}

static inline void win_ring_sqe_set_flags(_Inout_ win_ring_sqe* sqe, _In_ NT_IORING_SQE_FLAGS flags) {
    sqe->Flags = flags;
}

static inline void win_ring_sqe_set_data(_Inout_ win_ring_sqe* sqe, _In_ void* userData) {
    sqe->UserData = (uint64_t)(uintptr_t)userData;
}

static inline void win_ring_sqe_set_data64(_Inout_ win_ring_sqe* sqe, _In_ uint64_t userData) {
    sqe->UserData = userData;
}

static inline unsigned win_ring_sq_ready(_In_ win_ring* ring) {
    return ring->info.SubmissionQueue->Tail - ring->info.SubmissionQueue->Head;
}

static inline unsigned win_ring_sq_space_left(_In_ win_ring* ring) {
    return ring->info.SubmissionQueueSize - win_ring_sq_ready(ring);
}

static inline win_ring_sqe* win_ring_get_sqe(_Inout_ win_ring* ring) {
    // Do we need atomic operations?
    if (!win_ring_sq_space_left(ring)) return NULL;
    win_ring_sqe* sqe = &ring->info.SubmissionQueue->Entries[
        ring->info.SubmissionQueue->Tail & ring->info.SubmissionQueueRingMask
    ];
    ++ring->info.SubmissionQueue->Tail;
    return sqe;
}

static inline int win_ring_submit_and_wait_timeout(_Inout_ win_ring* ring, _In_ uint32_t numberOfEntries, _In_ uint64_t timeout) {
    NTSTATUS status = NtSubmitIoRing(ring->handle, NT_IORING_CREATE_REQUIRED_FLAG_NONE, numberOfEntries, &timeout);
    return win_ring_check_kernel_error(status);
}

static inline int win_ring_submit_and_wait(_Inout_ win_ring* ring, _In_ uint32_t numberOfEntries) {
    return win_ring_submit_and_wait_timeout(ring, numberOfEntries, INFINITE);
}

static inline int win_ring_submit(_Inout_ win_ring* ring) {
    NTSTATUS status = NtSubmitIoRing(ring->handle, NT_IORING_CREATE_REQUIRED_FLAG_NONE, 0, NULL);
    return win_ring_check_kernel_error(status);
}

// Do we need atomic operations?
#define win_ring_for_each_cqe(ring, head, cqe) for ( \
    head = (ring)->info.CompletionQueue->Head; \
    (cqe = head != (ring)->info.CompletionQueue->Tail \
        ? &(ring)->info.CompletionQueue->Entries[head & (ring)->info.CompletionQueueRingMask] \
        : NULL); \
    ++head \
)

static inline unsigned win_ring_cq_ready(_In_ win_ring* ring) {
    return ring->info.CompletionQueue->Tail - ring->info.CompletionQueue->Head;
}

static inline unsigned win_ring_cq_space_left(_In_ win_ring* ring) {
    return ring->info.CompletionQueueSize - win_ring_cq_ready(ring);
}

static inline win_ring_cqe* win_ring_peek_cqe(_In_ win_ring* ring) {
    uint32_t head;
    win_ring_cqe* cqe;
    win_ring_for_each_cqe(ring, head, cqe) {
        return cqe;
    }
    return NULL;
}

static inline win_ring_cqe* win_ring_wait_cqe(_In_ win_ring* ring) {
    win_ring_cqe* cqe = win_ring_peek_cqe(ring);
    if (cqe) return cqe;

    if (win_ring_submit_and_wait(ring, 1) < 0) return NULL;
    return win_ring_peek_cqe(ring);
}

static inline void* win_ring_cqe_get_data(_In_ win_ring_cqe* cqe) {
    return (void*)(uintptr_t)cqe->UserData;
}

static inline uint64_t win_ring_cqe_get_data64(_In_ win_ring_cqe* cqe) {
    return cqe->UserData;
}

static inline void win_ring_cq_clear(_Inout_ win_ring* ring) {
    ring->info.CompletionQueue->Head = ring->info.CompletionQueue->Tail;
}

static inline void win_ring_cq_advance(_Inout_ win_ring* ring, _In_ unsigned count) {
    ring->info.CompletionQueue->Head += count;
}

static inline void win_ring_cqe_seen(_Inout_ win_ring* ring, _In_ win_ring_cqe* cqe) {
    if (cqe) win_ring_cq_advance(ring, 1);
}

static inline int win_ring_register_event(_Inout_ win_ring* ring, _In_ HANDLE event) {
    NTSTATUS status = NtSetInformationIoRing(ring->handle, IoRingRegisterUserCompletionEventClass, sizeof event, &event);
    return win_ring_check_kernel_error(status);
}

#ifdef __cplusplus
}
#endif
