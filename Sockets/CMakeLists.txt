set(This Sockets)
set(Sources
    include/Sockets/ClientSocket.hpp
    include/Sockets/DatagramSocket.hpp
    include/Sockets/ServerSocket.hpp
    src/Abstractions.hpp
    src/ClientSocket.cpp
    src/Connection.hpp
    src/Connection.cpp
    src/DatagramSocket.cpp
    src/ServerSocket.cpp
)
if(MSVC)
    list(APPEND Sources
        src/AbstractionsWin32.cpp
    )
else()
    list(APPEND Sources
        src/AbstractionsPosix.cpp
        src/PipeSignal.cpp
        src/PipeSignal.hpp
    )
endif()

add_library(${This} ${Sources})
set_target_properties(${This} PROPERTIES FOLDER Libraries)
target_include_directories(${This} PUBLIC include)
if(UNIX)
    target_link_libraries(${This} PUBLIC pthread)
endif(UNIX)
