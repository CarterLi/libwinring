#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <stdio.h>

#include "libwinring.hpp"

struct conn_info {
    uint32_t handle;
    enum { READ, WRITE } type;
};
static_assert(sizeof (conn_info) == 8);

#define MAX_CONNECTIONS 512
#define MAX_MESSAGE_LEN 512

typedef char buf_type[MAX_CONNECTIONS][MAX_MESSAGE_LEN];
buf_type bufs;

[[noreturn]]
static void panic(DWORD dwMessageId = 0) {
    if (dwMessageId == 0) dwMessageId = GetLastError();
    throw_error(dwMessageId);
}

void add_read(win_ring_cpp* ring, HANDLE sock, size_t size) {
    const conn_info info = {
        .handle = (uint32_t)(uintptr_t)sock,
        .type = conn_info::READ,
    };
    ring
        ->get_sqe()
        ->prep_read(sock, bufs[info.handle], size, 0, NT_IORING_OP_FLAG_NONE)
        ->set_data64((uint64_t&)info);
}

void add_write(win_ring_cpp* ring, HANDLE sock, size_t size) {
    const conn_info info = {
        .handle = (uint32_t)(uintptr_t)sock,
        .type = conn_info::WRITE,
    };
    ring
        ->get_sqe()
        ->prep_write(sock, bufs[info.handle], size, 0, FILE_WRITE_FLAGS_NONE, NT_IORING_OP_FLAG_NONE)
        ->set_data64((uint64_t&)info);
}

const DWORD addr_len = sizeof(sockaddr_in) + 16;
struct {
    char accept_buf[addr_len * 2];
    WSAOVERLAPPED overlapped = {
        .hEvent = CreateEventA(nullptr, false, false, nullptr)
    };
    SOCKET clientSocket;
} accept_bundle;


void add_accept(SOCKET serverSocket) {
    accept_bundle.clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    DWORD bytesReceived;

    if (!AcceptEx(
        serverSocket,
        accept_bundle.clientSocket,
        accept_bundle.accept_buf,
        0,
        addr_len,
        addr_len,
        &bytesReceived,
        &accept_bundle.overlapped
    )) {
        int err = WSAGetLastError();
        if (err != ERROR_IO_PENDING) {
            closesocket(serverSocket);
            panic(err);
        }
    }
}

int main(void) {
    WSADATA wsaData;
    if (int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        iResult != NO_ERROR) {
        panic(iResult);
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) panic(WSAGetLastError());

    sockaddr_in service = {
        .sin_family = AF_INET,
        .sin_port = htons(27015),
    };
    inet_pton(AF_INET, "0.0.0.0", &service.sin_addr.s_addr);

    if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        closesocket(serverSocket);
        panic(WSAGetLastError());
    }

    if (listen(serverSocket, 32) == SOCKET_ERROR) {
        closesocket(serverSocket);
        panic(WSAGetLastError());
    }

    puts("Waiting for client to connect...");

    add_accept(serverSocket);

    win_ring_cpp ring(32);
    const HANDLE ringEvent = CreateEventA(nullptr, false, false, nullptr);
    if (!ringEvent) panic();
    ring.register_event(ringEvent);

    while (true) {
        HANDLE handles[] = { accept_bundle.overlapped.hEvent, ringEvent };
        auto index = WaitForMultipleObjectsEx(2, handles, false, INFINITE, true) - WAIT_OBJECT_0;
        if (index == 0) {
            auto clientHandle = (HANDLE)accept_bundle.clientSocket;
            printf("Client %p accepted\n", clientHandle);
            add_read(&ring, clientHandle, MAX_MESSAGE_LEN);
            add_accept(serverSocket);
        }
        else {
            for (auto* cqe : ring) {
                throw_on_error(cqe->ResultCode);
                auto data = cqe->get_data64();
                auto info = (conn_info&)data;
                auto clientHandle = (HANDLE)(uintptr_t)info.handle;

                switch (info.type) {
                case conn_info::READ:
                    if (cqe->Information > 0) {
                        add_write(&ring, clientHandle, cqe->Information);
                    }
                    else {
                        closesocket((SOCKET)clientHandle);
                        printf("Client %p disconnected\n", clientHandle);
                    }
                    break;
                case conn_info::WRITE:
                    add_read(&ring, clientHandle, MAX_MESSAGE_LEN);
                    break;
                }
            }
            ring.cq_clear();
        }
        ring.submit();
    }

    closesocket(serverSocket);
    WSACleanup();
}
