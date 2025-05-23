
cmake_minimum_required(VERSION 3.15...3.31)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

project(
  learning
  VERSION 1.0
  LANGUAGES CXX)

include(FetchContent)
FetchContent_Declare(sfml
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 3.0.0
  GIT_SHALLOW ON
  EXCLUDE_FROM_ALL
  SYSTEM)
FetchContent_MakeAvailable(sfml)

# Dear ImGui
FetchContent_Declare(
  imgui
  URL "https://github.com/ocornut/imgui/archive/v1.91.1.zip"
)
FetchContent_MakeAvailable(imgui)

# ImGui-SFML
FetchContent_Declare(
  imgui-sfml
  GIT_REPOSITORY https://github.com/SFML/imgui-sfml.git
  GIT_TAG        v3.0
)
set(IMGUI_DIR ${imgui_SOURCE_DIR})
set(IMGUI_SFML_FIND_SFML OFF)
FetchContent_MakeAvailable(imgui-sfml)

set(SHARED shared/easylogging++.cc)


add_executable(balls src/balls.cpp ${SHARED})
add_executable(pid src/pid.cpp ${SHARED})

target_include_directories(balls PRIVATE src shared)
target_include_directories(pid PRIVATE src shared)

target_link_libraries(balls PRIVATE sfml-graphics sfml-window sfml-audio sfml-network)
target_link_libraries(pid PRIVATE sfml-graphics sfml-window sfml-audio sfml-network)

target_link_libraries(balls PUBLIC ImGui-SFML::ImGui-SFML)
target_link_libraries(pid PUBLIC ImGui-SFML::ImGui-SFML)