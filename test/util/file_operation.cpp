#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "util/file.h"

TEST(file_operation, read_as_byte)
{
  using namespace std::literals;

  auto filepath = "../../resource/image/docs.png"sv;

  auto result = ar::read_file_as_bytes("../../resource/image/docs22.png"sv);
  ASSERT_FALSE(result.has_value());
  result = ar::read_file_as_bytes(filepath);
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(std::filesystem::file_size(filepath), result->size());
}

TEST(file_operation, writing)
{
  using namespace std::literals;
  auto filepath = "../../resource/image/docs.png"sv;
  auto dest_path = "../../resource/image/docs_copy.png"sv;

  auto result = ar::read_file_as_bytes(filepath);
  ASSERT_TRUE(result.has_value());

  auto res = ar::save_bytes_as_file(dest_path, result.value());
  ASSERT_TRUE(res);

  // the file already exists
  res = ar::save_bytes_as_file(dest_path, result.value());
  ASSERT_FALSE(res);

  // Check size
  EXPECT_EQ(std::filesystem::file_size(filepath), std::filesystem::file_size(dest_path));

  // remove file
  EXPECT_TRUE(std::filesystem::remove(dest_path));
}
