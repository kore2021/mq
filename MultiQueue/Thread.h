#pragma once

#include "IThreadable.h"

#include <atomic>
#include <cstddef>
#include <chrono>
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

  constexpr auto c_IdleTimeout = std::chrono::milliseconds(1);

  class Thread
  {
  public:
    Thread() = default;
    ~Thread();

    void run();
    void shutdown();
    void flush();

    void attach(ThreadableId id, std::weak_ptr<IThreadable> pThreadable);
    void detach(ThreadableId id);
    std::size_t getUtilization() const;

    void setThreadableEvent(ThreadableId id);
 
  private:
    mutable std::mutex m_mutex;
    std::atomic<bool> m_running = true;
    std::atomic_flag m_processing = ATOMIC_FLAG_INIT;
    std::unique_ptr<std::thread> m_pThread;

    std::set<ThreadableId> m_events;
    std::map<ThreadableId, std::weak_ptr<IThreadable>> m_tasks;
  };
}
