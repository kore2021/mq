#pragma once

#include "IQueueable.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>


namespace mq
{
  template <typename QueueKey>
  class IQueueListener;
}

namespace mq
{
  template <typename QueueKey>
  class Queue
  {
  public:
    class IListener
    {
    public:
      virtual ~IListener() = default;

      virtual void onEnqueued(const Queue& pQueue) = 0;
    };

    using Queueable = IQueueable<QueueKey>;

    Queue() = default;
    Queue(Queue&&) = default;
    Queue& operator=(Queue&&) = default;
    ~Queue() = default;

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void enqueue(std::unique_ptr<Queueable>);
    std::unique_ptr<Queueable> dequeue();
    size_t size() const;

    void addListener(IListener* pListener);
    void removeListener(IListener* pListener);

  private:
    mutable std::mutex m_mutex;
    std::queue<std::unique_ptr<Queueable>> m_queue;
    std::vector<IListener*> m_listeners;
  };
}

template <typename QueueKey>
void mq::Queue<QueueKey>::enqueue(std::unique_ptr<Queueable> pQueueable)
{
  std::vector<IListener*> listeners;
  {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.emplace(std::move(pQueueable));
    listeners = m_listeners;
  }

  for (const auto& pListener : listeners)
    pListener->onEnqueued(*this);
}

template <typename QueueKey>
std::unique_ptr<typename mq::Queue<QueueKey>::Queueable> mq::Queue<QueueKey>::dequeue()
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queue.empty())
    return{};

  auto result = std::move(m_queue.front());
  m_queue.pop();
  return result;
}

template <typename QueueKey>
size_t mq::Queue<QueueKey>::size() const
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  return m_queue.size();
}

template <typename QueueKey>
void mq::Queue<QueueKey>::addListener(IListener* pListener)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto it = std::find(m_listeners.begin(), m_listeners.end(), pListener);
  if (it != m_listeners.end())
    return;

  m_listeners.emplace_back(pListener);
}

template <typename QueueKey>
void mq::Queue<QueueKey>::removeListener(IListener* pListener)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto it = std::find(m_listeners.begin(), m_listeners.end(), pListener);
  m_listeners.erase(it);
}