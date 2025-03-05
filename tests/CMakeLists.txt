# CMakeLists.txt in tests/

add_library(test_utils utils.c)
target_link_libraries(test_utils err)

add_executable(executor_test executor_test.c)
target_link_libraries(executor_test executor mio future)

add_executable(hard_work_test hard_work_test.c)
target_link_libraries(hard_work_test executor mio future err)

add_executable(mio_test mio_test.c)
target_link_libraries(mio_test executor mio future test_utils)

add_executable(then_test then_test.c)
target_link_libraries(then_test executor mio future err test_utils)


enable_testing()
add_test(NAME ExecutorTest COMMAND executor_test)
add_test(NAME HardWorkTest COMMAND hard_work_test)
add_test(NAME MioTest COMMAND mio_test)
add_test(NAME ThenTest COMMAND then_test)
