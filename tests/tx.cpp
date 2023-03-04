#include "../src/libs/catch2/catch_amalgamated.hpp"
#include "../src/utils/utils.h"
#include "../src/utils/tx.h"

using Catch::Matchers::Equals;

namespace TTX {
  TEST_CASE("TxBlock", "[utils]") {
    SECTION("Simple Transaction 1") {
      TxBlock tx(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c"));

      REQUIRE(tx.nonce() == 2);
      REQUIRE(tx.gasPrice() == uint256_t("71000000000"));
      REQUIRE(tx.gas() ==  21000);
      REQUIRE(tx.to() == Address(std::string_view("0xf137c97b1345f0a7ec97d070c70cf96a3d71a1c9"), false));
      REQUIRE(tx.value() == uint256_t("7353873760000000"));
      REQUIRE(tx.data() == Hex::toBytes(""));
      REQUIRE(tx.from() == Address(std::string_view("0x279312b596f426aeea3adfaf79a4bb24c4368beb"), false));
      REQUIRE(tx.r() == uint256_t("97347950278129771725048946454297431073977916303978115819430708259653065028945"));
      REQUIRE(tx.s() == uint256_t("4694282873640443545574231586479962476127424271701698807158512046516151972188"));
      REQUIRE(tx.v() == 37);
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f86b02851087ee060082520894f137c97b1345f0a7ec97d070c70cf96a3d71a1c9871a204f293018008025a0d738fcbf48d672da303e56192898a36400da52f26932dfe67b459238ac86b551a00a60deb51469ae5b0dc4a9dd702bad367d1111873734637d428626640bcef15c")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Simple Transaction 2") {
      TxBlock tx(Hex::toBytes("0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e"));

      REQUIRE(tx.nonce() == 0);
      REQUIRE(tx.gasPrice() == uint256_t("101000000000"));
      REQUIRE(tx.gas() ==  258389);
      REQUIRE(tx.to() == Address(std::string_view("0xbcf935d206ca32929e1b887a07ed240f0d8ccd22"), false));
      REQUIRE(tx.value() == uint256_t("30000000000000000"));
      REQUIRE(tx.data() == Hex::toBytes("8853b53e00000000000000000000000000000000000000000000000000000000000a4d79"));
      REQUIRE(tx.from() == Address(std::string_view("0x279312b596f426aeea3adfaf79a4bb24c4368beb"), false));
      REQUIRE(tx.r() == uint256_t("41901809089693999974129375521417939741880099148096042771059180304686223129639"));
      REQUIRE(tx.v() == uint256_t("37"));
      REQUIRE(tx.s() == uint256_t("51778031291651447658465284139937418000921280056049493911617165601823117554558"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("0xf8908085178411b2008303f15594bcf935d206ca32929e1b887a07ed240f0d8ccd22876a94d74f430000a48853b53e00000000000000000000000000000000000000000000000000000000000a4d7925a05ca395600115460cf539c25ac9f3140f71b10db78eca64c43873921b9f96fc27a0727953c15ff2725c144ba16d458b29aa6fbfae3feade7c8c854b08223178337e")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Simple Transaction 3") {
      TxBlock tx(Hex::toBytes("f85d80808094000000000000000000000000000000000000dead808026a0ed1281a2c3412838d6a45dcd5e13559d375db76f3e6279d407f41067f07cc9a2a03f2563b27e3ef6279487fd3440afbcb470c66f7b2d719361da5d2efc7ff7c6ec"));

      REQUIRE(tx.nonce() == 0);
      REQUIRE(tx.gasPrice() == uint256_t("0"));
      REQUIRE(tx.gas() == 0);
      REQUIRE(tx.to() == Address(std::string_view("0x000000000000000000000000000000000000dead"), false));
      REQUIRE(tx.value() == uint256_t("0"));
      REQUIRE(tx.data() == Hex::toBytes(""));
      REQUIRE(tx.from() == Address(std::string_view("0xe4f293B99f4a1Ae30Bb150363a9095C2C6441948"), false));
      REQUIRE(tx.r() == uint256_t("107230843074752941982270147591989153003301096667951404750477248909976971037090"));
      REQUIRE(tx.v() == uint256_t("38"));
      REQUIRE(tx.s() == uint256_t("28561770887196671332427357807149642367323022549194404750708204793241343084268"));

      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f85d80808094000000000000000000000000000000000000dead808026a0ed1281a2c3412838d6a45dcd5e13559d375db76f3e6279d407f41067f07cc9a2a03f2563b27e3ef6279487fd3440afbcb470c66f7b2d719361da5d2efc7ff7c6ec")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Advanced Transaction 1") {
      TxBlock tx(Hex::toBytes("0xf8ae82d3548505d21dba008305cf809418df1967e5cc30ee53d399a8bbf71c3e60b44beb80b8430d079f8876b0de07c78a02254986f3473fabbb6b4aca5396627ec5c60480d05fa135405e021373121c55ca5bc2a2490000000000000000000000000000000000000000830150f7a0566e1e6e301e72698e948fee5ca0cd32eab301d66ba2fb4496809fb8cb5b3663a039ef7219cb5c105024f0f586d468f9d41d4ce431c4d3cb6824738ff50a9a0b32"));

      REQUIRE(tx.nonce() == 54100);
      REQUIRE(tx.gasPrice() == uint256_t("25000000000"));
      REQUIRE(tx.gas() == 380800);
      REQUIRE(tx.to() == Address(std::string_view("0x18df1967e5cc30ee53d399a8bbf71c3e60b44beb"), false));
      REQUIRE(tx.value() == uint256_t("0"));
      REQUIRE(tx.data() == Hex::toBytes("0d079f8876b0de07c78a02254986f3473fabbb6b4aca5396627ec5c60480d05fa135405e021373121c55ca5bc2a2490000000000000000000000000000000000000000"));
      REQUIRE(tx.from() == Address(std::string_view("0x0f4240939fda73af82df6694f3fb214cc6357202"), false));
      REQUIRE(tx.r() == uint256_t("39093468178338583917643482008412913746314798135970459109331174520004995135075"));
      REQUIRE(tx.v() == uint256_t("86263"));
      REQUIRE(tx.s() == uint256_t("26204896312226686822705788002875995149785418761187323356120226868870457723698"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("0xf8ae82d3548505d21dba008305cf809418df1967e5cc30ee53d399a8bbf71c3e60b44beb80b8430d079f8876b0de07c78a02254986f3473fabbb6b4aca5396627ec5c60480d05fa135405e021373121c55ca5bc2a2490000000000000000000000000000000000000000830150f7a0566e1e6e301e72698e948fee5ca0cd32eab301d66ba2fb4496809fb8cb5b3663a039ef7219cb5c105024f0f586d468f9d41d4ce431c4d3cb6824738ff50a9a0b32")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Advanced Transaction 2") {
      //  "nonce": 1230,
      //  "gasPrice": 25000000000,
      //  "gasLimit": 174298,
      //  "to": "0x60ae616a2155ee3d9a68541ba4544862310933d4",
      //  "value": 11904150190484301209,
      //  "data": "a2a1623d00000000000000000000000000000000000000000000001ef029ff29066a6bff000000000000000000000000000000000000000000000000000000000000008000000000000000000000000055476e450bd6931b64b2611bb21167dcab7a26c40000000000000000000000000000000000000000000000000000017e0edf45a20000000000000000000000000000000000000000000000000000000000000002000000000000000000000000b31f66aa3c1e785363f0875a1b74e27b85fd66c70000000000000000000000006e84a6216ea6dacc71ee8e6b0a5b7322eebc0fdd",
      //  "from": "0x55476e450bd6931b64b2611bb21167dcab7a26c4",
      //  "r": "a6460607154957e7e057b1c703750817e1108115cebe7d462c60d9b8c75f256c",
      //  "v": "0150f7",
      //  "s": "055efaf2abc34eea4b22e22925119d9add503aff648de9f77c2f8a19895a8c19"
      TxBlock tx(Hex::toBytes("0xf901578204ce8505d21dba008302a8da9460ae616a2155ee3d9a68541ba4544862310933d488a534098700b72599b8e4a2a1623d00000000000000000000000000000000000000000000001ef029ff29066a6bff000000000000000000000000000000000000000000000000000000000000008000000000000000000000000055476e450bd6931b64b2611bb21167dcab7a26c40000000000000000000000000000000000000000000000000000017e0edf45a20000000000000000000000000000000000000000000000000000000000000002000000000000000000000000b31f66aa3c1e785363f0875a1b74e27b85fd66c70000000000000000000000006e84a6216ea6dacc71ee8e6b0a5b7322eebc0fdd830150f7a0a6460607154957e7e057b1c703750817e1108115cebe7d462c60d9b8c75f256ca0055efaf2abc34eea4b22e22925119d9add503aff648de9f77c2f8a19895a8c19"));

      REQUIRE(tx.nonce() == 1230);
      REQUIRE(tx.gasPrice() == uint256_t("25000000000"));
      REQUIRE(tx.gas() == 174298);
      REQUIRE(tx.to() == Address(std::string_view("0x60ae616a2155ee3d9a68541ba4544862310933d4"), false));
      REQUIRE(tx.value() == uint256_t("11904150190484301209"));
      REQUIRE(tx.data() == Hex::toBytes("a2a1623d00000000000000000000000000000000000000000000001ef029ff29066a6bff000000000000000000000000000000000000000000000000000000000000008000000000000000000000000055476e450bd6931b64b2611bb21167dcab7a26c40000000000000000000000000000000000000000000000000000017e0edf45a20000000000000000000000000000000000000000000000000000000000000002000000000000000000000000b31f66aa3c1e785363f0875a1b74e27b85fd66c70000000000000000000000006e84a6216ea6dacc71ee8e6b0a5b7322eebc0fdd"));
      REQUIRE(tx.from() == Address(std::string_view("0x55476e450bd6931b64b2611bb21167dcab7a26c4"), false));
      REQUIRE(tx.r() == uint256_t("75207653760796144628811872402617741551981943257922567762153293068114258240876"));
      REQUIRE(tx.v() == 86263);
      REQUIRE(tx.s() == uint256_t("2429379845988038194593421584863147539691633078023313324097079077757425191961"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("0xf901578204ce8505d21dba008302a8da9460ae616a2155ee3d9a68541ba4544862310933d488a534098700b72599b8e4a2a1623d00000000000000000000000000000000000000000000001ef029ff29066a6bff000000000000000000000000000000000000000000000000000000000000008000000000000000000000000055476e450bd6931b64b2611bb21167dcab7a26c40000000000000000000000000000000000000000000000000000017e0edf45a20000000000000000000000000000000000000000000000000000000000000002000000000000000000000000b31f66aa3c1e785363f0875a1b74e27b85fd66c70000000000000000000000006e84a6216ea6dacc71ee8e6b0a5b7322eebc0fdd830150f7a0a6460607154957e7e057b1c703750817e1108115cebe7d462c60d9b8c75f256ca0055efaf2abc34eea4b22e22925119d9add503aff648de9f77c2f8a19895a8c19")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Transaction 1") {
      PrivKey privKey(Hex::toBytes("b9cabd91158568020828a524ccd77e3500ed1d83c0bd07b402db9444bd951484"));

      TxBlock tx(Address(std::string_view("0x000000000000000000000000000000000000dead"), false),
        Address(std::string_view("0xe4f293B99f4a1Ae30Bb150363a9095C2C6441948"), false),
        "",
        1,
        0,
        0,
        0,
        0,
        privKey
      );

      REQUIRE(tx.nonce() == 0);
      REQUIRE(tx.gasPrice() == uint256_t("0"));
      REQUIRE(tx.gas() == 0);
      REQUIRE(tx.to() == Address(std::string_view("0x000000000000000000000000000000000000dead"), false));
      REQUIRE(tx.value() == uint256_t("0"));
      REQUIRE(tx.data() == Hex::toBytes(""));
      REQUIRE(tx.from() == Address(std::string_view("0xe4f293B99f4a1Ae30Bb150363a9095C2C6441948"), false));
      REQUIRE(tx.r() == uint256_t("107230843074752941982270147591989153003301096667951404750477248909976971037090"));
      REQUIRE(tx.v() == uint256_t("38"));
      REQUIRE(tx.s() == uint256_t("28561770887196671332427357807149642367323022549194404750708204793241343084268"));

      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f85d80808094000000000000000000000000000000000000dead808026a0ed1281a2c3412838d6a45dcd5e13559d375db76f3e6279d407f41067f07cc9a2a03f2563b27e3ef6279487fd3440afbcb470c66f7b2d719361da5d2efc7ff7c6ec")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 1") {

      PrivKey privKey(Hex::toBytes("ce974dad85cf9593db9d5c3e89ca8c67ca0f841dc97f2c58c6ea2038e4fa6d8d"));

      TxBlock tx(
        Address(std::string_view("0x13b5c424686de186bc5268d5cfe6aa4200ca9aee"), false),
        Address(std::string_view("0x31Af43C5E5924610a9c02B669c7980D9eBdB9719"), false),
        Hex::toBytes("0xe426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e"),
        uint64_t(28383084),
        uint256_t("42968208492763873"),
        uint256_t("166903214424643"),
        uint256_t("769147246"),
        uint256_t("61182866117425671"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("42968208492763873"));
      REQUIRE(tx.gasPrice() == uint256_t("61182866117425671"));
      REQUIRE(tx.gas() == uint256_t("769147246"));
      REQUIRE(tx.to() == Address(std::string_view("0x13b5c424686de186bc5268d5cfe6aa4200ca9aee"), false));
      REQUIRE(tx.value() == uint256_t("166903214424643"));
      REQUIRE(tx.data() == Hex::toBytes("0xe426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e"));
      REQUIRE(tx.from() == Address(std::string_view("0x31Af43C5E5924610a9c02B669c7980D9eBdB9719"), false));
      REQUIRE(tx.r() == uint256_t("94826113454042106245186949538086438136818423085418174917527790019189474174262"));
      REQUIRE(tx.v() == uint256_t("56766204"));
      REQUIRE(tx.s() == uint256_t("13278497192674363787053637696586244540054004842091981766694124199690162709045"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8cc8798a75ba3d89ae187d95d7e1944fa07842dd8416e9413b5c424686de186bc5268d5cfe6aa4200ca9aee8697cc2ecec243b852e426208f118c6c7db391b3391dda9b94bb0e5c6da9514ad74b63fd6d723b38be421a039136c0015ef0c6bff94109cb9bc4942031949016b85e919fdca81f59f0e417bd696cf6e8f9203d792edc223a59d24e8403622efca0d1a5ada0b1f6269423c1ac5a5b9f8f3c491b66e73b05c4624f536e43ea55a536a01d5b5cf296ebb49b7de53e3bed2291cd4443fc66bf3c7e033574e44ca634a235")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 2") {

      PrivKey privKey(Hex::toBytes("4273ee344beb20abf60b0cd25c0e63f2a985d4b21fb120aa34b1af514fb16938"));

      TxBlock tx(
        Address(std::string_view("0x13fd291a144e1786eca477f0d88c52415a9f3259"), false),
        Address(std::string_view("0xD8665cC8A9D3DA28674fCa7D5b4C4451C5218cb4"), false),
        Hex::toBytes("0x59e381a7936a99e59aee6cda1a452993f4392c17ff25a8103b1d1d5f7a291c5a4f9203aa2bd9257b648ec1ff4b0428b320a0c73a4dac3630e091f4e626ee05d4748f007067915a1646bf09a26698c97ff3e38be1a0639feb5a9ca4b1a19d12387f04eb4df5d57bcd289deb0a5203ece671b6041e7f441d"),
        uint64_t(15974168),
        uint256_t("47805"),
        uint256_t("253679919924062"),
        uint256_t("0"),
        uint256_t("1330669"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("47805"));
      REQUIRE(tx.gasPrice() == uint256_t("1330669"));
      REQUIRE(tx.gas() == 0);
      REQUIRE(tx.to() == Address(std::string_view("0x13fd291a144e1786eca477f0d88c52415a9f3259"), false));
      REQUIRE(tx.value() == uint256_t("253679919924062"));
      REQUIRE(tx.data() == Hex::toBytes("0x59e381a7936a99e59aee6cda1a452993f4392c17ff25a8103b1d1d5f7a291c5a4f9203aa2bd9257b648ec1ff4b0428b320a0c73a4dac3630e091f4e626ee05d4748f007067915a1646bf09a26698c97ff3e38be1a0639feb5a9ca4b1a19d12387f04eb4df5d57bcd289deb0a5203ece671b6041e7f441d"));
      REQUIRE(tx.from() == Address(std::string_view("0xD8665cC8A9D3DA28674fCa7D5b4C4451C5218cb4"), false));
      REQUIRE(tx.r() == uint256_t("61899255066434115192965886843236107668289335694829477396257995125657051054184"));
      REQUIRE(tx.v() == uint256_t("31948371"));
      REQUIRE(tx.s() == uint256_t("25476705283788449402470764361125476239784898081557091453090923235371774982478"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8e482babd83144ded809413fd291a144e1786eca477f0d88c52415a9f325986e6b87583835eb87759e381a7936a99e59aee6cda1a452993f4392c17ff25a8103b1d1d5f7a291c5a4f9203aa2bd9257b648ec1ff4b0428b320a0c73a4dac3630e091f4e626ee05d4748f007067915a1646bf09a26698c97ff3e38be1a0639feb5a9ca4b1a19d12387f04eb4df5d57bcd289deb0a5203ece671b6041e7f441d8401e77e53a088d9bca02763fde4d4660761a6f4a81c97fd8d67b4a9ed6eb8eae8226f4bc068a038534ddf60f847746f1d8d5dcb591a5a6136ff0c0cddd488f0e327a8e801394e")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 3") {

      PrivKey privKey(Hex::toBytes("58f4fe7d000e888576d58968e97877e0c8ab9d5a8df3f17b258749ee720ad3bf"));

      TxBlock tx(
        Address(std::string_view("0x244c50e5c782fb2845f96f6c59a772688b2321fc"), false),
        Address(std::string_view("0x3b19BF9DB3498d5173405A933B1E594dB586DEA3"), false),
        Hex::toBytes("0x6049b0b9a404cc2bcfa22db59623d8a103c4d3975f925ab5b7906e05b499152f3e"),
        uint64_t(10232589),
        uint256_t("1075245937"),
        uint256_t("602299330856582901934"),
        uint256_t("154"),
        uint256_t("1042349258222"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("1075245937"));
      REQUIRE(tx.gasPrice() == uint256_t("1042349258222"));
      REQUIRE(tx.gas() == uint256_t("154"));
      REQUIRE(tx.to() == Address(std::string_view("0x244c50e5c782fb2845f96f6c59a772688b2321fc"), false));
      REQUIRE(tx.value() == uint256_t("602299330856582901934"));
      REQUIRE(tx.data() == Hex::toBytes("0x6049b0b9a404cc2bcfa22db59623d8a103c4d3975f925ab5b7906e05b499152f3e"));
      REQUIRE(tx.from() == Address(std::string_view("0x3b19BF9DB3498d5173405A933B1E594dB586DEA3"), false));
      REQUIRE(tx.r() == uint256_t("106171708737118638227505298187491079361212631236747646723026381403496356155021"));
      REQUIRE(tx.v() == uint256_t("20465214"));
      REQUIRE(tx.s() == uint256_t("50787001357978802353577461962811808328898244759889590180021256407205869962017"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f895844016f37185f2b0db75ee819a94244c50e5c782fb2845f96f6c59a772688b2321fc8920a695124ba217f0aea16049b0b9a404cc2bcfa22db59623d8a103c4d3975f925ab5b7906e05b499152f3e840138463ea0eabb0ebec46336ea95d2fe388ece3f7b4467dd8d346165bf0f4d533e886e8e8da070486c921eaebfcb3a7ff5455bdd27e335ea8e028b0dcda4103b2d32816ec321")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 4") {

      PrivKey privKey(Hex::toBytes("ce9f56bfbe48795a1d2e82590f24ab9b5fd3bd505a15b9e9f8668b82c2b73b0f"));

      TxBlock tx(
        Address(std::string_view("0xdd9bada36d88dac984e6d10a96061b65bc034a66"), false),
        Address(std::string_view("0x69a7d445801fd4abD49aAb934E1Dd166891dED4B"), false),
        Hex::toBytes("0x6129345086fa05e37c56ea3a9a24e102f3519b92f16f8f20281dcb28ef36eef04dd23f355693a24c4ce7de4ae663bfd5a47670092efefde1e612ff6fb380e3fe272121fb4c689454edb24d7287a8e4797f85934ab9514d20e77fb7a7c6a7e4921c614934ea2bb26737c34d8995a6d5505644d020f16d6a"),
        uint64_t(4),
        uint256_t("166203139407895"),
        uint256_t("25641422961996"),
        uint256_t("34091795"),
        uint256_t("19606"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("166203139407895"));
      REQUIRE(tx.gasPrice() == uint256_t("19606"));
      REQUIRE(tx.gas() == uint256_t("34091795"));
      REQUIRE(tx.to() == Address(std::string_view("0xdd9bada36d88dac984e6d10a96061b65bc034a66"), false));
      REQUIRE(tx.value() == uint256_t("25641422961996"));
      REQUIRE(tx.data() == Hex::toBytes("0x6129345086fa05e37c56ea3a9a24e102f3519b92f16f8f20281dcb28ef36eef04dd23f355693a24c4ce7de4ae663bfd5a47670092efefde1e612ff6fb380e3fe272121fb4c689454edb24d7287a8e4797f85934ab9514d20e77fb7a7c6a7e4921c614934ea2bb26737c34d8995a6d5505644d020f16d6a"));
      REQUIRE(tx.from() == Address(std::string_view("0x69a7d445801fd4abD49aAb934E1Dd166891dED4B"), false));
      REQUIRE(tx.r() == uint256_t("29909956667277947405050350872105443335782893747187181267748506888571805563830"));
      REQUIRE(tx.v() == uint256_t("44"));
      REQUIRE(tx.s() == uint256_t("24318193605574201467197384723936824765330748853960002839515341225241303581922"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8e78697292f15c017824c96840208331394dd9bada36d88dac984e6d10a96061b65bc034a668617521be83d4cb8776129345086fa05e37c56ea3a9a24e102f3519b92f16f8f20281dcb28ef36eef04dd23f355693a24c4ce7de4ae663bfd5a47670092efefde1e612ff6fb380e3fe272121fb4c689454edb24d7287a8e4797f85934ab9514d20e77fb7a7c6a7e4921c614934ea2bb26737c34d8995a6d5505644d020f16d6a2ca042206f805ee0b2629bc85ea639a2ab22994b1dffcf15d29d021d840fc53cfbb6a035c39c1cf43b89e2657a20376a61ad450dec94ed9e4882f7fce5f9936ed804e2")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 5") {

      PrivKey privKey(Hex::toBytes("e3db00db986aa53cdec1e67b76f725e6f9046a81dc38b00daecae0210cc89051"));

      TxBlock tx(
        Address(std::string_view("0x583d6b8bf349f56e270915bebd69ddb7b7dba713"), false),
        Address(std::string_view("0xc189D8321256aA070763ceC24f38AA506C8578EB"), false),
        Hex::toBytes("0xb25155d71f72ef6b8dce2b77f841f5eb9f36843ecebafd4f41400c905c67d67237243f91a25fbdbfbd1825709bffe3f4e402475534891ae1648247cf8784d1d3af12d49b3b73d5e65498c70eee4061e434958cf5ce81d7b7fe466d33f4ed371aa48d1e3140eebc8dbb961994dfdf4a03554a48ef"),
        uint64_t(122532425620),
        uint256_t("446456494416889"),
        uint256_t("70761463340533973"),
        uint256_t("44975"),
        uint256_t("5931598"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("446456494416889"));
      REQUIRE(tx.gasPrice() == uint256_t("5931598"));
      REQUIRE(tx.gas() == uint256_t("44975"));
      REQUIRE(tx.to() == Address(std::string_view("0x583d6b8bf349f56e270915bebd69ddb7b7dba713"), false));
      REQUIRE(tx.value() == uint256_t("70761463340533973"));
      REQUIRE(tx.data() == Hex::toBytes("0xb25155d71f72ef6b8dce2b77f841f5eb9f36843ecebafd4f41400c905c67d67237243f91a25fbdbfbd1825709bffe3f4e402475534891ae1648247cf8784d1d3af12d49b3b73d5e65498c70eee4061e434958cf5ce81d7b7fe466d33f4ed371aa48d1e3140eebc8dbb961994dfdf4a03554a48ef"));
      REQUIRE(tx.from() == Address(std::string_view("0xc189D8321256aA070763ceC24f38AA506C8578EB"), false));
      REQUIRE(tx.r() == uint256_t("33728412420010110634544133772280666465509343987860490859919957127579282303316"));
      REQUIRE(tx.v() == uint256_t("245064851276"));
      REQUIRE(tx.s() == uint256_t("35971828805000347591997551718058589105226611649758279390323922004983806784788"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8ea8701960cc0c1e3f9835a824e82afaf94583d6b8bf349f56e270915bebd69ddb7b7dba71387fb652d0e3308d5b874b25155d71f72ef6b8dce2b77f841f5eb9f36843ecebafd4f41400c905c67d67237243f91a25fbdbfbd1825709bffe3f4e402475534891ae1648247cf8784d1d3af12d49b3b73d5e65498c70eee4061e434958cf5ce81d7b7fe466d33f4ed371aa48d1e3140eebc8dbb961994dfdf4a03554a48ef85390f00df4ca04a919adc0321d82992186315ae485bd0a1134d0f8c26b736698b31a28cb5e954a04f8755668cc129e982f19f73beadeb994f2ae1fcc2551787299c146ed6dd7514")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 6") {

      PrivKey privKey(Hex::toBytes("9c025e26973e72ad88a01398892bbacfffb869e370f9176a85f9cf00b5529f5c"));

      TxBlock tx(
        Address(std::string_view("0x03ab524baf9055f8eb6cbdef3cc046293b26ab5a"), false),
        Address(std::string_view("0x118336Ce4357582B7e5bD8527Af3C96AB0259A8E"), false),
        Hex::toBytes("0x27957117aca3bd31050081fc1bca3f832af0f67e5089cbd529d6084b02ea9a0590a48d96b9f54b50c5951633e4e986a21c0022020d7b5b90784a21fd331339"),
        uint64_t(3348),
        uint256_t("3298460568"),
        uint256_t("4010139807720159582705"),
        uint256_t("3868713964"),
        uint256_t("217651810835353"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("3298460568"));
      REQUIRE(tx.gasPrice() == uint256_t("217651810835353"));
      REQUIRE(tx.gas() == uint256_t("3868713964"));
      REQUIRE(tx.to() == Address(std::string_view("0x03ab524baf9055f8eb6cbdef3cc046293b26ab5a"), false));
      REQUIRE(tx.value() == uint256_t("4010139807720159582705"));
      REQUIRE(tx.data() == Hex::toBytes("0x27957117aca3bd31050081fc1bca3f832af0f67e5089cbd529d6084b02ea9a0590a48d96b9f54b50c5951633e4e986a21c0022020d7b5b90784a21fd331339"));
      REQUIRE(tx.from() == Address(std::string_view("0x118336Ce4357582B7e5bD8527Af3C96AB0259A8E"), false));
      REQUIRE(tx.r() == uint256_t("10502639027093652197415820990432805378277876376849664108846986732183881262294"));
      REQUIRE(tx.v() == uint256_t("6731"));
      REQUIRE(tx.s() == uint256_t("39402026168141829781202846260200752448138126045533918557460407513078471976452"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8b684c49a839886c5f402de9b9984e697e3ec9403ab524baf9055f8eb6cbdef3cc046293b26ab5a89d963de8c7d7d6019f1b83f27957117aca3bd31050081fc1bca3f832af0f67e5089cbd529d6084b02ea9a0590a48d96b9f54b50c5951633e4e986a21c0022020d7b5b90784a21fd331339821a4ba017384874c7074f80e8ff52172337e6ec29725b215f89e08fe2eb3acb94a824d6a0571cc1aa1fca868d11cb3702269103277ba21208bf1811397a0f83fa2375e604")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 7") {

      PrivKey privKey(Hex::toBytes("4cc17c025fe144c49b52657a6fb79351f9174b2a83a6bf61d6b2f3a642526555"));

      TxBlock tx(
        Address(std::string_view("0xd73cdabcec0cc182de86b4c012ce243ebbc3ab4b"), false),
        Address(std::string_view("0xF3C9Fa35d016159B7366fF3b39efb696537bf454"), false),
        Hex::toBytes("0xbe3ca20e4c8698bf768eeb743dd5217ce55075396f34b97a9d600b5f4bd8562867150e7e2976193b5b7134b7bc834148dc8d62f82547272a5573bfed8b52ff4b904c2e38b6b6a43814ee33d2e671f6fc533daacd3af259f286af3a77c73e49a4016a01b48d9e6a3a3ab2644457d91870b93ca32384fc3c86ad69"),
        uint64_t(127),
        uint256_t("317214396621"),
        uint256_t("4075294"),
        uint256_t("64100"),
        uint256_t("58084728543541406"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("317214396621"));
      REQUIRE(tx.gasPrice() == uint256_t("58084728543541406"));
      REQUIRE(tx.gas() == uint256_t("64100"));
      REQUIRE(tx.to() == Address(std::string_view("0xd73cdabcec0cc182de86b4c012ce243ebbc3ab4b"), false));
      REQUIRE(tx.value() == uint256_t("4075294"));
      REQUIRE(tx.data() == Hex::toBytes("0xbe3ca20e4c8698bf768eeb743dd5217ce55075396f34b97a9d600b5f4bd8562867150e7e2976193b5b7134b7bc834148dc8d62f82547272a5573bfed8b52ff4b904c2e38b6b6a43814ee33d2e671f6fc533daacd3af259f286af3a77c73e49a4016a01b48d9e6a3a3ab2644457d91870b93ca32384fc3c86ad69"));
      REQUIRE(tx.from() == Address(std::string_view("0xF3C9Fa35d016159B7366fF3b39efb696537bf454"), false));
      REQUIRE(tx.r() == uint256_t("64964934541771256990259264250549852051238662887106602039427533313822255957922"));
      REQUIRE(tx.v() == uint256_t("289"));
      REQUIRE(tx.s() == uint256_t("6301098062197470351339339036831217944112883069997791278399467269450015569312"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8eb8549db7390cd87ce5bc0bbb6789e82fa6494d73cdabcec0cc182de86b4c012ce243ebbc3ab4b833e2f1eb87abe3ca20e4c8698bf768eeb743dd5217ce55075396f34b97a9d600b5f4bd8562867150e7e2976193b5b7134b7bc834148dc8d62f82547272a5573bfed8b52ff4b904c2e38b6b6a43814ee33d2e671f6fc533daacd3af259f286af3a77c73e49a4016a01b48d9e6a3a3ab2644457d91870b93ca32384fc3c86ad69820121a08fa0d993d03d62270eb1d0a6af36302deb66b7318cdff66a361dbbca098b47a2a00dee4b8ce2459940c008d8a1be69d413bf204da5617dcbd04c238009b42df5a0")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 8") {

      PrivKey privKey(Hex::toBytes("65a0c68847d8e5a08d086b688d8a70f3064519c962b3ce51d38e2c7876bb4a90"));

      TxBlock tx(
        Address(std::string_view("0xc6ff22a04cc9ed476eede000eb0ac51661225bde"), false),
        Address(std::string_view("0x48c3d257046F1BCf0E341dDf337E86E88000Fb14"), false),
        Hex::toBytes("0x1fa09bf8d8cd7ce5f37fd25dad940e9623433d1c4916b8d2f43240b3e38c1de579e2b064f7f0b0ec66af28c8a8f4ed19dfc033540e37a1ff95e5f5d95b7f3fc48922ab488e008f96e521"),
        uint64_t(24),
        uint256_t("994652379366"),
        uint256_t("632687366680052342885132338693323"),
        uint256_t("0"),
        uint256_t("0"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("994652379366"));
      REQUIRE(tx.gasPrice() == uint256_t("0"));
      REQUIRE(tx.gas() == 0);
      REQUIRE(tx.to() == Address(std::string_view("0xc6ff22a04cc9ed476eede000eb0ac51661225bde"), false));
      REQUIRE(tx.value() == uint256_t("632687366680052342885132338693323"));
      REQUIRE(tx.data() == Hex::toBytes("0x1fa09bf8d8cd7ce5f37fd25dad940e9623433d1c4916b8d2f43240b3e38c1de579e2b064f7f0b0ec66af28c8a8f4ed19dfc033540e37a1ff95e5f5d95b7f3fc48922ab488e008f96e521"));
      REQUIRE(tx.from() == Address(std::string_view("0x48c3d257046F1BCf0E341dDf337E86E88000Fb14"), false));
      REQUIRE(tx.r() == uint256_t("97479032438381344463729831536473987105298299390804055365328274435247431351780"));
      REQUIRE(tx.v() == uint256_t("84"));
      REQUIRE(tx.s() == uint256_t("19744900436705738079678163716238756248801587034434413912451019780848023872961"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8bb85e795e6d8e6808094c6ff22a04cc9ed476eede000eb0ac51661225bde8e1f31a323866448f957c82f1f84cbb84a1fa09bf8d8cd7ce5f37fd25dad940e9623433d1c4916b8d2f43240b3e38c1de579e2b064f7f0b0ec66af28c8a8f4ed19dfc033540e37a1ff95e5f5d95b7f3fc48922ab488e008f96e52154a0d7832d5ad5c45cd0c0369acad76d6e823447db6d9e9c5455fd91b3b5e63dfde4a02ba737b5720585b074ff0a52fc2ed21a07570dc6fa215bbfa8131eb51ffda5c1")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 9") {

      PrivKey privKey(Hex::toBytes("dc6ec2128df7cb4d99a32c35e18f14134d81bce141efadb843c65630e97de569"));

      TxBlock tx(
        Address(std::string_view("0xf3fd73725fe9dbc1c83b734ee624e3722d104508"), false),
        Address(std::string_view("0xC8b3C40aF3826Ad924c38C1B9929b30922258f9E"), false),
        Hex::toBytes("0xd798df4008c491d5fd4109efc3c69fcbea65857915bbcecf8b2c7f35ed364df117ef2892c74a84d658aace2e5b9f3f95c27fbcdfb7ef32cd778b91ca788c60f5bb"),
        uint64_t(8183060),
        uint256_t("8316924"),
        uint256_t("34912629659894336854262533947"),
        uint256_t("14863235"),
        uint256_t("28515024796434919"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("8316924"));
      REQUIRE(tx.gasPrice() == uint256_t("28515024796434919"));
      REQUIRE(tx.gas() == uint256_t("14863235"));
      REQUIRE(tx.to() == Address(std::string_view("0xf3fd73725fe9dbc1c83b734ee624e3722d104508"), false));
      REQUIRE(tx.value() == uint256_t("34912629659894336854262533947"));
      REQUIRE(tx.data() == Hex::toBytes("0xd798df4008c491d5fd4109efc3c69fcbea65857915bbcecf8b2c7f35ed364df117ef2892c74a84d658aace2e5b9f3f95c27fbcdfb7ef32cd778b91ca788c60f5bb"));
      REQUIRE(tx.from() == Address(std::string_view("0xC8b3C40aF3826Ad924c38C1B9929b30922258f9E"), false));
      REQUIRE(tx.r() == uint256_t("17427676054363578124022388637771774781667892741630619187104370396502693548914"));
      REQUIRE(tx.v() == uint256_t("16366156"));
      REQUIRE(tx.s() == uint256_t("28417473914346597470077860814587265403812237143524854486526314186493607674367"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8bb837ee7fc87654e4393c0c1e783e2cb8394f3fd73725fe9dbc1c83b734ee624e3722d1045088c70cf0ce638d2b0e9b167cf3bb841d798df4008c491d5fd4109efc3c69fcbea65857915bbcecf8b2c7f35ed364df117ef2892c74a84d658aace2e5b9f3f95c27fbcdfb7ef32cd778b91ca788c60f5bb83f9ba4ca02687b710215d7cc9e970c8d40b536ceae7c455f4a314cf722b2f93b83b989372a03ed3b8624d0a3dc0bb580bf2cecf16b6571a8f1b60e85b1dd48f6377d1ee01ff")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 10") {

      PrivKey privKey(Hex::toBytes("5def8a100e12adc56d41389d3ecf21ccd8097f4723fd806b1464b0e551999ba0"));

      TxBlock tx(
        Address(std::string_view("0xc51c568dc73472818ef69bfdb3fb88a4413e2c2a"), false),
        Address(std::string_view("0xBa2555Cd4066B26187B9a7D3e9aCF8D660fc13eb"), false),
        Hex::toBytes("0x641153d6d7aa0fbd6b04cd10e096f0d6e08f05ef94f1f9298cb9e64bbd6c8511fa03c705db9a51c64b5b2f327198880a477c1d4f6a03b000bdaa0a72a129eca78f0aaad2a6a2cdf7a79b50f89a49903df3f60db2801ed1159d0ace69843211b8554ebcbc1ab832965f04"),
        uint64_t(178),
        uint256_t("5589524"),
        uint256_t("2162595421881230633"),
        uint256_t("54269499395372"),
        uint256_t("0"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("5589524"));
      REQUIRE(tx.gasPrice() == uint256_t("0"));
      REQUIRE(tx.gas() == uint256_t("54269499395372"));
      REQUIRE(tx.to() == Address(std::string_view("0xc51c568dc73472818ef69bfdb3fb88a4413e2c2a"), false));
      REQUIRE(tx.value() == uint256_t("2162595421881230633"));
      REQUIRE(tx.data() == Hex::toBytes("0x641153d6d7aa0fbd6b04cd10e096f0d6e08f05ef94f1f9298cb9e64bbd6c8511fa03c705db9a51c64b5b2f327198880a477c1d4f6a03b000bdaa0a72a129eca78f0aaad2a6a2cdf7a79b50f89a49903df3f60db2801ed1159d0ace69843211b8554ebcbc1ab832965f04"));
      REQUIRE(tx.from() == Address(std::string_view("0xBa2555Cd4066B26187B9a7D3e9aCF8D660fc13eb"), false));
      REQUIRE(tx.r() == uint256_t("99344982956302989805388368010123744189350159115162537919938460660320251445938"));
      REQUIRE(tx.v() == uint256_t("392"));
      REQUIRE(tx.s() == uint256_t("6391088547484743499990842747328714291715718359240770801744594306898087206702"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8db83554a148086315b9a3bcd2c94c51c568dc73472818ef69bfdb3fb88a4413e2c2a881e0315140a1de529b86a641153d6d7aa0fbd6b04cd10e096f0d6e08f05ef94f1f9298cb9e64bbd6c8511fa03c705db9a51c64b5b2f327198880a477c1d4f6a03b000bdaa0a72a129eca78f0aaad2a6a2cdf7a79b50f89a49903df3f60db2801ed1159d0ace69843211b8554ebcbc1ab832965f04820188a0dba3448a36ed77b8cdc15eb6731ae2f9bcefc011932064780f3c1c86ac7c82b2a00e213a597fca7d29a7c0646d125c03fc6361c2df2ceadc6a34d32bc92b4d5f2e")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 11") {

      PrivKey privKey(Hex::toBytes("f37bb252d6e7d0a4332d020db4e46d9570bdc5af3026426e5a7ef2373a769f1b"));

      TxBlock tx(
        Address(std::string_view("0x23d1a55e919efb2ec8f5c12dc41f0c4c1c1dba6b"), false),
        Address(std::string_view("0x3176efd1bDf2A3dcaC2470bc18Be2974D68DA500"), false),
        Hex::toBytes("0x66136b47795c389b015845e6e5b07b6405c37fe21f4be1535db3479f62d8c62b9c7ef715513edc407eb882448c199df84440461d7e1108e17d57ebf3eed8abdf1f68e64787261bf1438432e1435ce7911e54a2d216d811ba0c6d450a96dfcbe4ec3287"),
        uint64_t(5384800),
        uint256_t("154444561560301"),
        uint256_t("0"),
        uint256_t("675141491205"),
        uint256_t("220"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("154444561560301"));
      REQUIRE(tx.gasPrice() == uint256_t("220"));
      REQUIRE(tx.gas() == uint256_t("675141491205"));
      REQUIRE(tx.to() == Address(std::string_view("0x23d1a55e919efb2ec8f5c12dc41f0c4c1c1dba6b"), false));
      REQUIRE(tx.value() == uint256_t("0"));
      REQUIRE(tx.data() == Hex::toBytes("0x66136b47795c389b015845e6e5b07b6405c37fe21f4be1535db3479f62d8c62b9c7ef715513edc407eb882448c199df84440461d7e1108e17d57ebf3eed8abdf1f68e64787261bf1438432e1435ce7911e54a2d216d811ba0c6d450a96dfcbe4ec3287"));
      REQUIRE(tx.from() == Address(std::string_view("0x3176efd1bDf2A3dcaC2470bc18Be2974D68DA500"), false));
      REQUIRE(tx.r() == uint256_t("32631024349644628398578636735434498467052748163886423657924786949277916936096"));
      REQUIRE(tx.v() == uint256_t("10769635"));
      REQUIRE(tx.s() == uint256_t("44914518430547189848816215656846956891591899596305206936635244683536732051022"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8d0868c776d3ab2ed81dc859d31919a059423d1a55e919efb2ec8f5c12dc41f0c4c1c1dba6b80b86366136b47795c389b015845e6e5b07b6405c37fe21f4be1535db3479f62d8c62b9c7ef715513edc407eb882448c199df84440461d7e1108e17d57ebf3eed8abdf1f68e64787261bf1438432e1435ce7911e54a2d216d811ba0c6d450a96dfcbe4ec328783a454e3a04824815a359690ce53c1f3b192f8ff315882196d4a6e930144990702230d37a0a0634cb7702d701b18f8a0ac876ce768404e726bfdd0ed410cf008942a7d58d64e")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 12") {

      PrivKey privKey(Hex::toBytes("0631b14107a829db59a741be5da8e94a560861d50ab945f1965c4db8c2d996ab"));

      TxBlock tx(
        Address(std::string_view("0x93725531e3371e2b0baeb1f75c5b5102c04bf443"), false),
        Address(std::string_view("0xBE953985d1f68bf58832519934466625513B9b82"), false),
        Hex::toBytes("0xbf2ac52475bb963dcc43d6f10349a55e1f30899461e358e0adaa237bb6b9afe845aaffea7087eda3f3c38b81f167fa694c775c33897768da59f74552ec800afe193b87e8443b812c154c690e9b30eda40adad171c0dfbfb5c8c97c755583a7fdf06a8377a1e2dc9a97eb8709660d0fee6a09dfb8"),
        uint64_t(16698672),
        uint256_t("15604721365784204"),
        uint256_t("278462507804053797447675096"),
        uint256_t("359889882875"),
        uint256_t("975"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("15604721365784204"));
      REQUIRE(tx.gasPrice() == uint256_t("975"));
      REQUIRE(tx.gas() == uint256_t("359889882875"));
      REQUIRE(tx.to() == Address(std::string_view("0x93725531e3371e2b0baeb1f75c5b5102c04bf443"), false));
      REQUIRE(tx.value() == uint256_t("278462507804053797447675096"));
      REQUIRE(tx.data() == Hex::toBytes("0xbf2ac52475bb963dcc43d6f10349a55e1f30899461e358e0adaa237bb6b9afe845aaffea7087eda3f3c38b81f167fa694c775c33897768da59f74552ec800afe193b87e8443b812c154c690e9b30eda40adad171c0dfbfb5c8c97c755583a7fdf06a8377a1e2dc9a97eb8709660d0fee6a09dfb8"));
      REQUIRE(tx.from() == Address(std::string_view("0xBE953985d1f68bf58832519934466625513B9b82"), false));
      REQUIRE(tx.r() == uint256_t("17103637993850359173595705482952538421627847171999677145514249041025481853701"));
      REQUIRE(tx.v() == uint256_t("33397380"));
      REQUIRE(tx.s() == uint256_t("8673049231952622709131466693865490621315805761444184653945596269384612129353"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8ef8737706951d3968c8203cf8553cb1bcefb9493725531e3371e2b0baeb1f75c5b5102c04bf4438be656bacbbc8d9c4ef82cd8b874bf2ac52475bb963dcc43d6f10349a55e1f30899461e358e0adaa237bb6b9afe845aaffea7087eda3f3c38b81f167fa694c775c33897768da59f74552ec800afe193b87e8443b812c154c690e9b30eda40adad171c0dfbfb5c8c97c755583a7fdf06a8377a1e2dc9a97eb8709660d0fee6a09dfb88401fd9a84a025d050e8758eab522a93642c3e36faa5855aa5e177f4e2ad6dd2839e4ec04b05a0132cc59b9050ed354552d6853c9e0aca45682ecf546bb1e0d406dc26ba832a49")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 13") {

      PrivKey privKey(Hex::toBytes("300da4395e4660e3610e81077635f615019e14e1b2a4cff99031ac35a468afe5"));

      TxBlock tx(
        Address(std::string_view("0x912bd947f01f8324bbc2f22348f09df0b0bb077f"), false),
        Address(std::string_view("0x16EE953f0E9D774BebE862c7d308ef82B7092a03"), false),
        Hex::toBytes("0xb41dff6f4be0db9f6ed217bb7955123ba9cbe30468e5f152a4273f3b483b752834c8bdd97127047e43fe8d117cffbd31a1b28481623bf9fbae09a86c024316af618e04cf896f9b37eac968c051da82f3b3757d9d7dca88eed7dd959abbf1154b64c3c22a2c3e92360ee02c05ff6b1af4"),
        uint64_t(2673409298),
        uint256_t("155862611548076"),
        uint256_t("0"),
        uint256_t("144"),
        uint256_t("0"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("155862611548076"));
      REQUIRE(tx.gasPrice() == uint256_t("0"));
      REQUIRE(tx.gas() == uint256_t("144"));
      REQUIRE(tx.to() == Address(std::string_view("0x912bd947f01f8324bbc2f22348f09df0b0bb077f"), false));
      REQUIRE(tx.value() == uint256_t("0"));
      REQUIRE(tx.data() == Hex::toBytes("0xb41dff6f4be0db9f6ed217bb7955123ba9cbe30468e5f152a4273f3b483b752834c8bdd97127047e43fe8d117cffbd31a1b28481623bf9fbae09a86c024316af618e04cf896f9b37eac968c051da82f3b3757d9d7dca88eed7dd959abbf1154b64c3c22a2c3e92360ee02c05ff6b1af4"));
      REQUIRE(tx.from() == Address(std::string_view("0x16EE953f0E9D774BebE862c7d308ef82B7092a03"), false));
      REQUIRE(tx.r() == uint256_t("67746362486140175217302158956936035599223008617663848430485657157695181274142"));
      REQUIRE(tx.v() == uint256_t("5346818632"));
      REQUIRE(tx.s() == uint256_t("26826830547398708938164866044010739572088980435352295316285545030194091344786"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8da868dc1979857ac80819094912bd947f01f8324bbc2f22348f09df0b0bb077f80b870b41dff6f4be0db9f6ed217bb7955123ba9cbe30468e5f152a4273f3b483b752834c8bdd97127047e43fe8d117cffbd31a1b28481623bf9fbae09a86c024316af618e04cf896f9b37eac968c051da82f3b3757d9d7dca88eed7dd959abbf1154b64c3c22a2c3e92360ee02c05ff6b1af485013eb1fa48a095c7151433f1baf0408295d0545c29776a3053b056152a56b446095f9fe5bc1ea03b4f72b0b2d7c619453dbbb455aa95e1fb2a526dea7e241ceb4cdcb955814392")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 14") {

      PrivKey privKey(Hex::toBytes("f91bc43eadd9db7314012c8980ac2f9493decf208a342a65867d5b3fe04b45e5"));

      TxBlock tx(
        Address(std::string_view("0x3a8970efe474c3770f7a6f4d68fcd2d84a5444ee"), false),
        Address(std::string_view("0x86aAcb83B27A8BAcfe0A7f1e850c4602f25Fd773"), false),
        Hex::toBytes("0xd4210fb5fe6b195ebc558373e7f724aecc3eeb0a4c852118bf61016f1bf2d8fb1da0b946790ac12cc009a4fde91d96833d5641054abb9bab3ac0f7c2c01c24ef9ba1c5d1c7a58b1e8ade7f06a6ac33a3ce95d66f082bc587900379b7042ccefe1e501ad2202c39da"),
        uint64_t(1061631902726),
        uint256_t("65517300210462"),
        uint256_t("49889051552335152"),
        uint256_t("56665"),
        uint256_t("0"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("65517300210462"));
      REQUIRE(tx.gasPrice() == uint256_t("0"));
      REQUIRE(tx.gas() == uint256_t("56665"));
      REQUIRE(tx.to() == Address(std::string_view("0x3a8970efe474c3770f7a6f4d68fcd2d84a5444ee"), false));
      REQUIRE(tx.value() == uint256_t("49889051552335152"));
      REQUIRE(tx.data() == Hex::toBytes("0xd4210fb5fe6b195ebc558373e7f724aecc3eeb0a4c852118bf61016f1bf2d8fb1da0b946790ac12cc009a4fde91d96833d5641054abb9bab3ac0f7c2c01c24ef9ba1c5d1c7a58b1e8ade7f06a6ac33a3ce95d66f082bc587900379b7042ccefe1e501ad2202c39da"));
      REQUIRE(tx.from() == Address(std::string_view("0x86aAcb83B27A8BAcfe0A7f1e850c4602f25Fd773"), false));
      REQUIRE(tx.r() == uint256_t("67841823613416993619816221933121431031920019476645394001955966189999424020285"));
      REQUIRE(tx.v() == uint256_t("2123263805487"));
      REQUIRE(tx.s() == uint256_t("44234357735577328652258117694498591999525436810732712943221466902197825394854"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8db863b966f67db1e8082dd59943a8970efe474c3770f7a6f4d68fcd2d84a5444ee87b13dd3fbf54530b868d4210fb5fe6b195ebc558373e7f724aecc3eeb0a4c852118bf61016f1bf2d8fb1da0b946790ac12cc009a4fde91d96833d5641054abb9bab3ac0f7c2c01c24ef9ba1c5d1c7a58b1e8ade7f06a6ac33a3ce95d66f082bc587900379b7042ccefe1e501ad2202c39da8601ee5c62882fa095fd1c8635167fba1b0c66e0d6a7c629c79836d1b19c151094450e2217e01f3da061cbc25dd859394b96bd14255371b0ad169936cbc90dbfb9c7a86e9f76ef58a6")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 15") {

      PrivKey privKey(Hex::toBytes("84b594d2f4a3b61092e3f4345e1ea7ad45575f14b3293b25ca6f006f3a4edf17"));

      TxBlock tx(
        Address(std::string_view("0x8d65b2a2437da6a0ff1e575a75f8f4fe22d056ea"), false),
        Address(std::string_view("0x221b103DAa30A040A0841C6dA0d18517E8543ADf"), false),
        Hex::toBytes("0xbea2c51b92c070a2ed7d78e3ccc3969d32b6d4db8d5af381f84a7c7dd9b4189872d328a189b653"),
        uint64_t(3908908295),
        uint256_t("101"),
        uint256_t("48204801149425226"),
        uint256_t("0"),
        uint256_t("0"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("101"));
      REQUIRE(tx.gasPrice() == uint256_t("0"));
      REQUIRE(tx.gas() == 0);
      REQUIRE(tx.to() == Address(std::string_view("0x8d65b2a2437da6a0ff1e575a75f8f4fe22d056ea"), false));
      REQUIRE(tx.value() == uint256_t("48204801149425226"));
      REQUIRE(tx.data() == Hex::toBytes("0xbea2c51b92c070a2ed7d78e3ccc3969d32b6d4db8d5af381f84a7c7dd9b4189872d328a189b653"));
      REQUIRE(tx.from() == Address(std::string_view("0x221b103DAa30A040A0841C6dA0d18517E8543ADf"), false));
      REQUIRE(tx.r() == uint256_t("40077103472939213917216714088794330167954387209849101968481696097270876379597"));
      REQUIRE(tx.v() == uint256_t("7817816625"));
      REQUIRE(tx.s() == uint256_t("11439632586527558346451877474182926292625972466444776731074339269901455249405"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f890658080948d65b2a2437da6a0ff1e575a75f8f4fe22d056ea87ab4202e0fad64aa7bea2c51b92c070a2ed7d78e3ccc3969d32b6d4db8d5af381f84a7c7dd9b4189872d328a189b6538501d1fa6a31a0589ad633091e9ef5cafe3fb863a66c8a39c9eab72f189b5f0ee2bbc11c9899cda0194a9a4383f94c787a2c40839683ff546680f5a67a932f1baee428818500cffd")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 16") {

      PrivKey privKey(Hex::toBytes("edbfd7f43c3c30fb5c745de37e8fe8af30bdb1ae8b150158cc737b4419345fe1"));

      TxBlock tx(
        Address(std::string_view("0xf13174546ec556878140dd71b46f6f4da9e31993"), false),
        Address(std::string_view("0x2362513232727Dc75FB039a3ED930362E048BB84"), false),
        Hex::toBytes("0x0238ac252b052fffc4fbe937b2d2f177f4bb71ed6fdfd5db7aec43c6ca517354f9e40845ac8c885a9b8a"),
        uint64_t(257916032524),
        uint256_t("276658344177"),
        uint256_t("0"),
        uint256_t("736708869608"),
        uint256_t("13607773"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("276658344177"));
      REQUIRE(tx.gasPrice() == uint256_t("13607773"));
      REQUIRE(tx.gas() == uint256_t("736708869608"));
      REQUIRE(tx.to() == Address(std::string_view("0xf13174546ec556878140dd71b46f6f4da9e31993"), false));
      REQUIRE(tx.value() == uint256_t("0"));
      REQUIRE(tx.data() == Hex::toBytes("0x0238ac252b052fffc4fbe937b2d2f177f4bb71ed6fdfd5db7aec43c6ca517354f9e40845ac8c885a9b8a"));
      REQUIRE(tx.from() == Address(std::string_view("0x2362513232727Dc75FB039a3ED930362E048BB84"), false));
      REQUIRE(tx.r() == uint256_t("60022452379101448916002375970910561335568670381562170901656745068511957729637"));
      REQUIRE(tx.v() == uint256_t("515832065084"));
      REQUIRE(tx.s() == uint256_t("35912843368171496208075247071389099629964968890061199429953144105302029573145"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f89985406a1f50f183cfa35d85ab87453de894f13174546ec556878140dd71b46f6f4da9e3199380aa0238ac252b052fffc4fbe937b2d2f177f4bb71ed6fdfd5db7aec43c6ca517354f9e40845ac8c885a9b8a857819fcac3ca084b3810f706632ecb179af29220867baf8c450c4e13f22cc98b43ff227f1c165a04f65f2f319553c5edad224760147ae501125ee4a98932117cad354905e1d1c19")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 17") {

      PrivKey privKey(Hex::toBytes("160f60d083fc9e64404d0c4d80763b4bf08d92bfd26975def9cec86c0d4e530f"));

      TxBlock tx(
        Address(std::string_view("0xfc3106d6242b762c37f21c021395dbfdc9a2c0d0"), false),
        Address(std::string_view("0x46489BE9A8614f072730892f1231Ddc66141cee5"), false),
        Hex::toBytes("0xbf32ad6cc8188ff0ac6a14727bad727dcdec7f4b17a8b99499946f386d38a69c1945077f7b9a199a2b6261f37700affb3c334e9e2ff2f088ab47f407559920e0e18e565dab9c6af48fffc3b2f49db7e0175ec32bfc30c542753e7f299ef8cfe3c3b3b555a937b6bff6"),
        uint64_t(3859201),
        uint256_t("64495395256274639"),
        uint256_t("115874354101689129224243392"),
        uint256_t("158090014746066"),
        uint256_t("442934797513"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("64495395256274639"));
      REQUIRE(tx.gasPrice() == uint256_t("442934797513"));
      REQUIRE(tx.gas() == uint256_t("158090014746066"));
      REQUIRE(tx.to() == Address(std::string_view("0xfc3106d6242b762c37f21c021395dbfdc9a2c0d0"), false));
      REQUIRE(tx.value() == uint256_t("115874354101689129224243392"));
      REQUIRE(tx.data() == Hex::toBytes("0xbf32ad6cc8188ff0ac6a14727bad727dcdec7f4b17a8b99499946f386d38a69c1945077f7b9a199a2b6261f37700affb3c334e9e2ff2f088ab47f407559920e0e18e565dab9c6af48fffc3b2f49db7e0175ec32bfc30c542753e7f299ef8cfe3c3b3b555a937b6bff6"));
      REQUIRE(tx.from() == Address(std::string_view("0x46489BE9A8614f072730892f1231Ddc66141cee5"), false));
      REQUIRE(tx.r() == uint256_t("24210201474944617136757839676808304308300143775095278614898013664302185954308"));
      REQUIRE(tx.v() == uint256_t("7718438"));
      REQUIRE(tx.s() == uint256_t("25615831148785201902918916971365977814809397621226041755544528275102948635028"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8e787e5223863e60ecf856720f8a4c9868fc8332be5d294fc3106d6242b762c37f21c021395dbfdc9a2c0d08b5fd95952973e70427608c0b869bf32ad6cc8188ff0ac6a14727bad727dcdec7f4b17a8b99499946f386d38a69c1945077f7b9a199a2b6261f37700affb3c334e9e2ff2f088ab47f407559920e0e18e565dab9c6af48fffc3b2f49db7e0175ec32bfc30c542753e7f299ef8cfe3c3b3b555a937b6bff68375c626a035867d0a33d495f09fe0fdadf1d6a8e7099ec0305a30ef52f5e6aa86146dbc04a038a20bf0853be3332b286b7d6e6dd25bb859ef98f2d2f15f0a9edbde5e4fb194")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 18") {

      PrivKey privKey(Hex::toBytes("8db207513831bfd954b280e9b7c26c9116a574e7a4da57ad8eb23ef9f20ce13d"));

      TxBlock tx(
        Address(std::string_view("0x2b990b906b7fda0e388fcce3c0a93324f2bd57a7"), false),
        Address(std::string_view("0xBbFad800C6ec7bF8f8404CbFCCFF9f563E1A2852"), false),
        Hex::toBytes("0x13e6cf536ac96d352af5272e7ea1ef859e13b79405fcd6237f1119dfedf8452de41f9ea1b262234272b67566e5bb0d91e7e8bf978c461029533cd447058e4b74e631365f439eecf6b14e"),
        uint64_t(1580898916),
        uint256_t("28"),
        uint256_t("4275786325748775013771"),
        uint256_t("575285134380"),
        uint256_t("0"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("28"));
      REQUIRE(tx.gasPrice() == uint256_t("0"));
      REQUIRE(tx.gas() == uint256_t("575285134380"));
      REQUIRE(tx.to() == Address(std::string_view("0x2b990b906b7fda0e388fcce3c0a93324f2bd57a7"), false));
      REQUIRE(tx.value() == uint256_t("4275786325748775013771"));
      REQUIRE(tx.data() == Hex::toBytes("0x13e6cf536ac96d352af5272e7ea1ef859e13b79405fcd6237f1119dfedf8452de41f9ea1b262234272b67566e5bb0d91e7e8bf978c461029533cd447058e4b74e631365f439eecf6b14e"));
      REQUIRE(tx.from() == Address(std::string_view("0xBbFad800C6ec7bF8f8404CbFCCFF9f563E1A2852"), false));
      REQUIRE(tx.r() == uint256_t("41740906523883930957854113441268343865860860371979365759638911414898997177652"));
      REQUIRE(tx.v() == uint256_t("3161797868"));
      REQUIRE(tx.s() == uint256_t("47985077140750179505598158282016296349900574841512331399834843738166849697029"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8ba1c808585f1aa842c942b990b906b7fda0e388fcce3c0a93324f2bd57a789e7ca74912e80664d8bb84a13e6cf536ac96d352af5272e7ea1ef859e13b79405fcd6237f1119dfedf8452de41f9ea1b262234272b67566e5bb0d91e7e8bf978c461029533cd447058e4b74e631365f439eecf6b14e84bc7534eca05c48841022f78e529e27c8b766464823cafc9d7396a482238f5b5c744e51ad34a06a169758b780d3c14959a9d6b20cd378a1d19b71dda0bd0b6b9384d930abf505")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 19") {

      PrivKey privKey(Hex::toBytes("36ad4e6f2cd9475c2dfe1a2cffe9a34ce21b0c23e3042345b4abae3914466664"));

      TxBlock tx(
        Address(std::string_view("0x0ec60eb0866819b94f347b5bb651c5129f988f59"), false),
        Address(std::string_view("0xa17f2cC8a5b23412adB8A80493d9535cAeA308FF"), false),
        Hex::toBytes("0x887c6c6bdf2df9f0ba4c9d77a5478b86b442c4a1a7f48e64951023813eb49e1995f42b0f7362e2308933ce6f46020f6469"),
        uint64_t(4184035),
        uint256_t("51399654609967"),
        uint256_t("33"),
        uint256_t("188"),
        uint256_t("0"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("51399654609967"));
      REQUIRE(tx.gasPrice() == uint256_t("0"));
      REQUIRE(tx.gas() == uint256_t("188"));
      REQUIRE(tx.to() == Address(std::string_view("0x0ec60eb0866819b94f347b5bb651c5129f988f59"), false));
      REQUIRE(tx.value() == uint256_t("33"));
      REQUIRE(tx.data() == Hex::toBytes("0x887c6c6bdf2df9f0ba4c9d77a5478b86b442c4a1a7f48e64951023813eb49e1995f42b0f7362e2308933ce6f46020f6469"));
      REQUIRE(tx.from() == Address(std::string_view("0xa17f2cC8a5b23412adB8A80493d9535cAeA308FF"), false));
      REQUIRE(tx.r() == uint256_t("14499699160040555180146549805717468339911654693717860252463463104748588844114"));
      REQUIRE(tx.v() == uint256_t("8368105"));
      REQUIRE(tx.s() == uint256_t("46347313227046846421925448360826696697551678454434037437567601096488353974422"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f898862ebf6a27942f8081bc940ec60eb0866819b94f347b5bb651c5129f988f5921b1887c6c6bdf2df9f0ba4c9d77a5478b86b442c4a1a7f48e64951023813eb49e1995f42b0f7362e2308933ce6f46020f6469837fafe9a0200e89f5134d31d9f13243d091aa3f9e917a680ebdca15092a9581fd78cb4452a06677a650e55f5dee646614f88458df50f51ff4a3124b735e2393de3222aec896")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Advanced Transaction 20") {

      PrivKey privKey(Hex::toBytes("a2419cbfc88964c88e4f4703645152c7ba94330c0adbb8a3e8b2271869426abf"));

      TxBlock tx(
        Address(std::string_view("0xf3611f06e176b22a95e98aa4c1cae63caadff7a5"), false),
        Address(std::string_view("0xE511558b42150B7385eA4357c09c57BE1768F589"), false),
        Hex::toBytes("0x02c3a9dd32683c6ceb4976dfa1529025747527baef0a25512c20d4dc9c099d69a32ba42e5e23d06f4d72d00faf83810d1198d7143df1cf0f1e3cefef9fb51caa7805e2562f56da8e51f7f187320a4ad42825eb7bb8dfaafd37acb153cafa113e9c385ef4a6ed2d50b4a3994171ccbd42"),
        uint64_t(709059731),
        uint256_t("697808132185"),
        uint256_t("16140914329333253289"),
        uint256_t("14735300"),
        uint256_t("290856003134"),
        privKey
      );

    

      REQUIRE(tx.nonce() == uint256_t("697808132185"));
      REQUIRE(tx.gasPrice() == uint256_t("290856003134"));
      REQUIRE(tx.gas() == uint256_t("14735300"));
      REQUIRE(tx.to() == Address(std::string_view("0xf3611f06e176b22a95e98aa4c1cae63caadff7a5"), false));
      REQUIRE(tx.value() == uint256_t("16140914329333253289"));
      REQUIRE(tx.data() == Hex::toBytes("0x02c3a9dd32683c6ceb4976dfa1529025747527baef0a25512c20d4dc9c099d69a32ba42e5e23d06f4d72d00faf83810d1198d7143df1cf0f1e3cefef9fb51caa7805e2562f56da8e51f7f187320a4ad42825eb7bb8dfaafd37acb153cafa113e9c385ef4a6ed2d50b4a3994171ccbd42"));
      REQUIRE(tx.from() == Address(std::string_view("0xE511558b42150B7385eA4357c09c57BE1768F589"), false));
      REQUIRE(tx.r() == uint256_t("23634387011009566906155224433171822207310979658358907079543748173698759947595"));
      REQUIRE(tx.v() == uint256_t("1418119497"));
      REQUIRE(tx.s() == uint256_t("48220139809513419283305973937806473823488590716684626195928088664390689451862"));
      REQUIRE_THAT(tx.rlpSerialize(), Equals(Hex::toBytes("f8e785a2789b18598543b85e663e83e0d7c494f3611f06e176b22a95e98aa4c1cae63caadff7a588e0000c1075ebc4a9b87002c3a9dd32683c6ceb4976dfa1529025747527baef0a25512c20d4dc9c099d69a32ba42e5e23d06f4d72d00faf83810d1198d7143df1cf0f1e3cefef9fb51caa7805e2562f56da8e51f7f187320a4ad42825eb7bb8dfaafd37acb153cafa113e9c385ef4a6ed2d50b4a3994171ccbd42845486c949a0344096c8b08bf9d7c28a1708b5b7291a43dc84003493504dc177c86ff45c754ba06a9ba1c7af3e612a42be6a01079085572fba27d56280288f2d5cf4c917a56f56")));
      REQUIRE(TxBlock(tx.rlpSerialize()) == tx);
    }    
  }

  TEST_CASE("TxValidator", "[utils]") {
    SECTION("Simple Transaction 1") {
      TxValidator tx(Hex::toBytes("f845808026a08a4591f48d6307bb4cb8a0b0088b544d923d00bc1f264c3fdf16f946fdee0b34a077a6f6e8b3e78b45478827604f070d03060f413d823eae7fab9b139be7a41d81"));
    
      REQUIRE(tx.from() == Address(std::string_view("0x684e1df8bc220d0361104900c6bfc3cb432a7f91"), false));
      REQUIRE(tx.data() == "");
      REQUIRE(tx.chainId() == 1);
      REQUIRE(tx.nHeight() == 0);
      REQUIRE(tx.r() == uint256_t("62542092898297513318091065473132818019153676414492161685328614432215420242740"));
      REQUIRE(tx.v() == uint256_t("38"));
      REQUIRE(tx.s() == uint256_t("54120229697417001168895707448544945543174086509397089052751740515397451128193"));
      REQUIRE(TxValidator(tx.rlpSerialize()) == tx);
    }

    SECTION("Simple Transaction 2") {
      TxValidator tx(Hex::toBytes("f86aa03051b7f769aaabd4ebb8ff991888c2891ef1d7b84cee2b44bb8274e8ed3687ff83139705820fa1a0609914de300a418c4ec1ef176efc1fbb64d73a0de0b7eee4fd31f11627e36412a01ed62c45f76f1ff05b4856ab7a6730c02bddfc891dd1fdd6b82469d41a07aaf9"));

      REQUIRE(tx.from() == Address(std::string_view("86aacb83b27a8bacfe0a7f1e850c4602f25fd773"), false));
      REQUIRE(tx.data() == Hex::toBytes("3051b7f769aaabd4ebb8ff991888c2891ef1d7b84cee2b44bb8274e8ed3687ff"));
      REQUIRE(tx.chainId() == 1983);
      REQUIRE(tx.nHeight() == 1283845);
      REQUIRE(tx.r() == uint256_t("43692505089998971098165444389401548482072441435873510818800117878139339891730"));
      REQUIRE(tx.v() == uint256_t("4001"));
      REQUIRE(tx.s() == uint256_t("13947796292493994833352662700174314547467087308070504932236667446919569058553"));
      REQUIRE(TxValidator(tx.rlpSerialize()) == tx);
    }

    SECTION("Simple Transaction 3") {
      TxValidator tx(Hex::toBytes("f86599bf2ac52475bb963dcc43d6f10349a55e1f30899461e358e0ad850248e27503824e22a0b7c4f669b54cb0fb656f5e0c26b27405ca20a90e391222b8a8277d760273b0ffa01a27ef8cb8824267dae1884df1e31d440dacd995b7da027012798e2bd14c2ae9"));

      REQUIRE(tx.from() == Address(std::string_view("be953985d1f68bf58832519934466625513b9b82"), false));
      REQUIRE(tx.data() == Hex::toBytes("bf2ac52475bb963dcc43d6f10349a55e1f30899461e358e0ad"));
      REQUIRE(tx.chainId() == 9983);
      REQUIRE(tx.nHeight() == 9812735235);
      REQUIRE(tx.r() == uint256_t("83121253994923096708158993555586412644047517737808184668241393003355830464767"));
      REQUIRE(tx.v() == uint256_t("20002"));
      REQUIRE(tx.s() == uint256_t("11830694409891746707341752477121503773671699126595264188661088501161385536233"));
      REQUIRE(TxValidator(tx.rlpSerialize()) == tx);
    }
    
    SECTION("Advanced Transaction 1") {
      TxValidator tx(Hex::toBytes("f8beb869bf32ad6cc8188ff0ac6a14727bad727dcdec7f4b17a8b99499946f386d38a69c1945077f7b9a199a2b6261f37700affb3c334e9e2ff2f088ab47f407559920e0e18e565dab9c6af48fffc3b2f49db7e0175ec32bfc30c542753e7f299ef8cfe3c3b3b555a937b6bff68709fe2a2f29f0e38802c2e691ec1a5432a0251c1883c988cbd6524e4b529f88fe0bee8a0bcbe8327bac145878196e3edb85a02b4b3fa33cfd8233f3250a0f3de0b21591eaba39a34a5c2f452bcb51e392c159"));

      REQUIRE(tx.from() == Address(std::string_view("3176efd1bdf2a3dcac2470bc18be2974d68da500"), false));
      REQUIRE(tx.data() == Hex::toBytes("bf32ad6cc8188ff0ac6a14727bad727dcdec7f4b17a8b99499946f386d38a69c1945077f7b9a199a2b6261f37700affb3c334e9e2ff2f088ab47f407559920e0e18e565dab9c6af48fffc3b2f49db7e0175ec32bfc30c542753e7f299ef8cfe3c3b3b555a937b6bff6"));
      REQUIRE(tx.chainId() == 99487423981758983);
      REQUIRE(tx.nHeight() == 2812731923755235);
      REQUIRE(tx.r() == uint256_t("16785216310284032835375842519030930913641476165503653675793754131846167190405"));
      REQUIRE(tx.v() == uint256_t("198974847963518002"));
      REQUIRE(tx.s() == uint256_t("19582405229853012967248411101119275113862280301818556892222043810722791735641"));
      REQUIRE(TxValidator(tx.rlpSerialize()) == tx);
    }

    SECTION("Advanced Transaction 2") {
      TxValidator tx(Hex::toBytes("f8cdb87759e381a7936a99e59aee6cda1a452993f4392c17ff25a8103b1d1d5f7a291c5a4f9203aa2bd9257b648ec1ff4b0428b320a0c73a4dac3630e091f4e626ee05d4748f007067915a1646bf09a26698c97ff3e38be1a0639feb5a9ca4b1a19d12387f04eb4df5d57bcd289deb0a5203ece671b6041e7f441d8810e5df9fe5659754882b44183d2e68b8e4a09a8756bb9245266c02b3b4610647ae2ee49415348d464a27acba245bb3e73822a006fe67e91197c95d4f41407b6c1826bfef6b5afe524a89fe57bb33a28c472f54"));

      std::cout << tx.from().hex() << std::endl;
      REQUIRE(tx.from() == Address(std::string_view("3b19bf9db3498d5173405a933b1e594db586dea3"), false));
      REQUIRE(tx.data() == Hex::toBytes("59e381a7936a99e59aee6cda1a452993f4392c17ff25a8103b1d1d5f7a291c5a4f9203aa2bd9257b648ec1ff4b0428b320a0c73a4dac3630e091f4e626ee05d4748f007067915a1646bf09a26698c97ff3e38be1a0639feb5a9ca4b1a19d12387f04eb4df5d57bcd289deb0a5203ece671b6041e7f441d"));
      REQUIRE(tx.chainId() == uint64_t(1558821746548956256));
      REQUIRE(tx.nHeight() == uint64_t(1217625152115021652));
      REQUIRE(tx.r() == uint256_t("69895301642667997718503088288108394088722063920289049448717429863144262940706"));
      REQUIRE(tx.v() == uint256_t("3117643493097912548"));
      REQUIRE(tx.s() == uint256_t("3163373409347351159788309865974693689278941880091802184379600742894805659476"));
      REQUIRE(TxValidator(tx.rlpSerialize()) == tx);
    }

    SECTION("Sign Tx 1") {
      PrivKey privKey(Hex::toBytes("58f4fe7d000e888576d58968e97877e0c8ab9d5a8df3f17b258749ee720ad3bf"));

      TxValidator tx(
        Address(std::string_view("0x3b19BF9DB3498d5173405A933B1E594dB586DEA3"), false),
        Hex::toBytes("0x59e381a7936a99e59aee6cda1a452993f4392c17ff25a8103b1d1d5f7a291c5a4f9203aa2bd9257b648ec1ff4b0428b320a0c73a4dac3630e091f4e626ee05d4748f007067915a1646bf09a26698c97ff3e38be1a0639feb5a9ca4b1a19d12387f04eb4df5d57bcd289deb0a5203ece671b6041e7f441d"),
        uint64_t(1558821746548956256),
        uint64_t(1217625152115021652),
        privKey
      );

      REQUIRE(tx.from() == Address(std::string_view("3b19bf9db3498d5173405a933b1e594db586dea3"), false));
      REQUIRE(tx.data() == Hex::toBytes("59e381a7936a99e59aee6cda1a452993f4392c17ff25a8103b1d1d5f7a291c5a4f9203aa2bd9257b648ec1ff4b0428b320a0c73a4dac3630e091f4e626ee05d4748f007067915a1646bf09a26698c97ff3e38be1a0639feb5a9ca4b1a19d12387f04eb4df5d57bcd289deb0a5203ece671b6041e7f441d"));
      REQUIRE(tx.chainId() == uint64_t(1558821746548956256));
      REQUIRE(tx.nHeight() == uint64_t(1217625152115021652));
      REQUIRE(tx.r() == uint256_t("69895301642667997718503088288108394088722063920289049448717429863144262940706"));
      REQUIRE(tx.v() == uint256_t("3117643493097912548"));
      REQUIRE(tx.s() == uint256_t("3163373409347351159788309865974693689278941880091802184379600742894805659476"));
      REQUIRE(TxValidator(tx.rlpSerialize()) == tx);
      REQUIRE(tx.rlpSerialize() == Hex::toBytes("f8cdb87759e381a7936a99e59aee6cda1a452993f4392c17ff25a8103b1d1d5f7a291c5a4f9203aa2bd9257b648ec1ff4b0428b320a0c73a4dac3630e091f4e626ee05d4748f007067915a1646bf09a26698c97ff3e38be1a0639feb5a9ca4b1a19d12387f04eb4df5d57bcd289deb0a5203ece671b6041e7f441d8810e5df9fe5659754882b44183d2e68b8e4a09a8756bb9245266c02b3b4610647ae2ee49415348d464a27acba245bb3e73822a006fe67e91197c95d4f41407b6c1826bfef6b5afe524a89fe57bb33a28c472f54"));
    }

    SECTION("Sign Tx 2") {
      PrivKey privKey(Hex::toBytes("3051b7f769aaabd4ebb8ff991888c2891ef1d7b84cee2b44bb8274e8ed3687ff"));

      TxValidator tx(
        Address(std::string_view("0x684e1dF8BC220D0361104900c6BFc3Cb432A7F91"), false),
        Hex::toBytes("3051b7f769aaabd4ebb8ff991888c2891ef1d7b84cee2b44bb8274e8ed3687ff"),
        1983,
        1283845,
        privKey
      );

      REQUIRE(tx.from() == Address(std::string_view("684e1df8bc220d0361104900c6bfc3cb432a7f91"), false));
      REQUIRE(tx.data() == Hex::toBytes("3051b7f769aaabd4ebb8ff991888c2891ef1d7b84cee2b44bb8274e8ed3687ff"));
      REQUIRE(tx.chainId() == 1983);
      REQUIRE(tx.nHeight() == 1283845);
      REQUIRE(tx.r() == uint256_t("71927725730505717221857210788095809598228783863615012560269885374844646275568"));
      REQUIRE(tx.v() == uint256_t("4001"));
      REQUIRE(tx.s() == uint256_t("42015177655885299653253653500250305985688210916354750173981693696427722864191"));
      REQUIRE(TxValidator(tx.rlpSerialize()) == tx);
      REQUIRE(tx.rlpSerialize() == Hex::toBytes("f86aa03051b7f769aaabd4ebb8ff991888c2891ef1d7b84cee2b44bb8274e8ed3687ff83139705820fa1a09f05a66ad8727ec5fda79a9fb2d05b779cd3e8944fbea22b8bdf5e517a4939f0a05ce3bf71d5979d0d2ba2414abeb22ea8893eda157283617a95beeca926b4f63f"));
    }
  }
}