
enable_testing()

find_package(GTest REQUIRED)

add_executable(crypto_test crypto/camelia.cpp
	crypto/rsa.cpp)
target_link_libraries(crypto_test PRIVATE nourton-common GTest::gtest GTest::gtest_main)

add_executable(util_test util/convert.cpp util/algorithm.cpp)
target_link_libraries(util_test PRIVATE nourton-common GTest::gtest GTest::gtest_main)