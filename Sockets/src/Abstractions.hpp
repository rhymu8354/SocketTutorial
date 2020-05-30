#pragma once

#ifdef _WIN32
/**
 * WinSock2.h should always be included first because if Windows.h is
 * included before it, WinSock.h gets included which conflicts
 * with WinSock2.h.
 *
 * Windows.h should always be included next because other Windows header
 * files, such as KnownFolders.h, don't always define things properly if
 * you don't include Windows.h beforehand.
 */
#include <WinSock2.h>
#include <Windows.h>
#include <WS2tcpip.h>
#include <IPHlpApi.h>
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "IPHlpApi")
#undef ERROR
#undef SendMessage
#undef min
#undef max
#define IPV4_ADDRESS_IN_SOCKADDR sin_addr.S_un.S_addr
#define SOCKADDR_LENGTH_TYPE int
#define IS_SOCKET_ERROR(x) (x == SOCKET_ERROR)
#define IS_INVALID_SOCKET(s) (s == INVALID_SOCKET)
#define MSG_NOSIGNAL 0
#define LAST_SOCKET_OPERATION_WOULD_BLOCK (WSAGetLastError() == WSAEWOULDBLOCK)
#define LAST_SOCKET_OPERATION_WAS_RESET (WSAGetLastError() == WSAECONNRESET)
#define SOCKET_DATAGRAM_LENGTH_TYPE int

#else /* POSIX */

#include <errno.h>
#include <fcntl.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <unistd.h>
#define IPV4_ADDRESS_IN_SOCKADDR sin_addr.s_addr
#define SOCKADDR_LENGTH_TYPE socklen_t
#define IS_SOCKET_ERROR(x) (x < 0)
#define INVALID_SOCKET -1
#define IS_INVALID_SOCKET(s) (s < 0)
#ifdef __APPLE__
#define MSG_NOSIGNAL 0
#endif /* __APPLE__ */
#define LAST_SOCKET_OPERATION_WOULD_BLOCK (errno == EWOULDBLOCK)
#define LAST_SOCKET_OPERATION_WAS_RESET (errno == ECONNRESET)
#define SOCKET int
#define closesocket close
#define SD_SEND SHUT_WR
#define SOCKET_DATAGRAM_LENGTH_TYPE size_t

#endif /* _WIN32 or POSIX */

#include <functional>
#include <memory>

namespace Sockets {

    class UsesSockets {
    public:
        UsesSockets();

    private:
        struct Impl;
        std::shared_ptr< Impl > impl_;
    };

    class SocketEventLoop {
    public:
        // Types
        using IsReadyToSend = std::function< bool() >;
        using OnSocketReady = std::function< bool() >;

        // Lifecycle
        ~SocketEventLoop() noexcept;
        SocketEventLoop(const SocketEventLoop&);
        SocketEventLoop(SocketEventLoop&&) noexcept;
        SocketEventLoop& operator=(const SocketEventLoop&);
        SocketEventLoop& operator=(SocketEventLoop&&) noexcept;

        // Constructor
        SocketEventLoop();

        // Methods
        void Start(
            SOCKET socket,
            IsReadyToSend isReadyToSend,
            OnSocketReady onSocketReady
        );
        void Stop();
        void UserEvent();

    private:
        struct Impl;
        std::shared_ptr< Impl > impl_;
    };

}
