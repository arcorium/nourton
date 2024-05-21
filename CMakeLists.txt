cmake_minimum_required(VERSION 3.15)

if (MSVC)
  if (POLICY CMP0091)
    cmake_policy(SET CMP0091 NEW)
  endif ()

  # Enable Hot Reload for MSVC compilers if supported.
  #  if (POLICY CMP0141)
  #    cmake_policy(SET CMP0141 NEW)
  #    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
  #  endif ()
endif ()

if (WIN32)
  set(VCPKG_TARGET_TRIPLET x64-windows-static)
elseif (UNIX)
  set(VCPKG_TARGET_TRIPLET x64-linux)
endif ()


project(nourton)

if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  message("--nourton--: installation is set on " ${CMAKE_CURRENT_BINARY_DIR}/nourton)
  set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR}/nourton CACHE PATH "install prefix" FORCE)
endif ()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
if (WIN32)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif ()

option(BUILD_TEST OFF)
option(BUILD_BENCH OFF)
option(ENABLE_INSTALL "install" ON)

add_subdirectory(thirdparty)

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(server)

if (BUILD_TEST)
  add_subdirectory(test)
endif ()

if (BUILD_BENCH)
  add_subdirectory(bench)
endif ()

if (ENABLE_INSTALL)
  message(" --nourton--: Install enabled")
  install(TARGETS nourton-client nourton-server DESTINATION .)
  if (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    install(DIRECTORY ./resource DESTINATION .)
  endif ()
  #    RUNTIME DESTINATION ./result)
endif ()