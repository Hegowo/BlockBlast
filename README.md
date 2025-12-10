# 🎮 BlockBlast V4.0

Un jeu de puzzle de placement de blocs écrit en C99 avec SDL 1.2, avec mode solo et multijoueur en ligne.

![Version](https://img.shields.io/badge/version-4.0-blue)
![Language](https://img.shields.io/badge/language-C99-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey)

## ✨ Fonctionnalités

### Modes de jeu
- **Mode Solo** : Puzzle classique avec sauvegarde automatique
- **Mode Classique** : Multijoueur tour par tour sur une grille partagée
- **Mode Rush** : Multijoueur chronométré où chaque joueur a sa propre grille

### Multijoueur
- Système de salons avec codes à 4 caractères
- Liste des serveurs publics (browser)
- Mode spectateur
- Leaderboard persistant
- Jusqu'à 4 joueurs par salon

### Audio & Visuel
- Interface cyberpunk/néon avec animations
- Effets sonores (placement, clear, game over)
- Musique de fond
- Volumes ajustables (musique et SFX)

### Technique
- **Build Standalone** : Tous les assets embarqués dans l'exécutable
- Sauvegarde/chargement automatique des parties
- Cross-platform : Windows (MinGW) et Linux (GCC)
- Fenêtre redimensionnable

---

## 📋 Prérequis

### Windows (MSYS2/MinGW64)

```bash
pacman -S mingw-w64-x86_64-SDL mingw-w64-x86_64-SDL_ttf mingw-w64-x86_64-SDL_mixer mingw-w64-x86_64-SDL_image mingw-w64-x86_64-gcc
```

### Linux (Debian/Ubuntu)

```bash
sudo apt-get install libsdl1.2-dev libsdl-ttf2.0-dev libsdl-mixer1.2-dev libsdl-image1.2-dev gcc
```

---

## 🔧 Compilation

### Build Standard (assets externes)

```powershell
# Windows PowerShell
.\build.ps1
```

```bash
# Linux
./build.sh
```

### Build Standalone (assets embarqués) ⭐

```powershell
# Crée un exécutable autonome avec tous les assets intégrés
.\build.ps1 -Embedded
```

Cette option :
1. Convertit tous les assets (fonts, sons) en code C
2. Compile le tout dans un seul exécutable
3. **Pas besoin du dossier `assets/`** pour distribuer !

### Options de build

| Option | Description |
|--------|-------------|
| `-Embedded` | Embarque tous les assets dans l'exe |
| `-Clean` | Nettoie le dossier bin/ avant compilation |

---

## 📁 Structure du Projet

```
BlockBlast/
├── 📁 assets/
│   ├── font.ttf                 # Police par défaut
│   ├── orbitron.ttf             # Police néon (Google Fonts)
│   └── 📁 sounds/
│       ├── click.wav            # Son de clic
│       ├── place.wav            # Son de placement
│       ├── clear.wav            # Son de ligne complétée
│       ├── gameover.wav         # Son de fin de partie
│       └── music.wav            # Musique de fond
│
├── 📁 client/
│   ├── main.c                   # Point d'entrée, boucle principale
│   ├── globals.c/h              # Variables globales
│   ├── game.c/h                 # Logique de jeu (grille, pièces)
│   ├── graphics.c/h             # Rendu graphique de base
│   ├── ui_components.c/h        # Composants UI (boutons, sliders)
│   ├── screens.c/h              # Écrans (menu, lobby, jeu...)
│   ├── input_handlers.c/h       # Gestion des entrées
│   ├── audio.c/h                # Système audio
│   ├── save_system.c/h          # Sauvegarde/chargement
│   ├── net_client.c/h           # Client réseau
│   └── embedded_assets.c/h      # Assets embarqués (généré)
│
├── 📁 server/
│   └── server_main.c            # Serveur de jeu
│
├── 📁 common/
│   ├── config.h                 # Constantes partagées
│   └── net_protocol.h           # Protocole réseau
│
├── 📁 tools/
│   └── bin2c.c                  # Outil de conversion assets→C
│
├── 📁 bin/                      # Exécutables compilés
│
├── build.ps1                    # Script de build Windows
├── build.sh                     # Script de build Linux
├── embed_assets.ps1             # Générateur d'assets embarqués
└── README.md
```

---

## 🚀 Utilisation

### Lancer le Serveur

```bash
./bin/blockblast_server
```

Le serveur affiche automatiquement :
- Le port d'écoute (défaut: 5000)
- Toutes les adresses IP locales disponibles
- Les instructions pour rejoindre

```
========================================
     BLOCKBLAST SERVER STARTED
========================================

  Port: 5000

  Adresses IP disponibles:
    - 192.168.1.180
    - ...

  Pour rejoindre, les joueurs doivent:
    1. Lancer BlockBlast
    2. Entrer l'IP du serveur
    3. Entrer le port: 5000
========================================
```

### Lancer le Client

```bash
./bin/blockblast
```

---

## 🎮 Contrôles

| Action | Contrôle |
|--------|----------|
| Sélectionner/Placer | Clic gauche |
| Expulser joueur (hôte) | Clic droit |
| Paramètres | Touche `P` |
| Menu pause / Retour | Touche `Échap` |
| Navigation spectateur | Flèches ← → |

---

## 🎯 Gameplay

### Mode Solo

1. Cliquez "SOLO" depuis le menu principal
2. Glissez les pièces du bas vers la grille
3. Complétez des lignes/colonnes pour les effacer
4. Le jeu se sauvegarde automatiquement
5. Partie terminée quand aucun coup n'est possible

### Mode Multijoueur Classique

1. Connectez-vous au serveur (Options → IP/Port)
2. Créez un salon ou rejoignez avec un code
3. L'hôte démarre quand 2+ joueurs sont présents
4. Placez vos pièces à tour de rôle sur la grille commune

### Mode Rush

1. Choisissez le mode "Rush" dans le lobby
2. Définissez la durée (1-5 minutes)
3. Chaque joueur a sa propre grille
4. Celui avec le plus de points à la fin gagne !

---

## ⚙️ Configuration

### Options du jeu (touche P)

- **Volume Musique** : 0-100%
- **Volume SFX** : 0-100%
- **IP Serveur** : Adresse du serveur multijoueur
- **Port** : Port du serveur (défaut: 5000)

### Paramètres du Lobby (hôte)

- **Mode de jeu** : Classique / Rush
- **Durée** (Rush) : 1, 2, 3, 4, ou 5 minutes
- **Visibilité** : Public / Privé

---

## 📦 Distribution

### Build Standard

Distribuez ces fichiers :
```
📁 BlockBlast/
├── blockblast.exe
├── blockblast_server.exe
├── SDL.dll
├── SDL_ttf.dll
├── SDL_mixer.dll
└── 📁 assets/
    └── (tous les fichiers)
```

### Build Standalone (-Embedded) ⭐

Distribuez uniquement :
```
📁 BlockBlast/
├── blockblast.exe       (9+ MB, contient tout)
├── blockblast_server.exe
├── SDL.dll
├── SDL_ttf.dll
└── SDL_mixer.dll
```

Les DLLs SDL se trouvent dans : `C:\msys64\mingw64\bin\`

---

## 🧩 Types de Pièces

| Forme | Taille | Couleur |
|-------|--------|---------|
| Bloc simple | 1×1 | Rouge |
| Barre horizontale | 2×1 | Vert |
| Barre longue | 3×1 | Bleu |
| Grande barre | 4×1 | Cyan |
| Grande barre | 5×1 | Orange |
| Carré | 2×2 | Jaune |
| Grand carré | 3×3 | Rose |
| Forme en L | Variable | Magenta |
| Forme en T | Variable | Violet |

---

## 📊 Scoring

| Action | Points |
|--------|--------|
| Placer une pièce | +10 |
| Effacer une ligne | +100 |
| Effacer une colonne | +100 |
| Combo (plusieurs lignes) | Bonus multiplicateur |

---

## 🔧 Détails Techniques

- **Langage** : C99
- **Graphiques** : SDL 1.2 + SDL_ttf + SDL_mixer + SDL_image
- **Réseau** : Sockets TCP avec protocole binaire
- **Grille** : 10×10 cellules
- **Fenêtre** : 540×960 (redimensionnable)
- **Port serveur** : 5000 (configurable dans config.h)

---

## 🐛 Dépannage

### Le jeu ne se lance pas

1. Vérifiez que les DLLs SDL sont présentes
2. En mode standard, vérifiez le dossier `assets/`
3. Lancez depuis un terminal pour voir les erreurs

### Impossible de rejoindre le serveur

1. Vérifiez l'IP et le port dans les options
2. Le serveur doit être lancé en premier
3. Vérifiez votre pare-feu
4. Utilisez l'IP locale (192.168.x.x) pour le réseau local

### Pas de son

1. Vérifiez les volumes dans les paramètres (touche P)
2. Vérifiez que les fichiers `.wav` sont présents (mode standard)

---

## 📝 Licence

Ce projet est fourni à des fins éducatives.

---

## 🙏 Crédits

- **SDL** : Simple DirectMedia Layer
- **Police Orbitron** : Google Fonts
- Développé avec ❤️ en C99
