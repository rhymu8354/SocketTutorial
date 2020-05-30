#pragma once

#include "Abstractions.hpp"

#include <functional>
#include <memory>
#include <stdint.h>
#include <string>

namespace Sockets {

    class Connection {
    public:
        // Types
        using OnReceived = std::function< void(const std::string&) >;
        using OnClosed = std::function< void() >;

        // Constructor
        Connection();

        // Methods
        void Close();
        void SendMessage(const std::string& message);
        void Start(
            SOCKET socket,
            OnReceived onReceived,
            OnClosed onClosed
        );

    private:
        // Properties
        struct Impl;
        std::shared_ptr< Impl > impl_;
    };

}
