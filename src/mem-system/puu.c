#include "memory.h"
#include "puu.h"
#include "mod-stack.h"

struct puu_t *puu_create(void)
{
    struct puu_t *puu;

    /* Initialize */
    puu = (struct puu_t)xcalloc(1, sizeof(struct puu_t));

    return puu;
};

void puu_free(struct puu_t *puu)
{
    /* TODO: may have to check if buffer cleared. */
    free(puu);
}

long long puu_access(struct puu_t *puu, enum puu_access_kind_t access_kind,
	unsigned int addr, int *witness_ptr, struct linked_list_t *event_queue,
	void *event_queue_item, struct mod_client_info_t *client_info)
{
	struct mod_stack_t *stack;
	int event;

	/* Create module stack with new ID */
	mod_stack_id++;
	stack = mod_stack_create(mod_stack_id,
		mod, addr, ESIM_EV_NONE, NULL);

	/* Initialize */
	stack->witness_ptr = witness_ptr;
	stack->event_queue = event_queue;
	stack->event_queue_item = event_queue_item;
	stack->client_info = client_info;

	if (access_kind == puu_access_write)
    {
        // TODO
        puu_buffer_append_check(puu, addr);
        puu->counter++;

        if (puu->counter == puu->counter_threshold)
        {
            // TODO: modify stack to agree with actual write data.
            esim_execute_event(EV_MOD_LOCAL_MEM_STORE, stack);
        }
    }
    else if (access_kind == puu_access_evict)
    {
        // TODO
    }

    return stack->id;
}

/* Append new entry to the tail of buffer list.
 */
void puu_buffer_append(struct puu_t *puu, unsigned int addr)
{
    struct puu_buffer_entry_t *new_buffer_entry;
    struct puu_buffer_t *new_buffer_node;

    new_buffer_entry = (struct puu_buffer_entry_t)xcalloc(1, sizeof(struct puu_buffer_entry_t));
    new_buffer_entry->addr = addr;

    new_buffer_node = (struct puu_buffer_t)xcalloc(1, sizeof(struct puu_buffer_t));
    new_buffer_node->entry = new_buffer_entry;
    new_buffer_node->prev = puu->buffer->tail;
    new_buffer_node->next = NULL;

    puu->buffer_tail->next = new_buffer_node;
    puu->buffer_tail = puu->buffer_tail->next;
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
