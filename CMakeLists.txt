cmake_minimum_required (VERSION 3.0)
project(vits VERSION 0.1.0)

set(CMAKE_C_STANDARD   20)
set(CMAKE_CXX_STANDARD 20)

add_subdirectory(ggml)

add_library(vits src/vits.cpp src/vits_model_data.cpp src/vits_tokenizer.cpp src/debug.cpp)
target_include_directories(vits PRIVATE ggml/include)

target_link_libraries(vits PRIVATE ggml)

find_library(LIBESPEAK_LIBRARIES espeak-ng)
if(LIBESPEAK_LIBRARIES)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ESPEAK REQUIRED espeak-ng)
    add_definitions(-DVITS_ESPEAK)
endif()

add_executable(main test/main.cpp)
target_include_directories(main PRIVATE src/include ggml/include ${ESPEAK_INCLUDE_DIRS})
target_link_libraries(main PRIVATE vits ggml ${ESPEAK_LIBRARIES})
