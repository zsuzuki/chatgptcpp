cmake_minimum_required(VERSION 3.20)
project(chatgptcpp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)

include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(nlohmann_json)

add_library(chatgptcpp
  src/client.cpp
)
target_include_directories(chatgptcpp
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_link_libraries(chatgptcpp
  PRIVATE
    CURL::libcurl
    nlohmann_json::nlohmann_json
)

add_executable(rock_paper_scissors
  examples/rock_paper_scissors.cpp
)
target_link_libraries(rock_paper_scissors
  PRIVATE
    chatgptcpp
)

enable_testing()
add_executable(chatgptcpp_tests
  tests/test_client.cpp
)
target_link_libraries(chatgptcpp_tests
  PRIVATE
    chatgptcpp
)
add_test(NAME chatgptcpp_tests COMMAND chatgptcpp_tests)
