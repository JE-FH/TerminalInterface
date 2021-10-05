#include "queue.h"
#include <stdlib.h>
#include <string.h>

Queue Queue_new(size_t element_size, size_t preallocation_count) {
	Queue rv;
	rv.allocated_element_counts = preallocation_count;
	rv.begin = 0;
	rv.end = 0;
	rv.element_size = element_size;

	rv.data = malloc(rv.allocated_element_counts * rv.element_size);
	return rv;
}

void* Queue_index(Queue* self, size_t idx) {
	return ((char*)self->data) + (idx % self->allocated_element_counts) * self->element_size; 
}

void Queue_enqueue(Queue* self, const void* item) {
	if (
		(self->begin % self->allocated_element_counts) == (self->end % self->allocated_element_counts) &&
		self->begin != self->end
	) {
		/*We ran out of space so we have to reallocate and move the data around
		  this is very expensive, so we allocate 4 times more space
		*/
		size_t new_allocation_count = self->allocated_element_counts * 4;

		void* new_data = malloc(new_allocation_count * self->element_size);

		if (new_data == NULL) {
			//crashes the program safely
			((int*)NULL)[0] = 0;
		}

		self->allocated_element_counts = new_allocation_count;
		
		size_t real_start = self->begin % self->allocated_element_counts;
		size_t real_end = self->end % self->allocated_element_counts;

		memcpy(new_data, ((char*)self->data) + real_start * self->element_size, (self->allocated_element_counts - real_start) * self->element_size);
		memcpy(((char*) new_data) + (self->allocated_element_counts - real_start) * self->element_size, self->data, real_start * self->element_size);
		
		free(self->data);
		self->data = new_data;
		self->begin = 0;
		self->end = self->allocated_element_counts;
		self->allocated_element_counts = new_allocation_count;
	}

	memcpy(((char*)self->data) + (self->end % self->allocated_element_counts) * self->element_size, item, self->element_size);
	self->end += 1;
}

bool Queue_dequeue(Queue* self, void* out) {
	if (self->begin == self->end) {
		return false;
	}

	void* raw_data = Queue_index(self, self->begin);
	memcpy(out, raw_data, self->element_size);
	self->begin += 1;
	return true;
}

/*You can only use this if you just dequeued*/
void Queue_undequeue(Queue* self, const void* old) {
	self->begin -= 1;
	void* raw_data = Queue_index(self, self->begin);
	memcpy(raw_data, old, self->element_size);
}

void Queue_free(Queue* self) {
	if (self->data != NULL) {
		free(self->data);
	}
	self->data = NULL;
	self->allocated_element_counts = 0;
	self->begin = 0;
	self->end = 0;
	self->element_size = 0;
}