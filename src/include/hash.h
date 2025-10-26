#ifndef HASH_H
#define HASH_H

#include <stddef.h>
#include <openssl/evp.h>

int string_to_hash(char *input,char *algo_name,unsigned char digest[EVP_MAX_MD_SIZE],unsigned int *digest_taille);
void bin_to_hex(unsigned char *digest_bin, unsigned int digest_taille, char *digest_hex);
#endif
