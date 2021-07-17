#pragma once

namespace mq
{
  template <typename QueueKey>
  class IConsumer
  {
  public:
    virtual ~IConsumer() = default;

    virtual void consume(const IQueueable<QueueKey>& value) = 0;
  };
}