#include "memory.h"
#include "puu.h"

struct puu_t *puu_create(void)
{
    struct puu_t *puu;

    /* Initialize */
    puu = xcalloc(1, sizeof(struct puu_t));

    return puu;
};

void puu_free(struct puu_t *puu)
{
    /* TODO: may have to check if buffer cleared. */
    free(puu);
}
