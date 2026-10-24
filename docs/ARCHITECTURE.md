# NexOS : architecture et état actuel

NexOS est ici un **prototype de noyau réellement amorçable pour PC x86 BIOS**, non une implémentation complète des exigences ambitieuses décrites dans le cahier des charges. L’image fonctionne dans QEMU et sur une machine x86 compatible BIOS/clavier PS/2/VGA texte. Le dépôt n’inclut pas encore une interface graphique, un navigateur, une compatibilité Windows réelle, un système de fichiers persistant ni un Store.

## Chemin de démarrage

1. `boot/boot.asm` est un secteur BIOS de 512 octets signé `0xAA55`. Il active A20, lit 100 secteurs en CHS depuis une disquette, charge le noyau à `0x10000`, configure une GDT plate et entre en mode protégé 32 bits.
2. `kernel/entry.asm` efface le BSS, installe la pile initiale et appelle `kmain`. Il fournit aussi les entrées IDT, le syscall `int 0x80`, les primitives CPU et un helper de changement de contexte qui n’est pas encore utilisé par le noyau.
3. `kernel/interrupts.c` installe IDT/PIC, les exceptions 0–31, les IRQ0/IRQ1, et deux appels d’exemple (`SYS_YIELD` no-op, `SYS_TICKS`). Le gate syscall est DPL0 ; aucun programme ring 3 ne peut encore l’appeler.
4. `drivers/pit.c` configure le timer PIT ; `drivers/keyboard.c` reçoit les scancodes PS/2.
5. Le noyau initialise un allocateur statique de 64 KiB et lance un shell noyau interactif ; le PIT fournit un tick, sans ordonnanceur multiprocessus actif.

## Périmètre (ne pas confondre démo et compatibilité réelle)

| Fonction | Statut |
| --- | --- |
| Boot BIOS 16 bits + passage protégé 32 bits | Implémenté et vérifié dans QEMU |
| VGA texte, console série, panic | Implémenté |
| Exceptions CPU, PIC, IRQ timer et clavier PS/2 | Implémenté (matériel PC legacy) |
| Allocateur noyau fixe 64 KiB | Implémenté, pas de mémoire virtuelle ni d’isolation |
| Shell noyau interactif sur la pile de démarrage | Pas de changement de contexte actif ; pas de ring 3 ni de processus isolés |
| Shell clavier | help, about, clear, uptime, mem, tasks, reboot, halt |
| Syscalls | ABI interne ring 0 ; ticks et yield no-op, sans tâches utilisateur |
| Pilotes disque, réseau, audio, USB, ACPI, SMP | Non implémentés |
| NexFS, persistance, permissions, utilisateurs, registre | Non implémentés |
| Fenêtres graphiques, applications, browser, Store, C# | Non implémentés |
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
 drivers/               PIT et clavier PS/2
 tests/                 tests de l’image et du démarrage QEMU
 tools/                 scripts build/test
 docs/                  documentation
 Makefile               compilation GNU Make
```

## Limites de sûreté

Ceci est un prototype expérimental, pas un système à utiliser pour des données sensibles. Le chargeur exécute un binaire fixe, sans ELF dynamique, paging, séparation des privilèges, système de fichiers ou validation cryptographique. Le test QEMU headless confirme le chemin BIOS et les journaux série, pas une compatibilité générale du matériel réel. En cas d’échec, vérifiez QEMU et son périphérique clavier PS/2.
