set(SOURCE_FILES src/aes/AES.cpp src/aes/AES.h)
add_library(aes STATIC ${SOURCE_FILES})

target_include_directories(aes PUBLIC src)


