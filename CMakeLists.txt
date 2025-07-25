cmake_minimum_required(VERSION 3.10)
project(crow_hello_world)

set(CMAKE_CXX_STANDARD 17)

# Crow is header-only, but needs Boost
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# Download Crow if not present
include(FetchContent)
FetchContent_Declare(
  crow
  GIT_REPOSITORY https://github.com/CrowCpp/crow.git
  GIT_TAG v1.0+1
)
FetchContent_MakeAvailable(crow)

add_executable(main main.cpp)
# Crow — header-only, просто добавляем include
target_include_directories(main PRIVATE ${crow_SOURCE_DIR}/include)

# Добавляем флаги компилятора
target_compile_options(main PRIVATE -Wall -Wextra)
