#pragma once


namespace mq
{
  class IQueueable;
}

namespace mq
{
  class IConsumer
  {
  public:
    virtual ~IConsumer() = default;

    virtual void consume(const IQueueable& value) = 0;
  };
}