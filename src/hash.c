/**
 * hash.c - Utilitaires de hachage
 * - calcule le condensat d'une chaîne avec l'algo choisi
 * - convertit un digest binaire en hexadécimal
 */

#include <openssl/evp.h>

#include <string.h>
#include "hash.h"

// Calcule le hash algo_name de input dans digest et met sa longueur dans digest_taille
int string_to_hash(char *input,char *algo_name,unsigned char digest[EVP_MAX_MD_SIZE],unsigned int *digest_taille)
{
    // Récupère le descripteur d'algorithme par son nom
    const EVP_MD *md = EVP_get_digestbyname(algo_name);
    if (!md){
        printf("hash.c : ERREUR > la variable EVP_MD est vide\n");
        return -1;
    }

    // Alloue un contexte de calcul EVP
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (!ctx){
        printf("hash.c : ERREUR > la variable EVP_MD_CTX est vide\n");
        return -1;
    }

    // Initialise le contexte avec l’algorithme choisi
    if (!EVP_DigestInit_ex(ctx, md,NULL)){
        printf("hash.c : ERREUR > la fonction EVP_DigestInit à échoué, les variables CTX et MD n'ont pas pus etre utilisé\n");
        return -1;
    }

    // Alimente le contexte avec les octets de input
    if (!EVP_DigestUpdate(ctx, input, strlen(input))){
        printf("hash.c : ERREUR > la fonction EVP_DigestUpdate à échoué, le calcul du hash à partir de l'entrée user à echoué\n");
        return -1;
    }

    // Termine le calcul et écrit le digest en binaire ainsi que sa taille 
    if (!EVP_DigestFinal_ex(ctx, digest, digest_taille)){
        printf("hash.c : ERREUR > la fonction EVP_DigestFinal à échoué, le Hash et la taille du Hash n'a pas pus etre attribué aux variables donnée\n");
        return -1;
    }

    // Libere le contexte
    EVP_MD_CTX_free(ctx);
    return 0;
}

// Convertit un digest binaire en chaîne hex minuscule dans digest_hex
void bin_to_hex(unsigned char *digest_bin, unsigned int digest_taille, char *digest_hex){
    char *hex = "0123456789abcdef";
    // Pour chaque octet produire deux caractères hexadécimaux nible haut et nible bas */
    for (unsigned int i = 0; i < digest_taille; i++){
        digest_hex[2 * i]     = hex[(digest_bin[i] >> 4) & 0xF];
        digest_hex[2 * i + 1] = hex[digest_bin[i] & 0xF];
    }
    digest_hex[2 * digest_taille] = '\0';
}
