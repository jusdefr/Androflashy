# NexOS : architecture et état actuel

NexOS est ici un **prototype de noyau réellement amorçable pour PC x86 BIOS**, non une implémentation complète des exigences ambitieuses décrites dans le cahier des charges. L’image fonctionne dans QEMU et sur une machine x86 compatible BIOS/clavier PS/2/VGA texte. Le dépôt inclut un bureau graphique VBE (fenêtres, souris, applications natives) mais pas de compatibilité Windows, de système de fichiers persistant, de pile réseau ni de Store.

## Chemin de démarrage

1. `boot/boot.asm` est un secteur BIOS de 512 octets signé `0xAA55`. Il active A20, lit 100 secteurs en CHS depuis une disquette, charge le noyau à `0x10000`, configure une GDT plate et entre en mode protégé 32 bits.
2. `kernel/entry.asm` efface le BSS, installe la pile initiale et appelle `kmain`. Il fournit aussi les entrées IDT, le syscall `int 0x80`, les primitives CPU et un helper de changement de contexte qui n’est pas encore utilisé par le noyau.
3. `kernel/interrupts.c` installe IDT/PIC, les exceptions 0–31, les IRQ0/IRQ1, et deux appels d’exemple (`SYS_YIELD` no-op, `SYS_TICKS`). Le gate syscall est DPL0 ; aucun programme ring 3 ne peut encore l’appeler.
4. `drivers/pit.c` configure le timer PIT ; `drivers/keyboard.c` reçoit les scancodes PS/2.
5. Le noyau initialise un allocateur statique de 64 KiB. Si un framebuffer linéaire VBE ≥ 640×480 en 32 bpp est présent, `kernel/kernel.c` lance le bureau graphique : thème, souris PS/2, fenêtres. Sinon il retombe sur le shell texte interactif (aucune régression). Le PIT fournit le tick et l’horloge de la barre des tâches.
6. `kernel/desktop.c` gère les fenêtres : z-order et focus par clic ou Alt+Tab, déplacement par la barre de titre, **redimensionnement par le bord droit, le bord bas et le coin bas-droit (poignée en hachures)**, maximisation/restauration par double-clic sur le titre, bornes 320×240 minimum et fermeture hors écran impossible. Les applications redessinent pour la taille courante (le Terminal adapte lignes et colonnes, NexBrowser borne son contenu au-dessus de la barre d’état).

## Périmètre (ne pas confondre démo et compatibilité réelle)

| Fonction | Statut |
| --- | --- |
| Boot BIOS 16 bits + passage protégé 32 bits | Implémenté et vérifié dans QEMU |
| VGA texte, console série, panic | Implémenté |
| Exceptions CPU, PIC, IRQ timer et clavier PS/2 | Implémenté (matériel PC legacy) |
| Allocateur noyau fixe 64 KiB | Implémenté, pas de mémoire virtuelle ni d’isolation |
| Shell noyau interactif sur la pile de démarrage | Pas de changement de contexte actif ; pas de ring 3 ni de processus isolés |
| Shell clavier | help, about, clear, uptime, mem, tasks, reboot, halt |
| Bureau VBE : fenêtres, barre des tâches, menu Démarrer, souris PS/2 | Implémenté (déplacement, redimensionnement borné, maximiser double-clic, Alt+Tab) |
| Applications natives embarquées : NexBrowser hors ligne, Terminal, Paramètres, Sécurité | Implémenté ; rendu texte du noyau, aucun moteur externe |
| Syscalls | ABI interne ring 0 ; ticks et yield no-op, sans tâches utilisateur |
| Pilotes disque, réseau, audio, USB, ACPI, SMP | Non implémentés |
| NexFS, persistance, permissions, utilisateurs, registre | Non implémentés |
| Store, C#, exécution d’applications tierces | Non implémentés |
| Exécution .exe Windows/DLL/MSI/SYS | Non implémentée ; aucun support Windows n’est revendiqué |

Le chargeur limite l’image noyau binaire à 50 KiB, pour tenir dans ses 100 secteurs. Le linker contrôle la plage physique sous la pile de démarrage ; l’image est une disquette brute de 1,44 Mo. L’accès disque ne supporte pour le moment que le chemin CHS de la disquette BIOS.

## Structure

```text
boot/boot.asm          chargeur BIOS x86
kernel/entry.asm       entrée noyau, exceptions, IRQ et syscall
kernel/kernel.c        initialisation du shell noyau
kernel/interrupts.c    IDT/PIC, dispatch exceptions, timer, syscall
kernel/scheduler.c     initialisation de l’ordonnanceur (inactif)
kernel/memory.c        tas statique noyau
kernel/console.c       VGA texte et journaux console/série
kernel/serial.c        UART COM1
kernel/shell.c         shell de diagnostic
kernel/string.c        primitives freestanding
kernel/linker.ld       disposition mémoire
kernel/kernel.h        ABI du noyau
kernel/gfx.c . theme.c framebuffer VBE, fonte 8x16 Latin-1, thème
kernel/desktop.c       bureau : fenêtres, barre, menu Démarrer, curseur
kernel/app_*.c         NexBrowser, Paramètres, Sécurité, Terminal (natifs)
kernel/font8x16.c      fonte bitmap générée depuis un PSF (tools/fontgen.py)
 drivers/               PIT, clavier PS/2 et souris PS/2 (IRQ12)
 tests/                 tests de l’image et du démarrage QEMU
 tools/                 scripts build/test
 docs/                  documentation
 Makefile               compilation GNU Make
```

## Limites de sûreté

Ceci est un prototype expérimental, pas un système à utiliser pour des données sensibles. Le chargeur exécute un binaire fixe, sans ELF dynamique, paging, séparation des privilèges, système de fichiers ou validation cryptographique. Le test QEMU headless confirme le chemin BIOS et les journaux série, pas une compatibilité générale du matériel réel. En cas d’échec, vérifiez QEMU et son périphérique clavier PS/2.
