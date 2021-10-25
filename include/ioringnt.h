#pragma once

// Modified from https://github.com/yardenshafir/IoRing_Demos/blob/main/ioringnt.h

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <Windows.h>
#include <ioringapi.h>
#include <ntioring_x.h>
#include <winternl.h>
#include <intrin.h>

EXTERN_C_START

//
// Data structures
//
typedef struct _IORING_SUB_QUEUE_HEAD {
    ULONG QueueHead;
    ULONG QueueTail;
    ULONG64 Alignment;
} IORING_SUB_QUEUE_HEAD, * PIORING_SUB_QUEUE_HEAD;

typedef struct _IORING_COMP_QUEUE_HEAD {
    ULONG QueueHead;
    ULONG QueueTail;
} IORING_COMP_QUEUE_HEAD, * PIORING_COMP_QUEUE_HEAD;

typedef struct _NT_IORING_INFO {
    ULONG Version;
    IORING_CREATE_FLAGS Flags;
    ULONG SubmissionQueueSize;
    ULONG SubQueueSizeMask;
    ULONG CompletionQueueSize;
    ULONG CompQueueSizeMask;
    union {
        PIORING_SUB_QUEUE_HEAD SubQueueBase;
        UINT64 PadX86_SubQueueBase;
    };
    union {
        PIORING_COMP_QUEUE_HEAD CompQueueBase;
        UINT64 PadX86_CompQueueBase;
    };
} NT_IORING_INFO, * PNT_IORING_INFO;

union IORING_HANDLE_UNION {
#ifdef __cplusplus
    IORING_HANDLE_UNION(HANDLE h): Handle(h) {}
    IORING_HANDLE_UNION(UINT32 index): Index(index) {}
#endif

    HANDLE Handle;
    UINT_PTR Index; // The size of integer is adjusted to prevent the another variable from partially initialized
};

union IORING_BUFFER_UNION {
#ifdef __cplusplus
    IORING_BUFFER_UNION(void* address): Address((UINT64)address) {}
    IORING_BUFFER_UNION(IORING_REGISTERED_BUFFER indexAndOffset): IndexAndOffset(indexAndOffset) {}
#endif

    UINT64 Address; // See above
    IORING_REGISTERED_BUFFER IndexAndOffset;
};

typedef struct _NT_IORING_SQE {
    ULONG Opcode;
    ULONG Flags;
    union {
        IORING_HANDLE_UNION File;
        UINT64 PadX86_File;
    };
    UINT64 FileOffset;
    union {
        IORING_BUFFER_UNION Buffer;
        IORING_OP_CODE OpcodeToCancel;
        const HANDLE* HandlesToRegister;
        const IORING_BUFFER_INFO* BuffersToRegister;
        UINT64 PadX86_Buffer;
    };
    ULONG BufferSize;
    ULONG BufferOffset;
    ULONG Key;
    UINT64 UserData;
    UINT64 Padding[4];
} NT_IORING_SQE, * PNT_IORING_SQE;

typedef struct _NT_IORING_CQE {
    UINT64 UserData;
    union {
        HRESULT ResultCode;
        UINT64 PadX86_ResultCode;
    };
    UINT64 Information;
} NT_IORING_CQE, * PNT_IORING_CQE;

typedef struct _IO_RING_STRUCTV1 {
    ULONG IoRingVersion;
    ULONG SubmissionQueueSize;
    ULONG CompletionQueueSize;
    ULONG RequiredFlags;
    ULONG AdvisoryFlags;
} IO_RING_STRUCTV1, * PIO_RING_STRUCTV1;

typedef struct _NT_IORING_CAPABILITIES {
    IORING_VERSION Version;
    IORING_OP_CODE MaxOpcode;
    ULONG FlagsSupported;
    ULONG MaxSubQueueSize;
    ULONG MaxCompQueueSize;
} NT_IORING_CAPABILITIES, * PNT_IORING_CAPABILITIES;

//
// Function definitions
//
__kernel_entry extern NTSTATUS NTAPI
NtSubmitIoRing(
    _In_ HANDLE Handle,
    _In_ IORING_CREATE_REQUIRED_FLAGS Flags,
    _In_ ULONG EntryCount,
    _In_opt_ PLONGLONG Timeout
);

__kernel_entry extern NTSTATUS NTAPI
NtCreateIoRing(
    _Out_ PHANDLE pIoRingHandle,
    _In_ ULONG CreateParametersSize,
    _In_ PIO_RING_STRUCTV1 CreateParameters,
    _In_ ULONG OutputParametersSize,
    _In_ NT_IORING_INFO* pRingInfo
);

__kernel_entry extern NTSTATUS NTAPI
NtQueryIoRingCapabilities(
    _In_ SIZE_T CapabilitiesLength,
    _Out_ PNT_IORING_CAPABILITIES Capabilities
);

__kernel_entry extern NTSTATUS NTAPI
NtSetInformationIoRing(
    _In_ HANDLE Handle,
    _In_ ULONG InformationClass,
    _In_ ULONG InformationLength,
    _In_ PVOID IoRingInformation
);

EXTERN_C_END

#ifdef _MSC_VER
#   pragma comment(lib, "ntdll")
#endif
