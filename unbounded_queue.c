#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "unbounded_queue.h"
#include <makestuff.h>

int queueInit(
	struct UnboundedQueue *self, size_t capacity, CreateFunc createFunc, DestroyFunc destroyFunc)
{
	int retVal;
	size_t i;
	Item item;
	self->itemArray = (Item *)calloc(capacity, sizeof(Item));
	CHECK_STATUS(self->itemArray == NULL, ENOMEM, exit);
	self->capacity = capacity;
	self->putIndex = 0;
	self->takeIndex = 0;
	self->numItems = 0;
	self->createFunc = createFunc;
	self->destroyFunc = destroyFunc;
	for ( i = 0; i < capacity; i++ ) {
		item = (*createFunc)();
		CHECK_STATUS(item == NULL, ENOMEM, cleanup);
		self->itemArray[i] = item;
	}
	return 0;
cleanup:
	for ( i = 0; i < capacity; i++ ) {
		(*destroyFunc)(self->itemArray[i]);
	}
	free(self->itemArray);
	self->itemArray = NULL;
exit:
	return retVal;
}

void queueDestroy(struct UnboundedQueue *self) {
	if ( self->itemArray ) {
		size_t i;
		for ( i = 0; i < self->capacity; i++ ) {
			(*self->destroyFunc)(self->itemArray[i]);
		}
		free(self->itemArray);
	}
}

// Everything is preserved if a reallocation fails
//
int queuePut(struct UnboundedQueue *self, Item *item) {
	int retVal = 0;
	if ( self->numItems == self->capacity ) {
		size_t i;
		Item *newArray;
		Item *const ptr = self->itemArray + self->takeIndex;
		const size_t firstHalfLength = self->capacity - self->takeIndex;
		const size_t secondHalfLength = self->takeIndex;
		const size_t newCapacity = 2 * self->capacity;
		Item item;
		newArray = (Item *)calloc(newCapacity, sizeof(Item));
		CHECK_STATUS(newArray == NULL, ENOMEM, cleanup);
		memcpy(newArray, ptr, firstHalfLength * sizeof(Item));
		if ( secondHalfLength ) {
			memcpy(
				newArray + firstHalfLength,
				self->itemArray,
				secondHalfLength * sizeof(Item)
			);
		}
		for ( i = self->capacity; i < newCapacity; i++ ) {
			item = (*self->createFunc)();
			CHECK_STATUS(item == NULL, ENOMEM, cleanup);
			newArray[i] = item;
		}
		self->itemArray = newArray;
		self->takeIndex = 0;
		self->putIndex = self->capacity;
		self->capacity = newCapacity;
	}
	*item = self->itemArray[self->putIndex];
cleanup:
	return retVal;
}

void queueCommitPut(struct UnboundedQueue *self) {
	self->numItems++;
	self->putIndex++;
	if ( self->putIndex == self->capacity ) {
		self->putIndex = 0;
	}
}

int queueTake(struct UnboundedQueue *self, Item *item) {
	int retVal = 0;
	CHECK_STATUS(self->numItems == 0, 1, cleanup);
	*item = self->itemArray[self->takeIndex];
cleanup:
	return retVal;
}

void queueCommitTake(struct UnboundedQueue *self) {
	self->numItems--;
	self->takeIndex++;
	if ( self->takeIndex == self->capacity ) {
		self->takeIndex = 0;
	}
}
