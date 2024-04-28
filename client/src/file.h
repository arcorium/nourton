#pragma once
#include <string>
#include <user.h>

#include "util/types.h"

namespace ar
{
 enum class FileFlag
 {
  Sent,
  Received
 };

 enum class FileFormat
 {
  None,
  Image,
  Archive,
  Document,
  Other,
 };

 struct FileProperty
 {
  std::string filename;
  usize size; // in byte
  FileFormat format;
  User* sender; // sender == nullptr, means the sender is itself
 };

 static constexpr std::string_view get_filename(std::string_view fullpath) noexcept
 {
  auto idx = fullpath.find_last_of('/');
  if (idx == std::string::npos)
  {
   idx = fullpath.find_last_of('\\');
   if (idx == std::string::npos)
    return fullpath;
  }
  auto last_idx = fullpath.find_last_of('.');
  if (last_idx == std::string::npos)
   return fullpath.substr(idx + 1);

  return fullpath.substr(idx + 1, last_idx - idx - 1);
 }

 // WARN: doesn't support multiple format file like .tar.xz or .7z.001
 static constexpr std::tuple<FileFormat, std::string_view> get_file_format(std::string_view filepath) noexcept
 {
  auto last_idx = filepath.find_last_of('.');
  if (last_idx == std::string::npos)
   return std::make_tuple(FileFormat::None, ""sv);

  std::string_view format_str = filepath.substr(last_idx + 1);
  constexpr static std::string_view image_formats[]{
   "jpeg"sv,
   "jpg"sv,
   "png"sv,
   "gif"sv,
   "tiff"sv,
   "bmp"sv,
   "svg"sv,
   "webp"sv,
   "heif"sv,
   "heic"sv,
   "raw"sv,
   "dds"sv,
  };

  constexpr static std::string_view archive_formats[]{
   "zip"sv,
   "rar"sv,
   "7z"sv,
   "tar"sv,
   "gz"sv,
   "bz2"sv,
   "cab"sv,
   "iso"sv,
   "arj"sv,
   "xz"sv,
  };

  constexpr static std::string_view document_formats[]{
   "pdf"sv,
   "docx"sv,
   "doc"sv,
   "xlsx"sv,
   "xls"sv,
   "pptx"sv,
   "ppt"sv,
   "odt"sv,
   "ods"sv,
   "odp"sv,
   "txt"sv,
  };

  if (std::ranges::any_of(image_formats, [&](std::string_view format) { return format == format_str; }))
   return std::make_tuple(FileFormat::Image, format_str);
  if (std::ranges::any_of(archive_formats, [&](std::string_view format) { return format == format_str; }))
   return std::make_tuple(FileFormat::Archive, format_str);
  if (std::ranges::any_of(document_formats, [&](std::string_view format) { return format == format_str; }))
   return std::make_tuple(FileFormat::Document, format_str);
  return std::make_tuple(FileFormat::Other, format_str);
 }
}
