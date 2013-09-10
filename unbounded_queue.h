#ifndef UNBOUNDED_QUEUE_H
#define UNBOUNDED_QUEUE_H

#include "libusbwrap.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef const void* Item;
	typedef Item (*CreateFunc)(void);
	typedef void (*DestroyFunc)(Item);

	struct UnboundedQueue {
		Item *itemArray;
		size_t capacity;
		size_t putIndex;
		size_t takeIndex;
		size_t numItems;
		CreateFunc createFunc;
		DestroyFunc destroyFunc;
	};

	USBStatus queueInit(
		struct UnboundedQueue *self, size_t capacity, CreateFunc createFunc, DestroyFunc destroyFunc
	);
	USBStatus queuePut(
		struct UnboundedQueue *self, Item *item  // never blocks, can ENOMEM
	);
	void queueCommitPut(
		struct UnboundedQueue *self
	);
	USBStatus queueTake(
		struct UnboundedQueue *self, Item *item  // returns NULL on empty
	);
	void queueCommitTake(
		struct UnboundedQueue *self
	);
	void queueDestroy(
		struct UnboundedQueue *self
	);
	static inline size_t queueSize(const struct UnboundedQueue *self) {
		return self->numItems;
	}

#ifdef __cplusplus
}
#endif

#endif
