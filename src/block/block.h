#pragma once
#include <string.h>
#include <time.h>
#include "../libs/Common.h"
typedef std::string error;

namespace snowman {

    class Block {

        // Parent returns the ID of this block's parent.
        dev::u256 parent();

        // Verify that the state transition this block would make if accepted is
        // valid. If the state transition is invalid, a non-nil error should be
        // returned.
        //
        // It is guaranteed that the Parent has been successfully verified.
        error verify();

        // Bytes returns the binary representation of this block.
        //
        // This is used for sending blocks to peers. The bytes should be able to be
        // parsed into the same block on another node.
        std::vector<byte> bytes();

        // Height returns the height of this block in the chain.
        uint64_t height();

        // Time this block was proposed at. This value should be consistent across
        // all nodes. If this block hasn't been successfully verified, any value can
        // be returned. If this block is the last accepted block, the timestamp must
        // be returned correctly. Otherwise, accepted blocks can return any value
        time_t timestamp();

    };

}
