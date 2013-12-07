#ifndef MEM_SYSTEM_PUU_H
#define MEM_SYSTEM_PUU_H

#include "module.h"

/* Access type */
enum puu_access_kind_t
{
	puu_access_invalid = 0,
	puu_access_write,
	puu_access_evict
};

struct puu_buffer_entry_t
{
    unsigned int addr;
};

struct puu_buffer_t
{
    struct puu_buffer_entry_t *entry;

    struct puu_buffer_t *prev; // Elder node.
    struct puu_buffer_t *next; // Newer node.
};

struct puu_t
{
    unsigned int counter_threshold;
    unsigned int counter;

    struct puu_buffer_t *buffer;
    struct puu_buffer_t *buffer_head; // Eldest node.
    struct puu_buffer_t *buffer_tail; // Newest node.
    struct puu_buffer_t *buffer_pivot;
};

struct puu_t *puu_create(void);
void puu_free(struct puu_t *puu);

long long puu_access(struct puu_t *puu, struct mod_t *mod,
    enum puu_access_kind_t access_kind, unsigned int addr);

void puu_buffer_flush(struct puu_t *puu, struct mod_t *mod);
void puu_buffer_append(struct puu_t *puu, unsigned int addr);
void puu_buffer_append_check(struct puu_t *puu, unsigned int addr);
void puu_buffer_del_head(struct puu_t *puu);

struct mod_t *puu_find_memory_mod(struct puu_t *puu, struct mod_t *top_mod);
#endif // MEM_SYSTEM_PUU_H
