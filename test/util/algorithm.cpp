#include <gtest/gtest.h>

#include <util/algorithm.h>

TEST(algorithm, gcd)
{
    auto result_1 = ar::gcd<u8>(12, 4);
    EXPECT_EQ(result_1, 4);

    auto result_2 = ar::gcd<u16>(12, 5);
    EXPECT_EQ(result_2, 1);

    auto result_3 = ar::gcd<u32>(103, 25);
    EXPECT_EQ(result_3, 1);

    auto result_4 = ar::gcd<u64>(120, 20);
    EXPECT_EQ(result_4, 20);

    auto result_5 = ar::gcd<u128>(98, 56);
    EXPECT_EQ(result_5, 14);


    auto result_6 = ar::gcd<u256>(12, 18);
    EXPECT_EQ(result_6, 6);

    auto result_7 = ar::gcd<u512>(42, 35);
    EXPECT_EQ(result_7, 7);

    auto result_8 = ar::gcd<u64>(99, 55);
    EXPECT_EQ(result_8, 11);

    auto result_9 = ar::gcd<u64>(1024, 600);
    EXPECT_EQ(result_9, 8);

    auto result_10 = ar::gcd<u64>(1777, 214);
    EXPECT_EQ(result_10, 1);

    // Test cases with zero
    auto result_11 = ar::gcd<u64>(12, 0);
    EXPECT_EQ(result_11, 12);

    auto result_12 = ar::gcd<u128>(0, 18);
    EXPECT_EQ(result_12, 18);

    auto result_13 = ar::gcd<u256>(0, 0);
    EXPECT_EQ(result_13, 0);
}

TEST(algorithm, gcd_extended)
{
    i32 x{}, y{};
    auto result = ar::gcd_extended<u32>(12, 4, x, y);
    EXPECT_EQ(12*x + 4*y, result);

    i128 x1{}, y1{};
    auto result_2 = ar::gcd_extended<u128>(103, 25, x1, y1);
    EXPECT_EQ(x1*103 + y1*25, result_2);
}

TEST(algorithm, mod_inverse)
{
    auto result_1 = ar::mod_inverse<u32>(12, 4);
    EXPECT_TRUE(!result_1.has_value());
    auto result_2 = ar::mod_inverse<u32>(12, 0);
    EXPECT_TRUE(!result_2.has_value());
    auto result_3 = ar::mod_inverse<u32>(0, 4);
    EXPECT_TRUE(!result_3.has_value());
    auto result_4 = ar::mod_inverse<u32>(0, 0);
    EXPECT_TRUE(!result_4.has_value());

    auto result_5 = ar::mod_inverse<u32>(1777, 214);
    EXPECT_TRUE(result_5.has_value());
    EXPECT_EQ(result_5.value(), 135);

    auto result_6 = ar::mod_inverse<u128>(103, 25);
    EXPECT_TRUE(result_6.value());
    EXPECT_EQ(result_6.value(), 17);
}

TEST(algorithm, random)
{
    auto result_1 = ar::random<u32>();
    auto result_2 = ar::random<u32>();
    ASSERT_NE(result_1, result_2);

    auto result_3 = ar::random<u32>();
    auto result_4 = ar::random<u64>();
    ASSERT_NE(result_3, result_4);
}

TEST(algorithm, random_bytes)
{
    auto result_1 = ar::random_bytes<16>();
    auto result_2 = ar::random_bytes<16>();
    EXPECT_EQ(result_1.size(), 16);
    EXPECT_EQ(result_2.size(), 16);

    for (usize i = 0; i < 16; ++i)
    {
        SCOPED_TRACE(i);
        EXPECT_NE(result_1[i], result_2[i]);
    }

    auto result_3 = ar::random_bytes<9>();
    EXPECT_EQ(result_3.size(), 9);
}

TEST(algorithm, is_prime)
{
    EXPECT_FALSE(ar::is_prime(1));
    EXPECT_FALSE(ar::is_prime(0));
    EXPECT_TRUE(ar::is_prime(2));
    EXPECT_TRUE(ar::is_prime(3));
    EXPECT_FALSE(ar::is_prime(15468));
    EXPECT_TRUE(ar::is_prime(15467));
}

TEST(algorithm, generate_prime_at_nth)
{
    auto result_1 = ar::nth_prime<u64>(124);
    EXPECT_TRUE(ar::is_prime(result_1));

    auto result_2 = ar::nth_prime<u128>(256);
    EXPECT_TRUE(ar::is_prime(result_2));

    auto result_3 = ar::nth_prime<u256>(1024);
    EXPECT_TRUE(ar::is_prime(result_3));
}

TEST(algorithm, mod_exponential)
{
    auto result_1 = ar::mod_exponential<u64>(2, 5, 13);
    EXPECT_EQ(result_1, 6);

    auto result_2 = ar::mod_exponential<u32>(11, 13, 19);
    EXPECT_EQ(result_2, 11);

    auto result_3 = ar::mod_exponential<u128>(105, 168, 138);
    EXPECT_EQ(result_3, u128{81});
}
