#include <lib/mhandle/mhandle.h>
#include <lib/esim/esim.h>
#include "memory.h"
#include "puu.h"
#include "mod-stack.h"
#include "local-mem-protocol.h"


struct puu_t *puu_create(void)
{
    struct puu_t *puu;

    /* Initialize */
    //puu = (struct puu_t)xcalloc(1,sizeof(struct puu_t));
    // do not need to make conversion. Ref:	list = xcalloc(1, sizeof(struct list_t)); (src/lib/util/list.c line:99)
    puu = xcalloc(1, sizeof(struct puu_t));

    puu->counter = 0;
    puu->counter_threshold = 20; // !!!MAGIC NUMBER HERE!!!
    puu->buffer = NULL;
    puu->buffer_head = NULL;
    puu->buffer_tail = NULL;
    puu->buffer_pivot = NULL;

    return puu;
};

void puu_free(struct puu_t *puu)
{
    /* TODO: may have to check if buffer cleared. */
    free(puu);
}

long long puu_access(struct puu_t *puu, struct mod_t *mod,
    enum puu_access_kind_t access_kind, unsigned int addr, int *witness_ptr,
    struct linked_list_t *event_queue, void *event_queue_item,
    struct mod_client_info_t *client_info)
{
	struct mod_stack_t *stack;
	int event;
	unsigned int addr_from_buf;

    if (access_kind == puu_access_write)
    {
        puu_buffer_append_check(puu, addr);

        if (puu->counter == puu->counter_threshold)
        {
            puu_buffer_flush(puu, mod, witness_ptr, event_queue,
                event_queue_item, client_info);
        }
    }
    else if (access_kind == puu_access_evict)
    {
        // TODO: check and eliminate duplicate entries in buffer.
        puu_buffer_flush(puu, mod, witness_ptr, event_queue, event_queue_item,
            client_info);
    }

    return 0;
}

/* Write all buffer entries into main memory.
 */
void puu_buffer_flush(struct puu_t *puu, struct mod_t *mod, int *witness_ptr,
    struct linked_list_t *event_queue, void *event_queue_item,
    struct mod_client_info_t *client_info)
{
    struct mod_stack_t *stack;
    unsigned int addr_from_buf;
    struct mod_t *memory_mod;

    memory_mod = puu_find_memory_mod(puu, mod);

    // Issue writes to memroy
    while (puu->counter--)
    {
        addr_from_buf = puu->buffer_head->entry->addr;
        puu_buffer_del_head(puu);

        // Create module stack for event
        mod_stack_id++;
        stack = mod_stack_create(mod_stack_id, memory_mod, addr_from_buf,
            ESIM_EV_NONE, NULL);
        stack->witness_ptr = witness_ptr;
        stack->event_queue = event_queue;
        // !!! HAVE TO FIGURE OUT WHAT IS EVENT_QUEUE FOR. POSSIBLY UNNECESSARY.
        stack->event_queue_item = event_queue_item;
        stack->client_info = client_info;

        esim_execute_event(EV_MOD_LOCAL_MEM_STORE, stack);
    }
}

/* Append new entry to the tail of buffer list.
 */
void puu_buffer_append(struct puu_t *puu, unsigned int addr)
{
    struct puu_buffer_entry_t *new_buffer_entry;
    struct puu_buffer_t *new_buffer_node;

    new_buffer_entry = xcalloc(1, sizeof(struct puu_buffer_entry_t));
    new_buffer_entry->addr = addr;

    new_buffer_node = xcalloc(1, sizeof(struct puu_buffer_t));
    new_buffer_node->entry = new_buffer_entry;
    new_buffer_node->prev = puu->buffer_tail;
    new_buffer_node->next = NULL;

    if (puu->buffer_tail != NULL)
    {
        puu->buffer_tail->next = new_buffer_node;
        puu->buffer_tail = puu->buffer_tail->next;
    }
    else // Buffer was empry.
    {
        puu->buffer_tail = new_buffer_node;
        puu->buffer_head = new_buffer_node;
    }

    puu->counter++;
}

/* Check if entry with same target address already exists in buffer.
 */
void puu_buffer_append_check(struct puu_t *puu, unsigned int addr)
{
    struct puu_buffer_t *buffer_node;

    buffer_node = puu->buffer_tail;
    if (puu->buffer_tail != NULL)
    {
        while (buffer_node != NULL)
        {
            if (buffer_node->entry->addr == addr)
            {
                // Entry of same address found. No need to append.
                return;
            }
            buffer_node = buffer_node->prev;
        }
    }
    puu_buffer_append(puu, addr);
}

/* Delete eldest entry in buffer.
 */
void puu_buffer_del_head(struct puu_t *puu)
{
    struct puu_buffer_entry_t *head_entry;

    if (puu->buffer_tail == puu->buffer_head) //empty buffer
    {
        return;
    }

    head_entry = puu->buffer_head->entry;
    puu->buffer_head = puu->buffer_head->next;
    puu->buffer_head->prev = NULL;
    free(head_entry);
}

/* Returns the module in the lowest level of given module.
 */
struct mod_t *puu_find_memory_mod(struct puu_t *puu, struct mod_t *top_mod)
{
    unsigned int addr_from_buf;
    struct mod_t *memory_mod;

    memory_mod = top_mod;
    addr_from_buf = puu->buffer_head->entry->addr; // Actually, address 0 would work as well.
    while (1)
    {
        if(memory_mod->kind == mod_kind_main_memory) break;
        memory_mod = mod_get_low_mod(memory_mod, addr_from_buf);
    }

    return memory_mod;
};
