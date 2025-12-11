# REPARTITION DE LA SOUTENANCE - BlockBlast

## Informations generales

| Information | Details |
|-------------|---------|
| **Projet** | BlockBlast - Jeu de puzzle multijoueur |
| **Equipe** | Arthur, Walid, Nathan |
| **Duree totale** | 15 minutes |
| **Format** | 3 parties de ~5 minutes chacune |

---

## Tableau de repartition

| Membre | Theme | Duree | Fichiers principaux |
|--------|-------|-------|---------------------|
| Arthur | Architecture generale et logique de jeu | ~10 min | `game.c`, `game.h`, `globals.c`, `main.c` |
| Walid | Interface graphique et experience utilisateur | ~10 min | `graphics.c`, `audio.c`, `screens.c`, `ui_components.c` |
| Nathan | Reseau et persistance des donnees | ~10 min | `server_main.c`, `net_client.c`, `save_system.c` |

---

## Partie 1 - Arthur (~10 min)

### Theme : Architecture generale et logique de jeu

#### Points a aborder

1. **Presentation du projet**
   - Concept du jeu BlockBlast (inspire de Block Blast/Tetris)
   - Objectif : placer des pieces sur une grille 8x8 et completer des lignes
   - Modes de jeu : Solo classique, Solo temps limite, Multijoueur

2. **Architecture modulaire**
   - Separation claire client/serveur
   - Organisation des fichiers sources :
     - `client/` : tout le code cote joueur
     - `server/` : serveur multijoueur independant
   - Avantages de cette architecture (maintenabilite, reutilisabilite)

3. **Logique de jeu**
   - Structure de la grille 8x8 (`int grid[GRID_SIZE][GRID_SIZE]`)
   - Systeme de pieces : 19 formes differentes definies dans `PIECES[]`
   - Detection de placement valide (`can_place_piece`)
   - Detection et suppression des lignes completes (`check_and_clear_lines`)
   - Calcul du score (points par cellule + bonus lignes multiples)

4. **Gestion des etats**
   - Machine a etats pour les ecrans (`GameScreenState`)
   - Structure `GameState` pour l'etat de la partie
   - Cycle de jeu : selection piece -> placement -> verification -> score

#### Fichiers a connaitre
- `client/game.c` : logique de jeu
- `client/game.h` : structures et constantes
- `client/globals.c` : variables globales
- `client/main.c` : boucle principale

---

## Partie 2 - Walid (~10 min)

### Theme : Interface graphique et experience utilisateur

#### Points a aborder

1. **Presentation du projet et SDL 1.2**
   - Presentation rapide du jeu BlockBlast
   - Pourquoi SDL 1.2 (simplicite, documentation, compatibilite)
   - Initialisation de la fenetre (800x600)

2. **Systeme graphique**
   - Rendu de la grille avec effets neon/cyberpunk
   - Dessin des pieces avec couleurs distinctes
   - Police Orbitron pour le style futuriste

3. **Systeme audio**
   - SDL_mixer pour la gestion du son
   - Musique de fond et effets sonores

4. **Build standalone**
   - Integration des assets dans l'executable
   - Outil `bin2c` pour convertir les fichiers binaires

#### Fichiers a connaitre
- `client/graphics.c` : fonctions de dessin
- `client/audio.c` : gestion du son
- `client/screens.c` : rendu des ecrans
- `client/ui_components.c` : composants UI

---

## Partie 3 - Nathan (~10 min)

### Theme : Reseau et persistance des donnees

#### Points a aborder

1. **Architecture client-serveur**
   - Protocole TCP/IP (fiabilite des donnees)
   - Serveur central qui gere plusieurs clients

2. **Protocole de communication**
   - Structure des paquets (`Packet`) avec type et donnees
   - Types de messages : `MSG_JOIN`, `MSG_CREATE_ROOM`, `MSG_UPDATE_GRID`, etc.

3. **Gestion des salons multijoueur**
   - Creation et rejoindre des salons
   - Modes de jeu : classique et temps limite
   - Systeme de spectateurs

4. **Persistance des donnees**
   - Sauvegarde locale chiffree (`data.arthur`)
   - Leaderboard serveur chiffre (`leaderboard.arthur`)
   - Algorithme de chiffrement XOR avec checksum

#### Fichiers a connaitre
- `server/server_main.c` : serveur complet
- `client/net_client.c` : client reseau
- `client/save_system.c` : sauvegarde chiffree

---

## Partie 3 - Arthur (~5 min)

### Theme : Architecture generale et logique de jeu

#### Points a aborder

1. **Architecture modulaire**
   - Separation claire client/serveur
   - Organisation des fichiers sources
   - Avantages de cette architecture

2. **Logique de jeu**
   - Structure de la grille 8x8
   - Systeme de pieces : 19 formes differentes
   - Detection de placement valide (`can_place_piece`)
   - Detection et suppression des lignes completes

3. **Gestion des etats et scoring**
   - Machine a etats pour les ecrans (`GameScreenState`)
   - Calcul du score (points + bonus lignes multiples)
   - Detection de fin de partie

#### Fichiers a connaitre
- `client/game.c` : logique de jeu
- `client/game.h` : structures et constantes
- `client/globals.c` : variables globales
- `client/main.c` : boucle principale

---

## Questions potentielles du professeur

### Questions pour Arthur (Architecture et logique)

1. **Pourquoi avoir choisi une architecture client-serveur plutot qu'un jeu local uniquement ?**
   - Reponse attendue : Permet le multijoueur en reseau, separation des responsabilites, evolutivite

2. **Comment fonctionne la detection des lignes completes ?**
   - Reponse attendue : Parcours de chaque ligne/colonne, verification si toutes les cellules sont occupees, suppression et decalage

3. **Quelle est la complexite algorithmique de la verification de placement d'une piece ?**
   - Reponse attendue : O(n) ou n est le nombre de cellules de la piece (maximum 5)

4. **Comment gerez-vous le cas ou le joueur ne peut plus placer aucune piece ?**
   - Reponse attendue : Fonction `check_valid_moves_exist` qui teste toutes les pieces sur toutes les positions

5. **Pourquoi utiliser une grille de taille fixe 8x8 ?**
   - Reponse attendue : Standard du jeu BlockBlast original, equilibre entre complexite et jouabilite

6. **Comment est calcule le score ?**
   - Reponse attendue : Points par cellule placee + bonus multiplicateur pour lignes multiples

### Questions pour Walid (Graphique et UX)

1. **Pourquoi avoir choisi SDL 1.2 plutot que SDL 2 ou une autre bibliotheque ?**
   - Reponse attendue : Simplicite, bonne documentation, suffisant pour un jeu 2D, compatibilite

2. **Comment avez-vous implemente les effets de lueur neon ?**
   - Reponse attendue : Plusieurs rectangles concentriques avec des couleurs de plus en plus sombres/transparentes

3. **Comment fonctionne le systeme d'integration des assets dans l'executable ?**
   - Reponse attendue : Conversion des fichiers binaires en tableaux de bytes C, chargement depuis la memoire avec SDL_RWops

4. **Quelle est la difference entre SDL_mixer et SDL_audio ?**
   - Reponse attendue : SDL_mixer est une surcouche qui facilite la gestion de plusieurs canaux audio et formats

5. **Comment est gere le rafraichissement de l'ecran ?**
   - Reponse attendue : Double buffering avec SDL_Flip, redessin complet a chaque frame

### Questions pour Arthur (Reseau et donnees)

6. **Pourquoi TCP plutot qu'UDP pour le multijoueur ?**
   - Reponse attendue : Fiabilite des donnees, ordre garanti, pas de gestion de perte de paquets a implementer

7. **Comment est structure un paquet reseau dans votre protocole ?**
   - Reponse attendue : Structure Packet avec type (int), pseudo, donnees specifiques au type

8. **Comment gerez-vous la deconnexion d'un joueur en cours de partie ?**
   - Reponse attendue : Detection via erreur de socket, notification aux autres joueurs, nettoyage de la room

9. **Quel algorithme de chiffrement utilisez-vous pour les sauvegardes ?**
   - Reponse attendue : XOR avec une cle secrete + checksum pour verification d'integrite

10. **Pourquoi stocker le leaderboard sur le serveur plutot que sur les clients ?**
    - Reponse attendue : Eviter la triche, centraliser les scores, coherence des donnees

### Questions pour Arthur (Architecture et logique)

11. **Pourquoi avoir choisi une architecture client-serveur plutot qu'un jeu local uniquement ?**
    - Reponse attendue : Permet le multijoueur en reseau, separation des responsabilites, evolutivite

12. **Comment fonctionne la detection des lignes completes ?**
    - Reponse attendue : Parcours de chaque ligne/colonne, verification si toutes les cellules sont occupees, suppression

13. **Quelle est la complexite algorithmique de la verification de placement d'une piece ?**
    - Reponse attendue : O(n) ou n est le nombre de cellules de la piece (maximum 5)

14. **Comment gerez-vous le cas ou le joueur ne peut plus placer aucune piece ?**
    - Reponse attendue : Fonction `check_valid_moves_exist` qui teste toutes les pieces sur toutes les positions

15. **Comment est calcule le score ?**
    - Reponse attendue : Points par cellule placee + bonus multiplicateur pour lignes multiples

### Questions generales (pour tous)

16. **Quelles ont ete les principales difficultes rencontrees ?**
    - Reponse libre selon l'experience de chacun

17. **Comment avez-vous organise le travail en equipe ?**
    - Reponse attendue : Repartition par modules, Git pour le versioning, communication reguliere

18. **Si vous deviez refaire le projet, que changeriez-vous ?**
    - Reponse libre, montre la reflexion critique

19. **Quelles ameliorations pourriez-vous apporter au jeu ?**
    - Reponses possibles : Plus de modes de jeu, meilleure IA, animations, compatibilite mobile

---

## Conseils pour la soutenance

### Avant la presentation
- [ ] Tester le jeu sur la machine de presentation
- [ ] Verifier que le serveur demarre correctement
- [ ] Preparer une partie en cours pour la demo
- [ ] Avoir le code source pret a montrer

### Pendant la presentation
- [ ] Parler clairement et pas trop vite
- [ ] Aller a l'essentiel (5 min c'est court !)
- [ ] Montrer du code concret pour illustrer
- [ ] Rester dans le temps imparti

### Pour les questions
- [ ] Ne pas paniquer si on ne sait pas repondre
- [ ] Etre honnete sur ce qu'on ne connait pas
- [ ] S'entraider entre membres de l'equipe

---

## Transitions entre les parties

**Walid -> Nathan** : "Voila pour la partie graphique et audio. Nathan va maintenant vous expliquer comment les joueurs peuvent jouer ensemble en reseau."

**Nathan -> Arthur** : "Maintenant que vous avez vu le cote reseau, Arthur va vous presenter l'architecture generale et la logique de jeu."

---

## Timing indicatif

| Temps | Action |
|-------|--------|
| 0:00 - 0:30 | Introduction generale (Arthur) |
| 0:30 - 10:00 | Partie 1 - Arthur |
| 10:00 - 10:30 | Transition + Demo rapide |
| 10:30 - 20:00 | Partie 2 - Walid |
| 20:00 - 20:30 | Transition |
| 20:30 - 30:00 | Partie 3 - Nathan |
| 30:00+ | Questions du professeur |

