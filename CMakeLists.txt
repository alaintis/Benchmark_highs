cmake_minimum_required(VERSION 3.15)
project(highs_example LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Tell CMake where to find HiGHS
find_package(Highs REQUIRED
    PATHS $ENV{HOME}/highs-install
)


add_executable(solve
    problem_reader.cpp
    testing.cpp
)

target_link_libraries(solve PRIVATE highs::highs)