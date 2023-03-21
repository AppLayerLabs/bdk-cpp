#include "tests.h"
#include "../src/utils/utils.h"
#include "../src/utils/secp256k1Wrapper.h"

void Tests::testSecp256k1() {
    PrivKey privateKey (Utils::hexToBytes("c73926a5d7c6474d1190b866777276936a29639e24544fe09714354b05b1ef40"));
    UncompressedPubkey pubkeyUncompressed = Secp256k1::toPub(privateKey);
    CompressedPubkey pubkeyCompressed = Secp256k1::toPubCompressed(privateKey);
    Hash messageHash = Utils::sha3("Message to Sign");

    auto signature = Secp256k1::sign(privateKey, messageHash);

    assert(signature.get() == Utils::hexToBytes("5d6847f11b49d660cf019a43c9dea7589372efcd7bd76631ac292448d27e341713673089e344e96d50f1709d20bb93db84a48d35c57cc39b8e925986a10231c100"));
    assert(Secp256k1::recover(signature, messageHash) == pubkeyUncompressed);
    assert(Secp256k1::verify(pubkeyUncompressed, signature, messageHash));

    assert(pubkeyCompressed.get() == Utils::hexToBytes("033c9272f4b883abbef0e51380f1e726cdeceb3f474d044ba594e7b4e8f5491984"));
    assert(pubkeyUncompressed.get() == Utils::hexToBytes("043c9272f4b883abbef0e51380f1e726cdeceb3f474d044ba594e7b4e8f5491984d05717a154cfda88eda22393c049af0f47b1d4292d0aa79fbfda084e3124fa7f"));

    assert(Secp256k1::toAddress(pubkeyCompressed).hex() == "5d83b229235fba526a859784105e432667f2546e");
    assert(Secp256k1::toAddress(pubkeyUncompressed).hex() == "5d83b229235fba526a859784105e432667f2546e");

    std::cout << "Secp256k1 OK" << std::endl;
    return;
}
