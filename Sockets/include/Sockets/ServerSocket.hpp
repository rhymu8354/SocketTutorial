#pragma once

#include <functional>
#include <memory>
#include <stdint.h>
#include <string>

namespace Sockets {

    class ServerSocket {
    public:
        // Types
        using OnReceived = std::function< void(const std::string&) >;
        using OnClosed = std::function< void() >;
        class Client {
        public:
            virtual void Close() = 0;
            virtual void SendMessage(const std::string& message) = 0;
            virtual void Start(
                OnReceived onReceived,
                OnClosed onClosed
            ) = 0;
        };
        using OnAcceptClient = std::function<
            void(
                std::shared_ptr< Client >&& client
            )
        >;

        // Constructor
        ServerSocket();

        // Methods
        bool Bind(uint16_t port = 0);
        bool Listen(OnAcceptClient onAcceptClient);

    private:
        // Properties
        struct Impl;
        std::shared_ptr< Impl > impl_;
    };

}
