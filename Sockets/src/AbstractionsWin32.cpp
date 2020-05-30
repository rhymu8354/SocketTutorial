#include "Abstractions.hpp"

#include <stdio.h>
#include <thread>

namespace Sockets {

    struct UsesSockets::Impl {
        bool wsaStartedUp = false;

        ~Impl() noexcept {
            if (wsaStartedUp) {
                (void)WSACleanup();
            }
        }

        Impl(const Impl&) = delete;
        Impl(Impl&&) noexcept = default;
        Impl& operator=(const Impl&) = delete;
        Impl& operator=(Impl&&) noexcept = default;

        Impl() {
            WSADATA wsaData;
            if (!WSAStartup(MAKEWORD(2, 0), &wsaData)) {
                wsaStartedUp = true;
            }
        }
    };

    UsesSockets::UsesSockets()
        : impl_(new Impl())
    {
    }

    struct SocketEventLoop::Impl {
        bool stop = false;
        HANDLE socketEvent = NULL;
        HANDLE userEvent = NULL;
        std::thread worker;

        ~Impl() noexcept {
            if (worker.joinable()) {
                if (worker.get_id() == std::this_thread::get_id()) {
                    worker.detach();
                } else {
                    stop = true;
                    (void)SetEvent(userEvent);
                    worker.join();
                }
            }
            if (socketEvent != NULL) {
                (void)WSACloseEvent(socketEvent);
            }
            if (userEvent != NULL) {
                (void)CloseHandle(userEvent);
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
                    const HANDLE handles[] = {
                        impl->userEvent,
                        impl->socketEvent
                    };
                    if (
                        WaitForMultipleObjects(
                            sizeof(handles) / sizeof(*handles),
                            handles,
                            FALSE,
                            INFINITE
                        ) == WAIT_OBJECT_0 + 1
                    ) {
                        WSANETWORKEVENTS networkEvents;
                        (void)WSAEnumNetworkEvents(socket, impl->socketEvent, &networkEvents);
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
        IsReadyToSend /* isReadyToSend */,
        OnSocketReady onSocketReady
    ) {
        impl_->socketEvent = WSACreateEvent();
        if (impl_->socketEvent == NULL) {
            fprintf(stderr, "error: unable to create socket event\n");
            return;
        }
        if (
            WSAEventSelect(
                socket,
                impl_->socketEvent,
                (FD_ACCEPT | FD_READ | FD_WRITE | FD_CLOSE)
            ) != 0
        ) {
            fprintf(stderr, "error: unable to configure socket event\n");
            return;
        }
        impl_->userEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (impl_->userEvent == NULL) {
            fprintf(stderr, "error: unable to create user event\n");
            return;
        }
        std::weak_ptr< Impl > implWeak(impl_);
        impl_->worker = std::thread(&Impl::Worker, implWeak, socket, onSocketReady);
    }

    void SocketEventLoop::Stop() {
        impl_->stop = true;
        (void)SetEvent(impl_->userEvent);
    }

    void SocketEventLoop::UserEvent() {
        (void)SetEvent(impl_->userEvent);
    }

}
