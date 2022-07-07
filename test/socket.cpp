#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#include "common.hpp"

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

struct conn_info {
    uint32_t handle;
    IORING_OP_CODE type;
};
static_assert(sizeof conn_info == 8);

#define MAX_CONNECTIONS 512
#define MAX_MESSAGE_LEN 512

typedef char buf_type[MAX_CONNECTIONS][MAX_MESSAGE_LEN];
buf_type bufs;

void add_read(win_ring_cpp* ring, HANDLE sock, size_t size) {
    const uint32_t fd = (uint32_t)(uintptr_t)sock;
    const conn_info info = {
        .handle = fd,
        .type = IORING_OP_READ,
    };
    ring
        ->get_sqe()
        ->prep_read(sock, bufs[fd], size, 0, NT_IORING_OP_FLAG_NONE)
        ->set_data64((uint64_t&)info);
}

void add_write(win_ring_cpp* ring, HANDLE sock, size_t size) {
    const uint32_t fd = (uint32_t)(uintptr_t)sock;
    const conn_info info = {
        .handle = fd,
        .type = IORING_OP_WRITE,
    };
    ring
        ->get_sqe()
        ->prep_write(sock, bufs[fd], size, 0, FILE_WRITE_FLAGS_NONE, NT_IORING_OP_FLAG_NONE)
        ->set_data64((uint64_t&)info);
}

int main(void)
{
    //----------------------
    // Initialize Winsock.
    WSADATA wsaData;
    if (int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        iResult != NO_ERROR) {
        wprintf(L"WSAStartup failed with error: %ld\n", iResult);
        return 1;
    }
    //----------------------
    // Create a SOCKET for listening for
    // incoming connection requests.
    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound.
    sockaddr_in service;
    service.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &service.sin_addr.s_addr);
    service.sin_port = htons(27015);

    if (bind(serverSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        wprintf(L"bind failed with error: %ld\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    //----------------------
    // Listen for incoming connection requests.
    // on the created socket
    if (listen(serverSocket, 1) == SOCKET_ERROR) {
        wprintf(L"listen failed with error: %ld\n", WSAGetLastError());
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    //----------------------
    // Create a SOCKET for accepting incoming requests.
    wprintf(L"Waiting for client to connect...\n");

    win_ring_cpp ring(32);

    {
        //----------------------
        // Accept the connection.
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        assert((uint64_t)clientSocket == (uint32_t)clientSocket);
        if (clientSocket == INVALID_SOCKET) {
            wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
            closesocket(serverSocket);
            WSACleanup();
            return 1;
        }
        wprintf(L"Client connected.\n");
        add_read(&ring, (HANDLE)clientSocket, MAX_MESSAGE_LEN);
    }

    while (true) {
        ring.submit_and_wait(1);
        for (auto* cqe : ring) {
            throw_on_error(cqe->ResultCode);
            auto data = cqe->get_data64();
            auto info = (conn_info&)data;
            auto clientHandle = (HANDLE)(uintptr_t)info.handle;

            switch (info.type) {
            case IORING_OP_READ:
                if (cqe->Information > 0) {
                    add_write(&ring, clientHandle, cqe->Information);
                }
                else {
                    goto exit;
                    closesocket((SOCKET)clientHandle);
                }
                break;
            case IORING_OP_WRITE:
                add_read(&ring, clientHandle, MAX_MESSAGE_LEN);
                break;
            }
        }
        ring.cq_clear();
    }

exit:
    // No longer need server socket
    closesocket(serverSocket);

    WSACleanup();
}