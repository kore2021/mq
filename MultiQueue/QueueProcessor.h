#pragma once

#include "IConsumer.h"
#include "Queue.h"
#include "QueueThread.h"

#include <exception>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>


namespace mq
{
  template <typename QueueKey>
  class QueueProcessor
  {
  public:
    QueueProcessor(size_t threadCount);
    ~QueueProcessor();

    unsigned int threadCount() const;

    void addQueue(const QueueKey& key);
    void removeQueue(const QueueKey& key);
    std::shared_ptr<Queue> getQueue(const QueueKey& key);

    void setConsumer(const QueueKey& key, std::weak_ptr<IConsumer> pConsumer);

    void run();
    void shutdown();
    void flush();

  private:
    const size_t m_threadCount = 0;
    size_t getUnderutilizedThreadId() const;

    struct QueueWithConsumer
    {
      std::shared_ptr<Queue> pQueue;
      std::weak_ptr<IConsumer> pConsumer;
      std::weak_ptr<QueueThread> pConsumerThread;
    };

    std::mutex m_queueMutex;
    std::unordered_map<QueueKey, QueueWithConsumer> m_queues;
    std::vector<std::shared_ptr<QueueThread>> m_threads;
  };
}

template <typename QueueKey>
mq::QueueProcessor<QueueKey>::QueueProcessor(size_t threadCount) : m_threadCount(threadCount)
{

}

template <typename QueueKey>
mq::QueueProcessor<QueueKey>::~QueueProcessor()
{
  shutdown();
}

template <typename QueueKey>
unsigned int mq::QueueProcessor<QueueKey>::threadCount() const
{
  return m_threadCount;
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::addQueue(const QueueKey& key)
{
  const std::lock_guard<std::mutex> lock(m_queueMutex);
  if (m_queues.count(key))
    return;

  m_queues.emplace(key, QueueWithConsumer{ std::make_shared<Queue>(), {} });
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::removeQueue(const QueueKey& key)
{
  const std::lock_guard<std::mutex> lock(m_queueMutex);
  m_queues.erase(key);
}

template <typename QueueKey>
std::shared_ptr<mq::Queue> mq::QueueProcessor<QueueKey>::getQueue(const QueueKey& key)
{
  const std::lock_guard<std::mutex> lock(m_queueMutex);
  auto it = m_queues.find(key);
  if (it == m_queues.end())
    return{};

  return it->second.pQueue;
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::setConsumer(const QueueKey& key, std::weak_ptr<IConsumer> pConsumer)
{
  const std::lock_guard<std::mutex> lock(m_queueMutex);
  auto it = m_queues.find(key);
  if (it == m_queues.end())
    return;

  if (auto pThread = it->second.pConsumerThread.lock())
    pThread->detach(*it->second.pQueue);

  it->second.pConsumer = pConsumer;

  if (!m_threads.empty())
  {
    const auto threadId = getUnderutilizedThreadId();
    auto& pThread = m_threads[threadId];
    pThread->attach(*it->second.pQueue, it->second.pConsumer);
    it->second.pConsumerThread = pThread;
  }
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::run()
{
  const std::lock_guard<std::mutex> lock(m_queueMutex);
  if (!m_threads.empty())
    throw std::exception();

  m_threads.resize(m_threadCount);
  for (size_t i = 0; i < m_threadCount; ++i)
    m_threads[i] = std::make_shared<QueueThread>();

  size_t threadIndex = 0;
  for (auto& queue : m_queues)
  {
    auto& pThread = m_threads[(threadIndex++) % m_threadCount];
    pThread->attach(*queue.second.pQueue, queue.second.pConsumer);
    queue.second.pConsumerThread = pThread;
  }

  for (auto& pThread : m_threads)
    pThread->run();
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::shutdown()
{
  const std::lock_guard<std::mutex> lock(m_queueMutex);
  for (const auto& pThread : m_threads)
    pThread->shutdown();

  for (auto& queue : m_queues)
    queue.second.pConsumerThread.reset();

  m_threads.clear();
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::flush()
{
  const std::lock_guard<std::mutex> lock(m_queueMutex);
  for (const auto& pThread : m_threads)
    pThread->flush();
}

template <typename QueueKey>
size_t mq::QueueProcessor<QueueKey>::getUnderutilizedThreadId() const
{
  auto minCapacity = std::numeric_limits<size_t>::max();
  size_t result = 0;
  for (size_t i = 0, ie = m_threads.size(); i < ie; ++i)
  {
    const auto capacity = m_threads[i]->capacity();
    if (capacity < minCapacity)
    {
      minCapacity = capacity;
      result = i;
    }
  }

  return result;
}