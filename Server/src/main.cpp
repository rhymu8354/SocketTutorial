#include <inttypes.h>
#include <mutex>
#include <signal.h>
#include <Sockets/ServerSocket.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <thread>
#include <unordered_set>

namespace {

    // This is the TCP port number on which to accept new connections.
    constexpr uint16_t port = 8000;

    // This holds the set of currently connected clients.
    std::unordered_set< std::shared_ptr< Sockets::ServerSocket::Client > > clients;

    // This is used to serialize access to the set of currently connected
    // clients.
    std::mutex mutex;

    // This flag is set by our SIGINT signal handler in order to cause the main
    // program's polling loop to exit and let the program clean up and
    // terminate.
    bool shutDown = false;

    // This function is provided to the event loop to be called if a client
    // connection is closed for reading from the other end.
    //
    // When this happens, remove the client from the set of connected clients.
    // This closes the connection from our end and releases system resources
    // used for the connection.
    void OnClosed(
        const std::shared_ptr< Sockets::ServerSocket::Client >& client
    ) {
        std::lock_guard< decltype(mutex) > lock(mutex);
        (void)clients.erase(client);
        printf("Client connection closed (%zu remain).\n", clients.size());
    }

    // This function is provided to the event loop to be called whenever a
    // message is received from a client connection.
    //
    // When this happens, simply print the message to standard output.
    void OnReceiveMessage(
        const std::shared_ptr< Sockets::ServerSocket::Client >& client,
        const std::string& message
    ) {
        printf("Received message: %s\n", message.c_str());
    }

    void OnAcceptClient(
        std::shared_ptr< Sockets::ServerSocket::Client >&& client
    ) {
        // Add the client to the set of connected clients.  If this client
        // was already in the set, return early (this should never happen,
        // but it's cheap to test).
        std::lock_guard< decltype(mutex) > lock(mutex);
        const auto clientInsertion = clients.insert(std::move(client));
        if (!clientInsertion.second) {
            return;
        }

        // Start the event loop of the connection object representing the
        // client, setting delegates to handle when messages are received
        // or when the connection is closed for reading from the other end.
        printf("New connection accepted (%zu total).\n", clients.size());
        const auto& clientsEntry = clientInsertion.first;
        const auto& newClient = *clientsEntry;
        std::weak_ptr< Sockets::ServerSocket::Client > clientWeak(newClient);
        newClient->Start(
            // onReceived
            [clientWeak](const std::string& message) {
                auto client = clientWeak.lock();
                if (!client) {
                    return;
                }
                OnReceiveMessage(client, message);
            },

            // onClosed
            [clientWeak]{
                auto client = clientWeak.lock();
                if (!client) {
                    return;
                }
                OnClosed(client);
            }
        );

        // Send a message to the client to test the server's ability to send a
        // message as well as the client's ability to receive it.
        newClient->SendMessage("Welcome!");
    }

    // This function is set up to be called whenever the SIGINT signal
    // (interrupt signal, typically sent when the user presses <Ctrl>+<C> on
    // the terminal) is sent to the program.  We just set a flag which is
    // checked in the program's polling loop to control when the loop is
    // exited.
    void OnSigInt(int) {
        shutDown = true;
    }

    // This is the function called from the main program in order to operate
    // the socket while a SIGINT handler is set up to control when the program
    // should terminate.
    int InterruptableMain() {
        // Make a socket and assign an address to it.
        Sockets::ServerSocket server;
        if (!server.Bind(port)) {
            return EXIT_FAILURE;
        }

        // Set up the socket to receive incoming connections.
        server.Listen(OnAcceptClient);
        printf("Now listening for connections on port %" PRIu16 "...\n", port);

        // Poll the flag set by our SIGINT handler, until it is set.
        while (!shutDown) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        printf("Program exiting.\n");
        return EXIT_SUCCESS;
    }

}

int main(int argc, char* argv[]) {
    // Catch SIGINT (interrupt signal, typically sent when the user presses
    // <Ctrl>+<C> on the terminal) during program execution.
    const auto previousInterruptHandler = signal(SIGINT, OnSigInt);
    const auto returnValue = InterruptableMain();
    (void)signal(SIGINT, previousInterruptHandler);
    return returnValue;
}
