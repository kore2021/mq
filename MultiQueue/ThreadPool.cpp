#include "ThreadPool.h"

using mq::Thread;
using mq::ThreadId;
using mq::ThreadPool;


ThreadPool::ThreadPool(std::size_t capacity) : m_capacity(capacity)
{

}

ThreadPool::~ThreadPool()
{
  shutdown();
}

std::shared_ptr<Thread> mq::ThreadPool::getThread(ThreadId threadId) const
{
  return m_threads.at(threadId);
}

std::pair<std::shared_ptr<Thread>, mq::ThreadId> mq::ThreadPool::getThread() const
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  if (m_threads.empty())
    return{};

  const auto threadId = getUnderutilizedThreadId();
  return{ m_threads.at(threadId), threadId };
}

bool ThreadPool::empty() const
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  return m_threads.empty();
}

void ThreadPool::run()
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

void ThreadPool::shutdown()
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto& pThread : m_threads)
    pThread->shutdown();

  m_threads.clear();
}

void ThreadPool::flush()
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  for (const auto& pThread : m_threads)
    pThread->flush();
}

ThreadId ThreadPool::getUnderutilizedThreadId() const
{
  auto minUtilization = std::numeric_limits<std::size_t>::max();
  auto threadId = ThreadId{ 0 };
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