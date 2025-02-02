#include "precompiledcontractexecutor.h"
#include "precompiles.h"
#include "bytes/hex.h"
#include <algorithm>
#include <ranges>

constexpr Address RANDOM_GENERATOR_ADDRESS = bytes::hex("0x1000000000000000000000000000100000000001");
constexpr Address ECRECOVER_ADDRESS = bytes::hex("0x0000000000000000000000000000000000000001");

constexpr auto ECRECOVER_CALL_COST = 3'000;

static inline const void *copyIn(void *out, const void *in, size_t len) {
  std::memcpy(out, in, len);
  return reinterpret_cast<const Byte*>(in) + len;
}

static void decodeBlake2(View<Bytes> data, std::span<uint64_t, 8> h, std::span<uint64_t, 16> m, std::span<uint64_t, 2> t, uint8_t& flag, uint32_t& rounds) {
  const void *in = data.data();

  in = copyIn(&rounds, in, sizeof(uint32_t));
  in = copyIn(h.data(), in, h.size_bytes());
  in = copyIn(m.data(), in, m.size_bytes());
  in = copyIn(t.data(), in, t.size_bytes());
  copyIn(&flag, in, sizeof(uint8_t));

  std::reverse(reinterpret_cast<Byte *>(&rounds), reinterpret_cast<Byte *>(&rounds + 1));
}

Bytes PrecompiledContractExecutor::execute(EncodedStaticCallMessage& msg) {
  if (msg.to() == RANDOM_GENERATOR_ADDRESS) {  
    return Utils::makeBytes(UintConv::uint256ToBytes(std::invoke(randomGen_)));
  }

  // assuming isPrecompiled() was already called
  switch (msg.to()[19]) {
    case 0x01: {
      msg.gas().use(ECRECOVER_CALL_COST);
      const auto [hash, v, r, s] = ABI::Decoder::decodeData<Hash, uint8_t, Hash, Hash>(msg.input());
      return ABI::Encoder::encodeData(ecrecover(hash, v, r, s));
    }

    case 0x02: {
      const uint64_t dynamicGasCost = ((msg.input().size() + 31) / 32) * 12;
      msg.gas().use(60 + dynamicGasCost);
      return Utils::makeBytes(sha256(msg.input()));
    }

    case 0x03: {
      const uint64_t dynamicGasCost = ((msg.input().size() + 31) / 32) * 120;
      msg.gas().use(600 + dynamicGasCost);
      // for some reason, it works when we encode as Address and decode as Bytes20
      // but it would make more sense if we encode and decode as Bytes20
      return ABI::Encoder::encodeData<Address>(Address(ripemd160(msg.input())));
    }

    case 0x04: {
      const uint64_t dynamicGasCost = ((msg.input().size() + 31) / 32) * 3;
      msg.gas().use(15 + dynamicGasCost);
      return Bytes(msg.input());
    }

    case 0x09: {
      if (msg.input().size() != 213) {
        throw DynamicException("Blake2F requires exacly 213 bytes");
      }

      std::array<uint64_t, 8> h;
      std::array<uint64_t, 16> m;
      std::array<uint64_t, 2> t;
      uint8_t flag;
      uint32_t rounds;

      decodeBlake2(msg.input(), h, m, t, flag, rounds);

      msg.gas().use(rounds); // That seems too cheap...

      blake2f(h, m, t[0], t[1], flag, rounds);

      Bytes output(h.size() * sizeof(uint64_t));
      std::memcpy(output.data(), h.data(), output.size());
      return output;
    }

    default:
      throw DynamicException("Precompiled contract not found");
  }
}

bool PrecompiledContractExecutor::isPrecompiled(View<Address> address) const {
  if (address == RANDOM_GENERATOR_ADDRESS) {
    return true;
  }

  if (std::ranges::any_of(address | std::views::take(19), [] (Byte b) { return b != 0; })) {
    return false;
  }

  return address[19] <= 0x04 || address[19] == 0x09;
}
