#pragma once

#include <cstddef>


namespace mq
{
  using ThreadableId = std::size_t;

  // The interface of a thread task
  class IThreadable
  {
  public:
    virtual ~IThreadable() = default;

    virtual void async() = 0;
    virtual std::size_t workload() const = 0;
  };
}