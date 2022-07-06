#include <stdnoreturn.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "libwinring.h"
#include <Windows.h>

#define BS	(32*1024)

noreturn
static void panic() {
    char buf[256] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, (sizeof(buf) / sizeof(*buf)), NULL);
    printf("ERROR: %s", buf);
    abort();
}

DWORD Win32FromHResult(HRESULT hr) {
    if ((hr & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0)) {
        return HRESULT_CODE(hr);
    }

    if (hr == S_OK) {
        return ERROR_SUCCESS;
    }

    // Not a Win32 HRESULT so return a generic error code.
    return ERROR_CAN_NOT_COMPLETE;
}

inline void panic_on_error(HRESULT hRes) {
    if (SUCCEEDED(hRes)) return;

    SetLastError(Win32FromHResult(hRes));
    panic();
}

static void clear_cqes(win_ring* ring, const char str[]) {
    panic_on_error(win_ring_submit_and_wait(ring, -1));

    unsigned head;
    win_ring_cqe* cqe;
    win_ring_for_each_cqe(ring, head, cqe) {
        panic_on_error(cqe->ResultCode);
        printf("%u %d %s\n", (unsigned)cqe->Information, (int)cqe->UserData, str);
    }
    win_ring_cq_clear(ring);
}

static void register_files_bufs(win_ring* ring, HANDLE infd, HANDLE outfd) {
    HANDLE fds[] = { infd, outfd };

    {
        win_ring_sqe* sqe = win_ring_get_sqe(ring);
        assert(sqe);

        win_ring_prep_register_files(
            sqe,
            fds,
            2,
            (NT_IORING_REG_FILES_FLAGS) {0},
            NT_IORING_OP_FLAG_NONE);
    }

    IORING_BUFFER_INFO buf_info;

    {
        static char static_buffers[BS];
        win_ring_sqe* sqe = win_ring_get_sqe(ring);
        assert(sqe);

        buf_info.Address = static_buffers;
        buf_info.Length = BS;

        win_ring_prep_register_buffers(
            sqe,
            &buf_info,
            1,
            (NT_IORING_REG_BUFFERS_FLAGS) {0},
            NT_IORING_OP_FLAG_NONE);
    }

    clear_cqes(ring, "register");
}

static void queue_read_write_pair(win_ring* ring, uint64_t offset, uint32_t buf_size) {
    {
        win_ring_sqe* sqe = win_ring_get_sqe(ring);
        assert(sqe);
        win_ring_prep_read(
            sqe,
            (NT_IORING_HANDLEREF) { .HandleIndex = 0 },
            (NT_IORING_BUFFERREF) { .FixedBuffer = { .BufferIndex = 0, .Offset = 0 } },
            buf_size,
            offset ,
            NT_IORING_OP_FLAG_REGISTERED_FILE | NT_IORING_OP_FLAG_REGISTERED_BUFFER);
        win_ring_sqe_set_flags(sqe, NT_IORING_SQE_FLAG_DRAIN_PRECEDING_OPS);
    }
    {
        win_ring_sqe* sqe = win_ring_get_sqe(ring);
        assert(sqe);
        win_ring_prep_write(
            sqe,
            (NT_IORING_HANDLEREF) { .HandleIndex = 1 },
            (NT_IORING_BUFFERREF) { .FixedBuffer = { .BufferIndex = 0, .Offset = 0 } },
            buf_size,
            offset,
            FILE_WRITE_FLAGS_NONE,
            NT_IORING_OP_FLAG_REGISTERED_FILE | NT_IORING_OP_FLAG_REGISTERED_BUFFER);
        win_ring_sqe_set_flags(sqe, NT_IORING_SQE_FLAG_DRAIN_PRECEDING_OPS);
    }
}

static void copy_file(win_ring* ring, HANDLE infd, HANDLE outfd, uint64_t size) {
    uint64_t offset;
    for (offset = 0; offset + BS <= size; offset += BS) {
        queue_read_write_pair(ring, offset, BS);
        if (win_ring_sq_space_left(ring) < 2) {
            clear_cqes(ring, "read_write");
        }
    };
    if (offset != size) {
        queue_read_write_pair(ring, offset, (uint32_t)(size - offset));
        clear_cqes(ring, "read_write_last");
    }
}

int main(int argc, const char* argv[]) {
#ifndef _DEBUG
    if (argc < 3) {
        printf("%s: infile outfile\n", argv[0]);
        return 1;
    }
    const char* in_path = argv[1];
    const char* out_path = argv[2];
#else
    const char* in_path = __FILE__;
    const char* out_path = "test.txt";
#endif

    HANDLE infd = CreateFileA(
        in_path,
        GENERIC_READ,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (infd == INVALID_HANDLE_VALUE) panic();

    LARGE_INTEGER size;
    if (!GetFileSizeEx(infd, &size)) panic();

    HANDLE outfd = CreateFileA(
        out_path,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
    if (outfd == INVALID_HANDLE_VALUE) panic();

    win_ring ring;
    panic_on_error(win_ring_queue_init(32, &ring));

    register_files_bufs(&ring, infd, outfd);

    copy_file(&ring, infd, outfd, (uint64_t)size.QuadPart);

    CloseHandle(outfd);
    CloseHandle(infd);

    win_ring_queue_exit(&ring);
}
