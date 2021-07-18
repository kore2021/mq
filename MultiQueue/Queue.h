#pragma once

#include "IQueueable.h"

#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>


namespace mq
{
  class IQueueable;
}

namespace mq
{
  class Queue
  {
  public:
    class IListener
    {
    public:
      virtual ~IListener() = default;

      virtual void onEnqueued() = 0;
    };

    Queue() = default;
    Queue(Queue&&) = default;
    Queue& operator=(Queue&&) = default;
    ~Queue();

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void enqueue(std::unique_ptr<IQueueable>);
    std::unique_ptr<IQueueable> dequeue();
    std::size_t size() const;

    void addListener(IListener* pListener);
    void removeListener(IListener* pListener);

  private:
    mutable std::mutex m_mutex;
    std::queue<std::unique_ptr<IQueueable>> m_queue;
    std::vector<IListener*> m_listeners;
  };
}
