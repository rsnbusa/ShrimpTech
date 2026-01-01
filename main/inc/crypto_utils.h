#ifndef CRYPTO_UTILS_H_
#define CRYPTO_UTILS_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Encrypt data using AES-256-CBC encryption
 * 
 * @param src Source data to encrypt
 * @param son Length of source data
 * @param dst Destination buffer for encrypted data (must be large enough)
 * @param cualKey 256-bit encryption key
 * @return int Size of encrypted data on success, ESP_FAIL on error
 */
int shrimp_aes_encrypt(const char* src, size_t son, char *dst, const char *cualKey);

/**
 * @brief Decrypt data using AES-256-CBC decryption
 * 
 * @param src Source encrypted data
 * @param son Length of source data
 * @param dst Destination buffer for decrypted data
 * @param cualKey 256-bit decryption key
 * @return int Size of decrypted data on success, ESP_FAIL on error
 */
int shrimp_aes_decrypt(const char* src, size_t son, char *dst, const unsigned char *cualKey);

#ifdef __cplusplus
}
#endif

#endif /* CRYPTO_UTILS_H_ */
