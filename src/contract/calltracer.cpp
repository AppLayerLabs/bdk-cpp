#include "calltracer.h"

namespace trace {

static Call::Type getCallType(const evmc_message& msg) {
  using enum Call::Type;

  if (msg.kind == EVMC_CALL)
    return (msg.flags == EVMC_STATIC) ? STATICCALL : CALL;

  if (msg.kind == EVMC_DELEGATECALL)
    return DELEGATECALL;
  
  throw DynamicException("evmc_message is not from a function call");
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
    case CALL:
      res["type"] = "CALL";
      break;

    case STATICCALL:
      res["type"] = "STATICCALL";
      break;

    case DELEGATECALL:
      res["type"] = "DELEGATECALL";
      break;
  }

  res["from"] = this->from.hex(true);
  res["to"] = this->to.hex(true);
  res["value"] = this->value.hex(true);
  res["gas"] = Hex::fromBytes(Utils::uintToBytes(static_cast<unsigned int>(this->gas)), true).forRPC();
  res["gasUsed"] = Hex::fromBytes(Utils::uintToBytes(static_cast<unsigned int>(this->gasUsed)), true).forRPC();
  res["input"] = Hex::fromBytes(this->input, true).forRPC();
  res["output"] = Hex::fromBytes(this->output, true).forRPC();

  if (!this->error.empty())
    res["error"] = this->error;

  res["calls"] = json::array();

  for (const auto& subcall : this->calls)
    res["calls"].push_back(subcall.toJson());

  return res;
}

CallTracer::CallTracer(Call rootCall) : root_(std::make_unique<Call>(std::move(rootCall))) {
  stack_.emplace_back(root_.get());
}

void CallTracer::traceIn(Call call) {
  if (stack_.empty()) [[unlikely]] {
    root_ = std::make_unique<Call>(std::move(call));
    stack_.emplace_back(root_.get());
    return;
  }

  Call& currCall = *stack_.back();
  Call& newCall = currCall.calls.emplace_back(std::move(call));
  stack_.emplace_back(&newCall);
}

void CallTracer::traceOutInternal(bytes::View output, int64_t gasUsed, std::string error) {
  if (stack_.empty()) [[unlikely]] {
    throw DynamicException("No function start was traced yet");
  }

  Call& curr = *stack_.back();
  curr.output = Utils::makeBytes(output);
  curr.gasUsed = gasUsed;
  curr.error = std::move(error);
  stack_.pop_back();
}

void CallTracer::traceOut(bytes::View output, int64_t gasUsed) {
  traceOutInternal(output, gasUsed, "");
}

void CallTracer::traceError(std::string error, int64_t gasUsed) {
  traceOutInternal(bytes::View(), gasUsed, std::move(error));
}

} // namespace trace
