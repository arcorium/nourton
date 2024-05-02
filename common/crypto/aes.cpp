#include "aes.h"

#include "util/algorithm.h"

#include <fmt/format.h>

#include "util/make.h"

namespace ar
{
	using namespace std::literals;

	AES::AES() noexcept
		: AES{std::span{random_bytes<16>().data(), 16}} {}

	std::expected<AES, std::string_view> AES::create(std::span<u8> key) noexcept
	{
		if (key.size() != KEY_BYTE)
			return ar::unexpected<std::string_view>(fmt::format("key need to be {} bytes long", KEY_BYTE));

		return AES{key};
	}

	std::expected<std::vector<u8>, std::string_view> AES::encrypt(block_type block) noexcept
	{
		if (block.size() % KEY_BYTE != 0)
			return ar::unexpected<std::string_view>("block size should be divisible by 16");

		std::vector<u8> result;
		try
		{
			auto result_raw = aes_.EncryptECB(block.data(), block.size(), key_.data());
			result.assign(result_raw, result_raw + block.size());
			// result = aes_.EncryptECB(std::vector<u8>{block.begin(), block.end()}, key_);
		}
		catch (...)
		{
			return ar::unexpected<std::string_view>("failed to encrypt block"sv);
		}
		return ar::make_expected<std::vector<u8>, std::string_view>(std::move(result));
	}

	std::tuple<usize, std::vector<u8>> AES::encrypts(std::span<u8> bytes) noexcept
	{
		auto remainder = bytes.size() % KEY_BYTE;
		auto block_count = bytes.size() / KEY_BYTE;

		auto first_blocks = bytes.subspan(0, block_count * KEY_BYTE);
		auto first_cipher_blocks = encrypt(first_blocks);

		if (!remainder)
			return std::make_tuple<usize, std::vector<u8>>(0, std::move(first_cipher_blocks.value()));

		std::array<u8, KEY_BYTE> remainder_bytes{};
		remainder_bytes.fill(0);
		for (usize i = 0; i < remainder; ++i)
			remainder_bytes[i] = bytes[block_count * KEY_BYTE + i];

		auto remainder_cipher_blocks = encrypt(remainder_bytes);

		first_cipher_blocks->append_range(remainder_cipher_blocks.value());

		return std::make_tuple<usize, std::vector<u8>>(KEY_BYTE - remainder, std::move(first_cipher_blocks.value()));
	}

	std::expected<std::vector<u8>, std::string_view> AES::decrypt(enc_block_type cipher_block) noexcept
	{
		if (cipher_block.size() % KEY_BYTE != 0)
			return ar::unexpected<std::string_view>("block size should be divisible by 16");

		std::vector<u8> result;
		try
		{
			auto result_raw = aes_.DecryptECB(cipher_block.data(), cipher_block.size(), key_.data());
			result.assign(result_raw, result_raw + cipher_block.size());
			// result = aes_.DecryptECB(std::vector<u8>{cipher_block.begin(), cipher_block.end()}, key_);
		}
		catch (...)
		{
			return ar::unexpected<std::string_view>("failed to decrypt block");
		}
		return ar::make_expected<std::vector<u8>, std::string_view>(std::move(result));
	}

	std::expected<std::vector<u8>, std::string_view> AES::decrypts(std::span<u8> cipher_bytes, usize filler) noexcept
	{
		if (cipher_bytes.size() % KEY_BYTE != 0)
			return ar::unexpected<std::string_view>(fmt::format("cipher bytes lengh should be divisible by {}", KEY_BYTE));

		// auto block_count = cipher_bytes.size() / KEY_BYTE;
		// std::vector<u8> decipher_result{};
		// decipher_result.reserve(cipher_bytes.size());
		// for (usize i = 0; i < cipher_bytes.size(); i += KEY_BYTE)
		// {
		// 	auto current_bytes = cipher_bytes.subspan(i, KEY_BYTE);
		// 	auto block = decrypt(current_bytes);
		// 	decipher_result.append_range(block.value());
		// }
		// PERF: decrypt it in one go, because the underlying implementation already divide it as long as the size is divisible by 16
		auto decipher_result = decrypt(cipher_bytes);
		if (!decipher_result.has_value())
			return ar::unexpected<std::string_view>(decipher_result.error());

		if (filler)
			decipher_result->resize(decipher_result->size() - filler);

		return ar::make_expected<std::vector<u8>, std::string_view>(decipher_result.value());
	}

	std::span<const u8> AES::key() const noexcept
	{
		return key_;
	}

	std::span<u8> AES::key() noexcept
	{
		return key_;
	}

	AES::AES(std::span<u8> key) noexcept
		: key_{key.begin(), key.end()}, aes_{key_length_} {}
}
