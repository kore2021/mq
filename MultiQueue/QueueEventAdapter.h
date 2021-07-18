#pragma once

#include "IThreadable.h"
#include "Queue.h"


namespace mq
{
  class Thread;
}

namespace mq
{
  // The class notifies a thread about a queue event
  class QueueEventAdapter : public Queue::IListener
  {
  public:
    // threadableEventId will be sent to a thread on a queue event
    QueueEventAdapter(Queue& queue, Thread& thread, ThreadableId threadableEventId);
    ~QueueEventAdapter() override;

    // Queue::IListener
    void onEnqueued() override;

    // 
    ThreadableId threadableEventId() const;

  private:
    Queue& m_queue;
    Thread& m_thread;
    const ThreadableId m_threadableEventId;
  };
}