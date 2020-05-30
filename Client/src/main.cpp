#include <future>
#include <inttypes.h>
#include <memory>
#include <Sockets/ClientSocket.hpp>
#include <stdio.h>
#include <stdlib.h>

namespace {

    // This is the IPv4 address of the server to which to connect.
    constexpr uint32_t address = 0x7f000001;

    // This is the TCP port number of the server to which to connect.
    constexpr uint16_t port = 8000;

    // This function is provided to the event loop to be called whenever a
    // message is received from the server.
    //
    // When this happens, simply print the message to standard output.
    void OnReceiveMessage(const std::string& message) {
        printf("Received message: %s\n", message.c_str());
    }

}

int main(int argc, char* argv[]) {
    // Make a socket and assign an address to it chosen by the operating
    // system from the set of available ephemeral ports.
    Sockets::ClientSocket client;
    if (!client.Bind()) {
        return EXIT_FAILURE;
    }

    // Connect to the server, setting up for a delegate to be called
    // whenever a message is received, and to set a promise when
    // the connection is closed.
    auto closed = std::make_shared< std::promise< void > >();
    if (
        !client.Connect(
            address,
            port,
            OnReceiveMessage,
            [closed]{ closed->set_value(); }
        )
    ) {
        return EXIT_FAILURE;
    }

    // Send a message to the server and then close our end (for writing).
    printf("Connected and sending message to port %" PRIu16 "...\n", port);
    client.SendMessage("Hello, World!");
    client.Close();

    // Wait for the connection to be closed (by reading from the other end).
    closed->get_future().wait();
    printf("Program exiting.\n");
    return EXIT_SUCCESS;
}
