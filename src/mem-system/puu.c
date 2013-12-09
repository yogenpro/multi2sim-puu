#include <lib/mhandle/mhandle.h>
#include <lib/esim/esim.h>
#include <lib/util/linked-list.h>
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
	puu->buffer1 = linked_list_create();
	puu->buffer2 = linked_list_create();

	puu->current_buffer = 1;

	return puu;
}

void puu_free(struct puu_t *puu)
{
    linked_list_free(puu->buffer1);
    linked_list_free(puu->buffer2);
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


    // Issue writes to memroy
    while (puu->counter--)
    {
        if (puu->current_buffer = 2)
        {
            addr_from_buf = puu->buffer1->head->data;
        }
        else
        {
            addr_from_buf = puu->buffer2->head->data;
        }
        puu_buffer_del_head(puu);

        // Create module stack for event
        mod_stack_id++;
        mod_client_info = mod_client_info_create(memory_mod);
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
    struct linked_list_t *buffer;
    unsigned int *data;

    if (puu->current_buffer == 1)
    {
        buffer = puu->buffer1;
    }
    else
    {
        buffer = puu->buffer2;
    }

    data = xcalloc(1, sizeof(unsigned int));
    *data = addr;
    linked_list_goto(buffer, buffer->count - 1);
    linked_list_add(buffer, data);

    puu->counter++;
}

/* Check if entry with same target address already exists in buffer.
 */
void puu_buffer_append_check(struct puu_t *puu, unsigned int addr)
{
    struct linked_list_t *buffer;

    if (puu->current_buffer == 1)
    {
        buffer = puu->buffer1;
    }
    else
    {
        buffer = puu->buffer2;
    }

    linked_list_goto(buffer, 0);
    while (!linked_list_is_end(buffer))
    {
        /* if same address exists in buffer, do nothing */
        if ((int *)(buffer->current->data) == addr) return;
        linked_list_next(buffer);
    }
    puu_buffer_append(puu, addr);
}

/* Delete eldest entry in the flushing buffer.
 */
void puu_buffer_del_head(struct puu_t *puu)
{
    struct linked_list_t *buffer;

    if (puu->current_buffer == 1)
    {
        buffer = puu->buffer2;
    }
    else
    {
        buffer = puu->buffer1;
    }
    linked_list_goto(buffer, 0);
    linked_list_remove(buffer);
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
        addr_from_buf = (int *)(puu->buffer1->head->data);
    }
    else
    {
        addr_from_buf = (int *)(puu->buffer2->head->data);
    }
    while (1)
    {
        if(memory_mod->kind == mod_kind_main_memory) break;
        memory_mod = mod_get_low_mod(memory_mod, addr_from_buf);
    }

    return memory_mod;
};
