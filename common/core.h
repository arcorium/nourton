#pragma once

#ifdef _DEBUG
  #define AR_DEBUG 1
#else
#define AR_DEBUG 0
#endif

#ifdef _WIN32
  #define AR_WINDOWS 1
#else
#define AR_WINDOWS 0
#endif

#ifdef linux
#define AR_LINUX 1
#else
  #define AR_LINUX 0
#endif

#ifdef AR_DEBUG
#define AR_SOURCE_LOCATION_VAR sl
#define AR_SOURCE_LOCATION_VAR_2 , AR_SOURCE_LOCATION_VAR
#define AR_SOURCE_LOCATION_PARAM std::source_location AR_SOURCE_LOCATION_VAR = std::source_location::current()
#define AR_SOURCE_LOCATION_PARAM_2 , AR_SOURCE_LOCATION_PARAM
#define AR_SOURCE_LOCATION_PARAM_DECL std::source_location AR_SOURCE_LOCATION_VAR
#define AR_SOURCE_LOCATION_PARAM_2_DECL , AR_SOURCE_LOCATION_PARAM_DECL
#else
  #define AR_SOURCE_LOCATION_PARAM
  #define AR_SOURCE_LOCATION_PARAM_2
  #define AR_SOURCE_LOCATION_PARAM_DECL
  #define AR_SOURCE_LOCATION_PARAM_2_DECL
#endif

constexpr static std::string_view PROGRAM_NAME = "nourton";
constexpr static std::string_view PROGRAM_VERSION = "1.0.1";

constexpr static std::string_view PROGRAM_CLIENT_NAME = "nourton-client";
constexpr static std::string_view PROGRAM_SERVER_NAME = "nourton-server";