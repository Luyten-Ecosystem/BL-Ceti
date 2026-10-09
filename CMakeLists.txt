cmake_minimum_required(VERSION 3.20)

project(BLCeti VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(BL
    src/main.c
    src/lexer.c
    src/parser.c
)

target_include_directories(BL PRIVATE include)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(BL PRIVATE
        -Wall
        -Wextra
        -Wpedantic
        -Wconversion
        -Wshadow
    )
endif()

install(TARGETS BL RUNTIME DESTINATION bin)
