add_executable(nourton-client
  src/main.cpp
  src/client.cpp
  src/client.h
  src/application.h
  src/window.cpp
  src/window.h
  src/connection.h
  src/connection.cpp
  src/state.cpp
  src/util/imgui.h
  src/handler.h
  src/widget.h
  src/application.cpp
  src/resource.cpp
  src/resource.h
  src/file.h
  src/util/safe.h
)

target_link_libraries(nourton-client PUBLIC imgui nourton-common glad icon_font tinyfd stb)

# copy resource dir
add_custom_command(TARGET nourton-client POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/resource $<TARGET_FILE_DIR:nourton-client>/resource)