#include "Abstractions.hpp"
#include "Connection.hpp"

#include <list>
#include <mutex>
#include <stddef.h>
#include <stdio.h>
#include <string>
#include <vector>

namespace {

    constexpr size_t maximumReadSize = 65536;

}

namespace Sockets {

    struct Connection::Impl {
        // Types
        struct Buffer {
            std::string message;
            size_t offset = 0;
        };

        // Properties
        std::list< Buffer > buffersToSend;
        bool readClosed = false;
        bool writeClosed = false;
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
            return !buffersToSend.empty();
        }

        bool OnSocketReady(
            OnReceived onReceived,
            OnClosed onClosed
        ) {
            std::unique_lock< decltype(mutex) > lock(mutex);
            if (error) {
                return true;
            }
            bool readReady = TryReadingSocket(onReceived, onClosed, lock);
            bool writeReady = TryWritingSocket(onClosed, lock);
            if (error) {
                socketEventLoop.Stop();
            }
            return !readReady && !writeReady;
        }

        bool TryReadingSocket(
            OnReceived onReceived,
            OnClosed onClosed,
            std::unique_lock< decltype(mutex) >& lock
        ) {
            if (readClosed) {
                return false;
            }
            const int amountReceived = recv(
                socket,
                (char*)receiveBuffer,
                (SOCKET_DATAGRAM_LENGTH_TYPE)maximumReadSize,
                0
            );
            if (IS_SOCKET_ERROR(amountReceived)) {
                if (!LAST_SOCKET_OPERATION_WOULD_BLOCK) {
                    error = true;
                    if (!LAST_SOCKET_OPERATION_WAS_RESET) {
                        fprintf(stderr, "error: unable to read socket\n");
                    }
                    lock.unlock();
                    onClosed();
                    lock.lock();
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
            } else {
                readClosed = true;
                lock.unlock();
                onClosed();
                lock.lock();
            }
            return false;
        }

        bool TryWritingSocket(
            OnClosed onClosed,
            std::unique_lock< decltype(mutex) >& lock
        ) {
            if (buffersToSend.empty()) {
                return false;
            }
            auto& buffer = buffersToSend.front();
            const auto amountSent = send(
                socket,
                buffer.message.c_str() + buffer.offset,
                (SOCKET_DATAGRAM_LENGTH_TYPE)(
                    buffer.message.length() - buffer.offset
                ),
                0
            );
            if (IS_SOCKET_ERROR(amountSent)) {
                if (!LAST_SOCKET_OPERATION_WOULD_BLOCK) {
                    error = true;
                    if (!LAST_SOCKET_OPERATION_WAS_RESET) {
                        fprintf(stderr, "error: unable to write socket\n");
                    }
                    lock.unlock();
                    onClosed();
                    lock.lock();
                }
            } else {
                buffer.offset += (size_t)amountSent;
                if (buffer.offset >= buffer.message.length()) {
                    buffersToSend.pop_front();
                }
                if (!buffersToSend.empty()) {
                    return true;
                }
                if (writeClosed) {
                    (void)shutdown(socket, SD_SEND);
                }
            }
            return false;
        }
    };

    Connection::Connection()
        : impl_(new Impl())
    {
    }

    void Connection::Close() {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        impl_->writeClosed = true;
        if (impl_->buffersToSend.empty()) {
            (void)shutdown(impl_->socket, SD_SEND);
        } else {
            impl_->socketEventLoop.UserEvent();
        }
    }

    void Connection::SendMessage(const std::string& message) {
        std::lock_guard< decltype(impl_->mutex) > lock(impl_->mutex);
        Impl::Buffer buffer;
        buffer.message = message;
        impl_->buffersToSend.push_back(std::move(buffer));
        impl_->socketEventLoop.UserEvent();
    }

    void Connection::Start(
        SOCKET socket,
        OnReceived onReceived,
        OnClosed onClosed
    ) {
        impl_->socket = socket;
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
                onReceived,
                onClosed
            ]{
                const auto impl = implWeak.lock();
                if (!impl) {
                    return true;
                }
                return impl->OnSocketReady(onReceived, onClosed);
            }
        );
    }

}
