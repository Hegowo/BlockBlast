# 🎮 BlockBlast - C & SDL 1.2

![Status](https://img.shields.io/badge/Status-Functional-brightgreen)
![Language](https://img.shields.io/badge/Language-C99-blue)
![Library](https://img.shields.io/badge/Library-SDL%201.2-orange)

**BlockBlast** est un jeu de puzzle stratégique codé entièrement en **C (C99)** utilisant la bibliothèque graphique **SDL 1.2**. Le projet intègre un mode solo classique et un mode multijoueur en ligne complet (architecture Client-Serveur TCP) permettant de jouer au tour par tour sur une grille partagée.

---

## ✨ Fonctionnalités

### 🕹️ Gameplay
- **Mode Solo :** Placez les blocs, complétez des lignes/colonnes pour marquer des points. Game Over si plus aucun placement n'est possible.
- **Mode Multijoueur En Ligne :**
    - **Système de Lobby :** Créez une salle privée ou rejoignez-en une via un **Code unique** (ex: `A4F2`).
    - **Tour par Tour Strict :** Chaque joueur joue un coup, puis c'est au suivant.
    - **Grille Partagée :** Tous les joueurs interagissent sur la même grille en temps réel.
    - **Leaderboard :** Classement des 5 meilleurs scores (persistant côté serveur).
    - **Chat / Info :** Système de popup pour les exclusions ou annulations de partie.

### 🛠️ Technique
- **Cross-Platform :** Code compatible Windows (MinGW/Winsock) et Linux (GCC/BSD Sockets).
- **Interface UI Custom :** Boutons, champs de saisie texte, rendus graphiques "style mobile" faits main.
- **Saisie Avancée :** Support complet des claviers (AZERTY/QWERTY), Majuscules, Pavé numérique via Unicode.
- **Réseau Robuste :** Protocole TCP custom avec alignement mémoire strict (`#pragma pack`) pour éviter les désynchronisations.

---

## 📂 Structure du Projet

```text
BlockBlast/
├── assets/              # Contient la police d'écriture (font.ttf)
├── client/              # Code source du Jeu (Client)
│   ├── main.c           # Point d'entrée, boucle principale, gestion des états
│   ├── game.c/.h        # Logique pure du jeu (grille, pièces)
│   ├── net_client.c/.h  # Gestion des sockets client
│   └── ui.c/.h          # (Intégré au main dans la version actuelle)
├── server/              # Code source du Serveur
│   └── server_main.c    # Logique serveur, gestion des salles et clients
├── common/              # Fichiers partagés
│   ├── config.h         # Constantes globales
│   └── net_protocol.h   # Définition des paquets réseaux
├── Makefile             # Script de compilation automatique
└── build.sh             # Script de compilation rapide (Bash)
````

-----

## 🚀 Installation et Compilation

### Prérequis

  - **Windows :** MSYS2 / MinGW64 installé.
  - **Linux :** GCC et les paquets de développement SDL (`libsdl1.2-dev`, `libsdl-ttf2.0-dev`).

### 1\. Compilation sous Windows (Git Bash / MSYS2)

Le plus simple est d'utiliser le script fourni ou la commande directe.

**Commande manuelle (Client) :**

```bash
gcc -std=c99 -I"C:/msys64/mingw64/include/SDL" -L"C:/msys64/mingw64/lib" client/main.c client/game.c client/net_client.c -o blockblast.exe -lmingw32 -lSDLmain -lSDL -lSDL_ttf -lws2_32 -mwindows
```

**Commande manuelle (Serveur) :**

```bash
gcc -std=c99 server/server_main.c -o blockblast_server.exe -lws2_32
```

⚠️ **Important :** Assurez-vous que les fichiers DLL (`SDL.dll`, `SDL_ttf.dll`, etc.) sont présents dans le même dossier que `blockblast.exe` pour lancer le jeu.

### 2\. Compilation sous Linux (VPS / Desktop)

Utilisez le `Makefile` ou les commandes suivantes :

```bash
# Client
gcc -std=c99 client/main.c client/game.c client/net_client.c -o blockblast -lSDL -lSDL_ttf

# Serveur (Pour VPS)
gcc -std=c99 server/server_main.c -o blockblast_server
```

-----

## 🌐 Déploiement du Serveur (VPS)

Pour jouer en ligne avec des amis, le serveur doit tourner sur une machine accessible (VPS).

1.  **Uploader** le dossier `server/` et `common/` sur votre VPS.
2.  **Compiler** le serveur (voir commande Linux ci-dessus).
3.  **Ouvrir le port 5000** dans le pare-feu :
    ```bash
    sudo ufw allow 5000/tcp
    ```
4.  **Lancer le serveur** :
    ```bash
    ./blockblast_server
    ```
5.  **Configurer le Client** :
    Modifiez la variable `online_ip` dans `client/main.c` avec l'IP de votre VPS, puis recompilez le jeu.

-----

## 🎮 Comment Jouer ?

### Contrôles

  - **Souris (Clic Gauche) :** Naviguer dans les menus, glisser-déposer les pièces sur la grille.
  - **Clic Droit (Lobby) :** Expulser un joueur (si vous êtes l'hôte).
  - **Clavier :** Saisir son pseudo et le code de la salle.

### Déroulement d'une partie en ligne

1.  Cliquez sur **Multi Online**.
2.  Entrez votre **Pseudo**.
3.  **Créer** une partie (vous recevez un code, ex: `XY98`) ou **Rejoindre** avec un code.
4.  Attendez les autres joueurs dans le Lobby.
5.  L'hôte clique sur **Lancer**.
6.  Le nom du joueur dont c'est le tour s'affiche en haut.
7.  Posez une pièce pour passer la main \!

-----

## 🐛 Dépannage

  - **Erreur "SDL.dll introuvable" :** Copiez les DLLs de `C:\msys64\mingw64\bin` vers le dossier du jeu ou ajoutez le chemin au PATH Windows.
  - **Caractères bizarres :** Le jeu utilise l'encodage UTF-8 pour les accents. Assurez-vous que votre police `assets/font.ttf` supporte les caractères accentués.
  - **Permission Denied à la compilation :** Le jeu ou le serveur est déjà lancé. Fermez-le avant de recompiler.

-----

## 📜 Crédits

Développé en C / SDL 1.2.
Concept inspiré du jeu mobile BlockBlast.
Réfléchi et développé par : Arthur KETCHEIAN, Walid HAMMOUTI, Nathan GIRAUD

```
```
