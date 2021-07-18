#pragma once

#include "IThreadable.h"
#include "Queue.h"
#include "Thread.h"


namespace mq
{
  class QueueEventAdapter : public Queue::IListener
  {
  public:
    QueueEventAdapter(Queue& queue, Thread& thread, ThreadableId threadableEventId);
    ~QueueEventAdapter() override;

    // Queue::IListener
    void onEnqueued() override;

    // self
    ThreadableId threadableEventId() const;

  private:
    Queue& m_queue;
    Thread& m_thread;
    const ThreadableId m_threadableEventId;
  };
}

mq::QueueEventAdapter::QueueEventAdapter(Queue& queue, Thread& thread, ThreadableId threadableEventId) :
  m_queue(queue),
  m_thread(thread),
  m_threadableEventId(threadableEventId)
{
  queue.addListener(this);
}

mq::QueueEventAdapter::~QueueEventAdapter()
{
  m_queue.removeListener(this);
}

void mq::QueueEventAdapter::onEnqueued()
{
  m_thread.setThreadableEvent(m_threadableEventId);
}

mq::ThreadableId mq::QueueEventAdapter::threadableEventId() const
{
  return m_threadableEventId;
}