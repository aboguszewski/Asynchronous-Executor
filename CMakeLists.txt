cmake_minimum_required(VERSION 3.10)

project(to_c_io VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD "11")

# We're using ASAN (Address Sanitizer), so that debugging memory corruptions should be easier.
# Make sure to test your program without `-fsanitize=address`, too!
set(CMAKE_C_FLAGS "-g -Wall -Wextra -Wno-sign-compare -Wno-unused-parameter -Wuninitialized -Wmissing-field-initializers -fsanitize=address")

include_directories(include)
include_directories(src)

add_library(err src/err.c)
add_library(mio src/mio.c)
add_library(future src/future_combinators.c src/future_examples.c)
add_library(executor src/executor.c)

target_link_libraries(mio PRIVATE err)
target_link_libraries(future PRIVATE mio)
target_link_libraries(executor PRIVATE future)
# target_link_libraries(executor PRIVATE mio future err)

add_subdirectory(tests)
