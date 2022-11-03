#include "../src/core/block.h"
#include "../src/core/transaction.h"
#include "../src/core/utils.h"



void benchmarkBlock() {
  std::string transactionBytes = Utils::hexToBytes("f86e8085012a05f20082520894798333f07163eb62d1e22cc2df1acfe597567882880de0b6b3a764000080824544a0cc7fb28c74b12a47910a97156b0485119856db37040a27ce1fcb7889744d78baa05ebf6d6ff721d1d86c33e652f4ba493a36079cf85ed01d870fe29dd93237a78f");

  Tx::Base tx(transactionBytes, false);

  std::cout << Utils::bytesToHex(tx.rlpSerialize(true)) << std::endl;
  std::string blockBytes;

  // Scope so we free block memory.
  {
    
    Block block(0, 1656356645000000, 0);
  
  
    for (uint64_t i = 0; i < 1000000; ++i) {
      if (i % 10000 == 0) {
        std::cout << i << " transactions appended" << std::endl;
      }
      block.appendTx(tx);
    }
    
    block.finalizeBlock();
    block.indexTxs();
  
    std::cout << "Block hash: " << Utils::bytesToHex(block.getBlockHash()) << std::endl;
  
    blockBytes = block.serializeToBytes(true);
  }
   

  std::cout << "Block size: " << blockBytes.size() << std::endl;


  std::cout << "Creating new block from serializedBlock's bytes" << std::endl;

  auto start = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());
  Block newBlock(blockBytes, true);  
  auto end = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch());

  std::cout << "Block creation time: " << end.count() - start.count() << "ms" << std::endl;

  std::string blockBytesTwo = newBlock.serializeToBytes(true);

  if (blockBytes == blockBytesTwo) {
    std::cout << "Block bytes match" << std::endl;
  } else {
    std::cout << "Block bytes do not match" << std::endl;
    throw std::runtime_error("Block bytes do not match");
  }

}