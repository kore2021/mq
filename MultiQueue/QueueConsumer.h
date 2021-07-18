#pragma once

#include "IThreadable.h"

#include <memory>


namespace mq
{
  class IConsumer;
  class Queue;
}

namespace mq
{
  // The class connects a queue with its consumer
  class QueueConsumer : public IThreadable
  {
  public:
    QueueConsumer(std::weak_ptr<Queue> pQueue, std::weak_ptr<IConsumer> pConsumer);
    ~QueueConsumer() override;

    // IThreadable
    void async() override;
    std::size_t workload() const override;

  private:
    std::weak_ptr<Queue> m_pQueue;
    std::weak_ptr<IConsumer> m_pConsumer;
  };
}
