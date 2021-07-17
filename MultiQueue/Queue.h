#pragma once

#include "IQueueable.h"

#include <algorithm>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>


namespace mq
{
  class Queue
  {
  public:
    class IListener
    {
    public:
      virtual ~IListener() = default;

      virtual void onEnqueued(const Queue& queue) = 0;
    };

    Queue() = default;
    Queue(Queue&&) = default;
    Queue& operator=(Queue&&) = default;
    ~Queue() = default;

    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    void enqueue(std::unique_ptr<IQueueable>);
    std::unique_ptr<IQueueable> dequeue();
    size_t size() const;

    void addListener(IListener* pListener);
    void removeListener(IListener* pListener);

  private:
    mutable std::mutex m_mutex;
    std::queue<std::unique_ptr<IQueueable>> m_queue;
    std::vector<IListener*> m_listeners;
  };
}

void mq::Queue::enqueue(std::unique_ptr<IQueueable> pQueueable)
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

std::unique_ptr<mq::IQueueable> mq::Queue::dequeue()
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queue.empty())
    return{};

  auto result = std::move(m_queue.front());
  m_queue.pop();
  return result;
}

size_t mq::Queue::size() const
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  return m_queue.size();
}

void mq::Queue::addListener(IListener* pListener)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto it = std::find(m_listeners.begin(), m_listeners.end(), pListener);
  if (it != m_listeners.end())
    return;

  m_listeners.emplace_back(pListener);
}

void mq::Queue::removeListener(IListener* pListener)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto it = std::find(m_listeners.begin(), m_listeners.end(), pListener);
  m_listeners.erase(it);
}