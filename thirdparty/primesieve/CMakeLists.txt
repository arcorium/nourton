project(primesieve CXX)

set(PRIMESIEVE_VERSION "12.3")
set(PRIMESIEVE_SOVERSION "12.3.0")

# Build options ######################################################

option(BUILD_PRIMESIEVE "Build primesieve binary" ON)
option(BUILD_STATIC_LIBS "Build static libprimesieve" ON)
option(BUILD_TESTS "Build test programs" OFF)

# By default we enable building the shared libprimesieve library.
# However, the Emscripten WebAssembly compiler does not
# support shared libraries, hence we disable building the
# shared libprimesieve when using Emscripten.
get_property(SHARED_LIBS_SUPPORTED GLOBAL PROPERTY TARGET_SUPPORTS_SHARED_LIBS)
if (SHARED_LIBS_SUPPORTED)
  option(BUILD_SHARED_LIBS "Build shared libprimesieve" ON)
else ()
  option(BUILD_SHARED_LIBS "Build shared libprimesieve" OFF)
endif ()

option(WITH_AUTO_VECTORIZATION "Enable compiler auto-vectorization" ON)
option(WITH_MULTIARCH "Enable runtime dispatching to fastest supported CPU instruction set" ON)
option(WITH_MSVC_CRT_STATIC "Link primesieve.lib with /MT instead of the default /MD" OFF)

# libprimesieve sanity check #########################################

if (NOT BUILD_SHARED_LIBS AND NOT BUILD_STATIC_LIBS)
  message(FATAL_ERROR "One or both of BUILD_SHARED_LIBS or BUILD_STATIC_LIBS must be set to ON")
endif ()

# Set default build type to Release ##################################

if (NOT CMAKE_VERSION VERSION_LESS 3.9)
  get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
elseif (CMAKE_CONFIGURATION_TYPES)
  set(isMultiConfig TRUE)
endif ()

if (NOT isMultiConfig AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif ()

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(ENABLE_ASSERT "ENABLE_ASSERT")
endif ()

# primesieve binary source files #####################################

set(BIN_SRC src/app/CmdOptions.cpp
  src/app/help.cpp
  src/app/main.cpp
  src/app/stressTest.cpp
  src/app/test.cpp)

# primesieve library source files ####################################

set(LIB_SRC src/api-c.cpp
  src/api.cpp
  src/CountPrintPrimes.cpp
  src/CpuInfo.cpp
  src/Erat.cpp
  src/EratSmall.cpp
  src/EratMedium.cpp
  src/EratBig.cpp
  src/iterator-c.cpp
  src/iterator.cpp
  src/IteratorHelper.cpp
  src/LookupTables.cpp
  src/MemoryPool.cpp
  src/PrimeGenerator.cpp
  src/nthPrime.cpp
  src/ParallelSieve.cpp
  src/popcount.cpp
  src/PreSieve.cpp
  src/PrimeSieve.cpp
  src/RiemannR.cpp
  src/SievingPrimes.cpp)

# Required includes ##################################################

include(CMakePackageConfigHelpers)

# Check if libatomic is needed (old 32-bit systems) ##################

include("${PROJECT_SOURCE_DIR}/cmake/libatomic.cmake")

# Check if compiler supports auto vectorization ######################

if (WITH_AUTO_VECTORIZATION)
  include("${PROJECT_SOURCE_DIR}/cmake/auto_vectorization.cmake")
endif ()

# Check if compiler supports x64 multiarch ###########################

if (WITH_MULTIARCH)
  include("${PROJECT_SOURCE_DIR}/cmake/multiarch_avx512_vbmi2.cmake")
endif ()

# libprimesieve (shared library) #####################################

find_package(Threads REQUIRED QUIET)


# libprimesieve-static ###############################################

add_library(libprimesieve-static STATIC ${LIB_SRC})
set_target_properties(libprimesieve-static PROPERTIES OUTPUT_NAME primesieve)
target_link_libraries(libprimesieve-static PRIVATE Threads::Threads ${LIBATOMIC})
target_compile_options(libprimesieve-static PRIVATE ${FTREE_VECTORIZE_FLAG} ${FVECT_COST_MODEL_FLAG})
target_compile_definitions(libprimesieve-static PRIVATE "${ENABLE_ASSERT}" "${ENABLE_MULTIARCH_AVX512}")

if (WITH_MSVC_CRT_STATIC)
  set_target_properties(libprimesieve-static PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
endif ()

if (TARGET libprimesieve)
  add_dependencies(libprimesieve-static libprimesieve)
endif ()

target_compile_features(libprimesieve-static
  PUBLIC
  cxx_alias_templates
  PRIVATE
  cxx_constexpr
  cxx_uniform_initialization
  cxx_lambdas)

target_include_directories(libprimesieve-static PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
#
#install(TARGETS libprimesieve-static
#	EXPORT primesieveStatic
#	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
#	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
#	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Static or shared linking ###########################################

# On Unix-like OSes we use shared linking against libprimesieve by
# default mainly because this is required by Linux distributions.
# On Windows we use static linking by default because the library's
# path is not encoded into the binary. This means that the binary
# will only work if the DLL is in the same directory or a directory
# that is in the PATH environment variable.

if (WIN32 AND TARGET libprimesieve-static)
  set(STATIC_LINKING ON)
elseif (NOT TARGET libprimesieve)
  set(STATIC_LINKING ON)
endif ()

add_library(primesieve::primesieve ALIAS libprimesieve-static)