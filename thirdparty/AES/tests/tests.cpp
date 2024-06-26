#include <iostream>
#include <vector>

#include "../src/AES.h"
#include "gtest/gtest.h"

const unsigned int BLOCK_BYTES_LENGTH = 16 * sizeof(unsigned char);

TEST(KeyLengths, KeyLength128) {
  AES aes(AESKeyLength::AES_128);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  unsigned char right[] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
                           0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};
  unsigned char *out = aes.EncryptECB(plain, BLOCK_BYTES_LENGTH, key);

  ASSERT_FALSE(memcmp(right, out, BLOCK_BYTES_LENGTH));
  delete[] out;
}

TEST(KeyLengths, KeyLength192) {
  AES aes(AESKeyLength::AES_192);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                         0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17};
  unsigned char right[] = {0xdd, 0xa9, 0x7c, 0xa4, 0x86, 0x4c, 0xdf, 0xe0,
                           0x6e, 0xaf, 0x70, 0xa0, 0xec, 0x0d, 0x71, 0x91};

  unsigned char *out = aes.EncryptECB(plain, BLOCK_BYTES_LENGTH, key);
  ASSERT_FALSE(memcmp(right, out, BLOCK_BYTES_LENGTH));
  delete[] out;
}

TEST(KeyLengths, KeyLength256) {
  AES aes(AESKeyLength::AES_256);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                         0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                         0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
  unsigned char right[] = {0x8e, 0xa2, 0xb7, 0xca, 0x51, 0x67, 0x45, 0xbf,
                           0xea, 0xfc, 0x49, 0x90, 0x4b, 0x49, 0x60, 0x89};

  unsigned char *out = aes.EncryptECB(plain, BLOCK_BYTES_LENGTH, key);
  ASSERT_FALSE(memcmp(right, out, BLOCK_BYTES_LENGTH));
  delete[] out;
}

TEST(ECB, EncryptDecryptOneBlock) {
  AES aes(AESKeyLength::AES_256);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                         0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                         0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

  unsigned char *out = aes.EncryptECB(plain, BLOCK_BYTES_LENGTH, key);
  unsigned char *innew = aes.DecryptECB(out, BLOCK_BYTES_LENGTH, key);
  ASSERT_FALSE(memcmp(innew, plain, BLOCK_BYTES_LENGTH));
  delete[] out;
  delete[] innew;
}

TEST(ECB, EncryptDecryptVectorOneBlock) {
  AES aes(AESKeyLength::AES_256);
  std::vector<unsigned char> plain = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                                      0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
                                      0xcc, 0xdd, 0xee, 0xff};

  std::vector<unsigned char> key = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
      0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
      0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

  std::vector<unsigned char> out = aes.EncryptECB(plain, key);
  std::vector<unsigned char> innew = aes.DecryptECB(out, key);
  ASSERT_EQ(innew, plain);
}

TEST(ECB, OneBlockEncrypt) {
  AES aes(AESKeyLength::AES_128);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  unsigned char right[] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
                           0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};
  unsigned char *out = aes.EncryptECB(plain, BLOCK_BYTES_LENGTH, key);

  ASSERT_FALSE(memcmp(right, out, BLOCK_BYTES_LENGTH));

  delete[] out;
}

TEST(ECB, OneBlockEncryptVector) {
  AES aes(AESKeyLength::AES_128);
  std::vector<unsigned char> plain = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                                      0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
                                      0xcc, 0xdd, 0xee, 0xff};
  std::vector<unsigned char> key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                                    0x0c, 0x0d, 0x0e, 0x0f};
  std::vector<unsigned char> right = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b,
                                      0x04, 0x30, 0xd8, 0xcd, 0xb7, 0x80,
                                      0x70, 0xb4, 0xc5, 0x5a};
  std::vector<unsigned char> out = aes.EncryptECB(plain, key);

  ASSERT_EQ(right, out);
}

TEST(ECB, OneBlockWithoutByteEncrypt) {
  AES aes(AESKeyLength::AES_128);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  EXPECT_THROW(
      aes.EncryptECB(plain, (BLOCK_BYTES_LENGTH - 1 * sizeof(unsigned char)),
                     key),
      std::length_error);
}

TEST(ECB, OneBlockPlusOneByteEncrypt) {
  AES aes(AESKeyLength::AES_128);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
                           0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0xaa};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

  EXPECT_THROW(
      aes.EncryptECB(plain, (BLOCK_BYTES_LENGTH + 1) * sizeof(unsigned char),
                     key),
      std::length_error);
}

TEST(ECB, TwoBlocksEncrypt) {
  AES aes(AESKeyLength::AES_128);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                           0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                           0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  unsigned char right[] = {
      0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30, 0xd8, 0xcd, 0xb7,
      0x80, 0x70, 0xb4, 0xc5, 0x5a, 0x07, 0xfe, 0xef, 0x74, 0xe1, 0xd5,
      0x03, 0x6e, 0x90, 0x0e, 0xee, 0x11, 0x8e, 0x94, 0x92, 0x93,
  };
  unsigned char *out = aes.EncryptECB(plain, 2 * BLOCK_BYTES_LENGTH, key);

  ASSERT_FALSE(memcmp(right, out, 2 * BLOCK_BYTES_LENGTH));

  delete[] out;
}

TEST(ECB, OneBlockDecrypt) {
  AES aes(AESKeyLength::AES_128);
  unsigned char encrypted[] = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30,
                               0xd8, 0xcd, 0xb7, 0x80, 0x70, 0xb4, 0xc5, 0x5a};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  unsigned char right[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  unsigned char *out = aes.DecryptECB(encrypted, BLOCK_BYTES_LENGTH, key);

  ASSERT_FALSE(memcmp(right, out, BLOCK_BYTES_LENGTH));

  delete[] out;
}

TEST(ECB, OneBlockDecryptVector) {
  AES aes(AESKeyLength::AES_128);
  std::vector<unsigned char> encrypted = {0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b,
                                          0x04, 0x30, 0xd8, 0xcd, 0xb7, 0x80,
                                          0x70, 0xb4, 0xc5, 0x5a};
  std::vector<unsigned char> key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                                    0x0c, 0x0d, 0x0e, 0x0f};
  std::vector<unsigned char> right = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                                      0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
                                      0xcc, 0xdd, 0xee, 0xff};
  std::vector<unsigned char> out = aes.DecryptECB(encrypted, key);

  ASSERT_EQ(right, out);
}

TEST(ECB, TwoBlocksDecrypt) {
  AES aes(AESKeyLength::AES_128);
  unsigned char encrypted[] = {
      0x69, 0xc4, 0xe0, 0xd8, 0x6a, 0x7b, 0x04, 0x30, 0xd8, 0xcd, 0xb7,
      0x80, 0x70, 0xb4, 0xc5, 0x5a, 0x07, 0xfe, 0xef, 0x74, 0xe1, 0xd5,
      0x03, 0x6e, 0x90, 0x0e, 0xee, 0x11, 0x8e, 0x94, 0x92, 0x93,
  };
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  unsigned char right[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                           0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                           0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};
  unsigned char *out = aes.DecryptECB(encrypted, 2 * BLOCK_BYTES_LENGTH, key);

  ASSERT_FALSE(memcmp(right, out, 2 * BLOCK_BYTES_LENGTH));

  delete[] out;
}

TEST(CBC, EncryptDecrypt) {
  AES aes(AESKeyLength::AES_256);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  unsigned char iv[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                         0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                         0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

  unsigned char *out = aes.EncryptCBC(plain, BLOCK_BYTES_LENGTH, key, iv);
  unsigned char *innew = aes.DecryptCBC(out, BLOCK_BYTES_LENGTH, key, iv);
  ASSERT_FALSE(memcmp(innew, plain, BLOCK_BYTES_LENGTH));
  delete[] out;
  delete[] innew;
}

TEST(CBC, EncryptDecryptVector) {
  AES aes(AESKeyLength::AES_256);
  std::vector<unsigned char> plain = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                                      0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
                                      0xcc, 0xdd, 0xee, 0xff};
  std::vector<unsigned char> iv = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff};
  std::vector<unsigned char> key = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
      0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
      0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

  std::vector<unsigned char> out = aes.EncryptCBC(plain, key, iv);
  std::vector<unsigned char> innew = aes.DecryptCBC(out, key, iv);
  ASSERT_EQ(innew, plain);
}

TEST(CBC, TwoBlocksEncrypt) {
  AES aes(AESKeyLength::AES_128);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                           0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  unsigned char iv[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  unsigned char right[] = {0x1b, 0x87, 0x23, 0x78, 0x79, 0x5f, 0x4f, 0xfd,
                           0x77, 0x28, 0x55, 0xfc, 0x87, 0xca, 0x96, 0x4d,
                           0x4c, 0x5b, 0xca, 0x1c, 0x48, 0xcd, 0x88, 0x00,
                           0x3a, 0x10, 0x52, 0x11, 0x88, 0x12, 0x5e, 0x00};

  unsigned char *out = aes.EncryptCBC(plain, 2 * BLOCK_BYTES_LENGTH, key, iv);
  ASSERT_FALSE(memcmp(out, right, 2 * BLOCK_BYTES_LENGTH));
  delete[] out;
}

TEST(CBC, TwoBlocksEncryptVector) {
  AES aes(AESKeyLength::AES_128);
  std::vector<unsigned char> plain = {
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa,
      0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
      0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  std::vector<unsigned char> iv = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff};
  std::vector<unsigned char> key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                                    0x0c, 0x0d, 0x0e, 0x0f};
  std::vector<unsigned char> right = {
      0x1b, 0x87, 0x23, 0x78, 0x79, 0x5f, 0x4f, 0xfd, 0x77, 0x28, 0x55,
      0xfc, 0x87, 0xca, 0x96, 0x4d, 0x4c, 0x5b, 0xca, 0x1c, 0x48, 0xcd,
      0x88, 0x00, 0x3a, 0x10, 0x52, 0x11, 0x88, 0x12, 0x5e, 0x00};

  std::vector<unsigned char> out = aes.EncryptCBC(plain, key, iv);

  ASSERT_EQ(out, right);
}

TEST(CBC, TwoBlocksDecrypt) {
  AES aes(AESKeyLength::AES_128);
  unsigned char encrypted[] = {0x1b, 0x87, 0x23, 0x78, 0x79, 0x5f, 0x4f, 0xfd,
                               0x77, 0x28, 0x55, 0xfc, 0x87, 0xca, 0x96, 0x4d,
                               0x4c, 0x5b, 0xca, 0x1c, 0x48, 0xcd, 0x88, 0x00,
                               0x3a, 0x10, 0x52, 0x11, 0x88, 0x12, 0x5e, 0x00};

  unsigned char iv[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  unsigned char right[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                           0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

  unsigned char *out =
      aes.DecryptCBC(encrypted, 2 * BLOCK_BYTES_LENGTH, key, iv);

  ASSERT_FALSE(memcmp(out, right, 2 * BLOCK_BYTES_LENGTH));
  delete[] out;
}

TEST(CBC, TwoBlocksDecryptVector) {
  AES aes(AESKeyLength::AES_128);
  std::vector<unsigned char> encrypted = {
      0x1b, 0x87, 0x23, 0x78, 0x79, 0x5f, 0x4f, 0xfd, 0x77, 0x28, 0x55,
      0xfc, 0x87, 0xca, 0x96, 0x4d, 0x4c, 0x5b, 0xca, 0x1c, 0x48, 0xcd,
      0x88, 0x00, 0x3a, 0x10, 0x52, 0x11, 0x88, 0x12, 0x5e, 0x00};

  std::vector<unsigned char> iv = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff};
  std::vector<unsigned char> key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                                    0x0c, 0x0d, 0x0e, 0x0f};
  std::vector<unsigned char> right = {
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa,
      0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
      0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

  std::vector<unsigned char> out = aes.DecryptCBC(encrypted, key, iv);

  ASSERT_EQ(out, right);
}

TEST(CFB, EncryptDecrypt) {
  AES aes(AESKeyLength::AES_256);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  unsigned char iv[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                         0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                         0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

  unsigned char *out = aes.EncryptCFB(plain, BLOCK_BYTES_LENGTH, key, iv);
  unsigned char *innew = aes.DecryptCFB(out, BLOCK_BYTES_LENGTH, key, iv);
  ASSERT_FALSE(memcmp(innew, plain, BLOCK_BYTES_LENGTH));
  delete[] out;
  delete[] innew;
}

TEST(CFB, EncryptDecryptVector) {
  AES aes(AESKeyLength::AES_256);
  std::vector<unsigned char> plain = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
                                      0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb,
                                      0xcc, 0xdd, 0xee, 0xff};
  std::vector<unsigned char> iv = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff};
  std::vector<unsigned char> key = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
      0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
      0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

  std::vector<unsigned char> out = aes.EncryptCFB(plain, key, iv);
  std::vector<unsigned char> innew = aes.DecryptCFB(out, key, iv);
  ASSERT_EQ(innew, plain);
}

TEST(CFB, EncryptTwoBlocks) {
  AES aes(AESKeyLength::AES_128);
  unsigned char plain[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                           0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  unsigned char iv[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  unsigned char right[] = {0x3c, 0x55, 0x3d, 0x01, 0x8a, 0x52, 0xe4, 0x54,
                           0xec, 0x4e, 0x08, 0x22, 0xc2, 0x8d, 0x55, 0xec,
                           0xe3, 0x5a, 0x40, 0xab, 0x30, 0x29, 0xf3, 0x0c,
                           0xe1, 0xdb, 0x30, 0x6c, 0xa1, 0x05, 0xcb, 0xa9};

  unsigned char *out = aes.EncryptCFB(plain, 2 * BLOCK_BYTES_LENGTH, key, iv);
  ASSERT_FALSE(memcmp(right, out, 2 * BLOCK_BYTES_LENGTH));
  delete[] out;
}

TEST(CFB, EncryptTwoBlocksVector) {
  AES aes(AESKeyLength::AES_128);
  std::vector<unsigned char> plain = {
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa,
      0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
      0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
  std::vector<unsigned char> iv = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff};
  std::vector<unsigned char> key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                                    0x0c, 0x0d, 0x0e, 0x0f};
  std::vector<unsigned char> right = {
      0x3c, 0x55, 0x3d, 0x01, 0x8a, 0x52, 0xe4, 0x54, 0xec, 0x4e, 0x08,
      0x22, 0xc2, 0x8d, 0x55, 0xec, 0xe3, 0x5a, 0x40, 0xab, 0x30, 0x29,
      0xf3, 0x0c, 0xe1, 0xdb, 0x30, 0x6c, 0xa1, 0x05, 0xcb, 0xa9};

  std::vector<unsigned char> out = aes.EncryptCFB(plain, key, iv);
  ASSERT_EQ(right, out);
}

TEST(CFB, DecryptTwoBlocks) {
  AES aes(AESKeyLength::AES_128);
  std::vector<unsigned char> encrypted = {
      0x3c, 0x55, 0x3d, 0x01, 0x8a, 0x52, 0xe4, 0x54, 0xec, 0x4e, 0x08,
      0x22, 0xc2, 0x8d, 0x55, 0xec, 0xe3, 0x5a, 0x40, 0xab, 0x30, 0x29,
      0xf3, 0x0c, 0xe1, 0xdb, 0x30, 0x6c, 0xa1, 0x05, 0xcb, 0xa9};
  std::vector<unsigned char> iv = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                   0xff, 0xff, 0xff, 0xff};
  std::vector<unsigned char> key = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05,
                                    0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
                                    0x0c, 0x0d, 0x0e, 0x0f};
  std::vector<unsigned char> right = {
      0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x99, 0xaa,
      0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55,
      0x66, 0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

  std::vector<unsigned char> out = aes.DecryptCFB(encrypted, key, iv);
  ASSERT_EQ(right, out);
}

TEST(CFB, DecryptTwoBlocksVector) {
  AES aes(AESKeyLength::AES_128);
  unsigned char encrypted[] = {0x3c, 0x55, 0x3d, 0x01, 0x8a, 0x52, 0xe4, 0x54,
                               0xec, 0x4e, 0x08, 0x22, 0xc2, 0x8d, 0x55, 0xec,
                               0xe3, 0x5a, 0x40, 0xab, 0x30, 0x29, 0xf3, 0x0c,
                               0xe1, 0xdb, 0x30, 0x6c, 0xa1, 0x05, 0xcb, 0xa9};
  unsigned char iv[] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
  unsigned char right[] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
                           0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                           0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

  unsigned char *out =
      aes.DecryptCFB(encrypted, 2 * BLOCK_BYTES_LENGTH, key, iv);
  ASSERT_FALSE(memcmp(right, out, 2 * BLOCK_BYTES_LENGTH));
  delete[] out;
}

TEST(LongData, EncryptDecryptOneKb) {
  AES aes(AESKeyLength::AES_256);
  unsigned int kbSize = 1024 * sizeof(unsigned char);
  unsigned char *plain = new unsigned char[kbSize];
  for (unsigned int i = 0; i < kbSize; i++) {
    plain[i] = i % 256;
  }

  unsigned char key[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
                         0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
                         0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

  unsigned char *out = aes.EncryptECB(plain, kbSize, key);
  unsigned char *innew = aes.DecryptECB(out, kbSize, key);
  ASSERT_FALSE(memcmp(innew, plain, kbSize));
  delete[] plain;
  delete[] out;
  delete[] innew;
}

TEST(LongData, EncryptDecryptVectorOneKb) {
  AES aes(AESKeyLength::AES_256);
  unsigned int kbSize = 1024 * sizeof(unsigned char);
  std::vector<unsigned char> plain(kbSize);
  for (unsigned int i = 0; i < kbSize; i++) {
    plain[i] = i % 256;
  }

  std::vector<unsigned char> key = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
      0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
      0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f};

  std::vector<unsigned char> out = aes.EncryptECB(plain, key);
  std::vector<unsigned char> innew = aes.DecryptECB(out, key);
  ASSERT_EQ(innew, plain);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
