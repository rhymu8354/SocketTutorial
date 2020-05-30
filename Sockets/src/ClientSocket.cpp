#include "Abstractions.hpp"
#include "Connection.hpp"

#include <Sockets/ClientSocket.hpp>
#include <string.h>

namespace Sockets {

    struct ClientSocket::Impl {
        Connection connection;
        SOCKET socket = INVALID_SOCKET;
        UsesSockets usesSockets;
    };

    ClientSocket::ClientSocket()
        : impl_(new Impl())
    {
    }

    bool ClientSocket::Bind(uint16_t port) {
        // Create the socket.
        impl_->socket = socket(AF_INET, SOCK_STREAM, 0);
        if (IS_INVALID_SOCKET(impl_->socket)) {
            fprintf(stderr, "error: unable to create socket\n");
            return false;
        }

        // Bind the socket.
        struct sockaddr_in socketAddress;
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        socketAddress.sin_port = htons(port);
        if (bind(impl_->socket, (struct sockaddr*)&socketAddress, sizeof(socketAddress))) {
            fprintf(stderr, "error: unable to bind socket\n");
            return false;
        }
        return true;
    }

    bool ClientSocket::Connect(
        uint32_t address,
        uint16_t port,
        OnReceived onReceived,
        OnClosed onClosed
    ) {
        struct sockaddr_in socketAddress;
        (void)memset(&socketAddress, 0, sizeof(socketAddress));
        socketAddress.sin_family = AF_INET;
        socketAddress.IPV4_ADDRESS_IN_SOCKADDR = htonl(address);
        socketAddress.sin_port = htons(port);
        if (
            connect(
                impl_->socket,
                (const sockaddr*)&socketAddress,
                (SOCKET_DATAGRAM_LENGTH_TYPE)sizeof(socketAddress)
            )
        ) {
            fprintf(stderr, "error: unable to connect\n");
            return false;
        }
        impl_->connection.Start(
            impl_->socket,
            onReceived,
            onClosed
        );
        return true;
    }

    void ClientSocket::Close() {
        impl_->connection.Close();
    }

    void ClientSocket::SendMessage(const std::string& message) {
        impl_->connection.SendMessage(message);
    }

}
