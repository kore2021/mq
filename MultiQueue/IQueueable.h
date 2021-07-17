#pragma once


namespace mq
{
  template <typename QueueKey>
  class IQueueable
  {
  public:
    virtual ~IQueueable() = default;

    virtual QueueKey queueKey() const = 0;
  };
}