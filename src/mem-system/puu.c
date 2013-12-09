#include <lib/mhandle/mhandle.h>
#include <lib/esim/esim.h>
#include "memory.h"
#include "puu.h"
#include "mod-stack.h"
#include "nmoesi-protocol.h"

// initialize, set threshold
struct puu_t *puu_create(void)
{
	struct puu_t *puu = xcalloc(1,sizeof(struct puu_t));

	puu->counter = 0;
	puu->threshold = 20;
	puu->buffer1 = NULL;
	puu->buffer2 = NULL;

	puu->current_buffer = 1;

	puu->buffer1_head = puu->buffer1;
	puu->buffer1_tail = puu->buffer1;

	puu->buffer2_head = puu->buffer2;
	puu->buffer2_tail = puu->buffer2;
}

void puu_free(struct puu_t *puu)
{
    struct puu_buffer_node_t *next_node;

    puu->buffer1 = puu->buffer1_head;
    while (puu->buffer1 != NULL)
    {
        next_node = puu->buffer1->next;
        if (puu->buffer1->entry != NULL) free (puu->buffer1->entry);
        free(puu->buffer1);
        puu->buffer1 = next_node;
    }
    puu->buffer2 = puu->buffer2_head;
    while (puu->buffer2 != NULL)
    {
        next_node = puu->buffer1->next;
        if (puu->buffer2->entry != NULL) free (puu->buffer2->entry);
        free(puu->buffer2);
        puu->buffer1 = next_node;
    }
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

        if (puu->counter >= puu->threshold)
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
    if (puu->current_buffer == 1)
    {
        puu->current_buffer = 2;
    }
    else
    {
        puu->current_buffer = 1;
    }

    memory_mod = puu_find_memory_mod(puu, mod);
    mod_client_info = mod_client_info_create(memory_mod);

    // Issue writes to memroy
    while (puu->counter--)
    {
        if (puu->current_buffer = 2)
        {
            addr_from_buf = puu->buffer1_head->entry->addr;
        }
        else
        {
            addr_from_buf = puu->buffer2_head->entry->addr;
        }
        puu_buffer_del_head(puu);

        // Create module stack for event
        mod_stack_id++;
        stack = mod_stack_create(mod_stack_id, memory_mod, addr_from_buf,
            ESIM_EV_NONE, NULL);
        stack->witness_ptr = NULL;
        stack->event_queue = NULL;
        stack->event_queue_item = NULL;
        stack->client_info = mod_client_info;

        esim_execute_event(EV_MOD_NMOESI_STORE, stack);
    }
}

/* Append new entry to the tail of buffer list.
 */
void puu_buffer_append(struct puu_t *puu, unsigned int addr)
{
    struct puu_buffer_entry_t *new_buffer_entry;
    struct puu_buffer_node_t *new_buffer_node;
    struct puu_buffer_node_t *buffer_tail;
    struct puu_buffer_node_t *buffer_head;

    new_buffer_entry = xcalloc(1, sizeof(struct puu_buffer_entry_t));
    new_buffer_entry->addr = addr;

    new_buffer_node = xcalloc(1, sizeof(struct puu_buffer_node_t));
    new_buffer_node->entry = new_buffer_entry;
    new_buffer_node->prev = NULL;
    new_buffer_node->next = NULL;

    if (puu->current_buffer = 1)
    {
        buffer_tail = puu->buffer1_tail;
        buffer_head = puu->buffer1_head;
    }
    else
    {
        buffer_tail = puu->buffer2_tail;
        buffer_head = puu->buffer2_head;
    }

    if (buffer_tail != NULL)
    {
        buffer_tail->next = new_buffer_node;
        new_buffer_node->prev = buffer_tail;
    }
    else /* Buffer was empty */
    {
        buffer_tail = new_buffer_node;
        buffer_head = new_buffer_node;
    }

    if (puu->current_buffer = 1)
    {
        puu->buffer1_tail = buffer_tail;
        puu->buffer1_head = buffer_head;
    }
    else
    {
        puu->buffer2_tail = buffer_tail;
        puu->buffer2_head = buffer_head;
    }

    puu->counter++;
}

/* Check if entry with same target address already exists in buffer.
 */
void puu_buffer_append_check(struct puu_t *puu, unsigned int addr)
{
    struct puu_buffer_node_t *buffer_node;

    if (puu->current_buffer == 1)
    {
        buffer_node = puu->buffer1_head;
    }
    else
    {
        buffer_node = puu->buffer2_head;
    }

    while (buffer_node != NULL)
    {
        if (buffer_node->entry->addr == addr)
        {
            // Entry of same address found. No need to append.
            return;
        }
        buffer_node = buffer_node->next;
    }

    puu_buffer_append(puu, addr);
}

/* Delete eldest entry in the flushing buffer.
 */
void puu_buffer_del_head(struct puu_t *puu)
{
    struct puu_buffer_node_t *head_node;

    if (puu->current_buffer == 1)
    {
        head_node = puu->buffer2_head;
        puu->buffer2 = head_node->next;
        puu->buffer2_head = puu->buffer2;
        puu->buffer2_head->prev = NULL;
    }
    else
    {
        head_node = puu->buffer1_head;
        puu->buffer1 = head_node->next;
        puu->buffer1_head = puu->buffer1;
        puu->buffer1_head->prev = NULL;
    }

    if (head_node->entry != NULL) free(head_node->entry);
    free(head_node);
}

/* Returns the module in the lowest level of given module.
 */
struct mod_t *puu_find_memory_mod(struct puu_t *puu, struct mod_t *top_mod)
{
    unsigned int addr_from_buf;
    struct mod_t *memory_mod;

    memory_mod = top_mod;
    if (puu->current_buffer = 2) /* Actually, address 0 would work as well. */
    {
        addr_from_buf = puu->buffer1_head->entry->addr;
    }
    else
    {
        addr_from_buf = puu->buffer2_head->entry->addr;
    }
    while (1)
    {
        if(memory_mod->kind == mod_kind_main_memory) break;
        memory_mod = mod_get_low_mod(memory_mod, addr_from_buf);
    }

    return memory_mod;
};
