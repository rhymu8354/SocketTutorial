#include "Abstractions.hpp"
#include "PipeSignal.hpp"

#include <algorithm>
#include <fcntl.h>
#include <stdio.h>
#include <thread>

namespace Sockets {

    struct UsesSockets::Impl {
    };

    UsesSockets::UsesSockets()
        : impl_(new Impl())
    {
    }

    struct SocketEventLoop::Impl {
        bool stop = false;
        PipeSignal userEvent;
        std::thread worker;

        ~Impl() noexcept {
            if (worker.joinable()) {
                if (worker.get_id() == std::this_thread::get_id()) {
                    worker.detach();
                } else {
                    stop = true;
                    userEvent.Set();
                    worker.join();
                }
            }
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) noexcept = default;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) noexcept = default;

        Impl() = default;

        static void Worker(
            std::weak_ptr< Impl > implWeak,
            SOCKET socket,
            IsReadyToSend isReadyToSend,
            OnSocketReady onSocketReady
        ) {
            bool wait = true;
            for (;;) {
                auto impl = implWeak.lock();
                if (
                    !impl
                    || impl->stop
                ) {
                    return;
                }
                if (wait) {
                    const int userEventSelectHandle = impl->userEvent.GetSelectHandle();
                    const int nfds = std::max(userEventSelectHandle, socket) + 1;
                    fd_set readfds, writefds;
                    FD_ZERO(&readfds);
                    FD_ZERO(&writefds);
                    FD_SET(socket, &readfds);
                    if (isReadyToSend()) {
                        FD_SET(socket, &writefds);
                    }
                    FD_SET(userEventSelectHandle, &readfds);
                    (void)select(nfds, &readfds, &writefds, NULL, NULL);
                    if (FD_ISSET(userEventSelectHandle, &readfds) != 0) {
                        impl->userEvent.Clear();
                    }
                }
                wait = onSocketReady();
            }
        }
    };

    SocketEventLoop::~SocketEventLoop() noexcept {
        Stop();
    }

    SocketEventLoop::SocketEventLoop(const SocketEventLoop&) = default;
    SocketEventLoop::SocketEventLoop(SocketEventLoop&&) noexcept = default;
    SocketEventLoop& SocketEventLoop::operator=(const SocketEventLoop&) = default;
    SocketEventLoop& SocketEventLoop::operator=(SocketEventLoop&&) noexcept = default;

    SocketEventLoop::SocketEventLoop()
        : impl_(new Impl())
    {
    }

    void SocketEventLoop::Start(
        SOCKET socket,
        IsReadyToSend isReadyToSend,
        OnSocketReady onSocketReady
    ) {
        int flags = fcntl(socket, F_GETFL, 0);
        flags |= O_NONBLOCK;
        (void)fcntl(socket, F_SETFL, flags);
        if (!impl_->userEvent.Initialize()) {
            fprintf(stderr, "error: unable to create user event\n");
            return;
        }
        impl_->userEvent.Clear();
        std::weak_ptr< Impl > implWeak(impl_);
        impl_->worker = std::thread(
            &Impl::Worker,
            implWeak,
            socket,
            isReadyToSend,
            onSocketReady
        );
    }

    void SocketEventLoop::Stop() {
        impl_->stop = true;
        impl_->userEvent.Set();
    }

    void SocketEventLoop::UserEvent() {
        impl_->userEvent.Set();
    }

}
