#include "QueueEventAdapter.h"

#include "Queue.h"
#include "Thread.h"

using mq::Queue;
using mq::QueueEventAdapter;
using mq::Thread;
using mq::ThreadableId;


QueueEventAdapter::QueueEventAdapter(Queue& queue, Thread& thread, ThreadableId threadableEventId) :
  m_queue(queue),
  m_thread(thread),
  m_threadableEventId(threadableEventId)
{
  queue.addListener(this);
}

QueueEventAdapter::~QueueEventAdapter()
{
  m_queue.removeListener(this);
}

void QueueEventAdapter::onEnqueued()
{
  m_thread.setThreadableEvent(m_threadableEventId);
}

ThreadableId mq::QueueEventAdapter::threadableEventId() const
{
  return m_threadableEventId;
}