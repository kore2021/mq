#pragma once


namespace mq
{
  class IQueueable;
}

namespace mq
{
  // The interface of a queue consumer that consumes IQueueable
  class IConsumer
  {
  public:
    virtual ~IConsumer() = default;

    virtual void consume(const IQueueable& value) = 0;
  };
}