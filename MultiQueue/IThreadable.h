#pragma once

#include <cstddef>


namespace mq
{
  using ThreadableId = std::size_t;

  class IThreadable
  {
  public:
    virtual ~IThreadable() = default;

    virtual void async() = 0;
    virtual std::size_t capacity() const = 0;
  };
}