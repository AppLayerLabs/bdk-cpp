/*
Copyright (c) [2023-2024] [AppLayer Developers]

This software is distributed under the MIT License.
See the LICENSE.txt file in the project root for more information.
*/

#include "calltracer.h"

#include "../utils/uintconv.h"

namespace trace {

static Call::Type getCallType(const evmc_message& msg) {
  using enum Call::Type;
  if (msg.kind == EVMC_CALL) return (msg.flags == EVMC_STATIC) ? STATICCALL : CALL;
  if (msg.kind == EVMC_DELEGATECALL) return DELEGATECALL;
  throw DynamicException("evmc_message is not from a function call");
}

Bytes encodeRevertReason(std::string_view reason) {
  FixedBytes<32> reasonEncoded{};

  const size_t count = std::min(reason.size(), reasonEncoded.size());
  std::copy_n(reason.begin(), count, reasonEncoded.begin());

  const uint256_t size(reason.size());
  const FixedBytes<32> sizeEncoded(UintConv::uint256ToBytes(size));

  return Utils::makeBytes(bytes::join(
    Hex::toBytes("0x08c379a0"),
    Hex::toBytes("0x0000000000000000000000000000000000000000000000000000000000000020"),
    sizeEncoded,
    reasonEncoded
  ));
}

std::string decodeRevertReason(bytes::View data) {
  if (data.size() != 100) {
    throw DynamicException("Encoded revert reason is expected to have exactly 100 bytes");
  }

  const size_t size = UintConv::bytesToUint256(data.subspan(36, 32)).convert_to<size_t>();

  std::string res;
  res.reserve(size);
  std::ranges::copy(data.subspan(68, size), std::back_inserter(res));
  return res;
}

Call::Call(const evmc_message& msg)
  : type(getCallType(msg)),
    from(msg.sender),
    to(msg.recipient),
    value(msg.value.bytes),
    gas(msg.gas),
    gasUsed(0),
    input(Utils::makeBytes(bytes::View(msg.input_data, msg.input_size))) {}

json Call::toJson() const {
  using enum Call::Type;
  json res;

  switch (this->type) {
    case CALL: res["type"] = "CALL"; break;
    case STATICCALL: res["type"] = "STATICCALL"; break;
    case DELEGATECALL: res["type"] = "DELEGATECALL"; break;
  }

  res["from"] = this->from.hex(true);
  res["to"] = this->to.hex(true);

  const uint256_t value = UintConv::bytesToUint256(this->value);
  res["value"] = Hex::fromBytes(Utils::uintToBytes(value), true).forRPC();

  res["gas"] = Hex::fromBytes(Utils::uintToBytes(this->gas), true).forRPC();
  res["gasUsed"] = Hex::fromBytes(Utils::uintToBytes(this->gasUsed), true).forRPC();
  res["input"] = Hex::fromBytes(this->input, true);

  if (!this->output.empty()) res["output"] = Hex::fromBytes(this->output, true);

  switch (this->status) {
    case Status::EXECUTION_REVERTED: {
      res["error"] = "execution reverted";
      try {
        std::string revertReason = decodeRevertReason(this->output);
        res["revertReason"] = std::move(revertReason);
      } catch (const std::exception& ignored) {}
      break;
    }
    case Status::OUT_OF_GAS: res["error"] = "out of gas"; break;
  }

  if (!this->calls.empty()) {
    res["calls"] = json::array();
    for (const auto& subcall : this->calls) res["calls"].push_back(subcall.toJson());
  }

  return res;
}

CallTracer::CallTracer(Call rootCall) : root_(std::make_unique<Call>(std::move(rootCall))) {
  stack_.emplace_back(root_.get());
}

const Call& CallTracer::root() const {
  if (!hasCalls()) throw DynamicException("root call does not exists since no call was traced");
  return *root_;
}

const Call& CallTracer::current() const {
  if (!hasCalls()) throw DynamicException("current call does not exists since no call was traced");
  if (isFinished()) throw DynamicException("call tracer is already finished, no call currently opened");
  return *stack_.back();
}

void CallTracer::push(Call call) {
  if (stack_.empty()) [[unlikely]] {
    root_ = std::make_unique<Call>(std::move(call));
    stack_.emplace_back(root_.get());
    return;
  }

  Call& currCall = *stack_.back();
  Call& newCall = currCall.calls.emplace_back(std::move(call));
  stack_.emplace_back(&newCall);
}

void CallTracer::pop(Bytes output, Status status, uint64_t gasUsed) {
  if (stack_.empty()) [[unlikely]] {
    throw DynamicException("No function start was traced yet");
  }

  Call& curr = *stack_.back();
  curr.output = std::move(output);
  curr.status = status;
  curr.gasUsed = gasUsed;
  stack_.pop_back();
}

void CallTracer::callStarted(Call call) {
  this->push(std::move(call));
}

void CallTracer::callOutOfGas() {
  this->pop(Bytes(), Status::OUT_OF_GAS, this->current().gas);
}

void CallTracer::callReverted(uint64_t gasUsed) {
  this->pop(Bytes(), Status::EXECUTION_REVERTED, gasUsed);
}

void CallTracer::callReverted(Bytes output, uint64_t gasUsed) {
  this->pop(std::move(output), Status::EXECUTION_REVERTED, gasUsed);
}

void CallTracer::callSucceeded(Bytes output, uint64_t gasUsed) {
  this->pop(std::move(output), Status::SUCCEEDED, gasUsed);
}

} // namespace trace
