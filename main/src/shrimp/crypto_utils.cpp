#define GLOBAL
#include "crypto_utils.h"
#include "includes.h"
#include "globals.h"
#include <string.h>
#include <stdlib.h>

extern "C" {
    #include "mbedtls/aes.h"
}

// Wrapper macros to use mbedtls AES functions
#define esp_aes_setkey_enc(ctx, key, bits) mbedtls_aes_setkey_enc(ctx, key, bits)
#define esp_aes_setkey_dec(ctx, key, bits) mbedtls_aes_setkey_dec(ctx, key, bits)
#define esp_aes_crypt_cbc(ctx, mode, len, iv, input, output) mbedtls_aes_crypt_cbc(ctx, mode, len, iv, input, output)
#define ESP_AES_ENCRYPT MBEDTLS_AES_ENCRYPT
#define ESP_AES_DECRYPT MBEDTLS_AES_DECRYPT

extern "C" {

int shrimp_aes_encrypt(const char* src, size_t son, char *dst, const char *cualKey)
{
	int theSize = son;
	int rem = theSize % 16;
	theSize += 16 - rem;  // Round to next 16 for AES
	bzero(iv, sizeof(iv));  // Global

	char *donde = (char*)calloc(theSize, 1);
	if (!donde)
		return ESP_FAIL;
	
	memcpy(donde, src, son);	

	if (esp_aes_setkey_enc(&actx, (const unsigned char*)cualKey, 256) == 0) {
		if (esp_aes_crypt_cbc(&actx, ESP_AES_ENCRYPT, theSize, (unsigned char*)iv, 
		                       (const unsigned char*)donde, (unsigned char*)dst) != 0) {
			free(donde);
			return ESP_FAIL;
		}
	} else {
		free(donde);
		return ESP_FAIL;
	}

	free(donde);
	return theSize;
}

int shrimp_aes_decrypt(const char* src, size_t son, char *dst, const unsigned char *cualKey)
{
	bzero(dst, son);
	bzero(iv, sizeof(iv));  // Global
	
	if (esp_aes_setkey_dec(&actx, (const unsigned char*)cualKey, 256) != 0)
		return ESP_FAIL;
	
	if (esp_aes_crypt_cbc(&actx, ESP_AES_DECRYPT, son, (unsigned char*)iv, 
	                       (const unsigned char*)src, (unsigned char*)dst) != 0)
		return ESP_FAIL;
	
	return son;
}

} // extern "C"
