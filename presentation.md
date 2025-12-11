# Présentation : Architecture Générale et Logique de Jeu
## BlockBlast - Ressources pour Présentation Orale

---

## 📋 TABLE DES MATIÈRES

1. [Architecture Modulaire](#1-architecture-modulaire)
2. [Logique de Jeu](#2-logique-de-jeu)
3. [Gestion des États et Scoring](#3-gestion-des-états-et-scoring)
4. [Fichiers Clés à Connaître](#4-fichiers-clés-à-connaître)
5. [Questions et Réponses](#5-questions-et-réponses)
6. [Extraits de Code Importants](#6-extraits-de-code-importants)

---

## 1. ARCHITECTURE MODULAIRE

### 1.1 Séparation Client/Serveur

**Structure du projet :**
```
BlockBlast/
├── client/          # Application cliente (interface utilisateur)
│   ├── main.c       # Point d'entrée, boucle principale
│   ├── game.c/h     # Logique de jeu
│   ├── screens.c/h  # Gestion des écrans
│   ├── net_client.c/h # Communication réseau
│   └── ...
├── server/          # Serveur de jeu
│   └── server_main.c # Gestion des parties multijoueur
└── common/          # Code partagé
    ├── config.h     # Constantes (grille, couleurs, etc.)
    └── net_protocol.h # Protocole réseau
```

**Avantages de cette architecture :**
- ✅ **Séparation des responsabilités** : Le client gère l'interface et l'affichage, le serveur gère la logique multijoueur
- ✅ **Multijoueur en réseau** : Permet à plusieurs joueurs de jouer ensemble
- ✅ **Évolutivité** : Facile d'ajouter de nouvelles fonctionnalités (modes de jeu, spectateurs, etc.)
- ✅ **Réutilisabilité** : Le code client peut être utilisé en mode solo sans serveur
- ✅ **Maintenance** : Code organisé et modulaire, facile à maintenir

**Communication Client/Serveur :**
- Utilise des sockets TCP/IP
- Protocole défini dans `common/net_protocol.h`
- Messages typés (MSG_LOGIN, MSG_PLACE_PIECE, MSG_GAME_OVER, etc.)
- Le serveur gère les salles de jeu, les tours, la synchronisation

### 1.2 Organisation des Fichiers Sources

**Modules principaux du client :**

| Fichier | Responsabilité |
|---------|----------------|
| `main.c` | Boucle principale, gestion des événements SDL |
| `game.c/h` | Logique de jeu (grille, pièces, placement, lignes) |
| `screens.c/h` | Rendu des différents écrans (menu, jeu, lobby) |
| `graphics.c/h` | Fonctions de rendu de base (rectangles, texte, blocs) |
| `ui_components.c/h` | Composants UI réutilisables (boutons, sliders) |
| `input_handlers.c/h` | Gestion des entrées utilisateur (souris, clavier) |
| `globals.c/h` | Variables globales et état de l'application |
| `net_client.c/h` | Communication réseau avec le serveur |
| `audio.c/h` | Gestion audio (musique, effets sonores) |
| `save_system.c/h` | Sauvegarde/chargement des parties |

**Principe de modularité :**
- Chaque module a une responsabilité claire
- Communication via interfaces bien définies (fichiers .h)
- Pas de dépendances circulaires
- Code réutilisable (ex: `render_game_grid_ex()` peut être appelée pour différents contextes)

---

## 2. LOGIQUE DE JEU

### 2.1 Structure de la Grille

**Définition dans `common/config.h` :**
```c
#define GRID_W 10  // Largeur de la grille (10 colonnes)
#define GRID_H 10  // Hauteur de la grille (10 lignes)
```

**Représentation en mémoire (`game.h`) :**
```c
typedef struct {
    int grid[GRID_H][GRID_W];  // Matrice 10x10
    // 0 = case vide
    // != 0 = case occupée (valeur = couleur en hexadécimal)
    // ...
} GameState;
```

**Caractéristiques :**
- Grille de 10x10 = 100 cellules au total
- Chaque cellule peut être vide (0) ou occupée (couleur)
- La grille est stockée dans `GameState` qui contient aussi le score, les pièces, etc.

### 2.2 Système de Pièces : 19 Formes Différentes

**Définition dans `game.c` :**
```c
static Piece piece_templates[] = {
    // 19 pièces différentes définies
    // Chaque pièce a :
    // - data[5][5] : matrice 5x5 représentant la forme
    // - w, h : largeur et hauteur réelles
    // - color : couleur en hexadécimal
};
#define NUM_TEMPLATES 19
```

**Structure d'une pièce (`game.h`) :**
```c
typedef struct {
    int data[5][5];  // Matrice 5x5 (taille max d'une pièce)
    int w, h;        // Dimensions réelles (w <= 5, h <= 5)
    int color;       // Couleur de la pièce
} Piece;
```

**Exemples de pièces :**
- Ligne de 4 blocs (1x4)
- Ligne de 5 blocs (1x5)
- Carré 2x2
- Carré 3x3
- Formes en L, T, Z, etc.

**Génération des pièces :**
```c
void generate_pieces(GameState *gs) {
    // Génère 3 pièces aléatoires parmi les 19 templates
    for (i = 0; i < 3; i++) {
        int r = rand() % NUM_TEMPLATES;  // 0 à 18
        gs->current_pieces[i] = piece_templates[r];
        gs->pieces_available[i] = 1;
    }
}
```

### 2.3 Détection de Placement Valide : `can_place()`

**Fonction dans `game.c` (lignes 357-378) :**
```c
int can_place(GameState *gs, int row, int col, Piece *p) {
    int i, j;
    
    // Parcourt chaque cellule de la pièce
    for (i = 0; i < p->h; i++) {
        for (j = 0; j < p->w; j++) {
            if (p->data[i][j]) {  // Si cette cellule de la pièce est occupée
                int gr = row + i;  // Position dans la grille
                int gc = col + j;
                
                // Vérification des limites
                if (gr < 0 || gr >= GRID_H || gc < 0 || gc >= GRID_W) {
                    return 0;  // Hors limites
                }
                
                // Vérification si la case est déjà occupée
                if (gs->grid[gr][gc] != 0) {
                    return 0;  // Case déjà occupée
                }
            }
        }
    }
    
    return 1;  // Placement valide
}
```

**Complexité algorithmique :**
- **O(n)** où n = nombre de cellules occupées dans la pièce
- Maximum 5 cellules par pièce (pièce 1x5)
- Donc complexité constante en pratique : **O(1)** ou **O(5)** au maximum

**Vérifications effectuées :**
1. ✅ Toutes les cellules de la pièce sont dans les limites de la grille
2. ✅ Aucune cellule de la pièce ne chevauche une case déjà occupée
3. ✅ Retourne 1 si valide, 0 sinon

### 2.4 Détection et Suppression des Lignes Complètes

**Fonction dans `game.c` : `place_piece_logic()` (lignes 380-461)**

**Algorithme de détection :**

**Pour les lignes (rows) :**
```c
for (i = 0; i < GRID_H; i++) {
    int full = 1;
    // Vérifie si toutes les cellules de la ligne sont occupées
    for (j = 0; j < GRID_W; j++) {
        if (gs->grid[i][j] == 0) {
            full = 0;  // Ligne incomplète
            break;
        }
    }
    if (full) {
        // Ligne complète détectée
        gs->cleared_rows[gs->num_cleared_rows++] = i;
        // Supprime la ligne
        for (j = 0; j < GRID_W; j++) {
            gs->grid[i][j] = 0;
        }
        gs->score += 100;  // Bonus de 100 points
        lines_cleared++;
    }
}
```

**Pour les colonnes (columns) :**
```c
for (j = 0; j < GRID_W; j++) {
    int full = 1;
    // Vérifie si toutes les cellules de la colonne sont occupées
    for (i = 0; i < GRID_H; i++) {
        if (gs->grid[i][j] == 0) {
            full = 0;  // Colonne incomplète
            break;
        }
    }
    if (full) {
        // Colonne complète détectée
        gs->cleared_cols[gs->num_cleared_cols++] = j;
        // Supprime la colonne
        for (i = 0; i < GRID_H; i++) {
            gs->grid[i][j] = 0;
        }
        gs->score += 100;  // Bonus de 100 points
        lines_cleared++;
    }
}
```

**Caractéristiques :**
- ✅ Parcourt chaque ligne puis chaque colonne
- ✅ Vérifie si toutes les cellules sont occupées (non nulles)
- ✅ Supprime la ligne/colonne en mettant toutes les cellules à 0
- ✅ Enregistre les indices des lignes/colonnes supprimées pour les effets visuels
- ✅ Ajoute 100 points par ligne/colonne complétée
- ✅ Gère les effets visuels (particules, animations)

**Complexité :**
- Pour une grille 10x10 : O(GRID_H + GRID_W) = O(20) = **O(1)** (constante)

---

## 3. GESTION DES ÉTATS ET SCORING

### 3.1 Machine à États pour les Écrans

**Définition dans `globals.h` :**
```c
enum GameScreenState {
    ST_MENU,          // Écran principal
    ST_OPTIONS,       // Configuration
    ST_SOLO,          // Mode solo
    ST_LOGIN,         // Connexion multijoueur
    ST_MULTI_CHOICE,  // Choix multijoueur
    ST_JOIN_INPUT,    // Saisie du code de partie
    ST_LOBBY,         // Salle d'attente
    ST_MULTI_GAME,    // Partie multijoueur
    ST_SERVER_BROWSER,// Liste des serveurs
    ST_SPECTATE       // Mode spectateur
};
```

**Gestion dans `main.c` :**
```c
extern int current_state;  // État actuel

// Dans la boucle principale
switch (current_state) {
    case ST_MENU:
        render_menu();
        break;
    case ST_SOLO:
        render_solo();
        break;
    case ST_MULTI_GAME:
        render_multi_game();
        break;
    // ...
}
```

**Avantages de cette approche :**
- ✅ Code organisé : chaque écran a sa fonction de rendu
- ✅ Transitions claires entre les états
- ✅ Facile d'ajouter de nouveaux écrans
- ✅ Gestion centralisée dans la boucle principale

### 3.2 Calcul du Score

**Système de scoring dans `place_piece_logic()` :**

**Points de base :**
```c
gs->score += 10;  // 10 points par pièce placée
```

**Bonus pour lignes complètes :**
```c
// Pour chaque ligne complétée
gs->score += 100;

// Pour chaque colonne complétée
gs->score += 100;
```

**Exemple de calcul :**
- Placer une pièce : **+10 points**
- Si cette pièce complète 1 ligne : **+100 points** (total : 110)
- Si cette pièce complète 1 ligne ET 1 colonne : **+200 points** (total : 210)
- Si cette pièce complète 2 lignes : **+200 points** (total : 210)

**Note importante :**
- Le code actuel ne semble pas avoir de multiplicateur pour plusieurs lignes simultanées
- Chaque ligne/colonne complétée donne 100 points, peu importe le nombre total
- Un bonus supplémentaire pourrait être ajouté pour les combos (2+ lignes en même temps)

### 3.3 Détection de Fin de Partie

**Fonction `check_valid_moves_exist()` dans `game.c` (lignes 463-483) :**

```c
int check_valid_moves_exist(GameState *gs) {
    int i, r, c;
    
    // Pour chaque pièce disponible
    for (i = 0; i < 3; i++) {
        if (!gs->pieces_available[i]) {
            continue;  // Pièce déjà utilisée
        }
        
        Piece *p = &gs->current_pieces[i];
        
        // Teste toutes les positions possibles dans la grille
        for (r = 0; r < GRID_H; r++) {
            for (c = 0; c < GRID_W; c++) {
                if (can_place(gs, r, c, p)) {
                    return 1;  // Au moins un placement valide trouvé
                }
            }
        }
    }
    
    return 0;  // Aucun placement valide possible
}
```

**Utilisation :**
- Appelée après chaque placement de pièce
- Si retourne 0 → **Game Over**
- Si retourne 1 → Le jeu continue

**Complexité :**
- Pour chaque pièce (3 max) : O(GRID_H × GRID_W × n)
- Où n = nombre de cellules dans la pièce (max 5)
- Total : **O(3 × 10 × 10 × 5) = O(1500)** = constante en pratique
- Peut être optimisé en arrêtant dès qu'un placement valide est trouvé

**Dans le code de jeu :**
```c
// Après placement d'une pièce
if (!check_valid_moves_exist(&game)) {
    game.game_over = 1;  // Fin de partie
}
```

---

## 4. FICHIERS CLÉS À CONNAÎTRE

### 4.1 `client/game.c` : Logique de Jeu

**Fonctions principales :**
- `init_game()` : Initialise une nouvelle partie
- `generate_pieces()` : Génère 3 pièces aléatoires
- `can_place()` : Vérifie si un placement est valide
- `place_piece_logic()` : Place une pièce et gère les lignes complètes
- `check_valid_moves_exist()` : Vérifie s'il reste des coups possibles

**Structures importantes :**
- `GameState` : État complet du jeu (grille, score, pièces, effets)
- `Piece` : Structure d'une pièce (forme, dimensions, couleur)

### 4.2 `client/game.h` : Structures et Constantes

**Définitions importantes :**
- `Piece` : Structure d'une pièce
- `GameState` : État du jeu
- `EffectsManager` : Gestion des effets visuels (particules, animations)

### 4.3 `client/globals.c` : Variables Globales

**Variables principales :**
- `GameState game` : État du jeu actuel
- `int current_state` : État de l'écran actuel
- `int selected_piece_idx` : Pièce sélectionnée par le joueur
- Variables de layout (positions, tailles)

**Fonctions utilitaires :**
- `recalculate_layout()` : Recalcule les positions selon la taille de la fenêtre

### 4.4 `client/main.c` : Boucle Principale

**Structure :**
1. **Initialisation** : SDL, polices, audio, jeu
2. **Boucle principale** :
   - Traitement des événements (clavier, souris)
   - Mise à jour du temps
   - Rendu selon l'état actuel
   - Affichage
3. **Nettoyage** : Fermeture SDL, libération mémoire

**Points clés :**
- Gestion des événements SDL (clics, touches, redimensionnement)
- Appel des fonctions de rendu selon `current_state`
- Gestion du temps (delta_time pour animations fluides)

---

## 5. QUESTIONS ET RÉPONSES

### Q1 : Pourquoi avoir choisi une architecture client-serveur plutôt qu'un jeu local uniquement ?

**Réponse attendue :**
> "Nous avons choisi une architecture client-serveur pour plusieurs raisons :
> 
> 1. **Multijoueur en réseau** : Permet à plusieurs joueurs de jouer ensemble à distance
> 2. **Séparation des responsabilités** : Le client gère l'interface utilisateur et l'affichage, tandis que le serveur gère la synchronisation, les tours de jeu, et la logique multijoueur
> 3. **Évolutivité** : Facile d'ajouter de nouvelles fonctionnalités comme le mode spectateur, les classements en ligne, ou différents modes de jeu
> 4. **Maintenance** : Code organisé et modulaire, chaque partie a sa responsabilité claire
> 5. **Réutilisabilité** : Le code client fonctionne aussi en mode solo sans serveur"

**Détails techniques :**
- Le serveur gère les salles de jeu, les codes de partie, la synchronisation des grilles
- Le client peut fonctionner en mode solo (sans connexion réseau)
- Communication via sockets TCP/IP avec un protocole défini dans `net_protocol.h`

### Q2 : Comment fonctionne la détection des lignes complètes ?

**Réponse attendue :**
> "La détection se fait en deux étapes dans la fonction `place_piece_logic()` :
> 
> 1. **Pour les lignes** : On parcourt chaque ligne de la grille (0 à GRID_H-1). Pour chaque ligne, on vérifie si toutes les cellules sont occupées (non nulles). Si oui, on supprime la ligne en mettant toutes ses cellules à 0, et on ajoute 100 points au score.
> 
> 2. **Pour les colonnes** : Même principe, mais on parcourt les colonnes (0 à GRID_W-1) et on vérifie verticalement.
> 
> La complexité est O(GRID_H + GRID_W) = O(20) pour une grille 10x10, donc constante et très rapide."

**Code de référence :**
- Lignes 398-423 de `game.c` pour les lignes
- Lignes 425-450 de `game.c` pour les colonnes

### Q3 : Quelle est la complexité algorithmique de la vérification de placement d'une pièce ?

**Réponse attendue :**
> "La fonction `can_place()` a une complexité **O(n)** où n est le nombre de cellules occupées dans la pièce. Comme une pièce a au maximum 5 cellules (pièce 1x5), la complexité est en pratique **O(1)** ou **O(5)** au maximum.
> 
> L'algorithme parcourt chaque cellule de la pièce (matrice 5x5, mais seules les cellules occupées sont vérifiées) et pour chacune :
> - Vérifie les limites de la grille
> - Vérifie si la case correspondante dans la grille est libre
> 
> C'est donc très rapide et efficace."

**Code de référence :**
- Lignes 357-378 de `game.c`

### Q4 : Comment gérez-vous le cas où le joueur ne peut plus placer aucune pièce ?

**Réponse attendue :**
> "Nous utilisons la fonction `check_valid_moves_exist()` qui teste systématiquement toutes les combinaisons possibles :
> 
> 1. Pour chaque pièce disponible (3 au maximum)
> 2. Pour chaque position possible dans la grille (10x10 = 100 positions)
> 3. On appelle `can_place()` pour vérifier si le placement est valide
> 
> Si au moins un placement valide est trouvé, la fonction retourne 1 et le jeu continue. Si aucun placement n'est possible, elle retourne 0 et on déclenche le Game Over.
> 
> Cette fonction est appelée après chaque placement de pièce pour détecter immédiatement la fin de partie."

**Code de référence :**
- Lignes 463-483 de `game.c`

**Complexité :**
- O(3 × 10 × 10 × 5) = O(1500) = constante en pratique
- Peut être optimisé en arrêtant dès qu'un placement valide est trouvé

### Q5 : Comment est calculé le score ?

**Réponse attendue :**
> "Le score est calculé dans la fonction `place_piece_logic()` avec deux composantes :
> 
> 1. **Points de base** : 10 points par pièce placée (ajoutés immédiatement après le placement)
> 
> 2. **Bonus lignes multiples** : 100 points par ligne complétée (ligne ou colonne). Si une pièce complète plusieurs lignes/colonnes, on ajoute 100 points pour chacune.
> 
> Exemple : Si je place une pièce qui complète 1 ligne et 1 colonne, j'obtiens 10 + 100 + 100 = 210 points.
> 
> Le score est stocké dans `GameState.score` et mis à jour en temps réel."

**Code de référence :**
- Ligne 396 : `gs->score += 10;` (points de base)
- Ligne 420 : `gs->score += 100;` (bonus ligne)
- Ligne 447 : `gs->score += 100;` (bonus colonne)

**Note :**
- Actuellement, il n'y a pas de multiplicateur pour les combos (plusieurs lignes simultanées)
- Chaque ligne/colonne donne 100 points, indépendamment du nombre total

---

## 6. EXTRATS DE CODE IMPORTANTS

### 6.1 Structure GameState

```c
typedef struct {
    int grid[GRID_H][GRID_W];        // Grille 10x10
    int score;                       // Score actuel
    int game_over;                   // 1 si fin de partie
    Piece current_pieces[3];         // 3 pièces disponibles
    int pieces_available[3];          // Disponibilité de chaque pièce
    EffectsManager effects;          // Effets visuels
    int cleared_rows[GRID_H];        // Indices des lignes supprimées
    int cleared_cols[GRID_W];        // Indices des colonnes supprimées
    int num_cleared_rows;            // Nombre de lignes supprimées
    int num_cleared_cols;            // Nombre de colonnes supprimées
} GameState;
```

### 6.2 Fonction can_place() (complète)

```c
int can_place(GameState *gs, int row, int col, Piece *p) {
    int i, j;
    
    for (i = 0; i < p->h; i++) {
        for (j = 0; j < p->w; j++) {
            if (p->data[i][j]) {
                int gr = row + i;
                int gc = col + j;
                
                if (gr < 0 || gr >= GRID_H || gc < 0 || gc >= GRID_W) {
                    return 0;
                }
                
                if (gs->grid[gr][gc] != 0) {
                    return 0;
                }
            }
        }
    }
    
    return 1;
}
```

### 6.3 Détection de lignes complètes (extrait)

```c
// Détection des lignes complètes
for (i = 0; i < GRID_H; i++) {
    int full = 1;
    for (j = 0; j < GRID_W; j++) {
        if (gs->grid[i][j] == 0) {
            full = 0;
            break;
        }
    }
    if (full) {
        gs->cleared_rows[gs->num_cleared_rows++] = i;
        // Suppression de la ligne
        for (j = 0; j < GRID_W; j++) {
            gs->grid[i][j] = 0;
        }
        gs->score += 100;
        lines_cleared++;
    }
}
```

### 6.4 Vérification de fin de partie

```c
int check_valid_moves_exist(GameState *gs) {
    int i, r, c;
    
    for (i = 0; i < 3; i++) {
        if (!gs->pieces_available[i]) {
            continue;
        }
        
        Piece *p = &gs->current_pieces[i];
        
        for (r = 0; r < GRID_H; r++) {
            for (c = 0; c < GRID_W; c++) {
                if (can_place(gs, r, c, p)) {
                    return 1;
                }
            }
        }
    }
    
    return 0;
}
```

### 6.5 Machine à états (extrait de main.c)

```c
switch (current_state) {
    case ST_MENU:
        render_menu();
        break;
    case ST_SOLO:
        render_solo();
        break;
    case ST_MULTI_GAME:
        render_multi_game();
        break;
    case ST_LOBBY:
        render_lobby();
        break;
    // ...
}
```

---

## 📊 RÉSUMÉ DES POINTS CLÉS

### Architecture
- ✅ Séparation claire client/serveur
- ✅ Code modulaire et organisé
- ✅ Communication réseau via sockets TCP/IP
- ✅ Fichiers communs dans `common/`

### Logique de Jeu
- ✅ Grille 10x10 (100 cellules)
- ✅ 19 formes de pièces différentes
- ✅ Vérification de placement : O(n) où n ≤ 5
- ✅ Détection lignes/colonnes : O(GRID_H + GRID_W) = O(20)

### Scoring et États
- ✅ 10 points par pièce + 100 points par ligne/colonne
- ✅ Machine à états pour les écrans (10 états différents)
- ✅ Détection de fin de partie : test exhaustif de toutes les positions

### Complexités
- `can_place()` : **O(1)** à **O(5)** (constante)
- Détection lignes : **O(20)** (constante)
- Vérification fin de partie : **O(1500)** (constante en pratique)

---

## 🎯 CONSEILS POUR LA PRÉSENTATION

1. **Commencez par l'architecture globale** : Montrez la séparation client/serveur
2. **Expliquez la logique de jeu avec des exemples** : Montrez comment une pièce est placée
3. **Utilisez les extraits de code** : Montrez les fonctions clés (`can_place`, `place_piece_logic`)
4. **Soyez précis sur les complexités** : Montrez que les algorithmes sont efficaces
5. **Préparez des réponses aux questions** : Utilisez les réponses fournies comme base

---

**Bon courage pour votre présentation ! 🚀**
