// SPDX-License-Identifier: BSD-3-Clause

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"
#include "producer.h"

void ts_list_push(ts_linked_list_t *list, unsigned long timestamp)
{
	ts_node_t *new_node = malloc(sizeof(ts_node_t));

	if (!new_node)
		return;

	new_node->timestamp = timestamp;
	new_node->next = NULL;

	if (list->head == NULL)
		list->head = new_node;
	else
		list->tail->next = new_node;

	list->tail = new_node;
}

void ts_list_pop(ts_linked_list_t *list)
{
	if (list->head == NULL)
		return;

	ts_node_t *temp = list->head;

	list->head = list->head->next;

	free(temp);
}

unsigned long ts_list_peek(ts_linked_list_t *list)
{
	if (list->head == NULL)
		return (unsigned long)-1;

	return list->head->timestamp;
}

void publish_data(struct so_ring_buffer_t *rb, const char *filename)
{
	char buffer[PKT_SZ];
	ssize_t sz;
	int fd;
	ts_linked_list_t *timestamp_list = rb->timestamp_list;

	fd = open(filename, O_RDONLY);
	DIE(fd < 0, "open");

	while ((sz = read(fd, buffer, PKT_SZ)) != 0) {
		DIE(sz != PKT_SZ, "packet truncated");

		struct so_packet_t *pkt = (struct so_packet_t *)buffer;

		ts_list_push(timestamp_list, pkt->hdr.timestamp);
		ring_buffer_enqueue(rb, buffer, sz);
	}

	ring_buffer_stop(rb);
}
