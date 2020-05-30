# SocketTutorial

This is an example workspace demonstrating various aspects of how to use
Berkeley Sockets (in UNIX or UNIX-like operating systems such as Linux or
macOS) or Windows Sockets in C++ programs.  It was designed to accompany the
video, [Tutorial: Using Sockets In C++](https://example.com/TODO_Put_YouTube_Link_Here_Once_Video_Is_Published).

## Usage

The `Sockets` library provides the foundations of encapsulating in C++ classes
the Berkeley Sockets or Windows Sockets operating system Application
Programming Interface (API) for accessing a computer network.  It contains
three externally available classes:

* `ClientSocket` represents a connection-oriented socket (i.e. TCP connection)
  used to connect a client to a remote server.
* `ServerSocket` represents a connection-oriented socket (i.e. TCP connection)
  used to listen as a server for incoming connections from clients.
* `DatagramSocket` represents a datagram-oriented socket (i.e. UDP endpoint)
  which can be used to send and receive datagrams on the network.

The `Receiver` and `Sender` programs accompany the `DatagramSocket` class and
demonstrate how to send and receive datagrams.

The `Client` program accompanies the `ClientSocket` class and demonstrates how
to make outgoing connections as a client to a remote server.

The `Server` program accompanies the `ServerSocket` class and demonstrates how
to set up a socket for accepting incoming connections from remote clients as a
server.

Internally, the `Sockets` library also includes the following classes, which
are used to handle various tasks in socket programming:

* `UsesSockets` is a class included in the compositions of other classes which
  use the operating system's socket APIs.  Its responsibility is to acquire and
  release, when appropriate, operating system resources required to use the
  socket APIs correctly.  It's important for the Windows implementation
  especially because it handles calling the `WSAStartup` and `WSACleanup`
  functions which must be called before using the API and after using the API
  (respectively).
* `SocketEventLoop` is a class included in the compositions of other
  classes which require an event loop to operate a socket.  It runs a worker
  thread to monitor the socket and trigger operations whenever the socket
  appears ready for them.
* `Connection` is a class used by the implementations of both the
  `ClientSocket` and `ServerSocket` classes in order to asynchronously handle
  the reading and writing of data for a socket.
* `PipeSignal` is a class which provides a file handle which can be provided to
  the `select` function in order to synchronize one thread with another.  While
  one thread waits on the handle using `select`, another thread can call the
  `Set` method provided by the class instance to wake up the first thread.
  It's implemented using an operating system pipe.

## Supported platforms / recommended toolchains

This is a collection of portable C++11 libraries and programs which depends on
the C++11 compiler and standard C and C++ libraries.  It should be supported on
almost any platform.  The following are recommended toolchains for popular
platforms.

* Windows -- [Visual Studio](https://www.visualstudio.com/) (Microsoft Visual
  C++)
* Linux -- clang or gcc
* MacOS -- Xcode (clang)

## Building

This project can stand alone or be included in larger projects.
[CMake](https://cmake.org/) files are included for your convenience to generate
a build system to compile the source code and link them into programs you can
run.

There are two distinct steps in the build process using CMake:

1. Generation of the build system, using CMake
2. Compiling, linking, etc., using CMake-compatible toolchain

### Prerequisites

* [CMake](https://cmake.org/) version 3.8 or newer
* C++11 toolchain compatible with CMake for your development platform (e.g.
  [Visual Studio](https://www.visualstudio.com/) on Windows)

### Build system generation

Generate the build system using [CMake](https://cmake.org/) from the solution
root.  For example:

```bash
mkdir build
cd build
cmake -G "Visual Studio 15 2017" -A "x64" ..
```

### Compiling, linking, et cetera

Either use [CMake](https://cmake.org/) or your toolchain's IDE to build.
For [CMake](https://cmake.org/):

```bash
cd build
cmake --build . --config Release
```

Each example program will be built under a subdirectory of the build folder.
For example, on Windows, the file `build/Server/Server.exe` will be built
if the build directory was named `build` as in the above example.
