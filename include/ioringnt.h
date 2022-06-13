#pragma once

// Generated with `pdbex.exe * ntkrnlmp.pdb -o ntkrnlmp.h`
// Modified by Carter Li

#include <stdint.h>
#include <assert.h>
#include <ntstatus.h>
#define WIN32_NO_STATUS
#ifndef NOMINMAX
#   define NOMINMAX 1
#endif
#ifndef WIN32_LEAD_AND_MEAN
#   define WIN32_LEAD_AND_MEAN 1
#endif
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Data structures
//
typedef enum _IORING_OP_CODE
{
    IORING_OP_NOP = 0,
    IORING_OP_READ = 1,
    IORING_OP_REGISTER_FILES = 2,
    IORING_OP_REGISTER_BUFFERS = 3,
    IORING_OP_CANCEL = 4,
    IORING_OP_WRITE = 5,
    IORING_OP_FLUSH = 6,
} IORING_OP_CODE;

typedef enum _NT_IORING_OP_FLAGS
{
    NT_IORING_OP_FLAG_NONE = 0,
    NT_IORING_OP_FLAG_REGISTERED_FILE = 1,
    NT_IORING_OP_FLAG_REGISTERED_BUFFER = 2,
} NT_IORING_OP_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(NT_IORING_OP_FLAGS);

typedef union _NT_IORING_HANDLEREF
{
#ifdef __cplusplus
    _NT_IORING_HANDLEREF(HANDLE h): Handle((uintptr_t)h) {}
    _NT_IORING_HANDLEREF(uint32_t index): HandleIndex(index) {}
#endif

    /* 0x0000 */ /* HANDLE */ uint64_t Handle;
    /* 0x0000 */ /* uint32_t */ uint64_t HandleIndex;
} NT_IORING_HANDLEREF, * PNT_IORING_HANDLEREF; /* size: 0x0008 */
static_assert (sizeof(NT_IORING_HANDLEREF) == 0x0008, "");

typedef enum _NT_IORING_SQE_FLAGS
{
    NT_IORING_SQE_FLAG_NONE = 0,
    NT_IORING_SQE_FLAG_DRAIN_PRECEDING_OPS = 1,
} NT_IORING_SQE_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(NT_IORING_SQE_FLAGS);

typedef struct _IORING_REGISTERED_BUFFER
{
    /* 0x0000 */ uint32_t BufferIndex;
    /* 0x0004 */ uint32_t Offset;
} IORING_REGISTERED_BUFFER, PIORING_REGISTERED_BUFFER; /* size: 0x0008 */
static_assert (sizeof(IORING_REGISTERED_BUFFER) == 0x0008, "");

typedef union _NT_IORING_BUFFERREF
{
#ifdef __cplusplus
    _NT_IORING_BUFFERREF(void* address): Address((uintptr_t)address) {}
    _NT_IORING_BUFFERREF(IORING_REGISTERED_BUFFER fixedBuffer): FixedBuffer(fixedBuffer) {}
#endif

    /* 0x0000 */ /* void* */ uint64_t Address;
    /* 0x0000 */ IORING_REGISTERED_BUFFER FixedBuffer;
} NT_IORING_BUFFERREF, * PNT_IORING_BUFFERREF; /* size: 0x0008 */
static_assert (sizeof(NT_IORING_BUFFERREF) == 0x0008, "");

typedef struct _NT_IORING_OP_READ
{
    /* 0x0000 */ NT_IORING_OP_FLAGS CommonOpFlags;
    /* 0x0004 */ uint32_t Padding;
    /* 0x0008 */ NT_IORING_HANDLEREF File;
    /* 0x0010 */ NT_IORING_BUFFERREF Buffer;
    /* 0x0018 */ uint64_t Offset;
    /* 0x0020 */ uint32_t Length;
    /* 0x0024 */ uint32_t Key;
} NT_IORING_OP_READ, * PNT_IORING_OP_READ; /* size: 0x0028 */
static_assert (sizeof(NT_IORING_OP_READ) == 0x0028, "");

typedef enum _NT_IORING_REG_FILES_REQ_FLAGS
{
    NT_IORING_REG_FILES_REQ_FLAG_NONE = 0,
} NT_IORING_REG_FILES_REQ_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(NT_IORING_REG_FILES_REQ_FLAGS);

typedef enum _NT_IORING_REG_FILES_ADV_FLAGS
{
    NT_IORING_REG_FILES_ADV_FLAG_NONE = 0,
} NT_IORING_REG_FILES_ADV_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(NT_IORING_REG_FILES_ADV_FLAGS);

typedef struct _NT_IORING_REG_FILES_FLAGS
{
    /* 0x0000 */ NT_IORING_REG_FILES_REQ_FLAGS Required;
    /* 0x0004 */ NT_IORING_REG_FILES_ADV_FLAGS Advisory;
} NT_IORING_REG_FILES_FLAGS, * PNT_IORING_REG_FILES_FLAGS; /* size: 0x0008 */
static_assert (sizeof(NT_IORING_REG_FILES_FLAGS) == 0x0008, "");

typedef struct _NT_IORING_OP_REGISTER_FILES
{
    /* 0x0000 */ NT_IORING_OP_FLAGS CommonOpFlags;
    /* 0x0004 */ NT_IORING_REG_FILES_FLAGS Flags;
    /* 0x000c */ uint32_t Count;
    union {
      /* 0x0010 */ const HANDLE* Handles;
      /* 0x0010 */ uint64_t PadX86_Handles;
    }; /* size: 0x0008 */
} NT_IORING_OP_REGISTER_FILES, * PNT_IORING_OP_REGISTER_FILES; /* size: 0x0018 */
static_assert (sizeof(NT_IORING_OP_REGISTER_FILES) == 0x0018, "");

typedef enum _NT_IORING_REG_BUFFERS_REQ_FLAGS
{
    NT_IORING_REG_BUFFERS_REQ_FLAG_NONE = 0,
} NT_IORING_REG_BUFFERS_REQ_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(NT_IORING_REG_BUFFERS_REQ_FLAGS);

typedef enum _NT_IORING_REG_BUFFERS_ADV_FLAGS
{
    NT_IORING_REG_BUFFERS_ADV_FLAG_NONE = 0,
} NT_IORING_REG_BUFFERS_ADV_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(NT_IORING_REG_BUFFERS_ADV_FLAGS);

typedef struct _NT_IORING_REG_BUFFERS_FLAGS
{
    /* 0x0000 */ NT_IORING_REG_BUFFERS_REQ_FLAGS Required;
    /* 0x0004 */ NT_IORING_REG_BUFFERS_ADV_FLAGS Advisory;
} NT_IORING_REG_BUFFERS_FLAGS, * PNT_IORING_REG_BUFFERS_FLAGS; /* size: 0x0008 */
static_assert (sizeof(NT_IORING_REG_BUFFERS_FLAGS) == 0x0008, "");

typedef struct _IORING_BUFFER_INFO {
    // According to https://windows-internals.com/one-year-to-i-o-ring-what-changed/ And <ntioring_x.h>
    // Address here should not be padded
    void* Address;
    uint32_t Length;
} IORING_BUFFER_INFO, PIORING_BUFFER_INFO;

typedef struct _NT_IORING_OP_REGISTER_BUFFERS
{
    /* 0x0000 */ NT_IORING_OP_FLAGS CommonOpFlags;
    /* 0x0004 */ NT_IORING_REG_BUFFERS_FLAGS Flags;
    /* 0x000c */ uint32_t Count;
    union {
      /* 0x0010 */ const IORING_BUFFER_INFO* Buffers;
      /* 0x0010 */ uint64_t PadX86_Buffers;
    }; /* size: 0x0008 */
} NT_IORING_OP_REGISTER_BUFFERS, * PNT_IORING_OP_REGISTER_BUFFERS; /* size: 0x0018 */
static_assert (sizeof(NT_IORING_OP_REGISTER_BUFFERS) == 0x0018, "");

typedef struct _NT_IORING_OP_CANCEL
{
    /* 0x0000 */ NT_IORING_OP_FLAGS CommonOpFlags;
    /* 0x0008 */ NT_IORING_HANDLEREF File;
    /* 0x0010 */ uint64_t CancelId;
} NT_IORING_OP_CANCEL, * PNT_IORING_OP_CANCEL; /* size: 0x0018 */
static_assert (sizeof(NT_IORING_OP_CANCEL) == 0x0018, "");

typedef struct _NT_IORING_OP_WRITE
{
    /* 0x0000 */ NT_IORING_OP_FLAGS CommonOpFlags;
    /* 0x0004 */ FILE_WRITE_FLAGS Flags;
    /* 0x0008 */ NT_IORING_HANDLEREF File;
    /* 0x0010 */ NT_IORING_BUFFERREF Buffer;
    /* 0x0018 */ uint64_t Offset;
    /* 0x0020 */ uint32_t Length;
    /* 0x0024 */ uint32_t Key;
} NT_IORING_OP_WRITE, * PNT_IORING_OP_WRITE; /* size: 0x0028 */
static_assert (sizeof(NT_IORING_OP_WRITE) == 0x0028, "");

typedef struct _NT_IORING_OP_FLUSH
{
    /* 0x0000 */ NT_IORING_OP_FLAGS CommonOpFlags;
    /* 0x0004 */ FILE_FLUSH_MODE FlushMode;
    /* 0x0008 */ NT_IORING_HANDLEREF File;
} NT_IORING_OP_FLUSH, * PNT_IORING_OP_FLUSH; /* size: 0x0010 */
static_assert (sizeof(NT_IORING_OP_FLUSH) == 0x0010, "");

typedef struct _NT_IORING_OP_RESERVED
{
    /* 0x0000 */ uint64_t Argument1;
    /* 0x0008 */ uint64_t Argument2;
    /* 0x0010 */ uint64_t Argument3;
    /* 0x0018 */ uint64_t Argument4;
    /* 0x0020 */ uint64_t Argument5;
    /* 0x0028 */ uint64_t Argument6;
} NT_IORING_OP_RESERVED, * PNT_IORING_OP_RESERVED; /* size: 0x0030 */
static_assert (sizeof(NT_IORING_OP_RESERVED) == 0x0030, "");

typedef struct _NT_IORING_SQE
{
    /* 0x0000 */ IORING_OP_CODE OpCode;
    /* 0x0004 */ NT_IORING_SQE_FLAGS Flags;
    /* 0x0008 */ uint64_t UserData;
    union
    {
        /* 0x0010 */ NT_IORING_OP_READ Read;
        /* 0x0010 */ NT_IORING_OP_REGISTER_FILES RegisterFiles;
        /* 0x0010 */ NT_IORING_OP_REGISTER_BUFFERS RegisterBuffers;
        /* 0x0010 */ NT_IORING_OP_CANCEL Cancel;
        /* 0x0010 */ NT_IORING_OP_WRITE Write;
        /* 0x0010 */ NT_IORING_OP_FLUSH Flush;
        /* 0x0010 */ NT_IORING_OP_RESERVED ReservedMaxSizePadding;
    }; /* size: 0x0030 */
} NT_IORING_SQE, * PNT_IORING_SQE; /* size: 0x0040 */
static_assert (sizeof(NT_IORING_SQE) == 0x0040, "");

typedef enum _IORING_VERSION
{
    IORING_VERSION_INVALID = 0,
    IORING_VERSION_1 = 1,
    IORING_VERSION_2 = 2,
    IORING_VERSION_3 = 300,
} IORING_VERSION;

typedef enum _NT_IORING_CREATE_REQUIRED_FLAGS
{
    NT_IORING_CREATE_REQUIRED_FLAG_NONE = 0,
} NT_IORING_CREATE_REQUIRED_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(NT_IORING_CREATE_REQUIRED_FLAGS);

typedef enum _NT_IORING_CREATE_ADVISORY_FLAGS
{
    NT_IORING_CREATE_ADVISORY_FLAG_NONE = 0,
} NT_IORING_CREATE_ADVISORY_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(NT_IORING_CREATE_ADVISORY_FLAGS);

typedef struct _NT_IORING_CREATE_FLAGS
{
    /* 0x0000 */ NT_IORING_CREATE_REQUIRED_FLAGS Required;
    /* 0x0004 */ NT_IORING_CREATE_ADVISORY_FLAGS Advisory;
} NT_IORING_CREATE_FLAGS; /* size: 0x0008 */
static_assert (sizeof(NT_IORING_CREATE_FLAGS) == 0x0008, "");

typedef enum _NT_IORING_SQ_FLAGS
{
    NT_IORING_SQ_FLAG_NONE = 0,
} NT_IORING_SQ_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(NT_IORING_SQ_FLAGS);

typedef struct _NT_IORING_SUBMISSION_QUEUE
{
    /* 0x0000 */ uint32_t Head;
    /* 0x0004 */ uint32_t Tail;
    /* 0x0008 */ NT_IORING_SQ_FLAGS Flags;
    /* 0x0010 */ NT_IORING_SQE Entries[];
} NT_IORING_SUBMISSION_QUEUE, * PNT_IORING_SUBMISSION_QUEUE; /* size: 0x0010 */
static_assert (sizeof (NT_IORING_SUBMISSION_QUEUE) == 0x0010, "");

typedef struct _NT_IORING_CQE
{
    /* 0x0000 */ uint64_t UserData; /* size: 0x0008 */
    union
    {
        /* 0x0008 */ HRESULT ResultCode;
        /* 0x0008 */ void * Pointer;
        /* 0x0008 */ uint64_t PadX86_Pointer;
    }; /* size: 0x0008 */
    /* 0x0010 */ uint64_t Information;
} NT_IORING_CQE, * PNT_IORING_CQE; /* size: 0x0018 */
static_assert (sizeof(NT_IORING_CQE) == 0x0018, "");

typedef struct _NT_IORING_COMPLETION_QUEUE
{
    /* 0x0000 */ uint32_t Head;
    /* 0x0004 */ uint32_t Tail;
    /* 0x0008 */ NT_IORING_CQE Entries[];
} NT_IORING_COMPLETION_QUEUE, * PNT_IORING_COMPLETION_QUEUE; /* size: 0x0008 */
static_assert (sizeof (NT_IORING_COMPLETION_QUEUE) == 0x0008, "");

typedef struct _NT_IORING_INFO
{
    /* 0x0000 */ IORING_VERSION IoRingVersion;
    /* 0x0004 */ NT_IORING_CREATE_FLAGS Flags;
    /* 0x000c */ uint32_t SubmissionQueueSize;
    /* 0x0010 */ uint32_t SubmissionQueueRingMask;
    /* 0x0014 */ uint32_t CompletionQueueSize;
    /* 0x0018 */ uint32_t CompletionQueueRingMask;
    union {
        /* 0x0020 */ NT_IORING_SUBMISSION_QUEUE* SubmissionQueue;
        /* 0x0020 */ uint64_t PadX86_SubmissionQueue;
    }; /* size: 0x0008 */
    union {
        /* 0x0028 */ NT_IORING_COMPLETION_QUEUE* CompletionQueue;
        /* 0x0028 */ uint64_t PadX86_CompletionQueue;
    }; /* size: 0x0008 */
} NT_IORING_INFO, * PNT_IORING_INFO; /* size: 0x0030 */
static_assert (sizeof (NT_IORING_INFO) == 0x0030, "");

typedef struct _NT_IORING_STRUCTV1 {
    IORING_VERSION IoRingVersion;
    uint32_t SubmissionQueueSize;
    uint32_t CompletionQueueSize;
    NT_IORING_CREATE_FLAGS Flags;
} NT_IORING_STRUCTV1, * PNT_IORING_STRUCTV1;

// Code used from <ntioring_x.h>
typedef enum _IORING_FEATURE_FLAGS {
  IORING_FEATURE_FLAGS_NONE = 0,
  IORING_FEATURE_UM_EMULATION = 1,
  IORING_FEATURE_SET_COMPLETION_EVENT = 2,
} IORING_FEATURE_FLAGS;
DEFINE_ENUM_FLAG_OPERATORS(IORING_FEATURE_FLAGS)

typedef struct _NT_IORING_CAPABILITIES {
    IORING_VERSION IoRingVersion;
    IORING_OP_CODE MaxOpCode;
    IORING_FEATURE_FLAGS FlagsSupported;
    uint32_t SubmissionQueueSize;
    uint32_t CompletionQueueSize;
} NT_IORING_CAPABILITIES, * PNT_IORING_CAPABILITIES;

typedef enum _NT_IORING_INFO_CLASS {
    IoRingInvalid = 0,
    IoRingRegisterUserCompletionEventClass = 1,
} NT_IORING_INFO_CLASS;
static_assert (sizeof (NT_IORING_INFO_CLASS) == 4, "");

//
// Function definitions
//
#ifndef __kernel_entry
#   define __kernel_entry
#endif
__kernel_entry extern NTSTATUS NTAPI
NtSubmitIoRing(
    _In_ HANDLE Handle,
    _In_ NT_IORING_CREATE_REQUIRED_FLAGS Flags,
    _In_ uint32_t WaitOperations,
    _In_opt_ uint64_t * Timeout
);

__kernel_entry extern NTSTATUS NTAPI
NtCreateIoRing(
    _Out_ PHANDLE pIoRingHandle,
    _In_ uint32_t CreateParametersSize,
    _In_ PNT_IORING_STRUCTV1 CreateParameters,
    _In_ uint32_t OutputParametersSize,
    _In_ PNT_IORING_INFO pRingInfo
);

__kernel_entry extern NTSTATUS NTAPI
NtQueryIoRingCapabilities(
    _In_ size_t CapabilitiesLength,
    _Out_ PNT_IORING_CAPABILITIES Capabilities
);

__kernel_entry extern NTSTATUS NTAPI
NtSetInformationIoRing(
    _In_ HANDLE Handle,
    _In_ NT_IORING_INFO_CLASS InformationClass,
    _In_ uint32_t InformationLength,
    _In_ void * IoRingInformation
);

#ifdef __cplusplus
}
#endif

#ifdef _MSC_VER
#   pragma comment(lib, "ntdll")
#endif
