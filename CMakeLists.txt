cmake_minimum_required(VERSION 3.12)
project(theseus)

# Use C++20 features when compiling
set(CMAKE_CXX_STANDARD 20)

set(OpenGL_GL_PREFERENCE "GLVND")
find_package(OpenGL REQUIRED)

add_subdirectory(thirdparty/glfw)
add_subdirectory(thirdparty/glm)
add_subdirectory(thirdparty/glew/build/cmake)
add_subdirectory(thirdparty/yaml-cpp)

include_directories(
    src/
    src/builders
    src/components
    src/components/combat
    src/components/controllers
    src/components/interactables
    src/components/inventory
    src/components/renderable
    src/components/renderable/particles
    src/components/traps
    src/components/utility
    src/events
    src/managers
    src/globals
    src/builders
    src/states
    src/items
    wolf
    wolf/extensions
    thirdparty/
    thirdparty/entt
    thirdparty/soloud/include
    thirdparty/imgui
    thirdparty/glfw/include
    thirdparty/glew/include
    thirdparty/yaml-cpp/include
    ${OPENGL_INCLUDE_DIRS})

# Wolf
file(GLOB_RECURSE WOLF_SOURCE_FILES
	${CMAKE_SOURCE_DIR}/wolf/*.c
    ${CMAKE_SOURCE_DIR}/wolf/*.cpp)

file(GLOB_RECURSE WOLF_HEADER_FILES
	${CMAKE_SOURCE_DIR}/wolf/*.h
    ${CMAKE_SOURCE_DIR}/wolf/*.hpp)

# ImGui
file(GLOB_RECURSE IMGUI_SOURCES
    ${CMAKE_SOURCE_DIR}/thirdparty/imgui/*.cpp)
add_library(ImGui STATIC ${IMGUI_SOURCES})

# SoLoud
file(GLOB_RECURSE SOLOUD_SOURCES
    ${CMAKE_SOURCE_DIR}/thirdparty/soloud/src/*.cpp
    ${CMAKE_SOURCE_DIR}/thirdparty/soloud/src/*.c)
add_library(SoLoud STATIC ${SOLOUD_SOURCES})

# Game source and headers
file(GLOB_RECURSE SOURCE_FILES
	${CMAKE_SOURCE_DIR}/src/*.c
	${CMAKE_SOURCE_DIR}/src/*.cpp)
	
file(GLOB_RECURSE HEADER_FILES
	${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.hpp)

add_executable(theseus
    ${WOLF_SOURCE_FILES}
    ${WOLF_HEADER_FILES}
    ${IMGUI_SOURCES}
    ${SOLOUD_SOURCES}
    ${SOURCE_FILES}
    ${HEADER_FILES}
    )

# Remove console from windows builds
set(SUBSYSTEM_LINKER_OPTIONS "-mwindows")

target_link_options(theseus PRIVATE ${SUBSYSTEM_LINKER_OPTIONS})

target_link_libraries(theseus glfw glew yaml-cpp ${OPENGL_LIBRARIES} ${GLFW_LIBRARIES})
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
