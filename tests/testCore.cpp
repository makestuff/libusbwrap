/*
 * Copyright (C) 2009-2012 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <iostream>
#include <cstdlib>
#include <errno.h>
#include <UnitTest++.h>
#include <makestuff.h>
#include "../unbounded_queue.h"

using namespace std;

extern "C" {
	static uint32 m_count = 1;
	static int m_allocFree = 0;
	uint32 *createInt(void) {
		uint32 *p;
		if ( m_count > 8 ) {
			return NULL;  // simulate out-of-memory
		}
		p = (uint32 *)malloc(sizeof(uint32));
		if ( p ) {
			*p = m_count++;
		}
		m_allocFree++;
		return p;
	}
	void destroyInt(uint32 *p) {
		if ( p ) {
			m_allocFree--;
			free((void*)p);
		}
	}
}

static inline uint32 deref(Item p) {
	return *((const uint32 *)p);
}

TEST(Queue_testInit) {
	struct UnboundedQueue queue;
	m_count = 1;

	// Create queue
	USBStatus status = queueInit(&queue, 4, (CreateFunc)createInt, (DestroyFunc)destroyInt);
	CHECK_EQUAL(USB_SUCCESS, status);

	// Verify
	CHECK_EQUAL(4UL, queue.capacity);
	CHECK_EQUAL(0UL, queue.putIndex);
	CHECK_EQUAL(0UL, queue.takeIndex);
	CHECK_EQUAL(0UL, queue.numItems);

	// Verify array
	CHECK_EQUAL(1UL, deref(queue.itemArray[0]));
	CHECK_EQUAL(2UL, deref(queue.itemArray[1]));
	CHECK_EQUAL(3UL, deref(queue.itemArray[2]));
	CHECK_EQUAL(4UL, deref(queue.itemArray[3]));

	// Clean up
	queueDestroy(&queue);
}

TEST(Queue_testPutNoWrap) {
	struct UnboundedQueue queue;
	uint32 *item;
	m_count = 1;

	// Create queue
	USBStatus status = queueInit(&queue, 4, (CreateFunc)createInt, (DestroyFunc)destroyInt);
	CHECK_EQUAL(USB_SUCCESS, status);

	// Put three items into the queue
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(1UL, *item);
	*item = 5;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(2UL, *item);
	*item = 6;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(3UL, *item);
	*item = 7;
	queueCommitPut(&queue);

	// Verify
	CHECK_EQUAL(4UL, queue.capacity);
	CHECK_EQUAL(3UL, queue.putIndex);
	CHECK_EQUAL(0UL, queue.takeIndex);
	CHECK_EQUAL(3UL, queue.numItems);

	// Verify array
	CHECK_EQUAL(5UL, deref(queue.itemArray[0]));
	CHECK_EQUAL(6UL, deref(queue.itemArray[1]));
	CHECK_EQUAL(7UL, deref(queue.itemArray[2]));
	CHECK_EQUAL(4UL, deref(queue.itemArray[3]));

	// Clean up
	queueDestroy(&queue);
}

TEST(Queue_testPutWrap) {
	struct UnboundedQueue queue;
	uint32 *item;
	m_count = 1;

	// Create queue
	USBStatus status = queueInit(&queue, 4, (CreateFunc)createInt, (DestroyFunc)destroyInt);
	CHECK_EQUAL(USB_SUCCESS, status);

	// Put four items into the queue
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(1UL, *item);
	*item = 5;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(2UL, *item);
	*item = 6;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(3UL, *item);
	*item = 7;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(4UL, *item);
	*item = 8;
	queueCommitPut(&queue);

	// Verify
	CHECK_EQUAL(4UL, queue.capacity);
	CHECK_EQUAL(0UL, queue.putIndex);
	CHECK_EQUAL(0UL, queue.takeIndex);
	CHECK_EQUAL(4UL, queue.numItems);

	// Verify array
	CHECK_EQUAL(5UL, deref(queue.itemArray[0]));
	CHECK_EQUAL(6UL, deref(queue.itemArray[1]));
	CHECK_EQUAL(7UL, deref(queue.itemArray[2]));
	CHECK_EQUAL(8UL, deref(queue.itemArray[3]));

	// Clean up
	queueDestroy(&queue);
}

TEST(Queue_testPutTake) {
	struct UnboundedQueue queue;
	uint32 *item;
	m_count = 1;

	// Create queue
	USBStatus status = queueInit(&queue, 4, (CreateFunc)createInt, (DestroyFunc)destroyInt);
	CHECK_EQUAL(USB_SUCCESS, status);

	// Put four items into the queue
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(1UL, *item);
	*item = 5;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(2UL, *item);
	*item = 6;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(3UL, *item);
	*item = 7;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(4UL, *item);
	*item = 8;
	queueCommitPut(&queue);

	// Take four items out of the queue
	status = queueTake(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(5UL, *item);
	queueCommitTake(&queue);
	status = queueTake(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(6UL, *item);
	queueCommitTake(&queue);
	status = queueTake(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(7UL, *item);
	queueCommitTake(&queue);
	status = queueTake(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(8UL, *item);
	queueCommitTake(&queue);

	// Verify
	CHECK_EQUAL(4UL, queue.capacity);
	CHECK_EQUAL(0UL, queue.putIndex);
	CHECK_EQUAL(0UL, queue.takeIndex);
	CHECK_EQUAL(0UL, queue.numItems);

	// Verify array
	CHECK_EQUAL(5UL, deref(queue.itemArray[0]));
	CHECK_EQUAL(6UL, deref(queue.itemArray[1]));
	CHECK_EQUAL(7UL, deref(queue.itemArray[2]));
	CHECK_EQUAL(8UL, deref(queue.itemArray[3]));

	// Clean up
	queueDestroy(&queue);
}

void testRealloc(const size_t offset) {
	struct UnboundedQueue queue;
	uint32 *item;
	m_count = 1;

	// Create queue
	USBStatus status = queueInit(&queue, 4, (CreateFunc)createInt, (DestroyFunc)destroyInt);
	CHECK_EQUAL(USB_SUCCESS, status);

	// Shift indices so realloc has to do more work
	queue.putIndex = offset;
	queue.takeIndex = offset;

	// Now fill the queue to capacity
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL((offset) % 4 + 1UL, *item);
	*item = 100;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL((offset + 1UL) % 4 + 1UL, *item);
	*item = 101;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL((offset + 2UL) % 4 + 1UL, *item);
	*item = 102;
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL((offset + 3UL) % 4 + 1UL, *item);
	*item = 103;
	queueCommitPut(&queue);

	// Verify
	CHECK_EQUAL(4UL, queue.capacity);
	CHECK_EQUAL(offset, queue.putIndex);
	CHECK_EQUAL(offset, queue.takeIndex);
	CHECK_EQUAL(4UL, queue.numItems);

	// Force reallocation
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(5UL, *item);
	*item = 104;
	queueCommitPut(&queue);

	// Verify
	CHECK_EQUAL(8UL, queue.capacity);
	CHECK_EQUAL(5UL, queue.putIndex);
	CHECK_EQUAL(0UL, queue.takeIndex);
	CHECK_EQUAL(5UL, queue.numItems);

	// Verify array
	CHECK_EQUAL(100UL, deref(queue.itemArray[0]));
	CHECK_EQUAL(101UL, deref(queue.itemArray[1]));
	CHECK_EQUAL(102UL, deref(queue.itemArray[2]));
	CHECK_EQUAL(103UL, deref(queue.itemArray[3]));
	CHECK_EQUAL(104UL, deref(queue.itemArray[4]));
	CHECK_EQUAL(6UL, deref(queue.itemArray[5]));
	CHECK_EQUAL(7UL, deref(queue.itemArray[6]));
	CHECK_EQUAL(8UL, deref(queue.itemArray[7]));

	// Take five items out of the queue
	status = queueTake(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(100UL, *item);
	queueCommitTake(&queue);
	status = queueTake(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(101UL, *item);
	queueCommitTake(&queue);
	status = queueTake(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(102UL, *item);
	queueCommitTake(&queue);
	status = queueTake(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(103UL, *item);
	queueCommitTake(&queue);
	status = queueTake(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	CHECK_EQUAL(104UL, *item);
	queueCommitTake(&queue);

	// Try to take one more
	status = queueTake(&queue, (Item*)&item);
	CHECK_EQUAL(USB_EMPTY_QUEUE, status);

	queueDestroy(&queue);
}

TEST(Queue_testRealloc) {
	testRealloc(0);
	testRealloc(1);
	testRealloc(2);
	testRealloc(3);
}

TEST(Queue_testAllocOOM) {
	struct UnboundedQueue queue;
	m_count = 1;

	// Create queue
	USBStatus status = queueInit(&queue, 9, (CreateFunc)createInt, (DestroyFunc)destroyInt);
	CHECK_EQUAL(USB_ALLOC_ERR, status);

	queueDestroy(&queue);
}

TEST(Queue_testReallocOOM) {
	struct UnboundedQueue queue;
	uint32 *item;
	m_count = 1;

	// Create queue
	USBStatus status = queueInit(&queue, 8, (CreateFunc)createInt, (DestroyFunc)destroyInt);
	CHECK_EQUAL(USB_SUCCESS, status);

	// Put eight items into the queue
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	queueCommitPut(&queue);
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_SUCCESS, status);
	queueCommitPut(&queue);

	// The ninth allocation fails
	status = queuePut(&queue, (Item*)&item);
	CHECK_EQUAL(USB_ALLOC_ERR, status);

	queueDestroy(&queue);
}

TEST(Queue_testAllocFreeMatching) {
	CHECK_EQUAL(0, m_allocFree);
}
