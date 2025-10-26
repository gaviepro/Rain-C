/**
 * main.c - start de Rain-C
 *  - Délègue au module menu l'analyse des arguments (-G / -L)
 *    et l'exécution du mode choisi.
 *  - Centralise le code de retour du programme : 0 = succès, -1 = erreur.
 * 
 *  On effectue aucune action du programme ici, le main ne sert que de lanceur 
 *  et toute la logique est dans le start() pour améliorer la lecture du programme
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "menu.h"

int main(int argc, char **argv){
    if(start(argc,argv)!=0){
        printf("main.c : ERREUR > La fonction start de menu.c à échoué\n");
        return -1;
    }
    return 0; 
}
