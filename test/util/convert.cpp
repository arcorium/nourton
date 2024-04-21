//
// Created by mizzh on 4/21/2024.
//

#include <gtest/gtest.h>

#include <util/types.h>
#include <util/convert.h>
#include <crypto/camellia.h>

TEST(byte, combine)
{
  u64 val = 0x1122334455667788;

  u32 lsb = val & ar::MASK_32BIT;
  u32 msb = static_cast<u32>(val >> 32);
  EXPECT_EQ(lsb, 0x55667788);
  EXPECT_EQ(msb, 0x11223344);

  auto result = ar::combine<u64>(lsb, msb);
  ASSERT_EQ(result, val);
}

TEST(byte, u128_downcast_by_convert)
{
  constexpr u16 VAL16 = std::numeric_limits<u16>::max() - 123;

  u128 val_16 = VAL16;
  auto converted_16 = val_16.convert_to<u16>();
  EXPECT_EQ(VAL16, converted_16);


  constexpr u16 VAL64 = std::numeric_limits<u64>::max() - 123;
  u128 val_64 = VAL64;
  auto converted_64 = val_64.convert_to<u64>();
  EXPECT_EQ(VAL64, converted_64);
}

TEST(convert, as_bytes)
{
  u64 val[1] = {0x1122334455667788};

  auto bytes = std::as_bytes(std::span{val});
  auto bytes2 = ar::as_span<const u8>(val[0]);

  for (usize i = 0; i < 8; ++i)
  {
    auto a = static_cast<u8>(bytes[i]);
    auto b = bytes2[i];
    ASSERT_EQ(a, b);
  }

  ASSERT_EQ(&val[0], (u64*)bytes2.data());
  EXPECT_EQ(&val[0], (u64*)bytes.data());

  // u128
}

TEST(convert, to_128)
{
  std::string_view text{"Hello my name is"};
  auto text_span = ar::as_span(text);
  // auto val = ar::to_128(text_span);

  // u128 val{0x1122334455667788};
}

TEST(convert, combine_to_bytes)
{
  u128 val{"22774453838368691933757882222884355840"};
  // Split
  u64 msb = (val & std::numeric_limits<u64>::max()).convert_to<u64>();
  u64 lsb = (val >> 64).convert_to<u64>();
  EXPECT_EQ(lsb, 0x1122334455667788);
  EXPECT_EQ(msb, 0x99AABBCCDDEEFF00);

  auto res = ar::combine_to_bytes(lsb, msb);
  auto res2 = ar::rawToBoost_uint128(res.data());

  // std::cout << std::hex << "VAL: " << val << std::endl;
  // std::cout << "RES: " << res2 << std::endl;


  ASSERT_EQ(res2, val);
}
