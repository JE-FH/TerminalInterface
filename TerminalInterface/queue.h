#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct Queue {
	void* data;
	size_t element_size;
	size_t allocated_element_counts;
	size_t begin;
	size_t end;
} Queue;

Queue Queue_new(size_t element_size, size_t preallocation_count);
void Queue_enqueue(Queue* self, const void* item);
bool Queue_dequeue(Queue* self, void* out);
void Queue_undequeue(Queue* self, const void* item);
void Queue_free(Queue* self);