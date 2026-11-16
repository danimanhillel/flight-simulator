cmake_minimum_required(VERSION 3.10)
project(FlightSimulator)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)

include_directories(${SDL2_INCLUDE_DIRS})

add_executable(flightsim main.cpp)
target_link_libraries(flightsim ${SDL2_LIBRARIES} OpenGL::GL)

if(MSVC)
    target_compile_options(flightsim PRIVATE /O2)
else()
    target_compile_options(flightsim PRIVATE -O3)
endif()
