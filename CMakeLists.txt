cmake_minimum_required(VERSION 3.16)
project(Animated3DFarmScene LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)

add_executable(Animated3DFarmScene
    src/main.cpp
    src/Application.cpp
    src/Camera.cpp
    src/Input.cpp
    src/Scene.cpp
    src/Animation.cpp
    src/Lighting.cpp
    src/graphics/Primitives.cpp
    src/objects/Farmhouse.cpp
)

target_include_directories(Animated3DFarmScene PRIVATE include)
target_link_libraries(Animated3DFarmScene PRIVATE glfw OpenGL::GL)
