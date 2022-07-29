// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2013-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#include <web3cpp/devcore/Guards.h>  // <boost/thread> conflicts with <thread>
#include <web3cpp/devcrypto/Common.h>
#include <cryptopp/aes.h>
#include <cryptopp/pwdbased.h>
#include <cryptopp/sha.h>
#include <cryptopp/modes.h>
#include <libscrypt.h>
#include <web3cpp/devcore/SHA3.h>
#include <web3cpp/devcore/RLP.h>
#include <web3cpp/devcrypto/AES.h>
#include <web3cpp/devcrypto/CryptoPP.h>
#include <web3cpp/devcrypto/Exceptions.h>
using namespace dev;
using namespace dev::crypto;

void dev::encrypt(Public const& _k, bytesConstRef _plain, bytes& o_cipher)
{
    bytes io = _plain.toBytes();
    Secp256k1PP::get()->encrypt(_k, io);
    o_cipher = std::move(io);
}

bool dev::decrypt(Secret const& _k, bytesConstRef _cipher, bytes& o_plaintext)
{
    bytes io = _cipher.toBytes();
    Secp256k1PP::get()->decrypt(_k, io);
    if (io.empty())
        return false;
    o_plaintext = std::move(io);
    return true;
}

void dev::encryptECIES(Public const& _k, bytesConstRef _plain, bytes& o_cipher)
{
    encryptECIES(_k, bytesConstRef(), _plain, o_cipher);
}

void dev::encryptECIES(Public const& _k, bytesConstRef _sharedMacData, bytesConstRef _plain, bytes& o_cipher)
{
    bytes io = _plain.toBytes();
    Secp256k1PP::get()->encryptECIES(_k, _sharedMacData, io);
    o_cipher = std::move(io);
}

bool dev::decryptECIES(Secret const& _k, bytesConstRef _cipher, bytes& o_plaintext)
{
    return decryptECIES(_k, bytesConstRef(),  _cipher, o_plaintext);
}

bool dev::decryptECIES(Secret const& _k, bytesConstRef _sharedMacData, bytesConstRef _cipher, bytes& o_plaintext)
{
    bytes io = _cipher.toBytes();
    if (!Secp256k1PP::get()->decryptECIES(_k, _sharedMacData, io))
        return false;
    o_plaintext = std::move(io);
    return true;
}

void dev::encryptSym(Secret const& _k, bytesConstRef _plain, bytes& o_cipher)
{
    // TODO: @alex @subtly do this properly.
    encrypt(KeyPair(_k).pub(), _plain, o_cipher);
}

bool dev::decryptSym(Secret const& _k, bytesConstRef _cipher, bytes& o_plain)
{
    // TODO: @alex @subtly do this properly.
    return decrypt(_k, _cipher, o_plain);
}

std::pair<bytes, h128> dev::encryptSymNoAuth(SecureFixedHash<16> const& _k, bytesConstRef _plain)
{
    h128 iv(Nonce::get().makeInsecure());
    return make_pair(encryptSymNoAuth(_k, iv, _plain), iv);
}

bytes dev::encryptAES128CTR(bytesConstRef _k, h128 const& _iv, bytesConstRef _plain)
{
    if (_k.size() != 16 && _k.size() != 24 && _k.size() != 32)
        return bytes();
    CryptoPP::SecByteBlock key(_k.data(), _k.size());
    try
    {
        CryptoPP::CTR_Mode<CryptoPP::AES>::Encryption e;
        e.SetKeyWithIV(key, key.size(), _iv.data());
        bytes ret(_plain.size());
        e.ProcessData(ret.data(), _plain.data(), _plain.size());
        return ret;
    }
    catch (CryptoPP::Exception& _e)
    {
        std::cerr << _e.what() << std::endl;
        return bytes();
    }
}

bytesSec dev::decryptAES128CTR(bytesConstRef _k, h128 const& _iv, bytesConstRef _cipher)
{
    if (_k.size() != 16 && _k.size() != 24 && _k.size() != 32)
        return bytesSec();
    CryptoPP::SecByteBlock key(_k.data(), _k.size());
    try
    {
        CryptoPP::CTR_Mode<CryptoPP::AES>::Decryption d;
        d.SetKeyWithIV(key, key.size(), _iv.data());
        bytesSec ret(_cipher.size());
        d.ProcessData(ret.writable().data(), _cipher.data(), _cipher.size());
        return ret;
    }
    catch (CryptoPP::Exception& _e)
    {
        std::cerr << _e.what() << std::endl;
        return bytesSec();
    }
}

bytesSec dev::pbkdf2(std::string const& _pass, bytes const& _salt, unsigned _iterations, unsigned _dkLen)
{
    bytesSec ret(_dkLen);
    if (CryptoPP::PKCS5_PBKDF2_HMAC<CryptoPP::SHA256>().DeriveKey(
        ret.writable().data(),
        _dkLen,
        0,
        reinterpret_cast<byte const*>(_pass.data()),
        _pass.size(),
        _salt.data(),
        _salt.size(),
        _iterations
    ) != _iterations)
        BOOST_THROW_EXCEPTION(CryptoException() << errinfo_comment("Key derivation failed."));
    return ret;
}

bytesSec dev::scrypt(std::string const& _pass, bytes const& _salt, uint64_t _n, uint32_t _r, uint32_t _p, unsigned _dkLen)
{
    bytesSec ret(_dkLen);
    if (libscrypt_scrypt(
        reinterpret_cast<uint8_t const*>(_pass.data()),
        _pass.size(),
        _salt.data(),
        _salt.size(),
        _n,
        _r,
        _p,
        ret.writable().data(),
        _dkLen
    ) != 0)
        BOOST_THROW_EXCEPTION(CryptoException() << errinfo_comment("Key derivation failed."));
    return ret;
}

KeyPair::KeyPair(Secret const& _sec):
    m_secret(_sec),
    m_public(toPublic(_sec))
{
    // Assign address only if the secret key is valid.
    if (m_public)
        m_address = toAddress(m_public);
}

KeyPair KeyPair::create()
{
    while (true)
    {
        KeyPair keyPair(Secret::random());
        if (keyPair.address())
            return keyPair;
    }
}

KeyPair KeyPair::fromEncryptedSeed(bytesConstRef _seed, std::string const& _password)
{
    return KeyPair(Secret(sha3(aesDecrypt(_seed, _password))));
}

h256 crypto::kdf(Secret const& _priv, h256 const& _hash)
{
    // H(H(r||k)^h)
    h256 s;
    sha3mac(Secret::random().ref(), _priv.ref(), s.ref());
    s ^= _hash;
    sha3(s.ref(), s.ref());
    
    if (!s || !_hash || !_priv)
        BOOST_THROW_EXCEPTION(InvalidState());
    return s;
}

Secret Nonce::next()
{
    Guard l(x_value);
    if (!m_value)
    {
        m_value = Secret::random();
        if (!m_value)
            BOOST_THROW_EXCEPTION(InvalidState());
    }
    m_value = sha3Secure(m_value.ref());
    return sha3(~m_value);
}
