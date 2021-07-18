#include <iostream>

#include "MultiQueue/IQueueable.h"
#include "MultiQueue/QueueProcessor.h"

#include <cmath>
#include <cstddef>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <utility>


using namespace std;

class QueueableDeal;
using QueueKey = string;
using QueueValueOwner = string;
using QueueValueCountable = int;
using QueueValue = tuple<QueueKey, QueueValueOwner, QueueValueCountable>;
using QueueProcessor = mq::QueueProcessor<QueueKey>;
using Producer = thread;
using ProducerStorage = map<string, std::unique_ptr<Producer>>;
using ConsumerStorage = map<string, std::shared_ptr<mq::IConsumer>>;
using DepositoryRecord = pair<string, QueueValue>;
using Depository = pair<mutex, vector<DepositoryRecord>>;
using Accountant = map<QueueKey, QueueValueCountable>;

ostream& operator<< (std::ostream& ostream, const QueueValue& value);
ostream& operator<< (std::ostream& ostream, const DepositoryRecord& value);

void addProducer(QueueProcessor& processor,const string& name, const vector<pair<string, size_t>>& wishlist, std::chrono::milliseconds latency,
  ProducerStorage& storage, Accountant& accountant);
void addBroker(QueueProcessor& processor, const string& name, const set<string>& capabilities, std::chrono::milliseconds latency, Depository& depo,
  ConsumerStorage& storage);

void printRecords(Depository& depo);
bool checkDepository(const Accountant& accountant, Depository& depo);

int main()
{
  cout << "MultiQueue test cases" << endl;
  Depository depository;

  QueueProcessor processor(4);
  processor.addQueue("RUB");
  processor.addQueue("USD");
  processor.addQueue("EUR");
  processor.connect(2);

  auto accountant = Accountant{};
  auto producers = ProducerStorage{};
  addProducer(processor, "Alice", {
      { "EUR",  4 },
      { "CNY",  1 },
    }, 20ms, producers, accountant);
  addProducer(processor, "Bob",   {
      { "RUB",  1 },
      { "RUB",  5 },
      { "USD", 10 },
    }, 50ms, producers, accountant);
  addProducer(processor, "Clare", {
      { "EUR",  1 },
      { "USD", 10 }
    }, 20ms, producers, accountant);
  auto consumers = ConsumerStorage{};
  addBroker(processor, "T. Rex", { "RUB", "USD" }, 50ms, depository, consumers);
  // There is a example how to replace a consumer on air
  // Timeout allows to demostrate it
  this_thread::sleep_for(100ms);
  addBroker(processor, "Acrocanthosaurus", { "USD" }, 10ms, depository, consumers);
  addBroker(processor, "Torvosaurus", { "EUR" }, 20ms, depository, consumers);

  addProducer(processor, "Dan", {
      { "USD", 11 }
    }, 20ms, producers, accountant);

  // wait for all producers
  for (auto& producer : producers)
    producer.second->join();

  processor.flushQueues();

  processor.connect(4);

  auto zombieProducers = ProducerStorage{};
  for (size_t i = 0; i < 2; ++i)
  {
    const auto name = "Zombie_" + to_string(i);
    addProducer(processor, name, {
      { "USD", 3 }
    }, 25ms, zombieProducers, accountant);
  }

  for (auto& producer : zombieProducers)
    producer.second->join();

  processor.flushQueues();

  // print the result state of depository
  printRecords(depository);

  // This function checks the results are correct
  if (checkDepository(accountant, depository))
    cout << endl << "Everything is fine." << endl;
  else
    cout << endl << "Something has gone wrong." << endl;

  return 0;
}

ostream& operator<< (std::ostream& ostream, const DepositoryRecord& value)
{
  ostream << value.first << "[" << get<0>(value.second) << "]: " << get<1>(value.second) << " - " << get<2>(value.second);
  return ostream;
}

class QueueableDeal : public mq::IQueueable
{
public:
  QueueableDeal(const QueueValue& value) : m_value(value) {}
  ~QueueableDeal() override = default;

  // self
  QueueValue value() const { return m_value; }

private:
  const QueueValue m_value;
};

void addProducer(QueueProcessor& processor, const string& name, const vector<pair<string, size_t>>& wishlist, std::chrono::milliseconds latency,
  ProducerStorage& producers, Accountant& accountant)
{
  for (const auto& wish : wishlist)
    if (wish.second)
      if (auto pQueue = processor.getQueue(wish.first))
        accountant[wish.first] += wish.second;

  producers[name] = std::make_unique<Producer>(
    [name, wishlist, latency, &processor]()
    {
      vector<tuple<std::weak_ptr<mq::Queue>, QueueKey, size_t>> wishes;
      for (const auto& wish : wishlist)
        if (wish.second)
          if (auto pQueue = processor.getQueue(wish.first))
            wishes.emplace_back(pQueue, wish.first, wish.second);

      auto hasDeals = false;
      do
      {
        hasDeals = false;
        for (auto& wish : wishes)
        {
          auto& wishValue = get<2>(wish);
          if (!wishValue)
            continue;

          if (auto pQueue = get<0>(wish).lock())
          {
            hasDeals = true;
            constexpr auto c_unitPerDeal = 2;
            // take only c_unitPerDeal units per a deal
            const auto dealValue = std::min<size_t>(c_unitPerDeal, wishValue);
            const auto& wishName = get<1>(wish);
            auto pDeal = std::make_unique<QueueableDeal>(QueueValue(wishName, name, dealValue));
            pQueue->enqueue(move(pDeal));
            wishValue -= dealValue;
          }
          else
          {
            wishValue = 0;
            continue;
          }
        }
        this_thread::sleep_for(latency);
      } while(hasDeals);
    });
}

void addBroker(QueueProcessor& processor, const string& name, const set<string>& capabilities, std::chrono::milliseconds latency, Depository& depo, ConsumerStorage& consumers)
{
  class DepositoryConsumer : public mq::IConsumer
  {
  public:
    DepositoryConsumer(const string name, std::chrono::milliseconds latency, Depository& depository) :
      m_name(name),
      m_latency(latency),
      m_depository(depository)
    {
    }

    // IConsumer
    void consume(const mq::IQueueable& value) override
    {
      {
        const auto& deal = dynamic_cast<const QueueableDeal&>(value);
        const std::lock_guard<std::mutex> lock(m_depository.first);
        m_depository.second.emplace_back(m_name, deal.value());
      }

      this_thread::sleep_for(m_latency);
    }

  private:
    const string m_name;
    const std::chrono::milliseconds m_latency;
    Depository& m_depository;
  };

  auto pConsumer = std::make_shared<DepositoryConsumer>(name, latency, depo);
  for (const auto& capability : capabilities)
    processor.setConsumer(capability, pConsumer);

  consumers[name] = pConsumer;
}

void printRecords(Depository& depo)
{
  const std::lock_guard<std::mutex> lock(depo.first);
  size_t index = 0;
  for (const auto& record : depo.second)
    cout << "\t" << index++ << ") " << record << endl;
}

bool checkDepository(const Accountant& accountant, Depository& depo)
{
  auto depoAccountant = Accountant{};
  const std::lock_guard<std::mutex> lock(depo.first);
  for (const auto& record : depo.second)
    depoAccountant[get<0>(record.second)] += get<2>(record.second);
  if (depoAccountant == accountant)
    return true;

  for (const auto& record : accountant)
    depoAccountant[record.first] -= record.second;

  cout << ">>> There are violations << " << endl;
  for (const auto& record : depoAccountant)
    cout << "[" << record.first << "]: " << record.second << endl;
  return false;
}