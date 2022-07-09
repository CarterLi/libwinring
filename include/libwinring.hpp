#pragma once

#include <stdexcept>

#include "libwinring.h"

DWORD Win32FromHResult(HRESULT hr) noexcept {
    if ((hr & 0xFFFF0000) == MAKE_HRESULT(SEVERITY_ERROR, FACILITY_WIN32, 0)) {
        return HRESULT_CODE(hr);
    }

    if (hr == S_OK) {
        return ERROR_SUCCESS;
    }

    // Not a Win32 HRESULT so return a generic error code.
    return ERROR_CAN_NOT_COMPLETE;
}

[[noreturn]]
static void throw_error(DWORD err_code) {
    char buf[256] = "";
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr, err_code, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buf, (sizeof(buf) / sizeof(*buf)), nullptr);
    throw std::runtime_error(buf);
}

inline void throw_on_error(HRESULT hr) noexcept(false) {
    if (SUCCEEDED(hr)) [[likely]] return;
    throw_error(Win32FromHResult(hr));
}

struct win_ring_sqe_cpp : win_ring_sqe {
    auto prep_nop() noexcept {
        win_ring_prep_nop(this);
        return this;
    }

    auto prep_read(
        _In_ NT_IORING_HANDLEREF file,
        NT_IORING_BUFFERREF buffer,
        _In_ uint32_t sizeToRead,
        _In_ uint64_t fileOffset,
        _In_ NT_IORING_OP_FLAGS commonOpFlags
    ) noexcept {
        win_ring_prep_read(this, file, buffer, sizeToRead, fileOffset, commonOpFlags);
        return this;
    }

    auto prep_register_files(
        _In_reads_(count) HANDLE const handles[],
        _In_ unsigned count,
        _In_ NT_IORING_REG_FILES_FLAGS flags,
        _In_ NT_IORING_OP_FLAGS commonOpFlags
    ) noexcept {
        win_ring_prep_register_files(this, handles, count, flags, commonOpFlags);
        return this;
    }

    auto prep_register_buffers(
        _In_reads_(count) IORING_BUFFER_INFO const buffers[],
        _In_ unsigned count,
        _In_ NT_IORING_REG_BUFFERS_FLAGS flags,
        _In_ NT_IORING_OP_FLAGS commonOpFlags
    ) noexcept {
        win_ring_prep_register_buffers(this, buffers, count, flags, commonOpFlags);
        return this;
    }

    auto prep_cancel(
        // file handle to be canceled
        _In_ NT_IORING_HANDLEREF file,
        // user data of the operation to be canceled
        // or 0 to cancel all operations associated with the file handle
        _In_opt_ uint64_t cancelId,
        _In_ NT_IORING_OP_FLAGS commonOpFlags
    ) noexcept {
        win_ring_prep_cancel(this, file, cancelId, commonOpFlags);
        return this;
    }

    auto prep_write(
        _In_ NT_IORING_HANDLEREF file,
        _In_ NT_IORING_BUFFERREF buffer,
        _In_ uint32_t sizeToWrite,
        _In_ uint64_t fileOffset,
        _In_ FILE_WRITE_FLAGS flags,
        _In_ NT_IORING_OP_FLAGS commonOpFlags
    ) noexcept {
        win_ring_prep_write(this, file, buffer, sizeToWrite, fileOffset, flags, commonOpFlags);
        return this;
    }

    auto prep_flush(
        _In_ NT_IORING_HANDLEREF file,
        _In_ FILE_FLUSH_MODE flushMode,
        _In_ NT_IORING_OP_FLAGS commonOpFlags
    ) noexcept {
        win_ring_prep_flush(this, file, flushMode, commonOpFlags);
        return this;
    }

    auto set_flags(_In_ NT_IORING_SQE_FLAGS flags) noexcept {
        win_ring_sqe_set_flags(this, flags);
        return this;
    }

    auto set_data(_In_ void* userData) noexcept {
        win_ring_sqe_set_data(this, userData);
        return this;
    }

    auto set_data64(_In_ uint64_t userData) noexcept {
        win_ring_sqe_set_data64(this, userData);
        return this;
    }
};

struct win_ring_cqe_cpp : win_ring_cqe {
    auto get_data() const noexcept {
        return win_ring_cqe_get_data(this);
    }

    auto get_data64() const noexcept {
        return win_ring_cqe_get_data64(this);
    }
};

struct win_ring_cpp : win_ring {
    explicit win_ring_cpp(uint32_t entries) {
        throw_on_error(win_ring_queue_init(entries, this));
    }

    ~win_ring_cpp() {
        throw_on_error(win_ring_queue_exit(this));
    }

    static win_ring_capabilities query_capabilities() {
        win_ring_capabilities capabilities;
        throw_on_error(win_ring_query_capabilities(&capabilities));
        return capabilities;
    }

    // SQ
    unsigned sq_ready() const noexcept {
        return win_ring_sq_ready(this);
    }

    unsigned sq_space_left() const noexcept {
        return win_ring_sq_space_left(this);
    }

    auto get_sqe() noexcept {
        return (win_ring_sqe_cpp*)win_ring_get_sqe(this);
    }

    // submit
    auto submit_and_wait_timeout(_In_ uint32_t numberOfEntries, _In_ uint64_t timeout) {
        throw_on_error(win_ring_submit_and_wait_timeout(this, numberOfEntries, timeout));
        return this;
    }

    auto submit_and_wait(_In_ uint32_t numberOfEntries) {
        throw_on_error(win_ring_submit_and_wait(this, numberOfEntries));
        return this;
    }

    auto submit() {
        throw_on_error(win_ring_submit(this));
        return this;
    }

    // CQ
    unsigned cq_ready() const noexcept {
        return win_ring_cq_ready(this);
    }

    unsigned cq_space_left() const noexcept {
        return win_ring_cq_space_left(this);
    }

    auto peek_cqe() const noexcept {
        return (win_ring_cqe_cpp*)win_ring_peek_cqe(this);
    }

    auto wait_cqe() noexcept {
        return (win_ring_cqe_cpp*)win_ring_wait_cqe(this);
    }

    auto cqe_seen(win_ring_cqe_cpp* cqe) noexcept {
        return win_ring_cqe_seen(this, cqe);
    }

    auto cq_clear() noexcept {
        return win_ring_cq_clear(this);
    }

    struct win_ring_cqe_iterator {
        win_ring_cqe_iterator& operator++() noexcept {
            ++head;
            return *this;
        }

        const auto* operator *() const noexcept {
            return (win_ring_cqe_cpp*)&ring->info.CompletionQueue->Entries[
                head & ring->info.CompletionQueueRingMask
            ];
        }

        bool operator !=(const win_ring_cqe_iterator& right) const noexcept {
            return head != right.head;
        }

        const win_ring_cpp* ring;
        uint32_t head;
    };

    win_ring_cqe_iterator begin() const noexcept {
        return { this, this->info.CompletionQueue->Head };
    }

    win_ring_cqe_iterator end() const noexcept {
        return { this, this->info.CompletionQueue->Tail };
    }

    // others
    auto register_event(_In_ HANDLE event) {
        throw_on_error(win_ring_register_event(this, event));
        return this;
    }
};