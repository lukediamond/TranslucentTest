cmake_minimum_required(VERSION 3.5.0)

file(GLOB RUNTIME_SOURCES src/*.cpp)
add_executable(runtime ${RUNTIME_SOURCES})
target_link_libraries(runtime
    SDL2
    assimp
    m
    pthread
    GL
    GLEW
)
