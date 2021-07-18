#pragma once

#include "IConsumer.h"
#include "IThreadable.h"
#include "Queue.h"

#include <memory>


namespace mq
{
  class QueueConsumer : public IThreadable
  {
  public:
    QueueConsumer(std::weak_ptr<Queue> pQueue, std::weak_ptr<IConsumer> pConsumer);
    ~QueueConsumer() override;

    // IThreadable
    void async() override;
    std::size_t capacity() const override;

  private:
    std::weak_ptr<Queue> m_pQueue;
    std::weak_ptr<IConsumer> m_pConsumer;
  };
}


mq::QueueConsumer::QueueConsumer(std::weak_ptr<Queue> pQueue, std::weak_ptr<IConsumer> pConsumer) :
  m_pQueue(pQueue),
  m_pConsumer(pConsumer)
{

}

mq::QueueConsumer::~QueueConsumer() = default;

void mq::QueueConsumer::async()
{
  if (auto pQueue = m_pQueue.lock())
    if (auto pConsumer = m_pConsumer.lock())
      if (auto pQueueable = pQueue->dequeue())
        pConsumer->consume(*pQueueable);
}

std::size_t mq::QueueConsumer::capacity() const
{
  if (auto pQueue = m_pQueue.lock())
    return pQueue->size();

  return 0;
}