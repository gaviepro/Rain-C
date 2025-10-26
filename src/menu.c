/**
 * menu.c - Interface CLI de RAIN-C
 * - Parse les arguments (-G/-L, -o, -a, -s) et affiche l’aide.
 * - Mode -G : orchestre la génération de la T3C (dict → T3C).
 * - Mode -L : orchestre la recherche (chargement T3C + recherche via -s ou stdin).
 * - Délègue la logique métier à control_dict.c et control_T3C.c.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "control_dict.h"
#include "control_T3C.h"


// Définition des variables globales
static int mode = -1;                       // -1 = non défini, 0 = mode -G, 1 = mode -L
static char *dict_path = NULL;              // chemin du dictionnaire donné par le user
static char *hash_search  = NULL;           // hash à chercher donné par le user
static char *t3c_path = "lab/rainbowTAB.t3c";   // chemin par défaut du fichier T3C (sortie en -G, entrée en -L)
static char *algo_choice = "sha256";        // algo par défaut (sha256)

// Help du programme
void help(char *prog){
    printf(
        "┌──────────────────────────────────────────────────────────────────────────────┐\n"
        "|                                 RAIN-C - HELP                                |\n"
        "|──────────────────────────────────────────────────────────────────────────────|\n"
        "| Usage                                                                        |\n"
        "|   %s -h -> Affiche l'aide                                        |\n"
        "|   %s -G <dict.txt> [-o <out.t3c>] [-a <algo>]                    |\n"
        "|   %s -L <table.t3c> [-s <condensat-hex>]                         |\n"
        "|                                                                              |\n"
        "| Description :                                                                |\n"
        "|   -G : Génère une table T3C (hash -> mdp) à partir d'un dictionnaire de mdp  |\n"
        "|        -o <out.t3c> : attribue le fichier de sortie de la table              |\n" 
        "|                       [rainbowTAB.t3c] default                               |\n"
        "|        -a <algo> : algo de hachage (sha256 | sha512 | blake2b512 | sha3-256  |\n"
        "|                    [sha256] default                                          |\n"
        "|                                                                              |\n"
        "|   -L <table.t3c> : Recherche dans une table T3C existante                    |\n"
        "|        -s <hash> : renvoie le mdp associée au condensat donnée               |\n"
        "|                                                                              |\n"
        "|──────────────────────────────────────────────────────────────────────────────|\n"
        "| Exemples                                                                     |\n"
        "|   Génération :                                                               |\n"
        "|     %s -G dict.txt -o rainbowTAB.t3c -a sha256                   |\n"
        "|                                                                              |\n"
        "|   Recherche (stdin) :                                                        |\n"
        "|     echo \"7c4b7e57...\" | %s -L rainbowTAB.t3c                    |\n"
        "|                                                                              |\n"
        "|   Recherche (unique) :                                                       |\n"
        "|     %s -L rainbowTAB.t3c -s 7c4b7e570b75...                      |\n"
        "└──────────────────────────────────────────────────────────────────────────────┘\n",
        prog, prog, prog, prog, prog, prog);
}

// Valide l’algorithme de hachage donné par -a
int algo_exist(char *algo_name){
    char *authAlgo[4] = {"sha256","sha512","blake2b512","sha3-256"};
    for (int i = 0; i < 4; ++i){
        if (strcmp(authAlgo[i], algo_name) == 0) 
            return 0;
    }
    return -1;
}

// Exécute le mode choisi :
// - mode == 0 : génération (-G) -> lit dictionnaire, calcule les hash de chaque mdp, remplit la table, et écrit dans un fichier T3C
// - mode == 1 : Recherche (-L) -> charge un fichier T3C, construit l'index à partir du fichier, cherche le hash donnée avec -s ou stdin
int exec_mode(void){
    if (mode == 0){
        t3c_table tab; // table T3C utilisé pour le programme en mémoire
        t3c_init(&tab); // initialise la structure à 0 pour commencer l'ecriture dans la table

        // Remplit la table depuis le dictionnaire en appliquant l’algo choisi
        if (dict_to_Table(dict_path, algo_choice, &tab) == -1){
            printf("menu.c : ERREUR > La fonction dict_to_table à échoué\n");
            t3c_free(&tab); // libère les allocations internes si une erreur est survenue
            return -1;
        }

        // Écrit la table dans un fichier T3C avec comme disposition hash<TAB>mdp
        if (create_t3c(&tab, t3c_path, algo_choice) != 0){
            printf("menu.c : ERREUR > La fonction create_t3c à échoué\n");
            t3c_free(&tab);
            return -1;
        }

        // Message de succès donnant le nombre d'entrées, le chemin du T3C et le nom de l'algo utilisé
        printf("Execution terminée avec brio ! %zu entrées écrites dans %s avec l'algorithme de Hachage %s\n", tab.nbobj, t3c_path, algo_choice);
        t3c_free(&tab); // nettoyage de la table en mémoire
        return 0;

    } else if (mode == 1){ // Mode recherche (-L)
        // Si -s absent : demander un condensat à l’utilisateur (1 seul lookup)
        if(hash_search == NULL){
            char buf[1025]; //tampon de stdin           
            printf("Ecrivez le Hash que vous recherchez : \n");
            scanf("%s",buf); // lit un mot
            return t3c_mode_lookup(t3c_path,buf); 
        }

        // Si -s est présent : on lance la recherche direct
        return t3c_mode_lookup(t3c_path, hash_search);
    }
    return -1;
}

// Analyse les arguments de la ligne de commande
int start(int argc, char **argv){
    if (argc < 2){
        // Aucun argument -> afficher l'aide et signaler une erreur
        help(argv[0]);
        return -1;
    }

    // mode G
    if (strcoll(argv[1], "-G") == 0){
        // Exige un fichier dictionnaire
        if (argc < 3 || argv[2][0] == '-'){ 
            help(argv[0]); 
            return -1; 
        }
        dict_path = argv[2]; // mémorise le chemin du dictionnaire
    
        // vérifie que le fichier est lisible
        if (access(dict_path, R_OK) != 0){ 
            printf("menu.c : ERREUR > Le dictionnaire est illisible changé de fichier ou vérifier qu'il soit bien créée\n");
            return -1; 
        }
        mode = 0; // passe en mode génération

        // Parcourt les options spécifiques à -G
        for (int i = 3; i < argc; ){
            // -o <out.t3c> : spécifie le chemin de sortie du T3C
            if (strcoll(argv[i], "-o") == 0 && i+1 < argc && argv[i+1][0] != '-'){
                t3c_path = argv[i+1]; 
                i += 2;
            }
            // -a <algo> : spécifie l'algorithme de hachage
            else if (strcoll(argv[i], "-a") == 0 && i+1 < argc && argv[i+1][0] != '-'){
                algo_choice = argv[i+1];
                if (algo_exist(algo_choice) != 0){
                    // si l'algo n'est pas dans la liste autorisée -> erreur et arrêt
                    printf("menu.c : ERREUR > L'algo choisi est invalide '%s' Algo authorisé : sha256 | sha512 | blake2b512 | sha3-256\n", algo_choice);
                    return -1;
                }
                i += 2;
            }
            // Option inconnue après -G -> erreur
            else {
                printf("menu.c : ERREUR > Option inconnue en mode -G: %s\n", argv[i]);
                return -1;
            }
        }
    }
    // mode recherche L
    else if (strcoll(argv[1], "-L") == 0){
        // Exige un fichier T3C
        if (argc < 3 || argv[2][0] == '-'){ 
            help(argv[0]); 
            return -1; 
        }
        t3c_path = argv[2]; // mémorise le chemin du T3C
        if (access(t3c_path, R_OK) != 0){ // vérifie la lisibilité du fichier T3C
            printf("menu.c : ERREUR > Le fichier T3C est illisible changé de fichier ou vérifier qu'il soit bien créée\n");
            return -1; 
        }
        mode = 1; // passe en mode Recherche

        // -s <hash> on assigne le <hash> à hash_search
        if(argc > 3 ){
            if (strcoll(argv[3], "-s") == 0 && 4 < argc && argv[4][0] != '-')
                hash_search = argv[4]; 
        }

    }else if(strcoll(argv[1], "-h") == 0){
        // -h -> afficher l'aide
        help(argv[0]);
        return 0;
    }else {
        // si pas de mode trouvé on envoie le help
        help(argv[0]);
        return -1;
    }

    // Lance l'exécution du mode chois
    return exec_mode();
}
