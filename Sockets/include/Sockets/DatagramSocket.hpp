#pragma once

#include <functional>
#include <memory>
#include <stdint.h>
#include <string>

namespace Sockets {

    class DatagramSocket {
    public:
        // Types
        using OnReceived = std::function< void(const std::string&) >;
        using OnSent = std::function< void() >;

        // Constructor
        DatagramSocket();

        // Methods
        bool Bind(uint16_t port = 0);
        void SendMessage(
            const std::string& message,
            uint32_t address,
            uint16_t port,
            OnSent onSent = nullptr
        );
        void Start(OnReceived onReceived);

    private:
        // Properties
        struct Impl;
        std::shared_ptr< Impl > impl_;
    };

}
