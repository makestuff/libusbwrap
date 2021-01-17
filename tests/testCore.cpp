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
#include <gtest/gtest.h>
#include <iostream>
#include <cstdlib>
#include <cerrno>
#include <makestuff/common.h>
#include "unbounded_queue.h"

extern "C" {
  static uint32 m_count = 1;
  static int m_allocFree = 0;
  uint32 *createInt(void) {
    uint32 *p;
    if (m_count > 8) {
      return NULL;  // simulate out-of-memory
    }
    p = (uint32 *)malloc(sizeof(uint32));
    if (p) {
      *p = m_count++;
    }
    m_allocFree++;
    return p;
  }
  void destroyInt(uint32 *p) {
    if (p) {
      m_allocFree--;
      free((void*)p);
    }
  }
}

static inline uint32 deref(Item p) {
  return *((const uint32 *)p);
}

TEST(Queue, testInit) {
  struct UnboundedQueue queue;
  m_count = 1;

  // Create queue
  USBStatus status = queueInit(&queue, 4, (CreateFunc)createInt, (DestroyFunc)destroyInt);
  ASSERT_EQ(USB_SUCCESS, status);

  // Verify
  ASSERT_EQ(4UL, queue.capacity);
  ASSERT_EQ(0UL, queue.putIndex);
  ASSERT_EQ(0UL, queue.takeIndex);
  ASSERT_EQ(0UL, queue.numItems);

  // Verify array
  ASSERT_EQ(1UL, deref(queue.itemArray[0]));
  ASSERT_EQ(2UL, deref(queue.itemArray[1]));
  ASSERT_EQ(3UL, deref(queue.itemArray[2]));
  ASSERT_EQ(4UL, deref(queue.itemArray[3]));

  // Clean up
  queueDestroy(&queue);
}

TEST(Queue, testPutNoWrap) {
  struct UnboundedQueue queue;
  uint32 *item;
  m_count = 1;

  // Create queue
  USBStatus status = queueInit(&queue, 4, (CreateFunc)createInt, (DestroyFunc)destroyInt);
  ASSERT_EQ(USB_SUCCESS, status);

  // Put three items into the queue
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(1UL, *item);
  *item = 5;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(2UL, *item);
  *item = 6;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(3UL, *item);
  *item = 7;
  queueCommitPut(&queue);

  // Verify
  ASSERT_EQ(4UL, queue.capacity);
  ASSERT_EQ(3UL, queue.putIndex);
  ASSERT_EQ(0UL, queue.takeIndex);
  ASSERT_EQ(3UL, queue.numItems);

  // Verify array
  ASSERT_EQ(5UL, deref(queue.itemArray[0]));
  ASSERT_EQ(6UL, deref(queue.itemArray[1]));
  ASSERT_EQ(7UL, deref(queue.itemArray[2]));
  ASSERT_EQ(4UL, deref(queue.itemArray[3]));

  // Clean up
  queueDestroy(&queue);
}

TEST(Queue, testPutWrap) {
  struct UnboundedQueue queue;
  uint32 *item;
  m_count = 1;

  // Create queue
  USBStatus status = queueInit(&queue, 4, (CreateFunc)createInt, (DestroyFunc)destroyInt);
  ASSERT_EQ(USB_SUCCESS, status);

  // Put four items into the queue
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(1UL, *item);
  *item = 5;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(2UL, *item);
  *item = 6;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(3UL, *item);
  *item = 7;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(4UL, *item);
  *item = 8;
  queueCommitPut(&queue);

  // Verify
  ASSERT_EQ(4UL, queue.capacity);
  ASSERT_EQ(0UL, queue.putIndex);
  ASSERT_EQ(0UL, queue.takeIndex);
  ASSERT_EQ(4UL, queue.numItems);

  // Verify array
  ASSERT_EQ(5UL, deref(queue.itemArray[0]));
  ASSERT_EQ(6UL, deref(queue.itemArray[1]));
  ASSERT_EQ(7UL, deref(queue.itemArray[2]));
  ASSERT_EQ(8UL, deref(queue.itemArray[3]));

  // Clean up
  queueDestroy(&queue);
}

TEST(Queue, testPutTake) {
  struct UnboundedQueue queue;
  uint32 *item;
  m_count = 1;

  // Create queue
  USBStatus status = queueInit(&queue, 4, (CreateFunc)createInt, (DestroyFunc)destroyInt);
  ASSERT_EQ(USB_SUCCESS, status);

  // Put four items into the queue
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(1UL, *item);
  *item = 5;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(2UL, *item);
  *item = 6;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(3UL, *item);
  *item = 7;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(4UL, *item);
  *item = 8;
  queueCommitPut(&queue);

  // Take four items out of the queue
  status = queueTake(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(5UL, *item);
  queueCommitTake(&queue);
  status = queueTake(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(6UL, *item);
  queueCommitTake(&queue);
  status = queueTake(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(7UL, *item);
  queueCommitTake(&queue);
  status = queueTake(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(8UL, *item);
  queueCommitTake(&queue);

  // Verify
  ASSERT_EQ(4UL, queue.capacity);
  ASSERT_EQ(0UL, queue.putIndex);
  ASSERT_EQ(0UL, queue.takeIndex);
  ASSERT_EQ(0UL, queue.numItems);

  // Verify array
  ASSERT_EQ(5UL, deref(queue.itemArray[0]));
  ASSERT_EQ(6UL, deref(queue.itemArray[1]));
  ASSERT_EQ(7UL, deref(queue.itemArray[2]));
  ASSERT_EQ(8UL, deref(queue.itemArray[3]));

  // Clean up
  queueDestroy(&queue);
}

void testRealloc(const size_t offset) {
  struct UnboundedQueue queue;
  uint32 *item;
  m_count = 1;

  // Create queue
  USBStatus status = queueInit(&queue, 4, (CreateFunc)createInt, (DestroyFunc)destroyInt);
  ASSERT_EQ(USB_SUCCESS, status);

  // Shift indices so realloc has to do more work
  queue.putIndex = offset;
  queue.takeIndex = offset;

  // Now fill the queue to capacity
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ((offset) % 4 + 1UL, *item);
  *item = 100;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ((offset + 1UL) % 4 + 1UL, *item);
  *item = 101;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ((offset + 2UL) % 4 + 1UL, *item);
  *item = 102;
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ((offset + 3UL) % 4 + 1UL, *item);
  *item = 103;
  queueCommitPut(&queue);

  // Verify
  ASSERT_EQ(4UL, queue.capacity);
  ASSERT_EQ(offset, queue.putIndex);
  ASSERT_EQ(offset, queue.takeIndex);
  ASSERT_EQ(4UL, queue.numItems);

  // Force reallocation
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(5UL, *item);
  *item = 104;
  queueCommitPut(&queue);

  // Verify
  ASSERT_EQ(8UL, queue.capacity);
  ASSERT_EQ(5UL, queue.putIndex);
  ASSERT_EQ(0UL, queue.takeIndex);
  ASSERT_EQ(5UL, queue.numItems);

  // Verify array
  ASSERT_EQ(100UL, deref(queue.itemArray[0]));
  ASSERT_EQ(101UL, deref(queue.itemArray[1]));
  ASSERT_EQ(102UL, deref(queue.itemArray[2]));
  ASSERT_EQ(103UL, deref(queue.itemArray[3]));
  ASSERT_EQ(104UL, deref(queue.itemArray[4]));
  ASSERT_EQ(6UL, deref(queue.itemArray[5]));
  ASSERT_EQ(7UL, deref(queue.itemArray[6]));
  ASSERT_EQ(8UL, deref(queue.itemArray[7]));

  // Take five items out of the queue
  status = queueTake(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(100UL, *item);
  queueCommitTake(&queue);
  status = queueTake(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(101UL, *item);
  queueCommitTake(&queue);
  status = queueTake(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(102UL, *item);
  queueCommitTake(&queue);
  status = queueTake(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(103UL, *item);
  queueCommitTake(&queue);
  status = queueTake(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  ASSERT_EQ(104UL, *item);
  queueCommitTake(&queue);

  // Try to take one more
  status = queueTake(&queue, (Item*)&item);
  ASSERT_EQ(USB_EMPTY_QUEUE, status);

  (void)item;
  (void)offset;
  queueDestroy(&queue);
}

TEST(Queue, testRealloc) {
  testRealloc(0);
  //testRealloc(1);
  //testRealloc(2);
  //testRealloc(3);
}

TEST(Queue, testAllocOOM) {
  struct UnboundedQueue queue;
  m_count = 1;

  // Create queue
  USBStatus status = queueInit(&queue, 9, (CreateFunc)createInt, (DestroyFunc)destroyInt);
  ASSERT_EQ(USB_ALLOC_ERR, status);

  queueDestroy(&queue);
}

TEST(Queue, testReallocOOM) {
  struct UnboundedQueue queue;
  uint32 *item;
  m_count = 1;

  // Create queue
  USBStatus status = queueInit(&queue, 8, (CreateFunc)createInt, (DestroyFunc)destroyInt);
  ASSERT_EQ(USB_SUCCESS, status);

  // Put eight items into the queue
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  queueCommitPut(&queue);
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_SUCCESS, status);
  queueCommitPut(&queue);

  // The ninth allocation fails
  status = queuePut(&queue, (Item*)&item);
  ASSERT_EQ(USB_ALLOC_ERR, status);

  queueDestroy(&queue);
}

TEST(Queue, testAllocFreeMatching) {
  ASSERT_EQ(0, m_allocFree);
}
