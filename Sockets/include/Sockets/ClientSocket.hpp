#pragma once

#include <functional>
#include <memory>
#include <stdint.h>
#include <string>

namespace Sockets {

    class ClientSocket {
    public:
        // Types
        using OnReceived = std::function< void(const std::string&) >;
        using OnClosed = std::function< void() >;

        // Constructor
        ClientSocket();

        // Methods
        bool Bind(uint16_t port = 0);
        bool Connect(
            uint32_t address,
            uint16_t port,
            OnReceived onReceived,
            OnClosed onClosed
        );
        void Close();
        void SendMessage(const std::string& message);

    private:
        // Properties
        struct Impl;
        std::shared_ptr< Impl > impl_;
    };

}
