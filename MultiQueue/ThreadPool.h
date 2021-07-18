#pragma once

#include "Thread.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <utility>


namespace mq
{
  class ThreadPool
  {
  public:
    ThreadPool(std::size_t capacity);
    ~ThreadPool();

    void run();
    void shutdown();
    void flush();

    std::pair<std::shared_ptr<Thread>, ThreadId> getThread() const;
    std::shared_ptr<Thread> getThread(ThreadId threadId) const;
    bool empty() const;

  private:
    ThreadId getUnderutilizedThreadId() const;

    mutable std::mutex m_mutex;
    std::size_t m_capacity;
    std::vector<std::shared_ptr<Thread>> m_threads;
  };
}

mq::ThreadPool::ThreadPool(std::size_t capacity) : m_capacity(capacity)
{

}

mq::ThreadPool::~ThreadPool()
{
  shutdown();
}

std::shared_ptr<mq::Thread> mq::ThreadPool::getThread(ThreadId threadId) const
{
  return m_threads.at(threadId);
}

std::pair<std::shared_ptr<mq::Thread>, mq::ThreadId> mq::ThreadPool::getThread() const
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (m_threads.empty())
    return{};

  const auto threadId = getUnderutilizedThreadId();
  return{ m_threads.at(threadId), threadId };
}

bool mq::ThreadPool::empty() const
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  return m_threads.empty();
}

void mq::ThreadPool::run()
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (!m_threads.empty())
    throw std::exception();

  m_threads.resize(m_capacity);
  for (auto& pThread : m_threads)
  {
    pThread = std::make_shared<Thread>();
    pThread->run();
  }
}

void mq::ThreadPool::shutdown()
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto& pThread : m_threads)
    pThread->shutdown();

  m_threads.clear();
}

void mq::ThreadPool::flush()
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto& pThread : m_threads)
    pThread->flush();
}

mq::ThreadId mq::ThreadPool::getUnderutilizedThreadId() const
{
  auto minUtilization = std::numeric_limits<std::size_t>::max();
  auto threadId = mq::ThreadId{ 0 };
  for (std::size_t i = 0, ie = m_threads.size(); i < ie; ++i)
  {
    const auto utilization = m_threads[i]->getUtilization();
    if (utilization == 0)
      return i;

    if (utilization < minUtilization)
    {
      minUtilization = utilization;
      threadId = i;
    }
  }

  return threadId;
}