#pragma once

#include "IThreadable.h"
#include "Queue.h"


namespace mq
{
  class Thread;
}

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