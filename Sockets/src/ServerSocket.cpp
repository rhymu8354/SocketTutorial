#include "Abstractions.hpp"
#include "Connection.hpp"

#include <Sockets/ServerSocket.hpp>
#include <string.h>

namespace Sockets {

    struct ClientImpl
        : public ServerSocket::Client
    {
        // Properties

        Connection connection;
        SOCKET socket = INVALID_SOCKET;

        // ClientConnection

        virtual void Close() override {
            connection.Close();
        }

        virtual void SendMessage(const std::string& message) override {
            connection.SendMessage(message);
        }

        virtual void Start(
            ServerSocket::OnReceived onReceived,
            ServerSocket::OnClosed onClosed
        ) override {
            connection.Start(socket, onReceived, onClosed);
        }
    };

    struct ServerSocket::Impl {
        // Properties

        bool error = false;
        SOCKET socket;
        SocketEventLoop socketEventLoop;
        UsesSockets usesSockets;

        // Methods

        bool OnSocketReady(OnAcceptClient onAcceptClient) {
            if (error) {
                return true;
            }
            const SOCKET clientSocket = accept(socket, NULL, NULL);
            if (IS_INVALID_SOCKET(clientSocket)) {
                if (!LAST_SOCKET_OPERATION_WOULD_BLOCK) {
                    error = true;
                    fprintf(stderr, "error: unable to read socket\n");
                }
                return true;
            } else {
                auto client = std::make_shared< ClientImpl >();
                client->socket = clientSocket;
                onAcceptClient(std::move(client));
            }
            return true;
        }
    };

    ServerSocket::ServerSocket()
        : impl_(new Impl())
    {
    }

    bool ServerSocket::Bind(uint16_t port) {
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

    bool ServerSocket::Listen(OnAcceptClient onAcceptClient) {
        if (listen(impl_->socket, SOMAXCONN)) {
            fprintf(stderr, "error: unable to listen on socket\n");
            return false;
        }
        std::weak_ptr< Impl > implWeak(impl_);
        impl_->socketEventLoop.Start(
            impl_->socket,

            // isReadyToSend
            []{ return false; },

            // onSocketReady
            [
                implWeak,
                onAcceptClient
            ]{
                const auto impl = implWeak.lock();
                if (!impl) {
                    return true;
                }
                return impl->OnSocketReady(onAcceptClient);
            }
        );
        return true;
    }

}
