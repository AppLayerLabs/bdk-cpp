#ifndef BDK_MESSAGES_OUTOFGAS_H
#define BDK_MESSAGES_OUTOFGAS_H

struct OutOfGas : std::runtime_error {
  OutOfGas() : std::runtime_error("Out of gas") {}
};

#endif // BDK_MESSAGES_OUTOFGAS_H
