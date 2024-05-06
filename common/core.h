#pragma once

#ifdef _DEBUG
  #define AR_DEBUG 1
#else
  #define AR_DEBUG 0
#endif

constexpr static std::string_view PROGRAM_NAME = "nourton";
constexpr static std::string_view PROGRAM_VERSION = "1.0";

constexpr static std::string_view PROGRAM_CLIENT_NAME = "nourton-client";
constexpr static std::string_view PROGRAM_SERVER_NAME = "nourton-server";
