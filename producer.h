/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_PRODUCER_H__
#define __SO_PRODUCER_H__

#include "ring_buffer.h"
#include "packet.h"

typedef struct ts_node {
	unsigned long timestamp;
	struct ts_node *next;
} ts_node_t;

typedef struct ts_linked_list {
	ts_node_t *head, *tail;
} ts_linked_list_t;

void ts_list_push(ts_linked_list_t *list, unsigned long timestamp);
unsigned long ts_list_peek(ts_linked_list_t *list);
void ts_list_pop(ts_linked_list_t *list);
void publish_data(struct so_ring_buffer_t *rb, const char *filename);

#endif /*__SO_PRODUCER_H__*/
