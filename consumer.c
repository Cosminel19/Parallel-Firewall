// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sched.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "producer.h"

void consumer_thread(so_consumer_ctx_t *ctx)
{
	so_packet_t packet;
	char log_line[256];
	ssize_t sz;

	int out_fd = ctx->out_fd;
	so_ring_buffer_t *rb = ctx->producer_rb;
	ts_linked_list_t *timestamp_list = rb->timestamp_list;

	while (1) {
		sz = ring_buffer_dequeue(rb, &packet, sizeof(packet));

		if (sz == -1)
			break;

		so_action_t action = process_packet(&packet);
		unsigned long hash = packet_hash(&packet);
		unsigned long timestamp = packet.hdr.timestamp;

		int len = snprintf(log_line, sizeof(log_line), "%s %016lx %lu\n",
						   RES_TO_STR(action), hash, timestamp);

		while (timestamp != ts_list_peek(timestamp_list))
			sched_yield();

		write(out_fd, log_line, len);
		ts_list_pop(timestamp_list);
	}

	free(ctx);
	pthread_exit(NULL);
}

void *get_thread(void *ctx_void)
{
	consumer_thread(ctx_void);
	return NULL;
}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
	int out_fd = open(out_filename, O_RDWR | O_CREAT | O_TRUNC, 0666);

	pthread_mutex_t *log_mutex = malloc(sizeof(pthread_mutex_t));

	if (!log_mutex) {
		close(out_fd);
		return -1;
	}

	pthread_mutex_init(log_mutex, NULL);

	for (int i = 0; i < num_consumers; i++) {
		so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));

		if (!ctx) {
			free(log_mutex);
			close(out_fd);
			return -1;
		}

		ctx->producer_rb = rb;
		ctx->out_fd = out_fd;
		ctx->log_mutex = log_mutex;

		pthread_create(&tids[i], NULL, get_thread, ctx);
	}

	return num_consumers;
}
