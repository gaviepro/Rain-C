#ifndef CONTROL_T3C_H
#define CONTROL_T3C_H

#include <stddef.h> 

typedef struct {
    char *mdp;        
    char *hash_hex;   
} t3c_entry;

typedef struct {
    t3c_entry *items;
    size_t nbobj;
    size_t maxSize;
} t3c_table;

typedef struct t3c_node_ {
    t3c_entry *items;
    struct t3c_node_ *left;
    struct t3c_node_ *right;
} t3c_node;

void t3c_init(t3c_table *table);
void t3c_free(t3c_table *table);
int  t3c_add(t3c_table *table, char *mdp, char *hash_hex);

int create_t3c(const t3c_table *table, char *path, char *algo_name);
int t3c_load(char *path, t3c_table *table);

t3c_node *t3c_index_build(t3c_table *table);
t3c_entry *t3c_lookup(t3c_node *node_root, char *hash_hex);
void t3c_index_free(t3c_node *node_root);

int t3c_mode_lookup(char *t3c_path, char *single_hash);

#endif
