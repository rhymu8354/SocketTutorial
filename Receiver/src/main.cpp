#include <inttypes.h>
#include <signal.h>
#include <Sockets/DatagramSocket.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <thread>

namespace {

    // This is the UDP port number at which we will receive messages.
    constexpr uint16_t port = 8000;

    // This flag is set by our SIGINT signal handler in order to cause the main
    // program's polling loop to exit and let the program clean up and
    // terminate.
    bool shutDown = false;

    // This function is provided to the event loop to be called whenever a
    // message is received at our socket.
    //
    // When this happens, simply print the message to standard output.
    void OnReceiveMessage(const std::string& message) {
        printf("Received message: %s\n", message.c_str());
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
        Sockets::DatagramSocket receiver;
        if (!receiver.Bind(port)) {
            return EXIT_FAILURE;
        }

        // Start the event loop to process the sending and receiving of
        // datagrams.
        receiver.Start(OnReceiveMessage);
        printf("Now listening for messages on port %" PRIu16 "...\n", port);

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
