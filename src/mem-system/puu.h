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
	void *data;
};

struct puu_buffer_node_t
{
	struct puu_buffer_node_t *prev; /* Elder */
	struct puu_buffer_node_t *next; /* Younger */
	struct puu_buffer_entry_t *entry;
};

struct puu_t
{
	unsigned int counter;
	unsigned int threshold;
	unsigned int current_buffer;

    /* buffer1 and buffer2 always point to head */
	struct puu_buffer_node_t *buffer1;
	struct puu_buffer_node_t *buffer2;
	struct puu_buffer_node_t *buffer1_head;
	struct puu_buffer_node_t *buffer1_tail;
	struct puu_buffer_node_t *buffer2_head;
	struct puu_buffer_node_t *buffer2_tail;
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
