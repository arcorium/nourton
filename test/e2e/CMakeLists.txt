
add_executable(e2e_test test.cpp)

target_link_libraries(e2e_test PRIVATE nourton-common GTest::gtest GTest::gtest_main)
