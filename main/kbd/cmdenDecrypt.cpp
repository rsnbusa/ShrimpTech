/**
 * @file cmdenDecrypt.cpp
 * @brief Console command for AES encryption testing and key generation
 * 
 * This file implements the endecrypt command which provides functionality
 * for testing AES encryption by generating encryption keys from numeric
 * inputs and displaying encrypted results. Used for security testing,
 * key generation verification, and encryption validation.
 * 
 * The command takes a numeric key, formats it as a 32-character encryption
 * key, encrypts the SUPERSECRET constant, and displays the first 4 bytes
 * of the encrypted output in hexadecimal format.
 * 
 * @note Part of the ShrimpIoT console command system
 * @note Requires SUPERSECRET constant to be defined
 */

#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

extern int aes_encrypt(const char* src, size_t son, char *dst,const char *cualKey);

/**
 * @brief Console command for AES encryption testing and key generation
 * 
 * This command generates an AES encryption key from a numeric input and 
 * encrypts the SUPERSECRET constant to display the first 4 bytes of the
 * encrypted result. Used for testing encryption functionality and generating
 * encryption keys.
 * 
 * @param argc Argument count from argtable3 parser
 * @param argv Argument vector from argtable3 parser
 * @return 0 on success
 * 
 * @note Usage: endecrypt -k <number>
 *       -k, --key: Numeric key (must be positive integer)
 * 
 * @note The key is formatted as a 16-digit zero-padded string, duplicated
 *       to create a 32-character encryption key
 */
int cmdEnDecrypt(int argc, char **argv)
{
    int dkey, err;
    char kkey[17], laclave[33];

    int nerrors = arg_parse(argc, argv, (void **)&endec);
    if (nerrors != 0) {
        arg_print_errors(stderr, endec.end, argv[0]);
        return 0;
    }

    if (endec.key->count) 
    {
        dkey = endec.key->ival[0];
        if (dkey <= 0)
        {
            printf("Error: Key must be a positive integer\n");
            return 0;
        }
        sprintf(kkey, "%016d", dkey);
        sprintf(laclave, "%s%s", kkey, kkey);
        
        char *aca = (char*)calloc(1000, 1);
        if (!aca)
        {
            printf("Error: Memory allocation failed\n");
            return 0;
        }
        
        err = aes_encrypt(SUPERSECRET, sizeof(SUPERSECRET), aca, laclave);
        if (err > 0)
        {
            printf("Encrypted result (first 4 bytes): %02x%02x%02x%02x\n", 
                   aca[0], aca[1], aca[2], aca[3]);
        }
        else
        {
            printf("Error: Encryption failed\n");
        }
        free(aca);
    }
    else
    {
        printf("No key specified. Use -k <number>\n");
    }
      
    return 0;
}
