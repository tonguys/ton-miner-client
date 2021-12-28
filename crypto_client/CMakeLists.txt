cmake_minimum_required(VERSION 3.6)
project(Client)

# ensure C++14
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()

#export compiler flags for code completion engines
set( CMAKE_EXPORT_COMPILE_COMMANDS 1 )

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

# Set global compiler warnings
if(MSVC)
    add_compile_options(/W3 /WX)
else()
    add_compile_options(-Wall -pedantic)
endif()

add_library(clientLib
    src/app.cpp
    src/app.hpp
    src/config.hpp
    src/executor.cpp
    src/executor.hpp
    src/client.hpp
    src/mockClient.hpp
    src/httpClient.cpp
    src/httpClient.hpp
    src/responses.cpp
    src/responses.hpp)
target_include_directories(clientLib PUBLIC src)
target_link_libraries(clientLib ${CONAN_LIBS})

enable_testing()

add_executable(clientTest tests/tests.cpp)
target_include_directories(clientLib PRIVATE tests)
target_link_libraries(clientTest PRIVATE clientLib)
add_test(test1 clientTest)

add_executable(client main.cpp)
target_link_libraries(client PRIVATE clientLib)


# Clang tidy as optional static analyzer
option(Template_USE_CLANG_TIDY "Use clang tidy for static analysis" OFF)
if (Template_USE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE NAMES "clang-tidy" "clang-tidy-6.0" "clang-tidy-5.0" "clang-tidy-4.0"
                 DOC "Path to clang-tidy executable")
    if(NOT CLANG_TIDY_EXE)
      message(STATUS "clang-tidy not found.")
    else()
      message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
      set_target_properties(clientLib
     PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
endif()

# LLVM AddressSanitizer to detecting memory errors
# Note that there are many other sanitizers in LLVM to help detect errors, see
# http://travistoptips.blogspot.co.uk/2015/11/sanitize-all-things.html
option(Template_MEMCHECK "Use LLVM AddressSanitizer to detecting memory errors" OFF)
if (Template_MEMCHECK)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(STATUS "Configuring with LLVM AddressSanitizer")
        set(Template_MEMCHECK_FLAGS -fno-optimize-sibling-calls 
                                    -fsanitize=address 
                                    -fsanitize-address-use-after-scope
                                    )
        target_compile_options(clientLib
     PUBLIC -O1 -g -fno-omit-frame-pointer ${Template_MEMCHECK_FLAGS})
        target_link_libraries(clientLib
     PUBLIC -g ${Template_MEMCHECK_FLAGS})
    else()
        message(FATAL_ERROR "clang compiler required with Template_MEMCHECK: found ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# Setup coverage testing for GCC or Clang
option(Template_ENABLE_COVERAGE "Enable coverage reporting for GCC or Clang" FALSE)
if (Template_ENABLE_COVERAGE)
    if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        message(STATUS "Configuring with coverage")
        target_compile_options(clientLib
     PUBLIC --coverage -O0)
        target_link_libraries(clientLib
     PUBLIC --coverage)
    else()
        message(FATAL_ERROR "GCC or Clang required with Template_ENABLE_COVERAGE: found ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()
