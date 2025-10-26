# Rain-C — Générateur de RainbowTable & Lookup de mots de passe

**Rain-C** est un utilitaire C qui **fabrique** des tables T3C de correspondances `condensat ↔ mot de passe` à partir d’un dictionnaire, puis **retrouve** le clair associé à un condensat en **chargeant** la table en mémoire et en l’**indexant** avec un **arbre binaire de recherche**.

---

## Ce que fait Rain-C

Le programme se pilote avec **2 modes**.

En **génération** (`-G`), vous fournissez un **dictionnaire** avec un mot de passe par ligne. Rain-C calcule les **condensats** et écrit un fichier **T3C** (`.t3c`) au format : `condensat<TAB>motdepasse`.  
En **recherche** (`-L`), vous fournissez une **T3C** existante et Rain-C la charge puis construit un **arbre binaire en mémoire** afin de retrouver le mot de passe correspondant au **condensat** fourni soit avec `-s`, soit **en flux** via `stdin`.

Pendant la génération et le chargement, des **barres de progression** indiquent l’avancement de la génération / recherche ce qui est pratique pour les gros dictionnaires.  
Les algorithmes pris en charge sont : `sha256`, `sha512`, `blake2b512` et `sha3-256`.

---

## Docker 

Construisez l’image depuis la **racine du projet** là où se trouve le `Dockerfile` :

```bash
sudo docker build -t rainc .
```

**Génération dans Docker :**
```bash
sudo docker run --rm -it -v "$PWD:/data" -w /data rainc -G lab/rockyou_1000.txt -o lab/rainbowTAB.t3c -a sha256
```

**Lookup d’un hash unique :**
```bash
sudo docker run --rm -it -v "$PWD:/data" -w /data rainc -L lab/rainbowTAB.t3c -s 496b7706d1f04a4d767c4117589596026110067cf81fda050056a0460b03e109
```

**Lookup via `stdin` (pipe/fichier) :**  
Quand vous alimentez Rain-C en **flux**, gardez `stdin` **ouvert** avec `-it` :

```bash
echo "496b7706d1f04a4d767c4117589596026110067cf81fda050056a0460b03e109" | sudo docker run -i --rm -v "$PWD:/data" -w /data rainc -L lab/rainbowTAB.t3c
```

**Lookup interactif :**  
Pour une **saisie utilisateur** (usage de `scanf`), utilisez un **TTY** avec `-it` :
```bash
sudo docker run -it --rm -v "$PWD:/data" -w /data rainc -L lab/rainbowTAB.t3c
```
## Compilation avec makeFile (Debian/Ubuntu)

```bash
sudo apt update
sudo apt install -y build-essential libssl-dev
```

Depuis la **racine du dépôt** :

```bash
make
./lab/rainc
```

### Usage (aide et options)
([...] -> optionnels)

```bash
./lab/rainc -h
./lab/rainc -G <dict.txt> [-o <out.t3c>] [-a <algo>]
./lab/rainc -L <table.t3c>  [-s <condensat-hex>]
```

**Description rapide :**  
`-G` génère une table T3C correspondances `hash -> mdp` à partir d’un dictionnaire 
`-o <out.t3c>` fixe le fichier de sortie (par défaut `rainbowTAB.t3c`)
`-a <algo>` choisit l’algorithme (`sha256 | sha512 | blake2b512 | sha3-256`) (par défaut `sha256`)

`-L <table.t3c>` recherche dans une T3C existante
`-s <hash>` renvoie le mot de passe associé au condensat fourni sinon lisez depuis `stdin`

