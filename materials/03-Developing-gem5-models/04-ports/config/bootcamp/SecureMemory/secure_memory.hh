#ifndef __BOOTCAMP_SECURE_MEMORY_SECURE_MEMORY_HH__
#define __BOOTCAMP_SECURE_MEMORY_SECURE_MEMORY_HH__

#include "params/SecureMemory.hh"
#include "sim/clocked_object.hh"

//26
#include "mem/packet.hh"
#include "mem/port.hh"
#include <queue>
#include "sim/eventq.hh"

//Sats
#include "base/statistics.hh"
#include "base/stats/group.hh"

//Defines (80)
#define ARITY 8
#define BLOCK_SIZE 64
#define HMAC_SIZE 8
#define PAGE_SIZE 4096


namespace gem5
{
class SecureMemory : public ClockedObject
{
  private:
    int bufferEntries;
    int responseBufferEntries;

    //48
    EventFunctionWrapper nextReqSendEvent;
    void processNextReqSendEvent();
    //End 48
    //49
    void scheduleNextReqSendEvent(Tick when);
    //End 49
    //54
    EventFunctionWrapper nextReqRetryEvent;
    void processNextReqRetryEvent();
    void scheduleNextReqRetryEvent(Tick when);
    //end54
    //59
    void recvReqRetry();
    //end59
    //extra
    Tick align(Tick when);
    //Extra 2
    AddrRangeList ranges;


    //80
    std::deque<uint64_t> integrity_levels;

    // variables to help refer to certain metadata types
    int root_level = 1;
    int hmac_level = 0;
    int data_level; // set after object construction in setup()
    int counter_level; // set after object construction in setup()
    // structures to know what is currently pending authentication, etc
    std::set<uint64_t> pending_tree_authentication;
    // a bit of a misnomer, we'll use this for hmacs so all tree nodes can go to pending_authentications
    std::set<uint64_t> pending_hmac;
    // fetched but not verified OR writes waiting for path to update
    std::set<PacketPtr> pending_untrusted_packets;

    // secure memory functions
    uint64_t getHmacAddr(uint64_t child_addr); // fetch address of the hmac for somed data
    uint64_t getParentAddr(uint64_t child_addr); // fetch parent node in the tree
    void verifyChildren(PacketPtr parent); // remove children from pending untrusted once trusted
    bool handleResponse(PacketPtr pkt) ;
    bool handleRequest(PacketPtr pkt);

    //end80


    struct SecureMemoryStats: public statistics::Group
    {
        statistics::Scalar totalbufferLatency;
        statistics::Scalar numRequestsFwded;
        statistics::Scalar totalResponseBufferLatency;
        statistics::Scalar numResponsesFwded;
        SecureMemoryStats(SecureMemory* secure_memory);
    };
    SecureMemoryStats stats;


    //27
    class CPUSidePort: public ResponsePort
    {
      private:
        SecureMemory* owner;
        bool needToSendRetry;
        PacketPtr blockedPacket;

      public:
        CPUSidePort(SecureMemory* owner, const std::string& name):
            ResponsePort(name), owner(owner), needToSendRetry(false), blockedPacket(nullptr)
        {}
        bool needRetry() const { return needToSendRetry; }
        bool blocked() const { return blockedPacket != nullptr; }
        void sendPacket(PacketPtr pkt);

        virtual AddrRangeList getAddrRanges() const override;
        virtual bool recvTimingReq(PacketPtr pkt) override;
        virtual Tick recvAtomic(PacketPtr pkt) override;
        virtual void recvFunctional(PacketPtr pkt) override;
        virtual void recvRespRetry() override;
    };
    //End 27

    //29
    class MemSidePort: public RequestPort
    {
      private:
        SecureMemory* owner;
        bool needToSendRetry;
        PacketPtr blockedPacket;

      public:
        MemSidePort(SecureMemory* owner, const std::string& name):
            RequestPort(name), owner(owner), needToSendRetry(false), blockedPacket(nullptr)
        {}
        bool needRetry() const { return needToSendRetry; }
        bool blocked() const { return blockedPacket != nullptr; }
        void sendPacket(PacketPtr pkt);

        virtual bool recvTimingResp(PacketPtr pkt) override;
        virtual void recvReqRetry() override;

    };
    //End 29

    //42
    private:
    template<typename T>
    class TimedQueue
    {
      private:
        Tick latency;

        std::queue<T> items;
        std::queue<Tick> insertionTimes;

      public:
        TimedQueue(Tick latency): latency(latency) {} //75

        Tick frontTime() { return insertionTimes.front(); }

        void push(T item, Tick insertion_time) {
            items.push(item);
            insertionTimes.push(insertion_time);
        }
        void pop() {
            items.pop();
            insertionTimes.pop();
        }

        T& front() { return items.front(); }
        bool empty() const { return items.empty(); }
        size_t size() const { return items.size(); }
        bool hasReady(Tick current_time) const {
            if (empty()) {
                return false;
            }
            return (current_time - insertionTimes.front()) >= latency;
        }
        Tick firstReadyTime() { return insertionTimes.front() + latency; }
    };
    //end 42

    TimedQueue<PacketPtr> buffer;

     //61
    TimedQueue<PacketPtr> responseBuffer;
    EventFunctionWrapper nextRespSendEvent;
    EventFunctionWrapper nextRespRetryEvent;
    void processNextRespSendEvent();
    void scheduleNextRespSendEvent(Tick when);
    void processNextRespRetryEvent();
    void scheduleNextRespRetryEvent(Tick when);
    void recvRespRetry();
    //end61

    AddrRangeList getAddrRanges() const;
    bool recvTimingReq(PacketPtr pkt);
    Tick recvAtomic(PacketPtr pkt);
    void recvFunctional(PacketPtr pkt);


    //31
    CPUSidePort cpuSidePort;
    MemSidePort memSidePort;
    //End 31

  public:
    SecureMemory(const SecureMemoryParams& params);
    //33
    virtual Port& getPort(const std::string& if_name, PortID idxInvalidPortID);
    //end 33
    bool recvTimingResp(PacketPtr pkt); //61
    virtual void init() override; //64

};


} // namespace gem5

#endif // __BOOTCAMP_SECURE_MEMORY_SECURE_MEMORY_HH__
