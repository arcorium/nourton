add_library(stb INTERFACE
  stb_image.h)

target_include_directories(stb INTERFACE .)
set_target_properties(stb PROPERTIES LINKER_LANGUAGE C)