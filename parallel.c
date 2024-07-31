// SPDX-License-Identifier: BSD-3-Clause

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>

#include "os_graph.h"
#include "os_threadpool.h"
#include "log/log.h"
#include "utils.h"

#define NUM_THREADS		4

static int sum;
static os_graph_t *graph;
static os_threadpool_t *tp;

/* TODO: Define graph synchronization mechanisms. */
pthread_mutex_t mutex_sum;
pthread_mutex_t mutex_neighbor;

/* TODO: Define graph task argument. */
typedef struct arg {
	unsigned int idx;
} arg_t;

static void task_function(void *arg);
static void process_node(unsigned int idx);

static void process_node(unsigned int idx)
{
	/* TODO: Implement thread-pool based processing of graph. */

	// Add the info to the sum for the initial node
	pthread_mutex_lock(&mutex_sum);
	graph->visited[idx] = PROCESSING;
	sum += graph->nodes[idx]->info;
	pthread_mutex_unlock(&mutex_sum);

	// Process the neighbors of the initial node
	for (unsigned int i = 0; i < graph->nodes[idx]->num_neighbours; i++) {
		arg_t *arg = malloc(sizeof(arg_t));

		arg->idx = graph->nodes[idx]->neighbours[i];

		pthread_mutex_lock(&mutex_neighbor);
		if (graph->visited[graph->nodes[idx]->neighbours[i]] == NOT_VISITED) {
			graph->visited[graph->nodes[idx]->neighbours[i]] = PROCESSING;
			os_task_t *task = create_task(task_function, (void *) arg, free);

			enqueue_task(tp, task);
		}
		pthread_mutex_unlock(&mutex_neighbor);
	}

	// Mark the initial node as DONE
	graph->visited[idx] = DONE;
}

static void task_function(void *arg)
{
	// Get the index of the current node
	arg_t *arg_casted = (arg_t *) arg;
	unsigned int idx = arg_casted->idx;

	// Add the info to the sum for the current node
	pthread_mutex_lock(&mutex_sum);
	sum += graph->nodes[idx]->info;
	pthread_mutex_unlock(&mutex_sum);

	// Process the neighbors of the current node
	for (unsigned int i = 0; i < graph->nodes[idx]->num_neighbours; i++) {
		arg_t *new_arg = malloc(sizeof(arg_t));

		new_arg->idx = graph->nodes[idx]->neighbours[i];

		pthread_mutex_lock(&mutex_neighbor);
		if (graph->visited[graph->nodes[idx]->neighbours[i]] == NOT_VISITED) {
			graph->visited[graph->nodes[idx]->neighbours[i]] = PROCESSING;
			os_task_t *task = create_task(task_function, (void *) new_arg, free);

			enqueue_task(tp, task);
		}
		pthread_mutex_unlock(&mutex_neighbor);
	}

	// Mark the current node as DONE
	pthread_mutex_lock(&mutex_neighbor);
	graph->visited[idx] = DONE;
	pthread_mutex_unlock(&mutex_neighbor);
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
	pthread_mutex_init(&mutex_sum, NULL);
	pthread_mutex_init(&mutex_neighbor, NULL);

	tp = create_threadpool(NUM_THREADS);
	process_node(0);
	wait_for_completion(tp);
	destroy_threadpool(tp);

	printf("%d", sum);

	return 0;
}
