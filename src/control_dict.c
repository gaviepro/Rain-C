/**
 * control_dict.c - Conversion dictionnaire en T3C
 * - Lit un dictionnaire ligne par ligne et ignore les lignes vides
 * - Calcule les condensats via string_to_hash(algo) + bin_to_hex()
 * - Alimente la table t3c avec t3c_table (mdp, hash) et affiche une barre de progression
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "control_dict.h"
#include "hash.h"

// Affiche une barre de progression sur stdout pour le hachage du dictionnaire
void progress_bar_dict(size_t nbligneDone, size_t nbligneMax, char *printText){
    if (!nbligneMax){
        printf("control_dict.c : ERREUR > Le dictionnaire transmis est vide ou une erreur est arrivé au calcul du nombre de ligne du dictionnaire\n");
        return;
    }
    unsigned int larg = 50;                                      // largeur visuelle de la barre
    unsigned int loading = nbligneDone * larg / nbligneMax;      // nombre de segments remplis
    int pourcentage = nbligneDone * 100  / nbligneMax;           // % accompli
    char *space = "";
    char *text  = "";

    printf("\r{");
    for (unsigned int i = 0; i < loading; ++i) 
        printf("=");
    for (unsigned int i = loading; i < larg; ++i) 
        printf("-");
    if (printText && printText[0] != '\0') {
        space = " ";
        text  = printText;
    }
    printf("} %3d%% [%zu / %zu]%s%s", pourcentage, nbligneDone, nbligneMax, space, text);
    if (nbligneDone >= nbligneMax) 
        printf("\n");
}

//Compte les lignes du dictionnaire 
size_t count_lignes_dict(FILE *fichier){
    char *ligne = NULL;
    size_t taille_buf  = 0;   // buffer auto-géré par getline
    ssize_t taille =0;
    size_t nbligneMax = 0;

    while ((taille = getline(&ligne, &taille_buf, fichier)) != -1){
        // Enleve les caracteres inutiles \n \r et place \0 a la fin 
        while (taille > 0 && (ligne[taille-1] == '\n' || ligne[taille-1] == '\r')) 
            ligne[taille--] = '\0';
        if (taille == 0)      // ligne vide ignorée
            continue; 
        nbligneMax++;         // compte les lignes non vide
    }
    free(ligne);              // libère le buffer alloué par getline
    rewind(fichier);          // remet le curseur au début pour la vraie lecture
    return nbligneMax;
}

// Convertit un fichier dictionnaire en table T3C selon algo_name
int dict_to_Table(char *path,char *algo_name, t3c_table *table){
    FILE *fichier = fopen(path, "r");
    if (!fichier) {
        printf("control_dict.c : ERREUR > Le dictionnaire est vide ou l'ouverture du dictionnaire à eu un probleme\n");
        return -1;
    }

    // Pré-compte pour la barre de progession
    size_t nbLigneMax = count_lignes_dict(fichier);
    if (nbLigneMax == 0){
        printf("control_t3c.c : ERREUR > Le fichier t3c est vide\n");
        fclose(fichier);
        return -1;
    }

    // Allocation mémoire parfaite en fonction des lignes du fichier 
    table->items = (t3c_entry*)malloc(nbLigneMax * sizeof *table->items);
    if (table->items == NULL){
        printf("control_t3c.c : ERREUR > malloc table (%zu)\n", nbLigneMax);
        fclose(fichier);
        return -1;
    }
    table->maxSize = nbLigneMax;
    table->nbobj   = 0;

    size_t nbligneDone = 0;
    int pourcentage;
    if(nbLigneMax >= 100){
        pourcentage = nbLigneMax / 100;
    }else{
        pourcentage = 1;
    }

    char *ligne = NULL;
    size_t taille_buf  = 0;  // taille dédié a l'allocation de la memoire de la ligne géré par getline
    ssize_t taille =0;

    // Lecture de chaque mot du dictionnaire
    while ((taille = getline(&ligne, &taille_buf, fichier)) != -1){
        // Enleve les caracteres inutiles \n \r et place \0 a la fin 
        while (taille > 0 && (ligne[taille - 1] == '\n' || ligne[taille - 1] == '\r')) 
            ligne[taille--] = '\0';
        if (taille == 0) 
            continue;        // ignore les lignes vides

        unsigned char digest[EVP_MAX_MD_SIZE];  // buffer du digest binaire
        unsigned int digest_taille = 0;         // longueur effective du digest

        // Hachage du mot courant avec l'algo choisi
        int retour = string_to_hash(ligne, algo_name, digest, &digest_taille);
        if (retour){
            printf("control_dict.c : ERREUR > Le hachage sur '%s' à rencontré un probleme (code de retour : %d)\n", ligne, retour);
            free(ligne);
            fclose(fichier);
            return -1;
        }

        // Conversion du digest binaire en hex
        char hex[EVP_MAX_MD_SIZE * 2 + 1];
        bin_to_hex(digest, digest_taille, hex);

        // Ajout du couple hash mdp dans la table T3C en mémoire
        if (t3c_add(table, ligne, hex) != 0){
            printf("control_dict.c : ERREUR > L'ajout d'un couple condensat et mdp dans la table à échoué\n");
            free(ligne);
            fclose(fichier);
            return -1;
        }

        // Mise à jour de la progression
        nbligneDone++;
        if ((nbligneDone % pourcentage) == 0 || nbligneDone == nbLigneMax)
            progress_bar_dict(nbligneDone, nbLigneMax, "( Hachage )");
    }

    // Nettoyage pour valgrind
    free(ligne);
    fclose(fichier);
    return 0;
}
