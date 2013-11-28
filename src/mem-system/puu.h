#ifndef MEM_SYSTEM_PUU_H
#define MEM_SYSTEM_PUU_H

struct puu_buffer_entry_t
{
    unsigned int addr;
    /* TODO: data, what is the type? */
};

struct puu_buffer_t
{
    struct puu_buffer_entry_t *entry;

    struct puu_buffer_t *prev;
    struct puu_buffer_t *next;
};

struct puu_t
{
    unsigned int counter_threshold;
    unsigned int counter;

    struct puu_buffer_t *buffer;
    struct puu_buffer_t *buffer_head;
    struct puu_buffer_t *buffer_tail;
    struct puu_buffer_t *buffer_pivot;
};

struct puu_t *puu_create(void);
void puu_free(struct puu_t *puu);

#endif // MEM_SYSTEM_PUU_H
