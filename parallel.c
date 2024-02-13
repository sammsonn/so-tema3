// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "log/log.h"
#include "os_graph.h"
#include "os_threadpool.h"
#include "utils.h"

#define NUM_THREADS 4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;
/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t graph_lock, sum_lock;

/* TODO: Define graph task argument. */
unsigned int *graph_task_arg;

void graph_task_function(void *arg)
{
	pthread_mutex_lock(&graph_lock);

	int idx = *(int *)arg;
	os_node_t *node = graph->nodes[idx];

	graph->visited[idx] = DONE;

	pthread_mutex_unlock(&graph_lock);

	pthread_mutex_lock(&sum_lock);
	sum += node->info;
	pthread_mutex_unlock(&sum_lock);

	for (unsigned int i = 0; i < node->num_neighbours; i++) {
		pthread_mutex_lock(&graph_lock);

		if (graph->visited[node->neighbours[i]] == NOT_VISITED) {
			graph->visited[node->neighbours[i]] = DONE;

			graph_task_arg = (unsigned int *)malloc(sizeof(unsigned int));
			*graph_task_arg = node->neighbours[i];

			os_task_t *task = create_task(graph_task_function, (void *)graph_task_arg, free);

			enqueue_task(tp, task);

			pthread_mutex_unlock(&graph_lock);
		} else {
			pthread_mutex_unlock(&graph_lock);
		}
	}
}

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */

	graph_task_arg = (unsigned int *)malloc(sizeof(unsigned int));
	*graph_task_arg = idx;

	pthread_mutex_lock(&graph_lock);

	os_task_t *task = create_task(graph_task_function, (void *)graph_task_arg, free);

	enqueue_task(tp, task);

	pthread_mutex_unlock(&graph_lock);
}

int main(int argc, char *argv[])
{
	FILE *input_file;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s input_file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	input_file = fopen(argv[1], "r");
	DIE(input_file == NULL, "fopen");

	graph = create_graph_from_file(input_file);

	/* TODO: Initialize graph synchronization mechanisms. */
	pthread_mutex_init(&graph_lock, NULL);
	pthread_mutex_init(&sum_lock, NULL);

	tp = create_threadpool(NUM_THREADS);
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);

	pthread_mutex_destroy(&graph_lock);
	pthread_mutex_destroy(&sum_lock);

	printf("%d", sum);

	return 0;
}
