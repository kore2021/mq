#include "Queue.h"

#include "IQueueable.h"

#include <algorithm>

using mq::Queue;
using mq::IQueueable;


Queue::~Queue() = default;

void Queue::enqueue(std::unique_ptr<IQueueable> pQueueable)
{
  std::vector<IListener*> listeners;
  {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_queue.emplace(std::move(pQueueable));
    listeners = m_listeners;
  }

  for (const auto& pListener : listeners)
    pListener->onEnqueued();
}

std::unique_ptr<mq::IQueueable> Queue::dequeue()
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queue.empty())
    return{};

  auto result = std::move(m_queue.front());
  m_queue.pop();
  return result;
}

std::size_t Queue::size() const
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  return m_queue.size();
}

void Queue::addListener(IListener* pListener)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto it = std::find(m_listeners.begin(), m_listeners.end(), pListener);
  if (it != m_listeners.end())
    return;

  m_listeners.emplace_back(pListener);
}

void Queue::removeListener(IListener* pListener)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto it = std::find(m_listeners.begin(), m_listeners.end(), pListener);
  m_listeners.erase(it);
}