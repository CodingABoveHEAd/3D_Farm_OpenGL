cmake_minimum_required(VERSION 3.16)
project(Animated3DFarmScene LANGUAGES CXX)

include(CTest)

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
    src/farmworld.cpp
    src/graphics/Primitives.cpp
    src/graphics/Shadow.cpp
    src/graphics/TextureManager.cpp
    src/objects/Farmhouse.cpp
    src/objects/Animals.cpp
    src/objects/Barn.cpp
    src/objects/Farmer.cpp
    src/objects/Pond.cpp
    src/objects/Bridge.cpp
    src/objects/PowerPlant.cpp
    src/objects/PowerSubstation.cpp
    src/objects/Vegetation.cpp
    src/objects/Tractor.cpp
    src/objects/windmill.cpp
    src/objects/sky.cpp
    src/objects/cloud.cpp
    src/objects/Birds.cpp
    src/objects/Village.cpp
)

target_include_directories(Animated3DFarmScene PRIVATE include)
target_link_libraries(Animated3DFarmScene PRIVATE glfw OpenGL::GL)

if(MSVC)
    target_compile_options(Animated3DFarmScene PRIVATE /W4 /permissive-)
else()
    target_compile_options(Animated3DFarmScene PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BUILD_TESTING)
    add_executable(CameraControlTests
        tests/CameraControlTests.cpp
        src/Camera.cpp
        src/Input.cpp
    )
    target_include_directories(CameraControlTests PRIVATE include)
    target_link_libraries(CameraControlTests PRIVATE glfw OpenGL::GL)
    if(MSVC)
        target_compile_options(CameraControlTests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(CameraControlTests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    add_test(NAME camera_controls COMMAND CameraControlTests)
endif()
