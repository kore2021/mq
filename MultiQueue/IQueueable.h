#pragma once


namespace mq
{
  // The interface of an object to store into a queue
  class IQueueable
  {
  public:
    virtual ~IQueueable() = default;
  };
}