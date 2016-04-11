
// libsse_crypto - An abstraction layer for high level cryptographic features.
// Copyright (C) 2015-2016 Raphael Bost
//
// This file is part of libsse_crypto.
//
// libsse_crypto is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// libsse_crypto is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with libsse_crypto.  If not, see <http://www.gnu.org/licenses/>.
//

#include "tdp.hpp"

#include "random.hpp"

#include <cstring>
#include <exception>
#include <iostream>
#include <iomanip>

#include <openssl/rsa.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/pem.h>

namespace sse
{

namespace crypto
{
	
    
static_assert(Tdp::kMessageSize == TdpInverse::kMessageSize, "Constants kMessageSize of Tdp and TdpInverse do not match");

#define RSA_MODULUS_SIZE TdpInverse::kMessageSize*8
#define RSA_PK RSA_3
    
class TdpImpl
{
public:
    static constexpr uint kMessageSpaceSize = Tdp::kMessageSize;
    
    TdpImpl(const std::string& pk);
    
    ~TdpImpl();
    
    
    RSA* get_rsa_key() const;
    void set_rsa_key(RSA* k);
    uint rsa_size() const;
    
    std::string public_key() const;
    
    void eval(const std::string &in, std::string &out) const;
    std::array<uint8_t, kMessageSpaceSize> eval(const std::array<uint8_t, kMessageSpaceSize> &in) const;

    std::string sample() const;
    std::array<uint8_t, kMessageSpaceSize> sample_array() const;

protected:
    TdpImpl();

    RSA *rsa_key_;
    
};

class TdpInverseImpl : public TdpImpl
{
public:
    TdpInverseImpl();
    TdpInverseImpl(const std::string& sk);
    TdpInverseImpl(const TdpInverseImpl& tdp);
    
    std::string private_key() const;
    void invert(const std::string &in, std::string &out) const;
    std::array<uint8_t, kMessageSpaceSize> invert(const std::array<uint8_t, kMessageSpaceSize> &in) const;
};

class TdpMultPoolImpl : public TdpImpl
{
public:
    TdpMultPoolImpl(const std::string& sk, const uint8_t size);
    
    ~TdpMultPoolImpl();
    
    std::array<uint8_t, TdpImpl::kMessageSpaceSize> eval(const std::array<uint8_t, kMessageSpaceSize> &in, const uint8_t order) const;
    void eval(const std::string &in, std::string &out, const uint8_t order) const;

    uint8_t maximum_order() const;
    uint8_t pool_size() const;
private:
    RSA **keys_;
    uint8_t keys_count_;
};

TdpImpl::TdpImpl() : rsa_key_(NULL)
{
    
}
TdpImpl::TdpImpl(const std::string& pk) : rsa_key_(NULL)
{
    // create a BIO from the std::string
    BIO *mem;
    mem = BIO_new_mem_buf(((void*)pk.data()), (int)pk.length());
    
    // read the key from the BIO
    rsa_key_ = PEM_read_bio_RSAPublicKey(mem,NULL,NULL,NULL);

    if(rsa_key_ == NULL)
    {
        throw std::runtime_error("Error when initializing the RSA key from public key.");
    }
    
    // close and destroy the BIO
    BIO_set_close(mem, BIO_NOCLOSE); // So BIO_free() leaves BUF_MEM alone
    BIO_free(mem);
}
    
inline RSA* TdpImpl::get_rsa_key() const
{
    return rsa_key_;
}
    
inline void TdpImpl::set_rsa_key(RSA* k)
{
    if(k == NULL)
    {
        throw std::runtime_error("Invalid input: k == NULL.");
    }

    rsa_key_ = k;
}

inline uint TdpImpl::rsa_size() const
{
    return ((uint) RSA_size(get_rsa_key()));
}

TdpImpl::~TdpImpl()
{
    RSA_free(rsa_key_);
}

std::string TdpImpl::public_key() const
{
    int ret;
    
    // initialize a buffer
    BIO *bio = BIO_new(BIO_s_mem());
    
    // write the key to the buffer
    ret = PEM_write_bio_RSAPublicKey(bio, rsa_key_);

    if(ret != 1)
    {
        throw std::runtime_error("Error when serializing the RSA public key.");
    }
    

    // put the buffer in a std::string
    size_t len = BIO_ctrl_pending(bio);
    void *buf = malloc(len);
    
    int read_bytes = BIO_read(bio, buf, (int)len);
    
    if(read_bytes == 0)
    {
        throw std::runtime_error("Error when reading BIO.");
    }

    std::string v(reinterpret_cast<const char*>(buf), len);
    
    BIO_free_all(bio);
    free(buf);
    
    return v;
}

void TdpImpl::eval(const std::string &in, std::string &out) const
{
    if(in.size() != rsa_size())
    {
        throw std::runtime_error("Invalid TDP input size. Input size should be kMessageSpaceSize bytes long.");
    }

    unsigned char rsa_out[RSA_size(rsa_key_)];
    
    RSA_public_encrypt((int)in.size(), (unsigned char*)in.data(), rsa_out, rsa_key_, RSA_NO_PADDING);
    
    out = std::string((char*)rsa_out,RSA_size(rsa_key_));
}

std::array<uint8_t, TdpImpl::kMessageSpaceSize> TdpImpl::eval(const std::array<uint8_t, kMessageSpaceSize> &in) const
{
    std::array<uint8_t, TdpImpl::kMessageSpaceSize> out;

    RSA_public_encrypt((int)in.size(), (unsigned char*)in.data(), out.data(), rsa_key_, RSA_NO_PADDING);
    
    return out;
}

std::string TdpImpl::sample() const
{
    // I don't really trust OpenSSL PRNG, but this is the simplest way
    int ret;
    BIGNUM *rnd;
    
    rnd = BN_new();
    
    ret = BN_rand_range(rnd, rsa_key_->n);
    if(ret != 1)
    {
        throw std::runtime_error("Invalid random number generation.");
    }
    
    unsigned char *buf = new unsigned char[BN_num_bytes(rnd)];
    BN_bn2bin(rnd, buf);
    
    std::string v(reinterpret_cast<const char*>(buf), BN_num_bytes(rnd));
    
    BN_free(rnd);
    delete [] buf;
    
    return v;
}

std::array<uint8_t, TdpImpl::kMessageSpaceSize> TdpImpl::sample_array() const
{
    // I don't really trust OpenSSL PRNG, but this is the simplest way
    int ret;
    BIGNUM *rnd;
    std::array<uint8_t, kMessageSpaceSize> out;
    
    rnd = BN_new();
    
    ret = BN_rand_range(rnd, rsa_key_->n);
    if(ret != 1)
    {
        throw std::runtime_error("Invalid random number generation.");
    }
    
    BN_bn2bin(rnd, out.data());
    BN_free(rnd);
    
    return out;
}


TdpInverseImpl::TdpInverseImpl()
{
    int ret;
    
    // initialize the key
    set_rsa_key(RSA_new());
    
    // generate a new random key
    
    unsigned long e = RSA_PK;
    BIGNUM *bne = NULL;
    bne = BN_new();
    ret = BN_set_word(bne, e);
    if(ret != 1)
    {
        throw std::runtime_error("Invalid BIGNUM initialization.");
    }
    
    ret = RSA_generate_key_ex(get_rsa_key(), RSA_MODULUS_SIZE, bne, NULL);
    if(ret != 1)
    {
        throw std::runtime_error("Invalid RSA key generation.");
    }
    
    BN_free(bne);
}

TdpInverseImpl::TdpInverseImpl(const std::string& sk)
{
    // create a BIO from the std::string
    BIO *mem;
    mem = BIO_new_mem_buf(((void*)sk.data()), (int)sk.length());

    EVP_PKEY* evpkey;
    evpkey = PEM_read_bio_PrivateKey(mem, NULL, NULL, NULL);

    if(evpkey == NULL)
    {
        throw std::runtime_error("Error when reading the RSA private key.");
    }

    // read the key from the BIO
    set_rsa_key( EVP_PKEY_get1_RSA(evpkey));

    
    // close and destroy the BIO
    BIO_set_close(mem, BIO_NOCLOSE); // So BIO_free() leaves BUF_MEM alone
    BIO_free(mem);
}

TdpInverseImpl::TdpInverseImpl(const TdpInverseImpl& tdp)
{
    set_rsa_key(RSAPrivateKey_dup(tdp.rsa_key_));
}
    
std::string TdpInverseImpl::private_key() const
{
    int ret;
    
    // create an EVP encapsulation
    EVP_PKEY* evpkey = EVP_PKEY_new();
    ret = EVP_PKEY_set1_RSA(evpkey, get_rsa_key());
    if(ret != 1)
    {
        throw std::runtime_error("Invalid EVP initialization.");
    }
    
    // initialize a buffer
    BIO *bio = BIO_new(BIO_s_mem());
    
    // write the key to the buffer
    ret = PEM_write_bio_PKCS8PrivateKey(bio, evpkey, NULL, NULL, 0, NULL, NULL);
    if(ret != 1)
    {
        throw std::runtime_error("Failure when writing private KEY.");
    }
    
    // put the buffer in a std::string
    size_t len = BIO_ctrl_pending(bio);
    void *buf = malloc(len);
    
    int read_bytes = BIO_read(bio, buf, (int)len);
    if(read_bytes == 0)
    {
        throw std::runtime_error("Error when reading BIO.");
    }
    
    
    std::string v(reinterpret_cast<const char*>(buf), len);
    
    EVP_PKEY_free(evpkey);
    BIO_free_all(bio);
    free(buf);
    
    return v;
}


void TdpInverseImpl::invert(const std::string &in, std::string &out) const
{
    int ret;
    unsigned char rsa_out[rsa_size()];

    if(in.size() != rsa_size())
    {
        throw std::runtime_error("Invalid TDP input size. Input size should be kMessageSpaceSize bytes long.");
    }
 
    ret = RSA_private_decrypt((int)in.size(), (unsigned char*)in.data(), rsa_out, get_rsa_key(), RSA_NO_PADDING);
    
    
    out = std::string((char*)rsa_out,ret);
}

std::array<uint8_t, TdpImpl::kMessageSpaceSize> TdpInverseImpl::invert(const std::array<uint8_t, kMessageSpaceSize> &in) const
{
    std::array<uint8_t, TdpImpl::kMessageSpaceSize> out;
    
    RSA_private_decrypt((int)in.size(), (unsigned char*)in.data(), out.data(), get_rsa_key(), RSA_NO_PADDING);
    
    return out;
}

TdpMultPoolImpl::TdpMultPoolImpl(const std::string& sk, const uint8_t size)
: TdpImpl(sk), keys_count_(size-1)
{
    if (size == 0) {
        throw std::invalid_argument("Invalid Multiple TDP pool input size. Pool size should be > 0.");
    }
    
    keys_ = new RSA* [keys_count_];
    
    keys_[0] = RSAPublicKey_dup(get_rsa_key());
    BN_mul_word(keys_[0]->e, RSA_PK);

    for (uint8_t i = 1; i < keys_count_; i++) {
        
        keys_[i] = RSAPublicKey_dup(keys_[i-1]);
        BN_mul_word(keys_[i]->e, RSA_PK);
    }
    
}

TdpMultPoolImpl::~TdpMultPoolImpl()
{
    for (uint8_t i = 0; i < keys_count_; i++) {
        RSA_free(keys_[i]);
    }
    delete [] keys_;
}

std::array<uint8_t, TdpImpl::kMessageSpaceSize> TdpMultPoolImpl::eval(const std::array<uint8_t, kMessageSpaceSize> &in, const uint8_t order) const
{
    std::array<uint8_t, TdpImpl::kMessageSpaceSize> out;

    if (order == 1) {
        // regular eval
        RSA_public_encrypt((int)in.size(), (unsigned char*)in.data(), out.data(), get_rsa_key(), RSA_NO_PADDING);

    }else if(order <= maximum_order()){
        // get the right RSA context, i.e. the one in keys_[order-1]
        RSA_public_encrypt((int)in.size(), (unsigned char*)in.data(), out.data(), keys_[order-2], RSA_NO_PADDING);
    }else{
        throw std::invalid_argument("Invalid order for this TDP pool. The input order must be less than the maximum order supported by the pool, and strictly positive.");
    }
    
    return out;
}


void TdpMultPoolImpl::eval(const std::string &in, std::string &out, const uint8_t order) const
{
    if(in.size() != rsa_size())
    {
        throw std::runtime_error("Invalid TDP input size. Input size should be kMessageSpaceSize bytes long.");
    }
    
    std::array<uint8_t, kMessageSpaceSize> a_in, a_out;
    std::copy(in.begin(), in.end(), a_in.begin());
    a_out = eval(a_in, order);
    
    out = std::string(a_out.begin(), a_out.end());
    
}

uint8_t TdpMultPoolImpl::maximum_order() const
{
    return keys_count_+1;
}
uint8_t TdpMultPoolImpl::pool_size() const
{
    return keys_count_+1;
}


Tdp::Tdp(const std::string& sk) : tdp_imp_(new TdpImpl(sk))
{
}

Tdp::~Tdp()
{
    delete tdp_imp_;
    tdp_imp_ = NULL;
}

std::string Tdp::public_key() const
{
    return tdp_imp_->public_key();
}
    
std::string Tdp::sample() const
{
    return tdp_imp_->sample();
}

std::array<uint8_t, Tdp::kMessageSize> Tdp::sample_array() const
{
    return tdp_imp_->sample_array();
}

void Tdp::eval(const std::string &in, std::string &out) const
{
    tdp_imp_->eval(in, out);
}

std::string Tdp::eval(const std::string &in) const
{
    std::string out;
    tdp_imp_->eval(in, out);
    
    return out;
}

std::array<uint8_t, Tdp::kMessageSize> Tdp::eval(const std::array<uint8_t, kMessageSize> &in) const
{
    return tdp_imp_->eval(in);
}

TdpInverse::TdpInverse() : tdp_inv_imp_(new TdpInverseImpl())
{
}

TdpInverse::TdpInverse(const std::string& sk) : tdp_inv_imp_(new TdpInverseImpl(sk))
{
}

TdpInverse::TdpInverse(const TdpInverse& tdp) : tdp_inv_imp_(new TdpInverseImpl(tdp.private_key()))
{
}

TdpInverse::~TdpInverse()
{
    delete tdp_inv_imp_;
    tdp_inv_imp_ = NULL;
}

std::string TdpInverse::public_key() const
{
    return tdp_inv_imp_->public_key();
}

std::string TdpInverse::private_key() const
{
    return tdp_inv_imp_->private_key();
}

std::string TdpInverse::sample() const
{
    return tdp_inv_imp_->sample();
}

std::array<uint8_t, TdpInverse::kMessageSize> TdpInverse::sample_array() const
{
    return tdp_inv_imp_->sample_array();
}
    
void TdpInverse::eval(const std::string &in, std::string &out) const
{
    tdp_inv_imp_->eval(in, out);
}

std::string TdpInverse::eval(const std::string &in) const
{
    std::string out;
    tdp_inv_imp_->eval(in, out);
    
    return out;
}

std::array<uint8_t, TdpInverse::kMessageSize> TdpInverse::eval(const std::array<uint8_t, kMessageSize> &in) const
{
    return tdp_inv_imp_->eval(in);
}

void TdpInverse::invert(const std::string &in, std::string &out) const
{
    tdp_inv_imp_->invert(in, out);
}

std::string TdpInverse::invert(const std::string &in) const
{
    std::string out;
    tdp_inv_imp_->invert(in, out);
    
    return out;
}

std::array<uint8_t, TdpInverse::kMessageSize> TdpInverse::invert(const std::array<uint8_t, kMessageSize> &in) const
{
    return tdp_inv_imp_->invert(in);
}
    
TdpMultPool::TdpMultPool(const std::string& pk, const uint8_t size) : tdp_pool_imp_(new TdpMultPoolImpl(pk, size))
{
}

TdpMultPool::~TdpMultPool()
{
    delete tdp_pool_imp_;
    tdp_pool_imp_ = NULL;
}

std::string TdpMultPool::public_key() const
{
    return tdp_pool_imp_->public_key();
}

std::string TdpMultPool::sample() const
{
    return tdp_pool_imp_->sample();
}

std::array<uint8_t, Tdp::kMessageSize> TdpMultPool::sample_array() const
{
    return tdp_pool_imp_->sample_array();
}

void TdpMultPool::eval(const std::string &in, std::string &out, uint8_t order) const
{
    tdp_pool_imp_->eval(in, out, order);
}

std::string TdpMultPool::eval(const std::string &in, uint8_t order) const
{
    std::string out;
    tdp_pool_imp_->eval(in, out, order);
    
    return out;
}

std::array<uint8_t, Tdp::kMessageSize> TdpMultPool::eval(const std::array<uint8_t, kMessageSize> &in, uint8_t order) const
{
    return tdp_pool_imp_->eval(in, order);
}
    
uint8_t TdpMultPool::maximum_order() const
{
    return tdp_pool_imp_->maximum_order();
}
uint8_t TdpMultPool::pool_size() const
{
    return tdp_pool_imp_->pool_size();
}

}
}