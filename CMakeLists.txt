
cmake_minimum_required(VERSION 3.15...3.31)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 20)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")


# This is your project statement. You should always list languages;
# Listing the version is nice here since it sets lots of useful variables
project(
  sspge
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

# Adding something we can run - Output name matches target name
add_executable(sspge client/main.cpp ${SHARED})
add_executable(sspges server/main.cpp ${SHARED})

target_include_directories(sspge PRIVATE client shared)
target_include_directories(sspges PRIVATE server shared)

# Make sure you link your targets with this command. It can also link libraries and
# even flags, so linking a target that does not exist will not give a configure-time error.
target_link_libraries(sspge PRIVATE sfml-graphics sfml-window sfml-audio sfml-network)
target_link_libraries(sspges PRIVATE sfml-network)

target_link_libraries(sspge PUBLIC ImGui-SFML::ImGui-SFML)