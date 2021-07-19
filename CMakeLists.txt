cmake_minimum_required(VERSION 3.7.0)
project(natsukashii)

if(CMAKE_VERSION VERSION_LESS "3.7.0")
  set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB IMGUI_SRC_BASE "${CMAKE_SOURCE_DIR}/include/external/imgui/*.cpp")
set(IMGUI_SRC_BACKEND 
  ${CMAKE_SOURCE_DIR}/include/external/imgui/backends/imgui_impl_glfw.cpp
  ${CMAKE_SOURCE_DIR}/include/external/imgui/backends/imgui_impl_opengl3.cpp)

set(IMGUI_SRC ${IMGUI_SRC_BASE} ${IMGUI_SRC_BACKEND})

add_compile_definitions(IMGUI_IMPL_OPENGL_LOADER_GLAD)

if(WIN32)
  set(FLAGS -static SDL2main SDL2 glfw3 OpenGL::GL gcc stdc++ winpthread winmm version Imm32 Setupapi)
else()
  set(FLAGS SDL2 glfw OpenGL::GL ${CMAKE_DL_LIBS} pthread)
endif()

file(GLOB_RECURSE EMU_SRC "${CMAKE_SOURCE_DIR}/src/*.cpp")

include_directories(
  ${CMAKE_SOURCE_DIR}/include/
  ${CMAKE_SOURCE_DIR}/include/core/
  ${CMAKE_SOURCE_DIR}/include/core/apu/
  ${CMAKE_SOURCE_DIR}/include/external/
  ${CMAKE_SOURCE_DIR}/include/external/glad/include/
  ${CMAKE_SOURCE_DIR}/include/external/imgui/
  ${CMAKE_SOURCE_DIR}/include/external/imgui/backends/
  ${CMAKE_SOURCE_DIR}/include/external/nativefiledialog-extended/
)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(SDL2 REQUIRED)
add_subdirectory(include/external/nativefiledialog-extended/)

add_compile_options(-O3 -g)

add_executable(${CMAKE_PROJECT_NAME}
  ${IMGUI_SRC}
  ${EMU_SRC}
  ${CMAKE_SOURCE_DIR}/include/external/glad/src/glad.c
)

target_link_libraries(${CMAKE_PROJECT_NAME} nfd ${FLAGS})