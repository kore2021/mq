#include "Thread.h"

#include "IThreadable.h"

#include <vector>

using mq::IThreadable;
using mq::Thread;
using mq::ThreadableId;


Thread::~Thread()
{
  shutdown();
}

void Thread::run()
{
  if (m_pThread)
    return;

  m_pThread = std::make_unique<std::thread>(
    [&]() {
      auto tasks = std::vector<std::pair<ThreadableId, std::weak_ptr<IThreadable>>>{};
      while (m_running)
      {
        tasks.clear();
        {
          std::unique_lock<std::mutex> lock(m_mutex);
          m_eventQuueeed.wait(lock, [&]() { return !m_running || !m_events.empty(); });
          for (const auto id : m_events)
          {
            auto it = m_tasks.find(id);
            if (it != m_tasks.end())
              tasks.emplace_back(id, it->second);
          }

          // we consume one unit of capacity per dequeuing, so we need to save a triggered state
          m_events.clear();
          for (const auto& task : tasks)
            if (auto pThreadable = task.second.lock())
              if (pThreadable->workload() > 1)
                m_events.insert(task.first);
        }
       
        for (const auto& pQueue : tasks)
          if (auto pThreadable = pQueue.second.lock())
            pThreadable->async();

        m_eventDequeued.notify_all();
      }
    });
}

void Thread::shutdown()
{
  if (!m_pThread)
    return;

  m_running = false;
  m_eventQuueeed.notify_all();
  m_pThread->join();
  m_pThread.reset();
}

void Thread::flush()
{
  std::unique_lock<std::mutex> lock(m_mutex);
  m_eventDequeued.wait(lock, [&]() { return !m_running || m_events.empty(); });
}

void Thread::attach(ThreadableId id, std::weak_ptr<IThreadable> pThreadable)
{
  {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_tasks[id] = pThreadable;
    m_events.insert(id);
  }
  m_eventQuueeed.notify_all();
}

void Thread::detach(ThreadableId id)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_tasks.erase(id);
  m_events.erase(id);
}

std::size_t Thread::getUtilization() const
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  return m_tasks.size();
}

void Thread::setThreadableEvent(ThreadableId id)
{
  {
    const std::lock_guard<std::mutex> lock(m_mutex);
    m_events.insert(id);
  }
  m_eventQuueeed.notify_all();
}