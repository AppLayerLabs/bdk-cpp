#include <algorithm>

#include "../src/utils/merkle.h"
#include "../src/utils/tx.h"

#include "../src/libs/catch2/catch_amalgamated.hpp"

using Catch::Matchers::Equals;

namespace TMerkle {
  TEST_CASE("Merkle Tests", "[utils]") {
    SECTION("Simple Merkle Tree Test") {
      std::vector<std::string> unhashedLeafs = {"ab", "bc", "cd", "de", "ef", "fg", "gh", "hi", "ij", "jk", "km", "mn"};
      std::vector<Hash> hashedLeafs;
      for(const std::string& leaf : unhashedLeafs)
      {
        hashedLeafs.emplace_back(Utils::sha3(leaf));
      }

      Merkle tree(hashedLeafs);

      auto proof = tree.getProof(3);
      auto leaf = tree.getLeaves()[3];
      auto root = tree.getRoot();
      auto badLeaf = tree.getLeaves()[4];

      REQUIRE_THAT(root.hex(), Equals("3fb0308018d8a6b4c2081699003624e9719774be2b7f65b7f9ac45f2bebc20b7"));
      REQUIRE(Merkle::verify(proof, leaf, root));
      REQUIRE(!Merkle::verify(proof, badLeaf, root));
    }

    SECTION("Random Merkle Tree Test") {
      std::vector<Hash> hashedLeafs {
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
        Hash::random(),
      };

      Merkle tree(hashedLeafs);

      auto proof = tree.getProof(5);
      auto leaf = tree.getLeaves()[5];
      auto root = tree.getRoot();
      auto badLeaf = tree.getLeaves()[6];

      REQUIRE(Merkle::verify(proof, leaf, root));
      REQUIRE(!Merkle::verify(proof, badLeaf, root));
    }

    SECTION("TxBlock Merkle Tree Test") {
      std::vector<TxBlock> txs {
        TxBlock(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c")),
        TxBlock(Hex::toBytes("f895844016f37185f2b0db75ee819a94244c50e5c782fb2845f96f6c59a772688b2321fc8920a695124ba217f0aea16049b0b9a404cc2bcfa22db59623d8a103c4d3975f925ab5b7906e05b499152f3e840138463ea0eabb0ebec46336ea95d2fe388ece3f7b4467dd8d346165bf0f4d533e886e8e8da070486c921eaebfcb3a7ff5455bdd27e335ea8e028b0dcda4103b2d32816ec321")),
        TxBlock(Hex::toBytes("f8e78697292f15c017824c96840208331394dd9bada36d88dac984e6d10a96061b65bc034a668617521be83d4cb8776129345086fa05e37c56ea3a9a24e102f3519b92f16f8f20281dcb28ef36eef04dd23f355693a24c4ce7de4ae663bfd5a47670092efefde1e612ff6fb380e3fe272121fb4c689454edb24d7287a8e4797f85934ab9514d20e77fb7a7c6a7e4921c614934ea2bb26737c34d8995a6d5505644d020f16d6a2ca042206f805ee0b2629bc85ea639a2ab22994b1dffcf15d29d021d840fc53cfbb6a035c39c1cf43b89e2657a20376a61ad450dec94ed9e4882f7fce5f9936ed804e2")),
        TxBlock(Hex::toBytes("f8ea8701960cc0c1e3f9835a824e82afaf94583d6b8bf349f56e270915bebd69ddb7b7dba71387fb652d0e3308d5b874b25155d71f72ef6b8dce2b77f841f5eb9f36843ecebafd4f41400c905c67d67237243f91a25fbdbfbd1825709bffe3f4e402475534891ae1648247cf8784d1d3af12d49b3b73d5e65498c70eee4061e434958cf5ce81d7b7fe466d33f4ed371aa48d1e3140eebc8dbb961994dfdf4a03554a48ef85390f00df4ca04a919adc0321d82992186315ae485bd0a1134d0f8c26b736698b31a28cb5e954a04f8755668cc129e982f19f73beadeb994f2ae1fcc2551787299c146ed6dd7514")),
        TxBlock(Hex::toBytes("f8b684c49a839886c5f402de9b9984e697e3ec9403ab524baf9055f8eb6cbdef3cc046293b26ab5a89d963de8c7d7d6019f1b83f27957117aca3bd31050081fc1bca3f832af0f67e5089cbd529d6084b02ea9a0590a48d96b9f54b50c5951633e4e986a21c0022020d7b5b90784a21fd331339821a4ba017384874c7074f80e8ff52172337e6ec29725b215f89e08fe2eb3acb94a824d6a0571cc1aa1fca868d11cb3702269103277ba21208bf1811397a0f83fa2375e604")),
        TxBlock(Hex::toBytes("f8eb8549db7390cd87ce5bc0bbb6789e82fa6494d73cdabcec0cc182de86b4c012ce243ebbc3ab4b833e2f1eb87abe3ca20e4c8698bf768eeb743dd5217ce55075396f34b97a9d600b5f4bd8562867150e7e2976193b5b7134b7bc834148dc8d62f82547272a5573bfed8b52ff4b904c2e38b6b6a43814ee33d2e671f6fc533daacd3af259f286af3a77c73e49a4016a01b48d9e6a3a3ab2644457d91870b93ca32384fc3c86ad69820121a08fa0d993d03d62270eb1d0a6af36302deb66b7318cdff66a361dbbca098b47a2a00dee4b8ce2459940c008d8a1be69d413bf204da5617dcbd04c238009b42df5a0")),
        TxBlock(Hex::toBytes("f8bb85e795e6d8e6808094c6ff22a04cc9ed476eede000eb0ac51661225bde8e1f31a323866448f957c82f1f84cbb84a1fa09bf8d8cd7ce5f37fd25dad940e9623433d1c4916b8d2f43240b3e38c1de579e2b064f7f0b0ec66af28c8a8f4ed19dfc033540e37a1ff95e5f5d95b7f3fc48922ab488e008f96e52154a0d7832d5ad5c45cd0c0369acad76d6e823447db6d9e9c5455fd91b3b5e63dfde4a02ba737b5720585b074ff0a52fc2ed21a07570dc6fa215bbfa8131eb51ffda5c1")),
        TxBlock(Hex::toBytes("f8bb837ee7fc87654e4393c0c1e783e2cb8394f3fd73725fe9dbc1c83b734ee624e3722d1045088c70cf0ce638d2b0e9b167cf3bb841d798df4008c491d5fd4109efc3c69fcbea65857915bbcecf8b2c7f35ed364df117ef2892c74a84d658aace2e5b9f3f95c27fbcdfb7ef32cd778b91ca788c60f5bb83f9ba4ca02687b710215d7cc9e970c8d40b536ceae7c455f4a314cf722b2f93b83b989372a03ed3b8624d0a3dc0bb580bf2cecf16b6571a8f1b60e85b1dd48f6377d1ee01ff")),
        TxBlock(Hex::toBytes("f8db83554a148086315b9a3bcd2c94c51c568dc73472818ef69bfdb3fb88a4413e2c2a881e0315140a1de529b86a641153d6d7aa0fbd6b04cd10e096f0d6e08f05ef94f1f9298cb9e64bbd6c8511fa03c705db9a51c64b5b2f327198880a477c1d4f6a03b000bdaa0a72a129eca78f0aaad2a6a2cdf7a79b50f89a49903df3f60db2801ed1159d0ace69843211b8554ebcbc1ab832965f04820188a0dba3448a36ed77b8cdc15eb6731ae2f9bcefc011932064780f3c1c86ac7c82b2a00e213a597fca7d29a7c0646d125c03fc6361c2df2ceadc6a34d32bc92b4d5f2e")),
        TxBlock(Hex::toBytes("f8d0868c776d3ab2ed81dc859d31919a059423d1a55e919efb2ec8f5c12dc41f0c4c1c1dba6b80b86366136b47795c389b015845e6e5b07b6405c37fe21f4be1535db3479f62d8c62b9c7ef715513edc407eb882448c199df84440461d7e1108e17d57ebf3eed8abdf1f68e64787261bf1438432e1435ce7911e54a2d216d811ba0c6d450a96dfcbe4ec328783a454e3a04824815a359690ce53c1f3b192f8ff315882196d4a6e930144990702230d37a0a0634cb7702d701b18f8a0ac876ce768404e726bfdd0ed410cf008942a7d58d64e")),
        TxBlock(Hex::toBytes("f8ef8737706951d3968c8203cf8553cb1bcefb9493725531e3371e2b0baeb1f75c5b5102c04bf4438be656bacbbc8d9c4ef82cd8b874bf2ac52475bb963dcc43d6f10349a55e1f30899461e358e0adaa237bb6b9afe845aaffea7087eda3f3c38b81f167fa694c775c33897768da59f74552ec800afe193b87e8443b812c154c690e9b30eda40adad171c0dfbfb5c8c97c755583a7fdf06a8377a1e2dc9a97eb8709660d0fee6a09dfb88401fd9a84a025d050e8758eab522a93642c3e36faa5855aa5e177f4e2ad6dd2839e4ec04b05a0132cc59b9050ed354552d6853c9e0aca45682ecf546bb1e0d406dc26ba832a49")),
        TxBlock(Hex::toBytes("f8da868dc1979857ac80819094912bd947f01f8324bbc2f22348f09df0b0bb077f80b870b41dff6f4be0db9f6ed217bb7955123ba9cbe30468e5f152a4273f3b483b752834c8bdd97127047e43fe8d117cffbd31a1b28481623bf9fbae09a86c024316af618e04cf896f9b37eac968c051da82f3b3757d9d7dca88eed7dd959abbf1154b64c3c22a2c3e92360ee02c05ff6b1af485013eb1fa48a095c7151433f1baf0408295d0545c29776a3053b056152a56b446095f9fe5bc1ea03b4f72b0b2d7c619453dbbb455aa95e1fb2a526dea7e241ceb4cdcb955814392")),
        TxBlock(Hex::toBytes("f8db863b966f67db1e8082dd59943a8970efe474c3770f7a6f4d68fcd2d84a5444ee87b13dd3fbf54530b868d4210fb5fe6b195ebc558373e7f724aecc3eeb0a4c852118bf61016f1bf2d8fb1da0b946790ac12cc009a4fde91d96833d5641054abb9bab3ac0f7c2c01c24ef9ba1c5d1c7a58b1e8ade7f06a6ac33a3ce95d66f082bc587900379b7042ccefe1e501ad2202c39da8601ee5c62882fa095fd1c8635167fba1b0c66e0d6a7c629c79836d1b19c151094450e2217e01f3da061cbc25dd859394b96bd14255371b0ad169936cbc90dbfb9c7a86e9f76ef58a6")),
        TxBlock(Hex::toBytes("f890658080948d65b2a2437da6a0ff1e575a75f8f4fe22d056ea87ab4202e0fad64aa7bea2c51b92c070a2ed7d78e3ccc3969d32b6d4db8d5af381f84a7c7dd9b4189872d328a189b6538501d1fa6a31a0589ad633091e9ef5cafe3fb863a66c8a39c9eab72f189b5f0ee2bbc11c9899cda0194a9a4383f94c787a2c40839683ff546680f5a67a932f1baee428818500cffd")),
        TxBlock(Hex::toBytes("f89985406a1f50f183cfa35d85ab87453de894f13174546ec556878140dd71b46f6f4da9e3199380aa0238ac252b052fffc4fbe937b2d2f177f4bb71ed6fdfd5db7aec43c6ca517354f9e40845ac8c885a9b8a857819fcac3ca084b3810f706632ecb179af29220867baf8c450c4e13f22cc98b43ff227f1c165a04f65f2f319553c5edad224760147ae501125ee4a98932117cad354905e1d1c19")),
        TxBlock(Hex::toBytes("f8e787e5223863e60ecf856720f8a4c9868fc8332be5d294fc3106d6242b762c37f21c021395dbfdc9a2c0d08b5fd95952973e70427608c0b869bf32ad6cc8188ff0ac6a14727bad727dcdec7f4b17a8b99499946f386d38a69c1945077f7b9a199a2b6261f37700affb3c334e9e2ff2f088ab47f407559920e0e18e565dab9c6af48fffc3b2f49db7e0175ec32bfc30c542753e7f299ef8cfe3c3b3b555a937b6bff68375c626a035867d0a33d495f09fe0fdadf1d6a8e7099ec0305a30ef52f5e6aa86146dbc04a038a20bf0853be3332b286b7d6e6dd25bb859ef98f2d2f15f0a9edbde5e4fb194"))
      };

      Merkle tree(txs);
      auto proof = tree.getProof(3);
      auto leaf = tree.getLeaves()[3];
      auto root = tree.getRoot();
      auto badLeaf = tree.getLeaves()[4];

      REQUIRE(root == Hash(Hex::toBytes("eef6cb29005b53e9f74ba5fe7e29759ab3623251804ff7e07608c2b49ae2c5f8")));
      REQUIRE(Merkle::verify(proof, leaf, root));
      REQUIRE(!Merkle::verify(proof, badLeaf, root));
    }
  }
}