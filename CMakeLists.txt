cmake_minimum_required(VERSION 3.20)
project(bdu CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(
        /W4 /WX
        /EHs-c- /GR-
    )
    add_compile_definitions(_HAS_EXCEPTIONS=0)
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic -Werror
        -fno-exceptions -fno-rtti
    )
endif()

add_executable(bdu
    src/main.cc
    src/bdu_options.cc
    src/size_calculator.cc
)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        v1.17.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(bdu_test
    tests/bdu_test.cc
    src/bdu_options.cc
    src/size_calculator.cc
)
target_link_libraries(bdu_test gtest_main)
add_test(NAME bdu_test COMMAND bdu_test)
