cmake_minimum_required (VERSION 3.5)
project(vits VERSION 0.1.0)

set(CMAKE_C_STANDARD   20)
set(CMAKE_CXX_STANDARD 20)

option(VITS_SERVER_SSL "Build SSL support for the server" OFF)

add_subdirectory(ggml)

add_library(vits src/vits.cpp src/vits_model_data.cpp src/vits_tokenizer.cpp src/debug.cpp)
target_include_directories(vits PUBLIC ggml/include)

target_link_libraries(vits PUBLIC ggml)

find_library(LIBESPEAK_LIBRARIES espeak-ng)
if(LIBESPEAK_LIBRARIES)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ESPEAK REQUIRED espeak-ng)
    add_definitions(-DVITS_ESPEAK)
endif()

add_executable(main test/main.cpp)
target_include_directories(main PUBLIC src/include ggml/include ${ESPEAK_INCLUDE_DIRS})
target_link_libraries(main PUBLIC vits ggml ${ESPEAK_LIBRARIES})


add_executable(server test/server.cpp)
if (VITS_SERVER_SSL)
   find_package(OpenSSL REQUIRED)
   target_link_libraries(server PRIVATE OpenSSL::SSL OpenSSL::Crypto)
   target_compile_definitions(server PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT) 
endif()
find_package(nlohmann_json 3.11.3 REQUIRED)
target_include_directories(server PUBLIC src/include ggml/include ${ESPEAK_INCLUDE_DIRS} cpp-httplib)
target_link_libraries(server PUBLIC vits ggml ${ESPEAK_LIBRARIES} nlohmann_json::nlohmann_json)
