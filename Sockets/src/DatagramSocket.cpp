#include "Abstractions.hpp"

#include <list>
#include <mutex>
#include <Sockets/DatagramSocket.hpp>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <thread>
#include <vector>

namespace {

    constexpr size_t maximumReadSize = 65536;

}

namespace Sockets {

    struct DatagramSocket::Impl {
        // Types
        struct Datagram {
            std::string message;
            uint32_t address;
            uint16_t port;
            OnSent onSent;
        };

        // Properties
        std::list< Datagram > datagramsToSend;
        bool error = false;
        std::mutex mutex;
        uint8_t receiveBuffer[maximumReadSize];
        SOCKET socket = INVALID_SOCKET;
        SocketEventLoop socketEventLoop;
        UsesSockets usesSockets;

        // Lifecycle

        ~Impl() noexcept {
            if (!IS_INVALID_SOCKET(socket)) {
                (void)closesocket(socket);
            }
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) noexcept = delete;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) noexcept = delete;

        // Constructor
        Impl() = default;

        // Methods

        bool IsReadyToSend() {
            std::lock_guard< decltype(mutex) > lock(mutex);
            return !datagramsToSend.empty();
        }

        bool OnSocketReady(
            OnReceived onReceived
        ) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            if (error) {
                return true;
            }
            bool readReady = TryReceivingDatagram(onReceived, lock);
            bool writeReady = TrySendingDatagram(lock);
            if (error) {
                socketEventLoop.Stop();
            }
            return !readReady && !writeReady;
        }

        bool TryReceivingDatagram(
            OnReceived onReceived,
            std::unique_lock< decltype(mutex) >& lock
        ) {
            const auto amountReceived = recvfrom(
                socket,
                (char*)receiveBuffer,
                (SOCKET_DATAGRAM_LENGTH_TYPE)maximumReadSize,
                MSG_NOSIGNAL,
                NULL,
                NULL
            );
            if (IS_SOCKET_ERROR(amountReceived)) {
                if (
                    !LAST_SOCKET_OPERATION_WOULD_BLOCK
                    && !LAST_SOCKET_OPERATION_WAS_RESET
                ) {
                    error = true;
                    fprintf(stderr, "error: unable to read socket\n");
                }
            } else if (amountReceived > 0) {
                const std::string message(
                    receiveBuffer,
                    receiveBuffer + amountReceived
                );
                lock.unlock();
                onReceived(message);
                lock.lock();
                return true;
            }
            return false;
        }

        bool TrySendingDatagram(
            std::unique_lock< decltype(mutex) >& lock
        ) {
            if (datagramsToSend.empty()) {
                return false;
            }
            const auto& datagram = datagramsToSend.front();
            struct sockaddr_in peerAddress;
            (void)memset(&peerAddress, 0, sizeof(peerAddress));
            peerAddress.sin_family = AF_INET;
            peerAddress.IPV4_ADDRESS_IN_SOCKADDR = htonl(datagram.address);
            peerAddress.sin_port = htons(datagram.port);
            const auto amountSent = sendto(
                socket,
                datagram.message.c_str(),
                (SOCKET_DATAGRAM_LENGTH_TYPE)datagram.message.length(),
                0,
                (const sockaddr*)&peerAddress,
                sizeof(peerAddress)
            );
            if (IS_SOCKET_ERROR(amountSent)) {
                if (!LAST_SOCKET_OPERATION_WOULD_BLOCK) {
                    error = true;
                    fprintf(stderr, "error: unable to write socket\n");
                }
                return true;
            } else {
                auto onSent = std::move(datagram.onSent);
                datagramsToSend.pop_front();
                if (onSent) {
                    lock.unlock();
                    onSent();
                    lock.lock();
                }
                return !datagramsToSend.empty();
            }
        }
    };

    DatagramSocket::DatagramSocket()
        : impl_(new Impl())
    {
    }

    bool DatagramSocket::Bind(uint16_t port) {
        // Create the socket.
        impl_->socket = socket(AF_INET, SOCK_DGRAM, 0);
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

    void DatagramSocket::SendMessage(
        const std::string& message,
        uint32_t address,
        uint16_t port,
        OnSent onSent
    ) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        impl_->datagramsToSend.push_back({message, address, port, onSent});
        impl_->socketEventLoop.UserEvent();
    }

    void DatagramSocket::Start(OnReceived onReceived) {
        std::weak_ptr< Impl > implWeak(impl_);
        impl_->socketEventLoop.Start(
            impl_->socket,

            // isReadyToSend
            [
                implWeak
            ]{
                const auto impl = implWeak.lock();
                if (!impl) {
                    return false;
                }
                return impl->IsReadyToSend();
            },

            // onSocketReady
            [
                implWeak,
                onReceived
            ]{
                const auto impl = implWeak.lock();
                if (!impl) {
                    return true;
                }
                return impl->OnSocketReady(onReceived);
            }
        );
    }

}
