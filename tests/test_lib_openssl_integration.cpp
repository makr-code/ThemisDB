#include <gtest/gtest.h>
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <vector>
#include <string>
#include <memory>

// Test fixture for OpenSSL library integration
class OpenSSLLibIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize OpenSSL error strings
        ERR_load_crypto_strings();
        OpenSSL_add_all_algorithms();
    }
    
    void TearDown() override {
        // Cleanup OpenSSL
        EVP_cleanup();
        ERR_free_strings();
    }
};

// Test 1: OpenSSL library linking and version check
TEST_F(OpenSSLLibIntegrationTest, LibraryLinkingAndVersion) {
    // Check OpenSSL version is available
    unsigned long version = OpenSSL_version_num();
    EXPECT_GT(version, 0UL) << "OpenSSL version check failed";
    
    const char* version_str = OpenSSL_version(OPENSSL_VERSION);
    ASSERT_NE(version_str, nullptr);
    EXPECT_GT(strlen(version_str), 0);
}

// Test 2: Random number generation (RAND API)
TEST_F(OpenSSLLibIntegrationTest, RandomNumberGeneration) {
    unsigned char buffer[32];
    
    // Generate random bytes
    int ret = RAND_bytes(buffer, sizeof(buffer));
    EXPECT_EQ(ret, 1) << "RAND_bytes failed";
    
    // Verify not all zeros (extremely unlikely with real random)
    bool has_nonzero = false;
    for (size_t i = 0; i < sizeof(buffer); ++i) {
        if (buffer[i] != 0) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
}

// Test 3: SHA-256 hashing
TEST_F(OpenSSLLibIntegrationTest, SHA256Hashing) {
    const std::string input = "test data for hashing";
    unsigned char hash[SHA256_DIGEST_LENGTH];
    
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), 
           input.length(), hash);
    
    // Verify hash is not all zeros
    bool has_nonzero = false;
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        if (hash[i] != 0) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
    
    // Verify deterministic - same input produces same hash
    unsigned char hash2[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), 
           input.length(), hash2);
    EXPECT_EQ(0, memcmp(hash, hash2, SHA256_DIGEST_LENGTH));
}

// Test 4: HMAC-SHA256
TEST_F(OpenSSLLibIntegrationTest, HMACSHA256) {
    const std::string key = "secret_key";
    const std::string data = "data to authenticate";
    unsigned char result[EVP_MAX_MD_SIZE];
    unsigned int result_len = 0;
    
    HMAC(EVP_sha256(), 
         key.c_str(), key.length(),
         reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
         result, &result_len);
    
    EXPECT_EQ(result_len, 32u); // SHA256 produces 32 bytes
    
    // Verify HMAC is deterministic
    unsigned char result2[EVP_MAX_MD_SIZE];
    unsigned int result_len2 = 0;
    HMAC(EVP_sha256(), 
         key.c_str(), key.length(),
         reinterpret_cast<const unsigned char*>(data.c_str()), data.length(),
         result2, &result_len2);
    
    EXPECT_EQ(result_len, result_len2);
    EXPECT_EQ(0, memcmp(result, result2, result_len));
}

// Test 5: AES-256-GCM encryption/decryption
TEST_F(OpenSSLLibIntegrationTest, AES256GCMEncryption) {
    const std::string plaintext = "Sensitive data to encrypt";
    unsigned char key[32]; // 256-bit key
    unsigned char iv[12];  // 96-bit IV for GCM
    
    RAND_bytes(key, sizeof(key));
    RAND_bytes(iv, sizeof(iv));
    
    // Encrypt
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    ASSERT_NE(ctx, nullptr);
    
    EXPECT_EQ(1, EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key, iv));
    
    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    int len = 0;
    EXPECT_EQ(1, EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                                   reinterpret_cast<const unsigned char*>(plaintext.c_str()),
                                   plaintext.size()));
    int ciphertext_len = len;
    
    EXPECT_EQ(1, EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len));
    ciphertext_len += len;
    
    unsigned char tag[16];
    EXPECT_EQ(1, EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag));
    
    EVP_CIPHER_CTX_free(ctx);
    
    // Decrypt
    ctx = EVP_CIPHER_CTX_new();
    ASSERT_NE(ctx, nullptr);
    
    EXPECT_EQ(1, EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key, iv));
    
    std::vector<unsigned char> decrypted(ciphertext_len + EVP_CIPHER_block_size(EVP_aes_256_gcm()));
    EXPECT_EQ(1, EVP_DecryptUpdate(ctx, decrypted.data(), &len,
                                   ciphertext.data(), ciphertext_len));
    int decrypted_len = len;
    
    EXPECT_EQ(1, EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag));
    EXPECT_EQ(1, EVP_DecryptFinal_ex(ctx, decrypted.data() + len, &len));
    decrypted_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    // Verify decryption matches plaintext
    EXPECT_EQ(plaintext.size(), static_cast<size_t>(decrypted_len));
    EXPECT_EQ(plaintext, std::string(reinterpret_cast<char*>(decrypted.data()), decrypted_len));
}

// Test 6: RSA key generation
TEST_F(OpenSSLLibIntegrationTest, RSAKeyGeneration) {
    BIGNUM* bn = BN_new();
    ASSERT_NE(bn, nullptr);
    EXPECT_EQ(1, BN_set_word(bn, RSA_F4));
    
    RSA* rsa = RSA_new();
    ASSERT_NE(rsa, nullptr);
    
    // Generate 2048-bit key
    EXPECT_EQ(1, RSA_generate_key_ex(rsa, 2048, bn, nullptr));
    
    // Verify key components exist
    const BIGNUM* n = nullptr;
    const BIGNUM* e = nullptr;
    RSA_get0_key(rsa, &n, &e, nullptr);
    ASSERT_NE(n, nullptr);
    ASSERT_NE(e, nullptr);
    
    EXPECT_GT(BN_num_bytes(n), 0);
    EXPECT_GT(BN_num_bytes(e), 0);
    
    RSA_free(rsa);
    BN_free(bn);
}

// Test 7: RSA encryption/decryption
TEST_F(OpenSSLLibIntegrationTest, RSAEncryptionDecryption) {
    // Generate RSA key pair
    BIGNUM* bn = BN_new();
    BN_set_word(bn, RSA_F4);
    
    RSA* rsa = RSA_new();
    RSA_generate_key_ex(rsa, 2048, bn, nullptr);
    
    const std::string plaintext = "Secret message";
    std::vector<unsigned char> encrypted(RSA_size(rsa));
    
    // Encrypt
    int encrypted_len = RSA_public_encrypt(
        plaintext.size(),
        reinterpret_cast<const unsigned char*>(plaintext.c_str()),
        encrypted.data(),
        rsa,
        RSA_PKCS1_OAEP_PADDING
    );
    EXPECT_GT(encrypted_len, 0);
    
    // Decrypt
    std::vector<unsigned char> decrypted(RSA_size(rsa));
    int decrypted_len = RSA_private_decrypt(
        encrypted_len,
        encrypted.data(),
        decrypted.data(),
        rsa,
        RSA_PKCS1_OAEP_PADDING
    );
    EXPECT_GT(decrypted_len, 0);
    
    std::string decrypted_str(reinterpret_cast<char*>(decrypted.data()), decrypted_len);
    EXPECT_EQ(plaintext, decrypted_str);
    
    RSA_free(rsa);
    BN_free(bn);
}

// Test 8: Digital signature creation and verification
TEST_F(OpenSSLLibIntegrationTest, DigitalSignature) {
    // Generate RSA key pair
    EVP_PKEY* pkey = EVP_PKEY_new();
    ASSERT_NE(pkey, nullptr);
    
    BIGNUM* bn = BN_new();
    BN_set_word(bn, RSA_F4);
    RSA* rsa = RSA_new();
    RSA_generate_key_ex(rsa, 2048, bn, nullptr);
    EVP_PKEY_assign_RSA(pkey, rsa);
    BN_free(bn);
    
    const std::string data = "Data to sign";
    
    // Create signature
    EVP_MD_CTX* mctx = EVP_MD_CTX_new();
    ASSERT_NE(mctx, nullptr);
    
    EXPECT_EQ(1, EVP_DigestSignInit(mctx, nullptr, EVP_sha256(), nullptr, pkey));
    EXPECT_EQ(1, EVP_DigestSignUpdate(mctx, data.data(), data.size()));
    
    size_t sig_len = 0;
    EXPECT_EQ(1, EVP_DigestSignFinal(mctx, nullptr, &sig_len));
    
    std::vector<unsigned char> signature(sig_len);
    EXPECT_EQ(1, EVP_DigestSignFinal(mctx, signature.data(), &sig_len));
    EVP_MD_CTX_free(mctx);
    
    // Verify signature
    mctx = EVP_MD_CTX_new();
    EXPECT_EQ(1, EVP_DigestVerifyInit(mctx, nullptr, EVP_sha256(), nullptr, pkey));
    EXPECT_EQ(1, EVP_DigestVerifyUpdate(mctx, data.data(), data.size()));
    EXPECT_EQ(1, EVP_DigestVerifyFinal(mctx, signature.data(), sig_len));
    
    EVP_MD_CTX_free(mctx);
    EVP_PKEY_free(pkey);
}

// Test 9: Base64 encoding/decoding
TEST_F(OpenSSLLibIntegrationTest, Base64EncodingDecoding) {
    const std::string input = "Test data for base64";
    
    // Encode
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO* mem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, mem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    
    BIO_write(b64, input.c_str(), input.size());
    BIO_flush(b64);
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(b64, &bptr);
    std::string encoded(bptr->data, bptr->length);
    
    BIO_free_all(b64);
    
    EXPECT_GT(encoded.size(), 0u);
    
    // Decode
    b64 = BIO_new(BIO_f_base64());
    mem = BIO_new_mem_buf(encoded.c_str(), encoded.size());
    b64 = BIO_push(b64, mem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    
    std::vector<char> decoded(input.size() + 1);
    int decoded_len = BIO_read(b64, decoded.data(), decoded.size());
    
    BIO_free_all(b64);
    
    EXPECT_GT(decoded_len, 0);
    EXPECT_EQ(input, std::string(decoded.data(), decoded_len));
}

// Test 10: PBKDF2 key derivation
TEST_F(OpenSSLLibIntegrationTest, PBKDF2KeyDerivation) {
    const std::string password = "user_password";
    unsigned char salt[16];
    RAND_bytes(salt, sizeof(salt));
    
    const int iterations = 100000;
    const int key_len = 32;
    unsigned char derived_key[key_len];
    
    int ret = PKCS5_PBKDF2_HMAC(
        password.c_str(), password.size(),
        salt, sizeof(salt),
        iterations,
        EVP_sha256(),
        key_len, derived_key
    );
    
    EXPECT_EQ(ret, 1);
    
    // Verify derived key is not all zeros
    bool has_nonzero = false;
    for (int i = 0; i < key_len; ++i) {
        if (derived_key[i] != 0) {
            has_nonzero = true;
            break;
        }
    }
    EXPECT_TRUE(has_nonzero);
    
    // Verify deterministic - same inputs produce same key
    unsigned char derived_key2[key_len];
    PKCS5_PBKDF2_HMAC(
        password.c_str(), password.size(),
        salt, sizeof(salt),
        iterations,
        EVP_sha256(),
        key_len, derived_key2
    );
    
    EXPECT_EQ(0, memcmp(derived_key, derived_key2, key_len));
}

// Test 11: EVP cipher API (high-level encryption interface)
TEST_F(OpenSSLLibIntegrationTest, EVPCipherAPI) {
    const std::string plaintext = "Test encryption data";
    unsigned char key[32];
    unsigned char iv[16];
    
    RAND_bytes(key, sizeof(key));
    RAND_bytes(iv, sizeof(iv));
    
    // Encrypt using EVP API
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    ASSERT_NE(ctx, nullptr);
    
    EXPECT_EQ(1, EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv));
    
    std::vector<unsigned char> ciphertext(plaintext.size() + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    int len = 0;
    EXPECT_EQ(1, EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                                   reinterpret_cast<const unsigned char*>(plaintext.c_str()),
                                   plaintext.size()));
    int ciphertext_len = len;
    
    EXPECT_EQ(1, EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len));
    ciphertext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    // Decrypt
    ctx = EVP_CIPHER_CTX_new();
    EXPECT_EQ(1, EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, key, iv));
    
    std::vector<unsigned char> decrypted(ciphertext_len + EVP_CIPHER_block_size(EVP_aes_256_cbc()));
    EXPECT_EQ(1, EVP_DecryptUpdate(ctx, decrypted.data(), &len,
                                   ciphertext.data(), ciphertext_len));
    int decrypted_len = len;
    
    EXPECT_EQ(1, EVP_DecryptFinal_ex(ctx, decrypted.data() + len, &len));
    decrypted_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    EXPECT_EQ(plaintext, std::string(reinterpret_cast<char*>(decrypted.data()), decrypted_len));
}

// Test 12: Error handling and error string retrieval
TEST_F(OpenSSLLibIntegrationTest, ErrorHandling) {
    // Force an error by using invalid parameters
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    
    // Try to encrypt with null cipher (should fail)
    int ret = EVP_EncryptInit_ex(ctx, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(ret, 0); // Should fail
    
    // Get error string
    unsigned long err = ERR_get_error();
    if (err != 0) {
        char err_buf[256];
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        EXPECT_GT(strlen(err_buf), 0u);
    }
    
    EVP_CIPHER_CTX_free(ctx);
}
