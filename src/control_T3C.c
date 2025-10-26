/**
 * control_T3C.c - Gestion de la table T3C et de la recherche
 *  - Lecture/écriture d'une table T3C
 *  - Stockage en mémoire dans une structure t3c_table
 *  - Construction d'un index de recherche sur le champ hash
 *  - Recherche d'un hash (mode -L), éventuellement en lisant des hashes depuis stdin.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "control_T3C.h"

/* Duplique une chaîne de caractere pour récuperer sa valeur et non l'adresse de la table
    - Alloue un nouveau buffer de taille strlen(str)+1
    - Copie le contenu, renvoie le pointeur alloué (ou NULL en cas d'échec)
*/
char *keepStr(char *str){
    size_t taille = strlen(str);
    char *str_cp = (char*)malloc(taille + 1);
    if (!str_cp) 
        return NULL;
    memcpy(str_cp, str, taille + 1);
    return str_cp;
}

/* Découpe une ligne T3C en place et renvoie un pointeur vers le mdp
   - Cherche la première tabulation, la remplace par '\0' pour terminer le hash.
   - password pointe juste après la tabulation
   - Supprime les caracteres intuilesespaces/tabs/\n/\r
   - Retourne:
       * pointeur sur 'password' si OK,
       * NULL si pas de tabulation ou si l'un des champs est vide.
*/
char *parse_t3c(char *ligne){
    char *tab = strchr(ligne, '\t');
    if (!tab)                    
        return NULL; 
    *tab = '\0';
    char *password = tab + 1;
    
    size_t taille_password = strlen(password);
    while (taille_password > 0 && (password[taille_password-1] == '\n' || password[taille_password-1] == '\r')) 
        password[taille_password--] = '\0';
    while (*password == ' ' || *password == '\t') 
        password++;

    if (*ligne == '\0' || *password == '\0') 
        return NULL;

    return password;
}

/* Affiche une barre de progression sur une seule ligne
   - nbligneDone / nbligneMax -> pourcentage calculé.
   - Si printText non vide il sera affiché à droite des compteurs 
*/
void progress_bar(size_t nbligneDone, size_t nbligneMax, char *printText){
    if (!nbligneMax){
        printf("control_t3c.c : ERREUR > Le dictionnaire transmis est vide ou une erreur est arrivé au calcul du nombre de ligne du dictionnaire\n");
        return;
    }
    unsigned int larg = 50;
    unsigned int loading = nbligneDone * larg / nbligneMax;
    int pourcentage = nbligneDone * 100  / nbligneMax;
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

/* Compte les lignes dans un fichier T3C pour estimer la progression et pré-allouer la table
   - Une ligne valide est non vide et contient au moins une tabulation \t
   - rewind() le fichier avant de retourner.
   - Retourne le nombre de lignes valides. 
*/
size_t count_lignes(FILE *fichier){
    char *ligne = NULL;
    size_t taille_buf  = 0; //gestion du malloc automatique avec get ligne, gere le malloc de ligne
    ssize_t taille =0;
    size_t nbligneMax = 0;

    while ((taille = getline(&ligne, &taille_buf, fichier)) != -1){
        while (taille > 0 && (ligne[taille-1] == '\n' || ligne[taille-1] == '\r')) 
            ligne[taille--] = '\0';
        if (taille == 0) 
            continue; 
        if (strchr(ligne, '\t') == NULL)
            continue;
        nbligneMax++;

    }
    free(ligne);
    rewind(fichier);
    return nbligneMax;
}

// Initialise une table T3C vide
void t3c_init(t3c_table *table){
    table->items  = NULL;
    table->nbobj  = 0;
    table->maxSize= 0;
}

/* Libère toutes les allocations liées à la table :
   - Libère pour chaque entrée les champs mdp et hash_hex 
   - Libère le tableau items
   - Ré-initialise la structure à l'état vide.
*/
void t3c_free(t3c_table *table){
    for (size_t i = 0; i < table->nbobj; ++i){
        free(table->items[i].mdp);
        free(table->items[i].hash_hex);
    }
    free(table->items);
    t3c_init(table);
}

/* Ajoute une entrée du couple hash et mdp dans la table
   - Duplique mdp et hash_hex (pour ne pas dépendre de tampons temporaires) 
*/
int t3c_add(t3c_table *table, char *mdp, char *hash_hex){
    /* 1) Vérifie la capacité : plus de réalloc ici */
    if (table->nbobj >= table->maxSize){
        printf("control_t3c.c : ERREUR > capacité insuffisante (nbobj=%zu, maxSize=%zu)\n",table->nbobj, table->maxSize);
        return -1;
    }

    char *password = keepStr(mdp);
    char *hash = keepStr(hash_hex);
    if (!password || !hash){
        free(password);
        free(hash);
        return -1; 
    }

    table->items[table->nbobj].mdp = password;
    table->items[table->nbobj].hash_hex = hash;
    table->nbobj++;
    return 0;
}

/* Écrit la table T3C dans un fichier texte
   - Ajoute un en-tête avec le nom d'algorithme
   - Affiche une barre de progression pendant l'écriture
*/
int create_t3c(const t3c_table *table, char *path, char *algo_name){
    FILE *fichier = fopen(path, "w");
    if (!fichier){
        printf("control_t3c.c : ERREUR > La Table t3c est vide ou l'ouverture du fichier t3c à eu un probleme\n");
        return -1;
    }
    fprintf(fichier, "# T3C\talgo=%s\tCols: hash\tdisplay\n",algo_name);

    size_t nbLigneMax = table->nbobj;
    int pourcentage;
    if(nbLigneMax >= 100){
            pourcentage = nbLigneMax / 100;
        }else{
            pourcentage = 1;
        }

    for (size_t i = 0; i < nbLigneMax; ++i){
        char *h = table->items[i].hash_hex;
        char *p = table->items[i].mdp;
        fprintf(fichier, "%s\t%s\n", h, p);

        size_t nbLigneDone = i + 1;
        if ((nbLigneDone % pourcentage) == 0 || nbLigneDone == nbLigneMax)
            progress_bar(nbLigneDone, nbLigneMax, "( Ecriture Table )");
    }

    fclose(fichier);
    return 0;
}

/* Charge un fichier T3C en mémoire :
   - Compte les lignes valides puis pré-alloue la table
   - Lit ligne par ligne, découpe en place avec parse_t3c() et ajoute via t3c_add()
   - Affiche la progression.
*/
int t3c_load(char *path, t3c_table *table){
    FILE *fichier = fopen(path, "r");
    if (!fichier){
        printf("control_t3c.c : ERREUR > La table est vide ou l'ouverture de la table t3C à eu un probleme\n");
        return -1;
    }
    t3c_init(table);

    size_t nbLigneMax = count_lignes(fichier);
    if (nbLigneMax == 0){
        printf("control_t3c.c : ERREUR > Le fichier t3c est vide\n");
        fclose(fichier);
        return -1;
    }
    
    table->items = (t3c_entry*)malloc(nbLigneMax * sizeof *table->items);
    if (table->items == NULL){
        printf("control_t3c.c : ERREUR > malloc table (%zu)\n", nbLigneMax);
        fclose(fichier);
        return -1;
    }
    table->maxSize = nbLigneMax;
    table->nbobj   = 0;

    size_t nbLigneDone = 0;
    int pourcentage;
    if (nbLigneMax >= 100){
        pourcentage = (int)(nbLigneMax / 100);
    } else {
        pourcentage = 1;
    }

    char *ligne = NULL;
    size_t taille_buf  = 0;
    ssize_t taille = 0;
    int retour =0;
    while ((taille = getline(&ligne, &taille_buf, fichier)) > 0){
        while (taille > 0 && (ligne[taille-1] == '\n' || ligne[taille-1] == '\r')) 
            ligne[taille--] = '\0';

        if (ligne[0] == '#' || ligne[0] == '\0') 
            continue;

        char *password = parse_t3c(ligne);
        if (!password) 
            continue;
        
        char *hash = ligne;
        if (t3c_add(table, password, hash) != 0) {
            retour = -1;
            break;
        }
        nbLigneDone++;
        if ((nbLigneDone % (size_t)pourcentage) == 0 || nbLigneDone == nbLigneMax){
            progress_bar(nbLigneDone, nbLigneMax-1, "( Chargement T3C )");
        }
    }
    printf("\n");

    free(ligne);
    fclose(fichier);

    if (retour != 0){
        t3c_free(table); 
        return -1;
    }

    return 0;
}

/* Insertion dans l'arbre de recherche d'index :
   - Si l'arbre est vide alloue la racine
   - Sinon, parcourt jusqu'à trouver le noeud vide où mettre le nouveau noeud
   - Ignore les doublons
*/
static t3c_node *t3c_index_insert(t3c_node *node, t3c_entry *table){
    t3c_node *copy_node;
    t3c_node *parent;
    int comp;

    if (node == NULL){
        t3c_node *newNode = (t3c_node*)malloc(sizeof *newNode);
        if (newNode == NULL)
            return NULL;

        newNode->items = table;
        newNode->left  = NULL;
        newNode->right = NULL;
        return newNode;
    }

    copy_node = node;
    parent = NULL;
    comp = 0;

    while (copy_node != NULL){
        parent = copy_node;
        comp = strcmp(table->hash_hex, copy_node->items->hash_hex);
        if (comp < 0){
            copy_node = copy_node->left;
        } else if (comp > 0){
            copy_node = copy_node->right;
        } else {
            return node;
        }
    }

    t3c_node *newNode = (t3c_node*)malloc(sizeof *newNode);
    if (newNode == NULL){
        return node;
    }

    newNode->items = table;
    newNode->left  = NULL;
    newNode->right = NULL;

    if (comp < 0){
        parent->left = newNode;
    } else {
        parent->right = newNode;
    }

    return node;
}

/* Construit l'index BST complet à partir de la table :
   - Insère chaque entrée du tableau
   - Si une allocation échoue on libère ce qui a été construit et renvoie NULL
*/
t3c_node *t3c_index_build(t3c_table *table){
    t3c_node *node = NULL;
    for (size_t i = 0; i < table->nbobj; ++i){
        node = t3c_index_insert(node, &table->items[i]);
        if (node == NULL){
            t3c_index_free(node);
            return NULL;
        }
    }
    return node;
}

/* Recherche d'un hash dans l'arbe
   - Compare le hash recherché au hash du noeud courant et descend à gauche/droite
   - Renvoie le pointeur vers l'entrée si trouvée sinon NULL
*/
t3c_entry *t3c_lookup(t3c_node *node, char *hash_hex){
    while (node){
        int comp = strcmp(hash_hex, node->items->hash_hex);
        if (comp < 0)      
            node = node->left;
        else if (comp > 0) 
            node = node->right;
        else              
            return node->items;
    }
    return NULL;
}

/* Libère récursivement un arbre :
   - Libère d'abord les sous-arbres gauche et droit, puis le noeud courant
*/
void t3c_index_free(t3c_node *node){
    if (node == NULL) return;
    t3c_index_free(node->left);
    t3c_index_free(node->right);
    free(node);
}

/* Mode -L :
   - Charge la table T3C en mémoire (t3c_load)
   - Construit l'index BST
   - Si un hash est fourni on effectue une recherche et affiche le mdp trouvé
   - Sinon on lit des hashes depuis stdin et affiche les mdp trouvés
   - Nettoie la mémoire avant de quitter 
*/
int t3c_mode_lookup(char *t3c_path, char *hash_search){
    t3c_table table;
    t3c_node *node = NULL;
    if (t3c_load(t3c_path, &table) != 0){
        printf("control_t3c.c : ERREUR > La fonction t3c_load a échoué\n");
        return -1;
    }

    node = t3c_index_build(&table);
    if (node == NULL && table.nbobj > 0){
        printf("control_t3c.c : ERREUR > La fonction t3c_index_build a échoué\n");
        t3c_free(&table);
        return -1;
    }

    if (hash_search && hash_search[0]){
        t3c_entry *items = t3c_lookup(node, hash_search);
        if (items) 
            puts(items->mdp);
        else
            printf("Le hash donnée est introuvable dans la table T3C\n");

    } else {
        char *ligne = NULL;
        size_t taille_buf  = 0; 
        ssize_t taille =0;

        while ((taille = getline(&ligne, &taille_buf, stdin)) != -1){
            while (taille > 0 && (ligne[taille - 1] == '\n' || ligne[taille - 1] == '\r')) 
                ligne[taille--] = '\0';

            if (!ligne[0]) 
                continue;
            t3c_entry *items = t3c_lookup(node, ligne);
            if (items) 
                puts(items->mdp);
            else
                printf("Le hash donnée est introuvable dans la table T3C\n");
        }
        free(ligne);
    }

    t3c_index_free(node);
    t3c_free(&table);
    return 0;
}
