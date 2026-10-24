# NexOS

**Prototype de noyau x86 BIOS amorçable**, écrit en C freestanding et assembleur x86. Le build crée une image brute BIOS de disquette (1,44 Mo), démarrable sous QEMU ou sur matériel PC compatible legacy BIOS. Ce n’est pas encore un OS de bureau complet : pas d’interface graphique, de navigateur, de stockage persistant ni de compatibilité Windows réelle.

> **Téléphone :** il est normal que l’OS ne démarre pas sur ton téléphone. Le binaire vise le processeur x86 en mode 32 bits et un démarrage PC BIOS ; les téléphones utilisent généralement ARM/UEFI ou Android. Tu peux consulter le code et les commits GitHub depuis mobile, mais pas démarrer cette image dessus. Il faut un PC x86 avec QEMU, ou adapter/reporter le noyau pour ARM et écrire un chargeur de démarrage adapté.

## Fonctionnalités présentes

- Bootloader BIOS 16 bits (512 octets), A20, GDT et passage en mode protégé x86 32 bits.
- Console VGA texte, affichage du logo, journal noyau sur UART COM1 et kernel panic.
- Table IDT, exceptions CPU, PIC 8259 et interruption logicielle `int 0x80`.
- PIT 100 Hz, clavier PS/2 IRQ1 avec Shift/Caps Lock et buffer circulaire.
- Petit allocateur noyau statique (64 KiB) avec fusion des blocs libérés.
- Shell interactif actif en ring 0 et tick PIT ; pas encore de commutation de contexte ni de processus.
- Build debug/release, image raw et tests d’image/démarrage dans QEMU.

Les détails et les fonctions non implémentées sont listés dans [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). L’objectif est un **prototype d’apprentissage**, pas un système sécurisé ou compatible Windows.

## Compilation

Prérequis sur une machine Linux : GCC avec support de compilation freestanding 32 bits, GNU binutils (`ld`, `objcopy`), GNU Make et NASM. Pour exécuter l’image : QEMU avec `qemu-system-i386`.

```sh
make debug        # noyau -O0, symboles de debug
make release      # image optimisée -O2 (valeur par défaut)
make test         # valide la signature, ELF et démarre le vrai noyau dans QEMU
make test-shell   # injecte « about » au clavier QEMU et vérifie la réponse
make clean
```

Le résultat généré est `build/nexos.img`. QEMU doit être installé pour `make test`; on peut aussi construire sans QEMU avec `make release` puis vérifier l’image avec `python3 tests/test_image.py build/nexos.img build/kernel.elf`.

### Démarrer

```sh
make run
```

La fenêtre VGA QEMU est la console du système. Cliquer dans la fenêtre pour capturer le clavier. Quitter l’émulateur avec `Ctrl+C` dans le terminal (QEMU est lancé avec sa fenêtre graphique).

Sur une plateforme avec QEMU sans affichage, il est aussi possible de lancer directement :

```sh
qemu-system-i386 -machine pc -m 32M \
  -drive file=build/nexos.img,format=raw,if=floppy -boot a \
  -serial stdio -monitor none -no-reboot
```

> **Précaution :** ce noyau est expérimental. Ne le démarre pas sur une machine contenant des données ou pilotes essentiels ; préfère l’émulateur.

## Shell

- `help` — aide
- `about` — version/architecture
- `clear` — effacer l’écran
- `uptime` — temps écoulé
- `mem` — vérifier l’allocation et la libération du tas
- `tasks` — état du timer et du registre de tâches (prototype)
- `reboot` — redémarrage matériel
- `halt` — arrêter le processeur

Le shell ne prétend pas implémenter un terminal POSIX/CMD ; aucun système de fichiers n’est encore présent.

## Architecture du dépôt

```text
boot/boot.asm           Boot BIOS PC x86
kernel/                 Entrée, noyau C, mémoire, interruptions, scheduler, shell
 drivers/               Timer PIT et clavier PS/2
 tests/                  Validation image et test boot QEMU
 tools/                  Raccourcis build/tests
 docs/                   Architecture, état et limites
 Makefile                Compilation Debug / Release, image brute et tests
```

## Applications, .exe, DLL et MSI

Aucune application utilisateur, GUI ou machine virtuelle Windows n’est incluse dans cette version. En particulier, `.exe`, `.dll`, `.msi`, `.sys`, `.bat`, `.lnk`, NexFS, C#, Store, réseau, audio, navigateur, et gestionnaire de fenêtres sont **non implémentés** ; ce README ne fournit pas d’exemples factices. C’est une base de noyau amorçable à étendre.

Pour la suite détaillée des couches nécessaires, voir [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).
