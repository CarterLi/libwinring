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
    ULONG64 Aligment;
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
    PIORING_SUB_QUEUE_HEAD SubQueueBase;
    PIORING_COMP_QUEUE_HEAD CompQueueBase;
} NT_IORING_INFO, * PNT_IORING_INFO;

typedef struct _NT_IORING_SQE {
    ULONG Opcode;
    ULONG Flags;
    HANDLE FileRef;
    LARGE_INTEGER FileOffset;
    PVOID Buffer;
    ULONG BufferSize;
    ULONG BufferOffset;
    ULONG Key;
    UINT_PTR UserData;
    PVOID Padding[4];
} NT_IORING_SQE, * PNT_IORING_SQE;

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

typedef struct _HIORING {
    ULONG SqePending;
    ULONG SqeCount;
    HANDLE handle;
    IORING_INFO Info;
    ULONG IoRingKernelAcceptedVersion;
} NT_HIORING, * PNT_HIORING;

//
// Function definitions
//
__kernel_entry NTSTATUS NTAPI
NtSubmitIoRing(
    _In_ HANDLE Handle,
    _In_ IORING_CREATE_REQUIRED_FLAGS Flags,
    _In_ ULONG EntryCount,
    _In_ PLARGE_INTEGER Timeout
);

__kernel_entry NTSTATUS NTAPI
NtCreateIoRing(
    _Out_ PHANDLE pIoRingHandle,
    _In_ ULONG CreateParametersSize,
    _In_ PIO_RING_STRUCTV1 CreateParameters,
    _In_ ULONG OutputParametersSize,
    _In_ NT_IORING_INFO* pRingInfo
);

__kernel_entry NTSTATUS NTAPI
NtQueryIoRingCapabilities(
    _In_ SIZE_T CapabilitiesLength,
    _Out_ PNT_IORING_CAPABILITIES Capabilities
);

__kernel_entry NTSTATUS NTAPI
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
