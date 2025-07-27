
cmake_minimum_required(VERSION 3.10)

# Включаем кеширование исходников и артефактов FetchContent
if(DEFINED ENV{FETCHCONTENT_BASE_DIR})
  set(FETCHCONTENT_BASE_DIR "$ENV{FETCHCONTENT_BASE_DIR}" CACHE PATH "FetchContent base dir")
else()
  set(FETCHCONTENT_BASE_DIR "${CMAKE_BINARY_DIR}/.fc_cache" CACHE PATH "FetchContent base dir")
endif()
set(FETCHCONTENT_QUIET OFF)



project(crow_hello_world)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_STANDARD 17)


# Crow is header-only, but needs Boost
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

# --- cmark (system) ---
find_path(CMARK_INCLUDE_DIR cmark.h)
find_library(CMARK_LIB cmark)
if(NOT CMARK_INCLUDE_DIR OR NOT CMARK_LIB)
  message(FATAL_ERROR "cmark library not found. Please install libcmark-dev (Debian/Ubuntu) or libcmark (Alpine).")
endif()

# Включаем ccache для кеширования компиляции, если доступен
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
endif()

# Download Crow if not present
include(FetchContent)
FetchContent_Declare(
  crow
  GIT_REPOSITORY https://github.com/CrowCpp/crow.git
  GIT_TAG v1.0+1
)
set(CROW_BUILD_TESTS OFF)
FetchContent_MakeAvailable(crow)




# --- redis-plus-plus ---
set(REDIS_PLUS_PLUS_BUILD_TEST OFF)
include(FetchContent)
FetchContent_Declare(
  redis-plus-plus
  GIT_REPOSITORY https://github.com/sewenew/redis-plus-plus.git
  GIT_TAG 1.3.11
)
set(REDIS_PLUS_PLUS_BUILD_TEST OFF)
FetchContent_MakeAvailable(redis-plus-plus)

# --- spdlog ---
include(FetchContent)
FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.13.0
)
set(SPDLOG_BUILD_TESTS OFF)
set(SPDLOG_BUILD_SHARED ON)
set(SPDLOG_FMT_EXTERNAL OFF)
FetchContent_MakeAvailable(spdlog)

# Цель для сборки только зависимостей (без install)
add_custom_target(deps_only
  COMMAND ${CMAKE_COMMAND} --build . --target redis++ spdlog
  COMMENT "Build all dependencies only"
)



###########################################################
# nlohmann_json
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)


# Основная цель проекта (только если есть main.cpp)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
  add_executable(main main.cpp)
  # Crow — header-only, просто добавляем include
  target_include_directories(main PRIVATE ${crow_SOURCE_DIR}/include)

  # Добавляем флаги компилятора
  target_compile_options(main PRIVATE -Wall -Wextra)

  target_include_directories(main PRIVATE ${redis-plus-plus_SOURCE_DIR}/include /usr/include/hiredis)

  # hiredis
  find_library(HIREDIS_LIB hiredis)
  target_link_libraries(main PRIVATE ${HIREDIS_LIB} redis++)

  # curl
  find_package(CURL REQUIRED)
  target_include_directories(main PRIVATE ${CURL_INCLUDE_DIRS})
  target_link_libraries(main PRIVATE ${CURL_LIBRARIES})

  target_include_directories(main PRIVATE ${CMARK_INCLUDE_DIR})
  target_link_libraries(main PRIVATE spdlog::spdlog ${CMARK_LIB})

  # nlohmann_json (header-only)
  target_include_directories(main PRIVATE ${nlohmann_json_SOURCE_DIR}/include)
endif()
