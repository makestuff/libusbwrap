#include <stdlib.h>
#include <string.h>
#include "unbounded_queue.h"
#include <makestuff.h>
#include "libusbwrap.h"

USBStatus queueInit(
	struct UnboundedQueue *self, size_t capacity, CreateFunc createFunc, DestroyFunc destroyFunc)
{
	USBStatus retVal;
	size_t i;
	Item item;
	self->itemArray = (Item *)calloc(capacity, sizeof(Item));
	CHECK_STATUS(self->itemArray == NULL, USB_ALLOC_ERR, exit);
	self->capacity = capacity;
	self->putIndex = 0;
	self->takeIndex = 0;
	self->numItems = 0;
	self->createFunc = createFunc;
	self->destroyFunc = destroyFunc;
	for ( i = 0; i < capacity; i++ ) {
		item = (*createFunc)();
		CHECK_STATUS(item == NULL, USB_ALLOC_ERR, cleanup);
		self->itemArray[i] = item;
	}
	return USB_SUCCESS;
cleanup:
	for ( i = 0; i < capacity; i++ ) {
		(*destroyFunc)(self->itemArray[i]);
	}
	free((void*)self->itemArray);
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
		free((void*)self->itemArray);
	}
}

// Everything is preserved if a reallocation fails
//
USBStatus queuePut(struct UnboundedQueue *self, Item *item) {
	USBStatus retVal = USB_SUCCESS;
	if ( self->numItems == self->capacity ) {
		size_t i;
		Item *newArray;
		Item *const ptr = self->itemArray + self->takeIndex;
		const size_t firstHalfLength = self->capacity - self->takeIndex;
		const size_t secondHalfLength = self->takeIndex;
		const size_t newCapacity = 2 * self->capacity;
		Item item;
		newArray = (Item *)calloc(newCapacity, sizeof(Item));
		CHECK_STATUS(newArray == NULL, USB_ALLOC_ERR, cleanup);
		memcpy((void*)newArray, ptr, firstHalfLength * sizeof(Item));
		if ( secondHalfLength ) {
			memcpy(
				(void*)(newArray + firstHalfLength),
				self->itemArray,
				secondHalfLength * sizeof(Item)
			);
		}
		for ( i = self->capacity; i < newCapacity; i++ ) {
			item = (*self->createFunc)();
			CHECK_STATUS(item == NULL, USB_ALLOC_ERR, cleanup);
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

USBStatus queueTake(struct UnboundedQueue *self, Item *item) {
	USBStatus retVal = 0;
	CHECK_STATUS(self->numItems == 0, USB_EMPTY_QUEUE, cleanup);
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
