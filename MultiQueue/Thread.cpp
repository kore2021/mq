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
          const std::lock_guard<std::mutex> lock(m_mutex);
          for (const auto id : m_events)
          {
            auto it = m_tasks.find(id);
            if (it != m_tasks.end())
              tasks.emplace_back(id, it->second);
          }

          // we consume one unit of capacity per dequeuing, so we need to save an evented state
          m_events.clear();
          for (const auto& task : tasks)
            if (auto pThreadable = task.second.lock())
              if (pThreadable->capacity() > 1)
                m_events.insert(task.first);

          if (!tasks.empty())
            m_processing.test_and_set();
        }
       
        for (const auto& pQueue : tasks)
          if (auto pThreadable = pQueue.second.lock())
            pThreadable->async();

        if (!tasks.empty())
          m_processing.clear();

        if (tasks.empty())
          std::this_thread::sleep_for(c_IdleTimeout);
        else
          std::this_thread::yield();
      }
    });
}

void Thread::shutdown()
{
  if (!m_pThread)
    return;

  m_running = false;
  m_pThread->join();
  m_pThread.reset();
}

void Thread::flush()
{
  size_t events = 0;
  do
  {
    {
      const std::lock_guard<std::mutex> lock(m_mutex);
      events = m_events.size();
      while (m_processing.test_and_set())
        std::this_thread::sleep_for(c_IdleTimeout);
      m_processing.clear();
    }

    if (events)
      std::this_thread::sleep_for(c_IdleTimeout);

  } while (events);
}

void Thread::attach(ThreadableId id, std::weak_ptr<IThreadable> pThreadable)
{
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_tasks[id] = pThreadable;
  m_events.insert(id);
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
  const std::lock_guard<std::mutex> lock(m_mutex);
  m_events.insert(id);
}