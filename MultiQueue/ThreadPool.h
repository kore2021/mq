#pragma once

#include "Thread.h"

#include <cstddef>
#include <exception>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>


namespace mq
{
  // The class is a storage of threads
  class ThreadPool
  {
  public:
    ThreadPool(std::size_t capacity);
    ~ThreadPool();

    // Run N threads, where N equals to the capacity
    void run();
    // Shutdown all threads
    void shutdown();
    // Flush accumulated tasks
    void flush();

    // Get a thread to work with it
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
