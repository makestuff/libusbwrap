#ifndef UNBOUNDED_QUEUE_H
#define UNBOUNDED_QUEUE_H

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

	int queueInit(
		struct UnboundedQueue *self, size_t capacity, CreateFunc createFunc, DestroyFunc destroyFunc
	);
	int queuePut(
		struct UnboundedQueue *self, Item *item  // never blocks, can ENOMEM
	);
	void queueCommitPut(
		struct UnboundedQueue *self
	);
	int queueTake(
		struct UnboundedQueue *self, Item *item  // returns NULL on empty
	);
	void queueCommitTake(
		struct UnboundedQueue *self
	);
	void queueDestroy(
		struct UnboundedQueue *self
	);

#ifdef __cplusplus
}
#endif

#endif
