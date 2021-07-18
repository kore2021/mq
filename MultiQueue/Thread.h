#pragma once

#include "IThreadable.h"

#include <atomic>
#include <cstddef>
#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <thread>
#include <utility>


namespace mq
{
  class IThreadable;
}

namespace mq
{
  using ThreadId = std::size_t;

  // There is a thread abstraction for IThreadable processing
  class Thread
  {
  public:
    Thread() = default;
    ~Thread();

    void run();
    void shutdown();
    void flush();

    // Assign multiple IThreadable for the thread
    void attach(ThreadableId id, std::weak_ptr<IThreadable> pThreadable);
    void detach(ThreadableId id);
    // Return estimated utilization 
    std::size_t getUtilization() const;

    // Notify that IThreadable should be processed
    void setThreadableEvent(ThreadableId id);
 
  private:
    mutable std::mutex m_mutex;
    std::atomic<bool> m_running = true;
    std::condition_variable m_eventQuueeed;
    std::condition_variable m_eventDequeued;
    std::unique_ptr<std::thread> m_pThread;

    std::set<ThreadableId> m_events;
    std::map<ThreadableId, std::weak_ptr<IThreadable>> m_tasks;
  };
}
