#include <lib/mhandle/mhandle.h>
#include <lib/esim/esim.h>
#include "memory.h"
#include "puu.h"
#include "mod-stack.h"
#include "local-mem-protocol.h"

// initialize, set threshold
struct puu_t *puu_create(void)
{
	struct puu_t *puu = xcalloc(1,sizeof(struct puu_t));

	puu->counter = 0;
	puu->threshold = 20;
	puu->buffer1 = xcalloc(1,sizeof(struct puu_buffer_node_t));
	puu->buffer2 = xcalloc(1,sizeof(struct puu_buffer_node_t));
	puu->buffer1->prev = NULL;
	puu->buffer1->next = NULL;
	puu->buffer1->entry = NULL;
	puu->buffer2->prev = NULL;
	puu->buffer2->next = NULL;
	puu->buffer2->entry = NULL;

	puu->current_buffer = puu->buffer1;

	puu->buffer_head = puu->current_buffer;
	puu->buffer_tail = NULL; // indicate buffer is empty
}

void puu_free(struct puu_t *puu)
{
    /* TODO: may have to check if buffer cleared. */
    free(puu->buffer1->entry);
    free(puu->buffer1);
    free(puu->buffer2->entry);
    free(puu->buffer2);
    free(puu);
}

long long puu_access(struct puu_t *puu, struct mod_t *mod,
    enum puu_access_kind_t access_kind, unsigned int addr)
{
//	struct mod_stack_t *stack;
//	int event;
//	unsigned int addr_from_buf;

    if (access_kind == puu_access_write)
    {
        puu_buffer_append_check(puu, addr);

        if (puu->counter == puu->counter_threshold)
        {
            puu_buffer_flush(puu, mod);
        }
    }
    else if (access_kind == puu_access_evict)
    {
        // TODO: check and eliminate duplicate entries in buffer.
        puu_buffer_flush(puu, mod);
    }

    return 0;
}

/* Write all buffer entries into main memory.
 */
void puu_buffer_flush(struct puu_t *puu, struct mod_t *mod)
{
    struct mod_stack_t *stack;
    unsigned int addr_from_buf;
    struct mod_t *memory_mod;
    struct mod_client_info_t *mod_client_info;
    struct puu_buffer_node_t *flush_buffer;

    /* buffer switch */
    flush_buffer = puu->current_buffer;
    if(flush_buffer==puu->buffer2)
    {
        puu->current_buffer = puu->buffer1;
    }
    else
    {
        puu->current_buffer = puu->buffer2;
    }

    memory_mod = puu_find_memory_mod(puu, mod);
    mod_client_info = mod_client_info_create(memory_mod);

    // Issue writes to memroy
    while (puu->counter--)
    {
        addr_from_buf = flush_buffer->head->entry->addr;
        puu_buffer_del_head(flush_buffer);

        // Create module stack for event
        mod_stack_id++;
        stack = mod_stack_create(mod_stack_id, memory_mod, addr_from_buf,
            ESIM_EV_NONE, NULL);
        stack->witness_ptr = NULL;
        stack->event_queue = NULL;
        stack->event_queue_item = NULL;
        stack->client_info = mod_client_info;

        esim_execute_event(EV_MOD_LOCAL_MEM_STORE, stack);
    }
}

/* Append new entry to the tail of buffer list.
 */
void puu_buffer_append(struct puu_t *puu, unsigned int addr)
{
    struct puu_buffer_entry_t *new_buffer_entry;
    struct puu_buffer_node_t *new_buffer_node;

    new_buffer_entry = xcalloc(1, sizeof(struct puu_buffer_entry_t));
    new_buffer_entry->addr = addr;

    new_buffer_node = xcalloc(1, sizeof(struct puu_buffer_node_t));
    new_buffer_node->entry = new_buffer_entry;
    new_buffer_node->prev = puu->current_buffer->tail;
    new_buffer_node->next = NULL;

    if (puu->current_buffer->tail != NULL)
    {
        puu->current_buffer->tail->next = new_buffer_node;
        puu->current_buffer->tail = new_buffer_node;
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
    struct puu_buffer_node_t *buffer_node;

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
void puu_buffer_del_head(struct puu_buffer_node_t *flush_buffer)
{
    struct puu_buffer_entry_t *head_entry;

//    can't be empty
//    if (flush_buffer->buffer_tail == flush_buffer->buffer_head) //empty buffer
//    {
//        return;
//    }

    head_entry = flush_buffer->head->entry;
    flush_buffer->head = flush_buffer->next;
    flush_buffer->prev = NULL;
    free(head_entry);
}

/* Returns the module in the lowest level of given module.
 */
struct mod_t *puu_find_memory_mod(struct puu_t *puu, struct mod_t *top_mod)
{
    unsigned int addr_from_buf;
    struct mod_t *memory_mod;

    memory_mod = top_mod;
    addr_from_buf = puu->current_buffer->head->entry->addr; // Actually, address 0 would work as well.
    while (1)
    {
        if(memory_mod->kind == mod_kind_main_memory) break;
        memory_mod = mod_get_low_mod(memory_mod, addr_from_buf);
    }

    return memory_mod;
};
