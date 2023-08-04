#include "../../src/libs/catch2/catch_amalgamated.hpp"
#include "../../src/utils/utils.h"
#include "../../src/utils/strings.h"
#include "../../src/utils/hex.h"

using Catch::Matchers::Equals;

namespace TUtils {
  TEST_CASE("Utils Namespace", "[utilsitself][utils]") {
    SECTION("Sha3 Test") {
      std::string sha3Input = "My SHA3 Input";
      auto sha3Output = Utils::sha3(Utils::stringToBytes(sha3Input));
      Bytes sha3ExpectedOutput = Bytes{0x10, 0x11, 0x40, 0xd6, 0xe7, 0x50, 0x6f, 0x80, 0x4c, 0xf7, 0xb0, 0x37, 0x0f, 0xa9, 0x0b, 0x04, 0xc5, 0xe9, 0x37, 0x4d, 0xdb, 0x0c, 0x8c, 0xbe, 0x12, 0xaf, 0x15, 0x0c, 0x8f, 0xf3, 0xee, 0x36};
      REQUIRE(sha3Output == sha3ExpectedOutput);
    }

    SECTION("uint256ToBytes Test") {
      uint256_t uint256Input = uint256_t("91830918212381802449294565349763096207758814059154440393436864477986483867239");
      auto uint256Output = Utils::uint256ToBytes(uint256Input);
      BytesArr<32> uint256ExpectedOutput = BytesArr<32> {0xcb, 0x06, 0x75, 0x32, 0x90, 0xff, 0xac, 0x16, 0x72, 0x05, 0xd0, 0xf5, 0x3b, 0x64, 0xac, 0xfd, 0x80, 0xbe, 0x11, 0xed, 0xbb, 0x26, 0xa2, 0x24, 0xbe, 0xd9, 0x23, 0x9a, 0xe6, 0x74, 0x0e, 0x67};
      REQUIRE(uint256Output == uint256ExpectedOutput);
    }

    SECTION("uint248ToBytes Test") {
      uint248_t uint248Input = uint248_t("452312848503266318373324160190117140051835877600158453279131187530910662650");
      auto uint248Output = Utils::uint248ToBytes(uint248Input);
      BytesArr<31> uint248ExpectedOutput = BytesArr<31> {0xff, 0xff, 0xff, 0xff, 0x3d, 0x87, 0xd5, 0x8f, 0x2e, 0x16, 0xa4, 0x50, 0x99, 0xdf, 0xde, 0x83, 0xe8, 0x45, 0xd0, 0x13, 0x5e, 0x48, 0x6c, 0xf6, 0x6b, 0xa7, 0xff, 0xff, 0xff, 0xff, 0xfa};
      REQUIRE(uint248Output == uint248ExpectedOutput);
    }

    SECTION("uint240ToBytes Test") {
      uint240_t uint240Input = uint240_t("1766847064778384329583297500742918515827483896875618958121606201292619770");
      auto uint240Output = Utils::uint240ToBytes(uint240Input);
      BytesArr<30> uint240ExpectedOutput = BytesArr<30>{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfa};
      REQUIRE(uint240Output == uint240ExpectedOutput);
    }

    SECTION("uint232ToBytes Test") {
      uint232_t uint232Input = uint232_t("6901746346790563717434755872277015452451108972170386555162524123799295");
      auto uint232Output = Utils::uint232ToBytes(uint232Input);
      BytesArr<29> uint232ExpectedOutput = BytesArr<29>{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x44, 0xe8, 0x0e, 0x70, 0x5c, 0x5f, 0x03, 0xf7, 0x10, 0xc1, 0x0f, 0x75, 0x8a, 0x39, 0x68, 0x1e, 0xb1, 0x5f, 0xfa, 0x0a, 0x1e, 0xff};
      REQUIRE(uint232Output == uint232ExpectedOutput);
    }

    SECTION("uint224ToBytes Test") {
      uint224_t uint224Input = uint224_t("26959946667150639194667015087019630673637144422540512481103110219215");
      auto uint224Output = Utils::uint224ToBytes(uint224Input);
      BytesArr<28> uint224ExpectedOutput = BytesArr<28> {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x65, 0x76, 0x9d, 0x7c, 0x26, 0x91, 0x98, 0xd1, 0x0f, 0x67, 0xe0, 0xc2, 0x0d, 0xbe, 0xd2, 0xd6, 0x51, 0x43, 0xac, 0x25, 0xcf};
      REQUIRE(uint224Output == uint224ExpectedOutput);
    }

    SECTION("uint216ToBytes Test") {
      uint216_t uint216Input = uint216_t("105312291618557186697918027613670432318815095400549111254310917535");
      auto uint216Output = Utils::uint216ToBytes(uint216Input);
      BytesArr<27> uint216ExpectedOutput = BytesArr<27> {0xff, 0xff, 0xff, 0xfd, 0xf5, 0xf9, 0xc3, 0xb5, 0xf8, 0x4a, 0xed, 0x4c, 0x56, 0x99, 0xc5, 0xec, 0xdb, 0xbe, 0xe1, 0xb8, 0x32, 0x77, 0x5d, 0xaf, 0xff, 0x15, 0x9f};
      REQUIRE(uint216Output == uint216ExpectedOutput);
    }

    SECTION("uint208ToBytes Test") {
      uint208_t uint208Input = uint208_t("411176139330301510518742295639337626245613966408394965837151255");
      auto uint208Output = Utils::uint208ToBytes(uint208Input);
      BytesArr<26> uint208ExpectedOutput = BytesArr<26> {0xff, 0xe0, 0x23, 0x5e, 0x91, 0xfb, 0x47, 0x91, 0xd9, 0x68, 0xea, 0x08, 0x5f, 0xb3, 0x4e, 0x6b, 0xcc, 0xfe, 0xad, 0xd4, 0xab, 0xe5, 0x20, 0x3f, 0xfc, 0x17};
      REQUIRE(uint208Output == uint208ExpectedOutput);
    }

    SECTION("uint200ToBytes Test") {
      uint200_t uint200Input = uint200_t("1606931844258990275541962019341162602522202913782792831301375");
      auto uint200Output = Utils::uint200ToBytes(uint200Input);
      BytesArr<25> uint200ExpectedOutput = BytesArr<25> {0xff, 0xff, 0xbf, 0x44, 0xdb, 0xda, 0x25, 0xf4, 0xcf, 0xa0, 0x53, 0x3c, 0x00, 0xc9, 0x61, 0xdb, 0xfa, 0x45, 0x00, 0xd2, 0x92, 0x81, 0xba, 0xf6, 0xff};
      REQUIRE(uint200Output == uint200ExpectedOutput);
    }

    SECTION("uint192ToBytes Test") {
      uint192_t uint192Input = uint192_t("6277101735386680163835789423207166416102355144464034112895");
      auto uint192Output = Utils::uint192ToBytes(uint192Input);
      BytesArr<24> uint192ExpectedOutput = BytesArr<24> {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0x1c, 0xc2, 0x2b, 0x56, 0x36, 0x47, 0xc5, 0x7c, 0xa2, 0x1a, 0x93, 0xf3, 0x08, 0x5a, 0x8b, 0x25, 0x7f};
      REQUIRE(uint192Output == uint192ExpectedOutput);
    }

    SECTION("uint184ToBytes Test") {
      uint184_t uint184Input = uint184_t("24519128653854221731733552434404946137899825954931634815");
      auto uint184Output = Utils::uint184ToBytes(uint184Input);
      BytesArr<23> uint184ExpectedOutput = BytesArr<23> {0xff, 0xfd, 0xdc, 0x9e, 0x27, 0x50, 0x33, 0x6b, 0x4a, 0x91, 0xcc, 0xe8, 0x44, 0x49, 0xbb, 0x90, 0x0d, 0xd0, 0x74, 0xe7, 0x24, 0x72, 0x7f};
      REQUIRE(uint184Output == uint184ExpectedOutput);
    }

    SECTION("uint176ToBytes Test") {
      uint176_t uint176Input = uint176_t("95180971304118053147396689196894323971171195136471135");
      auto uint176Output = Utils::uint176ToBytes(uint176Input);
      BytesArr<22> uint176ExpectedOutput = BytesArr<22> {0xfe, 0x65, 0x76, 0x9d, 0x7c, 0x26, 0x91, 0x38, 0x85, 0x28, 0x29, 0xfc, 0x3e, 0x82, 0xe5, 0xf0, 0x6c, 0xfe, 0xc8, 0x1f, 0x70, 0x5f};
      REQUIRE(uint176Output == uint176ExpectedOutput);
    }

    SECTION("uint168ToBytes Test") {
      uint168_t uint168Input = uint168_t("374114419156711147010143317175368451031918731001155");
      auto uint168Output = Utils::uint168ToBytes(uint168Input);
      BytesArr<21> uint168ExpectedOutput = BytesArr<21> {0xff, 0xfa, 0xbe, 0xc0, 0xe3, 0xb0, 0x36, 0x48, 0x66, 0x50, 0x4b, 0x1c, 0x90, 0xef, 0x13, 0xad, 0x42, 0x36, 0x72, 0xfd, 0x43};
      REQUIRE(uint168Output == uint168ExpectedOutput);
    }

    SECTION("uint160ToBytes Test") {
      uint160_t uint160Input = uint160_t("506797479317435130489084083375319966488594602593");
      auto uint160Output = Utils::uint160ToBytes(uint160Input);
      BytesArr<20> uint160ExpectedOutput = BytesArr<20> {0x58, 0xc5, 0x95, 0xbe, 0xdf, 0x1d, 0xea, 0x53, 0x2c, 0xf0, 0x6a, 0xf9, 0x09, 0x1a, 0x51, 0xb7, 0x5a, 0x11, 0xda, 0x61};
      REQUIRE(uint160Output == uint160ExpectedOutput);
    }

    SECTION("uint152ToBytes Test") {
      uint152_t uint152Input = uint152_t("5701991770823839524233143877797980545130986491");
      auto uint152Output = Utils::uint152ToBytes(uint152Input);
      BytesArr<19> uint152ExpectedOutput = BytesArr<19> {0xff, 0xaf, 0xa7, 0xc9, 0x9f, 0xec, 0x62, 0x36, 0xc7, 0x93, 0x33, 0xe4, 0x70, 0x1a, 0x7f, 0xe8, 0x28, 0x7b, 0xfb};
      REQUIRE(uint152Output == uint152ExpectedOutput);
    }

    SECTION("uint144ToBytes Test") {
      uint144_t uint144Input = uint144_t("22300745118530623141515718272648361505980415");
      auto uint144Output = Utils::uint144ToBytes(uint144Input);
      BytesArr<18> uint144ExpectedOutput = BytesArr<18> {0xff, 0xff, 0xff, 0xf0, 0x97, 0xb2, 0x0a, 0x93, 0xbd, 0xc2, 0x0f, 0xd1, 0xf9, 0x6a, 0x9b, 0x7f, 0xff, 0xff};
      REQUIRE(uint144Output == uint144ExpectedOutput);
    }

    SECTION("uint136ToBytes Test") {
      uint136_t uint136Input = uint136_t("87112285131760246616623899502532662132135");
      auto uint136Output = Utils::uint136ToBytes(uint136Input);
      BytesArr<17> uint136ExpectedOutput = BytesArr<17> {0xff, 0xff, 0xff, 0xd8, 0x8e, 0x94, 0x95, 0xee, 0xc9, 0x84, 0xf6, 0x26, 0xc7, 0xe9, 0x3f, 0xfd, 0xa7};
      REQUIRE(uint136Output == uint136ExpectedOutput);
    }

    SECTION("uint128ToBytes Test") {
      uint128_t uint128Input = uint128_t("340282366920938463463374607431768211401");
      auto uint128Output = Utils::uint128ToBytes(uint128Input);
      BytesArr<16> uint128ExpectedOutput = BytesArr<16> {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc9 };
      REQUIRE(uint128Output == uint128ExpectedOutput);
    }

    SECTION("uint120ToBytes Test") {
      uint120_t uint120Input = uint120_t("1329227915784915812903807060281344575");
      auto uint120Output = Utils::uint120ToBytes(uint120Input);
      BytesArr<15> uint120ExpectedOutput = BytesArr<15> {0xff, 0xff, 0xfe, 0xfd, 0x81, 0x8d, 0x0a, 0xce, 0x2d, 0x1a, 0xdc, 0x44, 0x9f, 0x42, 0x3f};
      REQUIRE(uint120Output == uint120ExpectedOutput);
    }

    SECTION("uint112ToBytes Test") {
      uint112_t uint112Input = uint112_t("5191296858514827628530496129220091");
      auto uint112Output = Utils::uint112ToBytes(uint112Input);
      BytesArr<14> uint112ExpectedOutput = BytesArr<14> {0xff, 0xf3, 0x60, 0xd3, 0x5e, 0xf3, 0x85, 0xc9, 0x7e, 0xa0, 0x4f, 0x94, 0x3d, 0xfb};
      REQUIRE(uint112Output == uint112ExpectedOutput);
    }

    SECTION("uint104ToBytes Test") {
      uint104_t uint104Input = uint104_t("20212409603611670423941251286011");
      auto uint104Output = Utils::uint104ToBytes(uint104Input);
      BytesArr<13> uint104ExpectedOutput = BytesArr<13> {0xff, 0x1d, 0xd1, 0x5b, 0x6a, 0x21, 0xe0, 0x63, 0x45, 0x02, 0xbf, 0x43, 0xfb};
      REQUIRE(uint104Output == uint104ExpectedOutput);
    }

    SECTION("uint96ToBytes Test") {
      uint96_t uint96Input = uint96_t("79128162514264331593543951331");
      auto uint96Output = Utils::uint96ToBytes(uint96Input);
      BytesArr<12> uint96ExpectedOutput = BytesArr<12> {0xff, 0xad, 0x48, 0x2d, 0x23, 0x37, 0xed, 0xb8, 0x20, 0x21, 0xa3, 0xe3};
      REQUIRE(uint96Output == uint96ExpectedOutput);
    }

    SECTION("uint88ToBytes Test") {
      uint88_t uint88Input = uint88_t("309185109821345061724781051");
      auto uint88Output = Utils::uint88ToBytes(uint88Input);
      BytesArr<11> uint88ExpectedOutput = BytesArr<11> {0xff, 0xc0, 0x7e, 0x63, 0x6f, 0xba, 0x4a, 0xde, 0x54, 0x79, 0xfb};
      REQUIRE(uint88Output == uint88ExpectedOutput);
    }

    SECTION("uint80ToBytes Test") {
      uint80_t uint80Input = uint80_t("1208925819614629174106171");
      auto uint80Output = Utils::uint80ToBytes(uint80Input);
      BytesArr<10> uint80ExpectedOutput = BytesArr<10> {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf6, 0xd8, 0x3b};
      REQUIRE(uint80Output == uint80ExpectedOutput);
    }

    SECTION("uint72ToBytes Test") {
      uint72_t uint72Input = uint72_t("1722366182819645213691");
      auto uint72Output = Utils::uint72ToBytes(uint72Input);
      BytesArr<9> uint72ExpectedOutput = BytesArr<9> {0x5d, 0x5e, 0xa1, 0xe5, 0xc9, 0x8e, 0x75, 0xcb, 0xfb};
      REQUIRE(uint72Output == uint72ExpectedOutput);
    }

    SECTION("uint64ToBytes Test") {
      uint64_t uint64Input = uint64_t(11155010102558518614ULL);
      auto uint64Output = Utils::uint64ToBytes(uint64Input);
      BytesArr<8> uint64ExpectedOutput = BytesArr<8> {0x9a, 0xce, 0x8e, 0x96, 0x24, 0xe4, 0xed, 0x56};
      REQUIRE(uint64Output == uint64ExpectedOutput);
    }

    SECTION("uint56ToBytes Test") {
      uint56_t uint56Input = uint56_t(72051594031927931);
      auto uint56Output = Utils::uint56ToBytes(uint56Input);
      BytesArr<7> uint56ExpectedOutput = BytesArr<7> {0xff, 0xfa, 0x8b, 0x03, 0xc6, 0x12, 0x7b};
      REQUIRE(uint56Output == uint56ExpectedOutput);
    }

    SECTION("uint48ToBytes Test") {
      uint48_t uint48Input = uint48_t(281414176110651);
      auto uint48Output = Utils::uint48ToBytes(uint48Input);
      BytesArr<6> uint48ExpectedOutput = BytesArr<6> {0xff, 0xf1, 0xd8, 0x00, 0x78, 0x3b};
      REQUIRE(uint48Output == uint48ExpectedOutput);
    }

    SECTION("uint40ToBytes Test") {
      uint40_t uint40Input = uint40_t(1019511621771);
      auto uint40Output = Utils::uint40ToBytes(uint40Input);
      BytesArr<5> uint40ExpectedOutput = BytesArr<5> {0xed, 0x5f, 0xa0, 0xc8, 0x8b};
      REQUIRE(uint40Output == uint40ExpectedOutput);
    }

    SECTION("uint32ToBytes Test") {
      uint32_t uint32Input = 2004601498;
      auto uint32Output = Utils::uint32ToBytes(uint32Input);
      BytesArr<4> uint32ExpectedOutput = BytesArr<4> {0x77, 0x7b, 0xca, 0x9a};
      REQUIRE(uint32Output == uint32ExpectedOutput);
    }

    SECTION("uint24ToBytes Test") {
      uint24_t uint24Input = 16117211;
      auto uint24Output = Utils::uint24ToBytes(uint24Input);
      BytesArr<3> uint24ExpectedOutput = BytesArr<3> {0xf5, 0xed, 0xdb};
      REQUIRE(uint24Output == uint24ExpectedOutput);
    }

    SECTION("uint16ToBytes Test") {
      uint16_t uint16Input = 65452;
      auto uint16Output = Utils::uint16ToBytes(uint16Input);
      BytesArr<2> uint16ExpectedOutput = BytesArr<2> {0xff, 0xac};
      REQUIRE(uint16Output == uint16ExpectedOutput);
    }

    SECTION("uint8ToBytes Test") {
      uint8_t uint8Input = 120;
      auto uint8Output = Utils::uint8ToBytes(uint8Input);
      BytesArr<1> uint8ExpectedOutput = BytesArr<1> {0x78};
      REQUIRE(uint8Output == uint8ExpectedOutput);
    }

    SECTION("randBytes Test") {
      Bytes randBytesOutput = Utils::randBytes(32);
      REQUIRE(randBytesOutput.size() == 32);
    }

    SECTION("bytesToUint256 Test") {
      FixedBytes<32> bytesStr(std::string("\xcb\x06\x75\x32\x90\xff\xac\x16\x72\x05\xd0\xf5\x3b\x64\xac\xfd\x80\xbe\x11\xed\xbb\x26\xa2\x24\xbe\xd9\x23\x9a\xe6\x74\x0e\x67"));
      auto uint256Output = Utils::bytesToUint256(bytesStr.get());
      uint256_t uint256ExpectedOutput = uint256_t("91830918212381802449294565349763096207758814059154440393436864477986483867239");
      REQUIRE(uint256Output == uint256ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint256(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint256(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint248 Test") {
      BytesArr<31> bytesArr = BytesArr<31> {0xff, 0xff, 0xff, 0xff, 0x3d, 0x87, 0xd5, 0x8f, 0x2e, 0x16, 0xa4, 0x50, 0x99, 0xdf, 0xde, 0x83, 0xe8, 0x45, 0xd0, 0x13, 0x5e, 0x48, 0x6c, 0xf6, 0x6b, 0xa7, 0xff, 0xff, 0xff, 0xff, 0xfa};
      auto uint248Output = Utils::bytesToUint248(bytesArr);
      uint248_t uint248ExpectedOutput = uint248_t("452312848503266318373324160190117140051835877600158453279131187530910662650");
      REQUIRE(uint248Output == uint248ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint248(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint248(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint240 Test") {
      BytesArr<30> bytesArr = BytesArr<30>{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfa};
      auto uint240Output = Utils::bytesToUint240(bytesArr);
      uint240_t uint240ExpectedOutput = uint240_t("1766847064778384329583297500742918515827483896875618958121606201292619770");
      REQUIRE(uint240Output == uint240ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint240(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint240(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint232 Test") {
      BytesArr<29> bytesArr = BytesArr<29>{0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x44, 0xe8, 0x0e, 0x70, 0x5c, 0x5f, 0x03, 0xf7, 0x10, 0xc1, 0x0f, 0x75, 0x8a, 0x39, 0x68, 0x1e, 0xb1, 0x5f, 0xfa, 0x0a, 0x1e, 0xff};
      auto uint232Output = Utils::bytesToUint232(bytesArr);
      uint232_t uint232ExpectedOutput = uint232_t("6901746346790563717434755872277015452451108972170386555162524123799295");
      REQUIRE(uint232Output == uint232ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint232(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint232(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint224 Test") {
      BytesArr<28> bytesArr = BytesArr<28> {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x65, 0x76, 0x9d, 0x7c, 0x26, 0x91, 0x98, 0xd1, 0x0f, 0x67, 0xe0, 0xc2, 0x0d, 0xbe, 0xd2, 0xd6, 0x51, 0x43, 0xac, 0x25, 0xcf};
      auto uint224Output = Utils::bytesToUint224(bytesArr);
      uint224_t uint224ExpectedOutput = uint224_t("26959946667150639194667015087019630673637144422540512481103110219215");
      REQUIRE(uint224Output == uint224ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint224(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint224(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint216 Test") {
      BytesArr<27> bytesArr = BytesArr<27> {0xff, 0xff, 0xff, 0xfd, 0xf5, 0xf9, 0xc3, 0xb5, 0xf8, 0x4a, 0xed, 0x4c, 0x56, 0x99, 0xc5, 0xec, 0xdb, 0xbe, 0xe1, 0xb8, 0x32, 0x77, 0x5d, 0xaf, 0xff, 0x15, 0x9f};
      auto uint216Output = Utils::bytesToUint216(bytesArr);
      uint216_t uint216ExpectedOutput = uint216_t("105312291618557186697918027613670432318815095400549111254310917535");
      REQUIRE(uint216Output == uint216ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint216(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint216(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint208 Test") {
      BytesArr<26> bytesArr = BytesArr<26> {0xff, 0xe0, 0x23, 0x5e, 0x91, 0xfb, 0x47, 0x91, 0xd9, 0x68, 0xea, 0x08, 0x5f, 0xb3, 0x4e, 0x6b, 0xcc, 0xfe, 0xad, 0xd4, 0xab, 0xe5, 0x20, 0x3f, 0xfc, 0x17};
      auto uint208Output = Utils::bytesToUint208(bytesArr);
      uint208_t uint208ExpectedOutput = uint208_t("411176139330301510518742295639337626245613966408394965837151255");
      REQUIRE(uint208Output == uint208ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint208(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint208(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint200 Test") {
      BytesArr<25> bytesArr = BytesArr<25> {0xff, 0xff, 0xbf, 0x44, 0xdb, 0xda, 0x25, 0xf4, 0xcf, 0xa0, 0x53, 0x3c, 0x00, 0xc9, 0x61, 0xdb, 0xfa, 0x45, 0x00, 0xd2, 0x92, 0x81, 0xba, 0xf6, 0xff};
      auto uint200Output = Utils::bytesToUint200(bytesArr);
      uint200_t uint200ExpectedOutput = uint200_t("1606931844258990275541962019341162602522202913782792831301375");
      REQUIRE(uint200Output == uint200ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint200(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint200(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint192 Test") {
      FixedBytes<24> bytesStr = BytesArr<24> {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf9, 0x1c, 0xc2, 0x2b, 0x56, 0x36, 0x47, 0xc5, 0x7c, 0xa2, 0x1a, 0x93, 0xf3, 0x08, 0x5a, 0x8b, 0x25, 0x7f};
      auto uint192Output = Utils::bytesToUint192(bytesStr.get());
      uint192_t uint192ExpectedOutput = uint192_t("6277101735386680163835789423207166416102355144464034112895");
      REQUIRE(uint192Output == uint192ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint192(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint192(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint184 Test") {
      BytesArr<23> bytesArr = BytesArr<23> {0xff, 0xfd, 0xdc, 0x9e, 0x27, 0x50, 0x33, 0x6b, 0x4a, 0x91, 0xcc, 0xe8, 0x44, 0x49, 0xbb, 0x90, 0x0d, 0xd0, 0x74, 0xe7, 0x24, 0x72, 0x7f};
      auto uint184Output = Utils::bytesToUint184(bytesArr);
      uint184_t uint184ExpectedOutput = uint184_t("24519128653854221731733552434404946137899825954931634815");
      REQUIRE(uint184Output == uint184ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint184(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint184(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint176 Test") {
      BytesArr<22> bytesArr = BytesArr<22> {0xff, 0xff, 0xff, 0xff, 0x3d, 0x87, 0xd5, 0x8f, 0x2e, 0x16, 0xa4, 0x50, 0x99, 0xdf, 0xde, 0x83, 0xe8, 0x45, 0xd0, 0x13, 0x5e, 0x48};
      auto uint176Output = Utils::bytesToUint176(bytesArr);
      uint176_t uint176ExpectedOutput = uint176_t("95780971287177379879234105465257117190357643745779272");
      REQUIRE(uint176Output == uint176ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint176(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint176(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint168 Test") {
      BytesArr<21> bytesArr = BytesArr<21> {0xff, 0xff, 0xff, 0x3d, 0x87, 0xd5, 0x8f, 0x2e, 0x16, 0xa4, 0x50, 0x99, 0xdf, 0xde, 0x83, 0xe8, 0x45, 0xd0, 0x13, 0x5e, 0x48};
      auto uint168Output = Utils::bytesToUint168(bytesArr);
      uint168_t uint168ExpectedOutput = uint168_t("374144402216037378897559585538161667218367340305992");
      REQUIRE(uint168Output == uint168ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint168(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint168(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint160 Test") {
      FixedBytes<20> bytesStr(std::string("\x58\xc5\x95\xbe\xdf\x1d\xea\x53\x2c\xf0\x6a\xf9\x09\x1a\x51\xb7\x5a\x11\xda\x61"));
      auto uint160Output = Utils::bytesToUint160(bytesStr.get());
      uint160_t uint160ExpectedOutput = uint160_t("506797479317435130489084083375319966488594602593");
      REQUIRE(uint160Output == uint160ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint160(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint160(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint152 Test") {
      BytesArr<19> bytesArr = BytesArr<19> {0xff, 0xaf, 0xa7, 0xc9, 0x9f, 0xec, 0x62, 0x36, 0xc7, 0x93, 0x33, 0xe4, 0x70, 0x1a, 0x7f, 0xe8, 0x28, 0x7b, 0xfb};
      auto uint152Output = Utils::bytesToUint152(bytesArr);
      uint152_t uint152ExpectedOutput = uint152_t("5701991770823839524233143877797980545130986491");
      REQUIRE(uint152Output == uint152ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint152(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint152(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint144 Test") {
      BytesArr<18> bytesArr = BytesArr<18> {0xff, 0xff, 0xff, 0xf0, 0x97, 0xb2, 0x0a, 0x93, 0xbd, 0xc2, 0x0f, 0xd1, 0xf9, 0x6a, 0x9b, 0x7f, 0xff, 0xff};
      auto uint144Output = Utils::bytesToUint144(bytesArr);
      uint144_t uint144ExpectedOutput = uint144_t("22300745118530623141515718272648361505980415");
      REQUIRE(uint144Output == uint144ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint144(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint144(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint136 Test") {
      BytesArr<17> bytesArr = BytesArr<17> {0xff, 0xff, 0xff, 0xd8, 0x8e, 0x94, 0x95, 0xee, 0xc9, 0x84, 0xf6, 0x26, 0xc7, 0xe9, 0x3f, 0xfd, 0xa7};
      auto uint136Output = Utils::bytesToUint136(bytesArr);
      uint136_t uint136ExpectedOutput = uint136_t("87112285131760246616623899502532662132135");
      REQUIRE(uint136Output == uint136ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint136(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint136(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint128 Test") {
      BytesArr<16> bytesArr = BytesArr<16> {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xc9 };
      auto uint128Output = Utils::bytesToUint128(bytesArr);
      uint128_t uint128ExpectedOutput = uint128_t("340282366920938463463374607431768211401");
      REQUIRE(uint128Output == uint128ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint128(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint128(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint120 Test") {
      BytesArr<15> bytesArr = BytesArr<15> {0xff, 0xff, 0xff, 0xff, 0x3d, 0x87, 0xd5, 0x8f, 0x2e, 0x16, 0xa4, 0x50, 0x99, 0xdf, 0xde};
      auto uint120Output = Utils::bytesToUint120(bytesArr);
      uint120_t uint120ExpectedOutput = uint120_t("1329227995549816797027838164786077662");
      REQUIRE(uint120Output == uint120ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint120(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint120(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint112 Test") {
      BytesArr<14> bytesArr = BytesArr<14> {0xff, 0xf3, 0x60, 0xd3, 0x5e, 0xf3, 0x85, 0xc9, 0x7e, 0xa0, 0x4f, 0x94, 0x3d, 0xfb};
      auto uint112Output = Utils::bytesToUint112(bytesArr);
      uint112_t uint112ExpectedOutput = uint112_t("5191296858514827628530496129220091");
      REQUIRE(uint112Output == uint112ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint112(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint112(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }

    SECTION("bytesToUint104 Test") {
      BytesArr<13> bytesArr = BytesArr<13> {0xff, 0x1d, 0xd1, 0x5b, 0x6a, 0x21, 0xe0, 0x63, 0x45, 0x02, 0xbf, 0x43, 0xfb};
      auto uint104Output = Utils::bytesToUint104(bytesArr);
      uint104_t uint104ExpectedOutput = uint104_t("20212409603611670423941251286011");
      REQUIRE(uint104Output == uint104ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xfffffffffffffffffffffffffffffffffff");
      try { Utils::bytesToUint104(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint104(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);

    }


    SECTION("bytesToUint96 Test") {
      BytesArr<12> bytesArr = BytesArr<12> {0xff, 0xad, 0x48, 0x2d, 0x23, 0x37, 0xed, 0xb8, 0x20, 0x21, 0xa3, 0xe3};
      auto uint96Output = Utils::bytesToUint96(bytesArr);
      uint96_t uint96ExpectedOutput = uint96_t("79128162514264331593543951331");
      REQUIRE(uint96Output == uint96ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffffff");
      try { Utils::bytesToUint96(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint96(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint88 Test") {
      BytesArr<11> bytesArr = BytesArr<11> {0xff, 0xc0, 0x7e, 0x63, 0x6f, 0xba, 0x4a, 0xde, 0x54, 0x79, 0xfb};
      auto uint88Output = Utils::bytesToUint88(bytesArr);
      uint88_t uint88ExpectedOutput = uint88_t("309185109821345061724781051");
      REQUIRE(uint88Output == uint88ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffff");
      try { Utils::bytesToUint88(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint88(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }


    SECTION("bytesToUint80 Test") {
      BytesArr<10> bytesArr = BytesArr<10> {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf6, 0xd8, 0x3b};
      auto uint80Output = Utils::bytesToUint80(bytesArr);
      uint80_t uint80ExpectedOutput = uint80_t("1208925819614629174106171");
      REQUIRE(uint80Output == uint80ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffffff");
      try { Utils::bytesToUint80(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint80(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint72 Test") {
      BytesArr<9> bytesArr = BytesArr<9> {0x5d, 0x5e, 0xa1, 0xe5, 0xc9, 0x8e, 0x75, 0xcb, 0xfb};
      auto uint72Output = Utils::bytesToUint72(bytesArr);
      uint72_t uint72ExpectedOutput = uint72_t("1722366182819645213691");
      REQUIRE(uint72Output == uint72ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffffffff");
      try { Utils::bytesToUint72(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint72(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint64 Test") {
      FixedBytes<8> bytesStr(std::string("\x9a\xce\x8e\x96\x24\xe4\xed\x56"));
      auto uint64Output = Utils::bytesToUint64(bytesStr.get());
      uint64_t uint64ExpectedOutput = uint64_t(11155010102558518614ULL);
      REQUIRE(uint64Output == uint64ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffffff");
      try { Utils::bytesToUint64(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint64(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint56 Test") {
      BytesArr<7> bytesArr = BytesArr<7> {0xff, 0xfa, 0x8b, 0x03, 0xc6, 0x12, 0x7b};
      auto uint56Output = Utils::bytesToUint56(bytesArr);
      uint64_t uint56ExpectedOutput = uint64_t(72051594031927931);
      REQUIRE(uint56Output == uint56ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffffff");
      try { Utils::bytesToUint56(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint56(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint48 Test") {
      BytesArr<6> bytesArr = BytesArr<6> {0xff, 0xf1, 0xd8, 0x00, 0x78, 0x3b};
      auto uint48Output = Utils::bytesToUint48(bytesArr);
      uint64_t uint48ExpectedOutput = uint64_t(281414176110651);
      REQUIRE(uint48Output == uint48ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffffff");
      try { Utils::bytesToUint48(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint48(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint40 Test") {
      BytesArr<5> bytesArr = BytesArr<5> {0xed, 0x5f, 0xa0, 0xc8, 0x8b};
      auto uint40Output = Utils::bytesToUint40(bytesArr);
      uint64_t uint40ExpectedOutput = uint64_t(1019511621771);
      REQUIRE(uint40Output == uint40ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffffff");
      try { Utils::bytesToUint40(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint40(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint32 Test") {
      FixedBytes<4> bytesStr(std::string("\x77\x7b\xca\x9a"));
      auto uint32Output = Utils::bytesToUint32(bytesStr.get());
      uint32_t uint32ExpectedOutput = 2004601498;
      REQUIRE(uint32Output == uint32ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xffffff");
      Bytes hiStr = Hex::toBytes("0xffffffffff");
      try { Utils::bytesToUint32(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint32(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint24 Test") {
      BytesArr<3> bytesArr = BytesArr<3> {0xf5, 0xed, 0xdb};
      auto uint24Output = Utils::bytesToUint24(bytesArr);
      uint32_t uint24ExpectedOutput = uint32_t(16117211);
      REQUIRE(uint24Output == uint24ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xfffffff");
      Bytes hiStr = Hex::toBytes("0xffffffff");
      try { Utils::bytesToUint24(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint24(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint16 Test") {
      FixedBytes<2> bytesStr(std::string("\xff\xac"));
      auto uint16Output = Utils::bytesToUint16(bytesStr.get());
      uint16_t uint16ExpectedOutput = 65452;
      REQUIRE(uint16Output == uint16ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0xff");
      Bytes hiStr = Hex::toBytes("0xffffff");
      try { Utils::bytesToUint16(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint16(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("bytesToUint8 Test") {
      FixedBytes<1> bytesStr(std::string("\x78"));
      auto uint8Output = Utils::bytesToUint8(bytesStr.get());
      uint8_t uint8ExpectedOutput = 120;
      REQUIRE(uint8Output == uint8ExpectedOutput);

      bool catchLo = false;
      bool catchHi = false;
      Bytes loStr = Hex::toBytes("0x");
      Bytes hiStr = Hex::toBytes("0xffff");
      try { Utils::bytesToUint8(loStr); } catch (std::exception &e) { catchLo = true; }
      try { Utils::bytesToUint8(hiStr); } catch (std::exception &e) { catchHi = true; }
      REQUIRE(catchLo == true);
      REQUIRE(catchHi == true);
    }

    SECTION("padLeftBytes Test") {
      Bytes inputBytes = Hex::toBytes("0xabcdef");
      Bytes outputBytes = Utils::padLeftBytes(inputBytes, 10, 0x00);
      Bytes outputBytes2 = Utils::padLeftBytes(inputBytes, 20, 0x11);
      Bytes expectedOutputBytes = Hex::toBytes("0x00000000000000abcdef");
      Bytes expectedOutputBytes2 = Hex::toBytes("0x1111111111111111111111111111111111abcdef");
      REQUIRE(outputBytes == expectedOutputBytes);
      REQUIRE(outputBytes2 == expectedOutputBytes2);
    }

    SECTION("padRightBytes Test") {
      Bytes inputBytes = Hex::toBytes("0xabcdef");
      Bytes outputBytes = Utils::padRightBytes(inputBytes, 10, 0x00);
      Bytes outputBytes2 = Utils::padRightBytes(inputBytes, 20, 0x11);
      Bytes expectedOutputBytes = Hex::toBytes("0xabcdef00000000000000");
      Bytes expectedOutputBytes2 = Hex::toBytes("0xabcdef1111111111111111111111111111111111");
      REQUIRE(outputBytes == expectedOutputBytes);
      REQUIRE(outputBytes2 == expectedOutputBytes2);
    }

    SECTION("fromBigEndian Test") {
      std::string_view inputBytes("\x10\x11\x40\xd6\xe7\x50\x6f\x80\x4c\xf7\xb0\x37\x0f\xa9\x0b\x04\xc5\xe9\x37\x4d\xdb\x0c\x8c\xbe\x12\xaf\x15\x0c\x8f\xf3\xee\x36");

      auto uint256Output = Utils::fromBigEndian<uint256_t>(inputBytes);
      auto uint64Ouput12To20 = Utils::fromBigEndian<uint64_t>(inputBytes.substr(12, 8));
      auto uint64Ouput20To28 = Utils::fromBigEndian<uint64_t>(inputBytes.substr(20, 8));
      auto uint64Output24To28 = Utils::fromBigEndian<uint64_t>(inputBytes.substr(24, 4));
      auto uint32Output28To32 = Utils::fromBigEndian<uint32_t>(inputBytes.substr(28, 4));
      auto uint160Output5To25 = Utils::fromBigEndian<uint160_t>(inputBytes.substr(5, 20));
      uint256_t uint256ExpectedOutput = uint256_t("7267489482988504755957722036644729207517128093499486419604741885099068616246");
      uint64_t uint64ExpectedOutput12To20 = uint64_t(1128445296761190221);
      uint64_t uint64ExpectedOutput20To28 = uint64_t(15784145542011884812ULL);
      uint64_t uint64ExpectedOutput24To28 = uint64_t(313464076);
      uint32_t uint32ExpectedOutput28To32 = uint32_t(2415128118);
      uint160_t uint160ExpectedOutput5To25 = uint160_t("459205820946237488389499242237511570682479951378");
      REQUIRE(uint256Output == uint256ExpectedOutput);
      REQUIRE(uint64Ouput12To20 == uint64ExpectedOutput12To20);
      REQUIRE(uint64Ouput20To28 == uint64ExpectedOutput20To28);
      REQUIRE(uint64Output24To28 == uint64ExpectedOutput24To28);
      REQUIRE(uint32Output28To32 == uint32ExpectedOutput28To32);
      REQUIRE(uint160Output5To25 == uint160ExpectedOutput5To25);
    }

    SECTION("toLower Test") {
      std::string inputStr = "ABCDEF";
      std::string outputStr = inputStr;
      Utils::toLower(outputStr);
      std::string expectedOutputStr = "abcdef";
      REQUIRE_THAT(outputStr, Equals(expectedOutputStr));
    }

    SECTION("toUpper Test") {
      std::string inputStr = "abcdef";
      std::string outputStr = inputStr;
      Utils::toUpper(outputStr);
      std::string expectedOutputStr = "ABCDEF";
      REQUIRE_THAT(outputStr, Equals(expectedOutputStr));
    }

    SECTION("appendBytes Test") {
      Bytes int1 = Bytes{0x78, 0xF0, 0xB2, 0x91};
      Bytes int2 = Bytes{0xAC, 0x26, 0x0E, 0x43};
      Bytes res = Bytes{0x78, 0xF0, 0xB2, 0x91, 0xAC, 0x26, 0x0E, 0x43};
      Utils::appendBytes(int1, int2);
      REQUIRE(int1 == res);
    }

    SECTION("bytesToString Test") {
      Bytes b1 = Bytes{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37};
      Bytes b2 = Bytes{0x30, 0x42, 0x34, 0x48, 0x52, 0x36, 0x33, 0x39};
      std::string s1 = Utils::bytesToString(b1);
      std::string s2 = Utils::bytesToString(b2);
      REQUIRE_THAT(s1, Equals("01234567"));
      REQUIRE_THAT(s2, Equals("0B4HR639"));
    }

    SECTION("stringToBytes Test") {
      std::string s1 = "01234567";
      std::string s2 = "0B4HR639";
      Bytes b1 = Utils::stringToBytes(s1);
      Bytes b2 = Utils::stringToBytes(s2);
      REQUIRE(b1 == Bytes{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37});
      REQUIRE(b2 == Bytes{0x30, 0x42, 0x34, 0x48, 0x52, 0x36, 0x33, 0x39});
    }
  }
}

