#include "kernel.h"

/* Un seul contexte ring 0 est actif dans ce prototype. Le PIT fournit un
   compteur global mais ne commute aucun contexte ; cette initialisation garde
   une interface explicite pour un futur scheduler validé. */
void scheduler_init(void) {
    klog("INFO", "scheduler multi-taches non active dans cette version");
}
