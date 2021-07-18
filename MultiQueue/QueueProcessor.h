#pragma once

#include "IConsumer.h"
#include "Queue.h"
#include "QueueConsumer.h"
#include "QueueEventAdapter.h"
#include "ThreadPool.h"

#include <cstddef>
#include <exception>
#include <map>
#include <memory>
#include <mutex>
#include <utility>


namespace mq
{
  // The class uses ThreadPool with several threads for queue multi-processing
  template <typename QueueKey>
  class QueueProcessor
  {
  public:
    // It allows to create a processor with launched threads
    QueueProcessor(std::size_t threadCount = 0);
    ~QueueProcessor();

    // These functions allow to control queues
    void addQueue(const QueueKey& key);
    void removeQueue(const QueueKey& key);

    // A client is able to add elements to a queue directly
    std::shared_ptr<Queue> getQueue(const QueueKey& key);

    // Flush queues if you need to wait until all elements are processed
    void flushQueues();

    // Assign a consumer for an existent or non-existent queue
    void setConsumer(const QueueKey& key, std::weak_ptr<IConsumer> pConsumer);

    // Connect queue and consumers to threads
    void connect(std::size_t threadCount);
    // Connect a shared thread pool
    void connect(std::shared_ptr<ThreadPool> pThreadPool);
    // Disconnect the current thread pool
    void disconnect();
  private:
    // A thread will get a new task if queue and consumer exist
    void connectQueueWithConsumerToThread(const QueueKey& key);

    // This structure holds a connection
    struct Connection
    {
      ThreadId threadId = 0;
      std::unique_ptr<QueueEventAdapter> pEventAdapter;
      std::shared_ptr<QueueConsumer> pConsumer;

      Connection(ThreadId threadId, std::unique_ptr<QueueEventAdapter> pEventAdapter, std::shared_ptr<QueueConsumer> pConsumer) :
        threadId(threadId),
        pEventAdapter(std::move(pEventAdapter)),
        pConsumer(pConsumer) {}
    };

    std::mutex m_mutex;
    std::shared_ptr<ThreadPool> m_pThreadPool;
    std::map<QueueKey, std::weak_ptr<IConsumer>> m_consumers;
    std::map<QueueKey, std::shared_ptr<Queue>> m_queues;
    std::map<QueueKey, Connection> m_connections;
    ThreadableId m_lastThreadableId = 0;
  };
}

template <typename QueueKey>
mq::QueueProcessor<QueueKey>::QueueProcessor(std::size_t threadCount)
{
  connect(threadCount);
}

template <typename QueueKey>
mq::QueueProcessor<QueueKey>::~QueueProcessor()
{
  disconnect();
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::addQueue(const QueueKey& key)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (m_queues.count(key))
    return;

  auto emplaced = m_queues.emplace(key, std::make_shared<Queue>());

  if (m_pThreadPool && !m_pThreadPool->empty())
    connectQueueWithConsumerToThread(key);
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::removeQueue(const QueueKey& key)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_connections.find(key);
  if (it != m_queueEventAdapters.end())
  {
    m_threadPool.get(it->second.threadId)->detach(it->second.pAdapter->threadableEventId());
    m_connections.erase(it);
  }

  m_queues.erase(key);
}

template <typename QueueKey>
std::shared_ptr<mq::Queue> mq::QueueProcessor<QueueKey>::getQueue(const QueueKey& key)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_queues.find(key);
  if (it == m_queues.end())
    return{};

  return it->second;
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::flushQueues()
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_pThreadPool)
    return;

  m_pThreadPool->flush();
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::setConsumer(const QueueKey& key, std::weak_ptr<IConsumer> pConsumer)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  auto it = m_connections.find(key);
  if (it != m_connections.end())
  {
    if (!m_pThreadPool)
      throw std::exception();

    m_pThreadPool->getThread(it->second.threadId)->detach(it->second.pEventAdapter->threadableEventId());
    m_connections.erase(it);
  }

  if (pConsumer.expired())
  {
    m_consumers.erase(key);
    return;
  }

  m_consumers[key] = pConsumer;

  if (m_pThreadPool && !m_pThreadPool->empty())
    connectQueueWithConsumerToThread(key);
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::connect(std::size_t threadCount)
{
  if (!threadCount)
    return;

  auto pThreadPool = std::make_shared<ThreadPool>(threadCount);
  pThreadPool->run();
  connect(pThreadPool);
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::connect(std::shared_ptr<ThreadPool> pThreadPool)
{
  disconnect();

  const std::lock_guard<std::mutex> lock(m_mutex);
  m_pThreadPool = pThreadPool;
  if (!m_pThreadPool->empty())
    for (const auto& queue : m_queues)
      connectQueueWithConsumerToThread(queue.first);
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::disconnect()
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_pThreadPool)
    return;

  for (const auto& connection : m_connections)
    m_pThreadPool->getThread(connection.second.threadId)->detach(connection.second.pEventAdapter->threadableEventId());
  m_connections.clear();

  m_pThreadPool.reset();
}

template <typename QueueKey>
void mq::QueueProcessor<QueueKey>::connectQueueWithConsumerToThread(const QueueKey& key)
{
  auto itQueue = m_queues.find(key);
  if (itQueue == m_queues.end())
    return;

  auto itConsumer = m_consumers.find(key);
  if (itConsumer == m_consumers.end())
    return;

  if (m_connections.count(key) || !m_pThreadPool)
    throw std::exception();

  // threadableId allows to have an unique identification of an event source
  const auto thread = m_pThreadPool->getThread();
  const auto threadableId = ++m_lastThreadableId;
  auto pEventAdapter = std::make_unique<QueueEventAdapter>(*itQueue->second, *thread.first, threadableId);
  auto pQueueConsumer = std::make_shared<QueueConsumer>(itQueue->second, itConsumer->second);

  auto connection = Connection{ thread.second, std::move(pEventAdapter), pQueueConsumer };
  thread.first->attach(connection.pEventAdapter->threadableEventId(), pQueueConsumer);
  m_connections.emplace(key, std::move(connection));
}
