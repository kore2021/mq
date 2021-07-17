#pragma once

#include "Queue.h"

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <utility>


namespace mq
{
  constexpr auto c_IdleTimeout = std::chrono::milliseconds(1);

  class QueueThread : public Queue::IListener
  {
  public:
    QueueThread() = default;
    ~QueueThread() override;

    void run();
    void shutdown();
    void flush();

    void attach(Queue& queue, std::weak_ptr<IConsumer> pConsumer);
    void detach(Queue& queue);
    size_t capacity() const;

    // Queue::IListener
    void onEnqueued(const Queue&) override;
 
  private:
    mutable std::mutex m_mutex;
    std::atomic<bool> m_running = true;
    std::atomic_flag m_dequeuing = ATOMIC_FLAG_INIT;
    std::unique_ptr<std::thread> m_pThread;

    std::set<const Queue*> m_enqueued;
    std::map<Queue*, std::weak_ptr<IConsumer>> m_consumers;
  };
}

mq::QueueThread::~QueueThread()
{
  shutdown();
}

void mq::QueueThread::run()
{
  if (m_pThread)
    return;

  m_pThread = std::make_unique<std::thread>(
    [&]() {
      auto enqueued = std::vector<std::pair<Queue*, std::weak_ptr<IConsumer>>>{};
      while (m_running)
      {
        enqueued.clear();
        {
          const std::lock_guard<std::mutex> lock(m_mutex);
          for (const auto pQueue : m_enqueued)
          {
            auto it = m_consumers.find(const_cast<Queue*>(pQueue));
            if (it != m_consumers.end())
              enqueued.emplace_back(const_cast<Queue*>(pQueue), it->second);
          }

          // we have to clear only one-element queue, because we consume one element per dequeuing
          m_enqueued.clear();
          for (const auto& queue : enqueued)
            if (queue.first->size() > 1)
              m_enqueued.insert(queue.first);

          if (!enqueued.empty())
            m_dequeuing.test_and_set();
        }
       
        for (const auto& queue : enqueued)
          if (auto pConsumer = queue.second.lock())
            if (auto pQueueable = queue.first->dequeue())
              pConsumer->consume(*pQueueable);

        if (!enqueued.empty())
          m_dequeuing.clear();

        if (enqueued.empty())
          std::this_thread::sleep_for(c_IdleTimeout);
        else
          std::this_thread::yield();
      }
    });
}

void mq::QueueThread::shutdown()
{
  if (!m_pThread)
    return;

  m_running = false;
  m_pThread->join();
  m_pThread.reset();
}

void mq::QueueThread::flush()
{
  size_t enqueued = 0;
  do
  {
    {
      const std::lock_guard<std::mutex> lock(m_mutex);
      enqueued = m_enqueued.size();
      while (m_dequeuing.test_and_set())
        std::this_thread::sleep_for(c_IdleTimeout);
      m_dequeuing.clear();
    }

    if (enqueued)
      std::this_thread::sleep_for(c_IdleTimeout);

  } while (enqueued);
}

void mq::QueueThread::attach(Queue& queue, std::weak_ptr<IConsumer> pConsumer)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_consumers[&queue] = pConsumer;
  m_enqueued.insert(&queue);
  queue.addListener(this);
}

void mq::QueueThread::detach(Queue& queue)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  queue.removeListener(this);
  m_enqueued.erase(&queue);
  m_consumers.erase(&queue);
}

size_t mq::QueueThread::capacity() const
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  return m_consumers.size();
}

void mq::QueueThread::onEnqueued(const Queue& queue)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_enqueued.insert(&queue);
}