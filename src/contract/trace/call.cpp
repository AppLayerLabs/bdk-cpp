#include "call.h"
#include "contract/abi.h"

namespace trace {

json Call::toJson() const {
  using enum CallType;
  json res;

  switch (this->type) {
    case CALL: res["type"] = "CALL"; break;
    case STATICCALL: res["type"] = "STATICCALL"; break;
    case DELEGATECALL: res["type"] = "DELEGATECALL"; break;
  }

  res["from"] = this->from.hex(true);
  res["to"] = this->to.hex(true);

  const uint256_t value = Utils::bytesToUint256(this->value);
  res["value"] = Hex::fromBytes(Utils::uintToBytes(value), true).forRPC();

  res["gas"] = Hex::fromBytes(Utils::uintToBytes(this->gas), true).forRPC();
  res["gasUsed"] = Hex::fromBytes(Utils::uintToBytes(this->gasUsed), true).forRPC();
  res["input"] = Hex::fromBytes(this->input, true);

  if (!this->output.empty()) res["output"] = Hex::fromBytes(this->output, true);

  switch (this->status) {
    case CallStatus::EXECUTION_REVERTED: {
      res["error"] = "execution reverted";
      try {
        std::string revertReason = ABI::Decoder::decodeError(this->output);
        res["revertReason"] = std::move(revertReason);
      } catch (const std::exception& ignored) {}
      break;
    }
    case CallStatus::OUT_OF_GAS: res["error"] = "out of gas"; break;
  }

  if (!this->calls.empty()) {
    res["calls"] = json::array();
    for (const auto& subcall : this->calls) res["calls"].push_back(subcall.toJson());
  }

  return res;
}

} // namespace trace
