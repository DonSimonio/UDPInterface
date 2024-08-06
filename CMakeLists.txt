cmake_minimum_required(VERSION 3.10)
project(UDPInterface VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Add sources
set(SOURCES
    source/UDPReceiver.cpp
    source/UDPSender.cpp
)

# Add headers
set(HEADERS
    include/UDPEnums.hpp
    include/UDPInterface.hpp
)

# TODO(s.bandel): link correctly .dll files
# Add libaries
set(LIBRARIES
)

# Add library
add_library(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBRARIES})

# Add dependencies
