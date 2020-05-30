#include <future>
#include <inttypes.h>
#include <memory>
#include <signal.h>
#include <Sockets/DatagramSocket.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

namespace {

    // This is the IPv4 address to which to send the message.
    constexpr uint32_t address = 0x7f000001;

    // This is the UDP port number to which to send the message.
    constexpr uint16_t port = 8000;

    // This function is provided to the event loop to be called whenever a
    // message is received at our socket.
    //
    // This shouldn't happen unless some other program decides to try to send a
    // datagram to the UDP port we happen to be using, but if it does, simply
    // print the message to standard output.
    void OnReceiveMessage(const std::string& message) {
        printf("Received message: %s\n", message.c_str());
    }

}

int main(int argc, char* argv[]) {
    // Make a socket and assign an address to it chosen by the operating
    // system from the set of available ephemeral ports.
    Sockets::DatagramSocket sender;
    if (!sender.Bind()) {
        return EXIT_FAILURE;
    }

    // Start the event loop to process the sending and receiving of datagrams.
    sender.Start(OnReceiveMessage);

    // Send a single datagram to another program.  Provide a promise whose
    // value will be set once the event loop has finished providing the
    // datagram to the operating system to be sent on the network.
    printf("Now sending message to port %" PRIu16 "...\n", port);
    auto sent = std::make_shared< std::promise< void > >();
    sender.SendMessage(
        "Hello, World!",
        address,
        port,
        [sent]{ sent->set_value(); }
    );

    // Wait for the datagram to be sent, and then clean up and terminate the
    // program.
    sent->get_future().wait();
    printf("Program exiting.\n");
    return EXIT_SUCCESS;
}
