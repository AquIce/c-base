cmake_minimum_required(VERSION 3.25)

project(
    base
    VERSION 0.1.0
    DESCRIPTION "Portable C foundation library"
    LANGUAGES C
)

set(CMAKE_C_STANDARD			23)
set(CMAKE_C_STANDARD_REQUIRED	ON)
set(CMAKE_C_EXTENSIONS			OFF)

option(BASE_BUILD_TESTS			"Build tests"			ON)
option(BASE_BUILD_EXAMPLES		"Build examples"		ON)

include(cmake/Sanitizers.cmake)

add_library(base STATIC)
add_library(base::base ALIAS base)

target_include_directories(base
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

find_package(Threads REQUIRED)

target_link_libraries(base
    PUBLIC
        Threads::Threads
)

if(WIN32)
    target_link_libraries(base
        PUBLIC
            ws2_32
    )
endif()

add_subdirectory(src/foundation)
# add_subdirectory(src/math)
# add_subdirectory(src/runtime)
# add_subdirectory(src/network)

if(BASE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BASE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
