#include "QueueConsumer.h"

#include "IConsumer.h"
#include "Queue.h"

using mq::IConsumer;
using mq::Queue;
using mq::QueueConsumer;


QueueConsumer::QueueConsumer(std::weak_ptr<Queue> pQueue, std::weak_ptr<IConsumer> pConsumer) :
  m_pQueue(pQueue),
  m_pConsumer(pConsumer)
{

}

QueueConsumer::~QueueConsumer() = default;

void QueueConsumer::async()
{
  if (auto pQueue = m_pQueue.lock())
    if (auto pConsumer = m_pConsumer.lock())
      if (auto pQueueable = pQueue->dequeue())
        pConsumer->consume(*pQueueable);
}

std::size_t QueueConsumer::workload() const
{
  if (auto pQueue = m_pQueue.lock())
    return pQueue->size();

  return 0;
}