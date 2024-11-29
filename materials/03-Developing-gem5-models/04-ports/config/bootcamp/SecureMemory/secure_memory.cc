#include "bootcamp/secure-memory/secure_memory.hh"

#include "debug/SecureMemory.hh"

namespace gem5{

//Stats
SecureMemory::SecureMemoryStats::SecureMemoryStats(SecureMemory* secure_memory):
    statistics::Group(secure_memory),
    ADD_STAT(totalbufferLatency, statistics::units::Tick::get(), "Total inspection buffer latency."),
    ADD_STAT(numRequestsFwded, statistics::units::Count::get(), "Number of requests forwarded."),
    ADD_STAT(totalResponseBufferLatency, statistics::units::Tick::get(), "Total response buffer latency."),
    ADD_STAT(numResponsesFwded, statistics::units::Count::get(), "Number of responses forwarded.")
{}


//Get Port
Port& SecureMemory::getPort(const std::string &if_name, PortID idx) {
    if (if_name == "cpu_side_port") {
        return cpuSidePort;
    } else if (if_name == "mem_side_port") {
        return memSidePort;
    } else {
        return ClockedObject::getPort(if_name, idx);
    }
}


//CPU Side Port
Tick
SecureMemory::CPUSidePort::recvAtomic(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Received pkt: %s in atomic mode.\n", __func__, pkt->print());
    return owner->recvAtomic(pkt);
}

void
SecureMemory::CPUSidePort::recvFunctional(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Received pkt: %s in functional mode.\n", __func__, pkt->print());
    owner->recvFunctional(pkt);
}

AddrRangeList
SecureMemory::CPUSidePort::getAddrRanges() const
{
    return owner->getAddrRanges();
}

bool
SecureMemory::recvTimingReq(PacketPtr pkt)
{
    uint64_t possible_metadata_size = integrity_levels.size()*2;
    if (buffer.size() + possible_metadata_size > bufferEntries) { //Era >= pero creo que no consideraba el paquete en si a leer
        return false;
    }
    handleRequest(pkt);
    scheduleNextReqSendEvent(nextCycle());
    return true;
}


//Secure memory
AddrRangeList
SecureMemory::getAddrRanges() const
{
    return memSidePort.getAddrRanges();
}

void
SecureMemory::recvFunctional(PacketPtr pkt) //Doble definition? No, the previus one was SecureMemory::Port and this is SecureMemory.
{
    memSidePort.sendFunctional(pkt);
}

Tick
SecureMemory::recvAtomic(PacketPtr pkt)
{
    return clockPeriod() + memSidePort.sendAtomic(pkt);
}

//Timing
bool
SecureMemory::CPUSidePort::recvTimingReq(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Received pkt: %s in timing mode.\n", __func__, pkt->print());
    if (owner->recvTimingReq(pkt)) { //changed from "owner->recvTimingReq"
        return true;
    }
    needToSendRetry = true;
    return false;
}


void
SecureMemory::processNextReqSendEvent()
{
    panic_if(memSidePort.blocked(), "Should never try to send if blocked!");
    panic_if(!buffer.hasReady(curTick()), "Should never try to send if no ready packets!");

    PacketPtr pkt = buffer.front(); //76
    memSidePort.sendPacket(pkt);
    buffer.pop();

    scheduleNextReqRetryEvent(nextCycle());
    scheduleNextReqSendEvent(nextCycle());
    // stats.numRequestsFwded++;
    // PacketPtr pkt = buffer.front();
    stats.numRequestsFwded++;
    stats.totalbufferLatency += curTick() - buffer.frontTime();

}

void
SecureMemory::processNextReqRetryEvent()
{
    panic_if(!cpuSidePort.needRetry(), "Should never try to send retry if not needed!");
    cpuSidePort.sendRetryReq();
}

void
SecureMemory::scheduleNextReqRetryEvent(Tick when)
{
    if (cpuSidePort.needRetry() && !nextReqRetryEvent.scheduled()) {
        schedule(nextReqRetryEvent, when);
    }
}

void
SecureMemory::scheduleNextReqSendEvent(Tick when)
{
    bool port_avail = !memSidePort.blocked();
    bool have_items = !buffer.empty();

    if (port_avail && have_items && !nextReqSendEvent.scheduled()) {
        Tick schedule_time = buffer.firstReadyTime();
        schedule(nextReqSendEvent, schedule_time);
    }
}

void
SecureMemory::recvReqRetry()
{
    scheduleNextReqSendEvent(nextCycle());
}



//Mem Side Port
void
SecureMemory::MemSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blocked(), "Should never try to send if blocked!");

    DPRINTF(SecureMemory, "%s: Sending pkt: %s.\n", __func__, pkt->print());
    if (!sendTimingReq(pkt)) {
        DPRINTF(SecureMemory, "%s: Failed to send pkt: %s.\n", __func__, pkt->print());
        blockedPacket = pkt;
    }
}

void
SecureMemory::MemSidePort::recvReqRetry()
{
    panic_if(!blocked(), "Should never receive retry if not blocked!");

    DPRINTF(SecureMemory, "%s: Received retry signal.\n", __func__);
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);

    if (!blocked()) {
        owner->recvReqRetry();
    }
}


Tick
SecureMemory::align(Tick when)
{
    return clockEdge((Cycles) std::ceil((when - curTick()) / clockPeriod()));
}

// Too-Much-Code
void
SecureMemory::CPUSidePort::sendPacket(PacketPtr pkt)
{
    panic_if(blocked(), "Should never try to send if blocked!");

    DPRINTF(SecureMemory, "%s: Sending pkt: %s.\n", __func__, pkt->print());
    if (!sendTimingResp(pkt)) {
        DPRINTF(SecureMemory, "%s: Failed to send pkt: %s.\n", __func__, pkt->print());
        blockedPacket = pkt;
    }
}

// Too-Much-Code
void
SecureMemory::CPUSidePort::recvRespRetry()
{
    panic_if(!blocked(), "Should never receive retry if not blocked!");

    DPRINTF(SecureMemory, "%s: Received retry signal.\n", __func__);
    PacketPtr pkt = blockedPacket;
    blockedPacket = nullptr;
    sendPacket(pkt);

    if (!blocked()) {
        owner->recvRespRetry();
    }
}

// Too-Much-Code
void
SecureMemory::recvRespRetry()
{
    scheduleNextRespSendEvent(nextCycle());
}


// Too-Much-Code
bool
SecureMemory::MemSidePort::recvTimingResp(PacketPtr pkt)
{
    DPRINTF(SecureMemory, "%s: Received pkt: %s in timing mode.\n", __func__, pkt->print());
    if (owner->recvTimingResp(pkt)) { //changed from owner->recvTimingResp
        return true;
    }
    needToSendRetry = true;
    return false;
}


// Too-Much-Code
bool
SecureMemory::recvTimingResp(PacketPtr pkt)
{
    if (responseBuffer.size() >= responseBufferEntries) {
        return false;
    }
    //responseBuffer.push(pkt, curTick());
    handleResponse(pkt);
    scheduleNextRespSendEvent(nextCycle());
    return true;
}



// Too-Much-Code
void
SecureMemory::processNextRespSendEvent()
{
    panic_if(cpuSidePort.blocked(), "Should never try to send if blocked!");
    panic_if(!responseBuffer.hasReady(curTick()), "Should never try to send if no ready packets!");

    //stats.numResponsesFwded++;
    //stats.totalResponseBufferLatency += curTick() - responseBuffer.frontTime();
    PacketPtr pkt = responseBuffer.front();
    cpuSidePort.sendPacket(pkt);
    responseBuffer.pop();

    scheduleNextRespRetryEvent(nextCycle());
    scheduleNextRespSendEvent(nextCycle());
    //stats.numResponsesFwded++;
    //PacketPtr pkt = responseBuffer.front();
    stats.numResponsesFwded++;
    stats.totalResponseBufferLatency += curTick() - responseBuffer.frontTime();
}

// Too-Much-Code
void
SecureMemory::scheduleNextRespSendEvent(Tick when)
{
    bool port_avail = !cpuSidePort.blocked();
    bool have_items = !responseBuffer.empty();

    if (port_avail && have_items && !nextRespSendEvent.scheduled()) {
        Tick schedule_time = align(std::max(when, responseBuffer.firstReadyTime()));
        schedule(nextRespSendEvent, schedule_time);
    }
}

// Too-Much-Code
void
SecureMemory::processNextRespRetryEvent()
{
    panic_if(!memSidePort.needRetry(), "Should never try to send retry if not needed!");
    memSidePort.sendRetryResp();
}

// Too-Much-Code
void
SecureMemory::scheduleNextRespRetryEvent(Tick when)
{
    if (memSidePort.needRetry() && !nextRespRetryEvent.scheduled()) {
        schedule(nextRespRetryEvent, align(when));
    }
}

//SecureMemory Constructor
SecureMemory::SecureMemory(const SecureMemoryParams& params):
    ClockedObject(params),
    cpuSidePort(this, name() + ".cpu_side_port"),
    memSidePort(this, name() + ".mem_side_port"),
    bufferEntries(params.inspection_buffer_entries),
    buffer(clockPeriod()),
    responseBufferEntries(params.response_buffer_entries),
    responseBuffer(clockPeriod()),
    nextReqSendEvent([this](){ processNextReqSendEvent(); }, name() + ".nextReqSendEvent"),
    nextReqRetryEvent([this](){ processNextReqRetryEvent(); }, name() + ".nextReqRetryEvent"),
    nextRespSendEvent([this](){ processNextRespSendEvent(); }, name() + ".nextRespSendEvent"),
    nextRespRetryEvent([this](){ processNextRespRetryEvent(); }, name() + ".nextRespRetryEvent"),
    stats(this) //73
{}

void
SecureMemory::init()
{
    cpuSidePort.sendRangeChange();

    // setup address range for secure memory metadata
    AddrRangeList ranges_total = memSidePort.getAddrRanges();
    assert(ranges_total.size() == 1);

    //for the metadata

    //middle of the memory
    uint64_t my_end = ranges_total.front().start() + ranges_total.front().size()/2;
    AddrRange myrange = AddrRange(ranges_total.front().start(),my_end);

    //for te cpu
    ranges = AddrRangeList(); //inicializated in .hh
    AddrRange cpurange = AddrRange(my_end,ranges_total.front().end());

    uint64_t start = ranges.front().start();
    uint64_t end = ranges.front().end();

    uint64_t hmac_bytes = ((end - start) / BLOCK_SIZE) * HMAC_SIZE;
    uint64_t counter_bytes = ((end - start) / PAGE_SIZE) * BLOCK_SIZE;

    // initialize integrity_levels
    uint64_t tree_offset = end + hmac_bytes;

    integrity_levels.push_front(start); // where does data start?
    integrity_levels.push_front(tree_offset); // where does tree start?

    uint64_t bytes_on_level = counter_bytes;
    do {
        integrity_levels.push_front(tree_offset + bytes_on_level); // level starting address
        tree_offset += bytes_on_level;
        bytes_on_level /= ARITY;
    } while (bytes_on_level > 1);

    integrity_levels.push_front(end); // hmac start
    integrity_levels.shrink_to_fit();

    data_level = integrity_levels.size() - 1;
    counter_level = data_level - 1;
}

/// MERKLE TREE LETS GO

uint64_t
SecureMemory::getHmacAddr(uint64_t child_addr)
{
    //AddrRangeList ranges = memSidePort.getAddrRanges();
    assert(ranges.size() == 1);

    uint64_t start = ranges.front().start();
    uint64_t end = ranges.front().end();

    if (!(child_addr >= start && child_addr < end)) {
        // this is a check for something that isn't metadata
        return (uint64_t) -1;
    }

    // raw location, not word aligned
    uint64_t hmac_addr = integrity_levels[hmac_level] + ((child_addr / BLOCK_SIZE) * HMAC_SIZE);

    // word aligned
    return hmac_addr - (hmac_addr % BLOCK_SIZE);
}



uint64_t
SecureMemory::getParentAddr(uint64_t child_addr)
{
    //AddrRangeList ranges = memSidePort.getAddrRanges();
    assert(ranges.size() == 1);

    uint64_t start = ranges.front().start();
    uint64_t end = ranges.front().end();

    if (child_addr >= start && child_addr < end) {
        // child is data, get the counter
        return integrity_levels[counter_level] + ((child_addr / PAGE_SIZE) * BLOCK_SIZE);
    }

    for (int i = counter_level; i > root_level; i--) {
        if (child_addr >= integrity_levels[i] && child_addr < integrity_levels[i - 1]) {
            // we belong to this level
            uint64_t index_in_level = (child_addr - integrity_levels[i]) / BLOCK_SIZE;
            return integrity_levels[i - 1] + ((index_in_level / ARITY) * BLOCK_SIZE);
        }
    }

    assert(child_addr == integrity_levels[root_level]);
    // assert(false); // we shouldn't ever get here
    return (uint64_t) -1;
}


void
SecureMemory::verifyChildren(PacketPtr owner) //Parameter renamed from "parent"
{
    if (owner->getAddr() < integrity_levels[hmac_level]) {
        bool awaiting_hmac = false;
        for (uint64_t addr: pending_hmac) {
            if (addr == owner->getAddr()) {
                awaiting_hmac = true;
            }
        }

        if (!awaiting_hmac) {
            // we are authenticated!
            pending_tree_authentication.erase(owner->getAddr());

            if (owner->isWrite()) {
                // also send writes for all of the metadata
                memSidePort.sendPacket(owner);
            } else {
                cpuSidePort.sendPacket(owner);
            }
        }

        return;
    }

    std::vector<PacketPtr> to_call_verify;

    // verify all packets that have returned and are waiting
    for (auto it = pending_untrusted_packets.begin();
              it != pending_untrusted_packets.end(); ) {
        if (getParentAddr((*it)->getAddr()) == owner->getAddr()) {
            // someone was untrusted and waiting for us
            to_call_verify.push_back(*it);
            it = pending_untrusted_packets.erase(it);
        } else {
            ++it;
        }
    }

    // all done, free/remove node
    delete owner;

    for (PacketPtr pkt: to_call_verify) {
        verifyChildren(pkt);
    }
}


bool
SecureMemory::handleResponse(PacketPtr pkt)
{
    if (pkt->isWrite() && pkt->getAddr() < integrity_levels[hmac_level]) {
        //cpuSidePort.sendPacket(pkt);
        responseBuffer.push(pkt, curTick());
        return true;
    }

    if (pkt->getAddr() >= integrity_levels[hmac_level] && pkt->getAddr() < integrity_levels[counter_level]) {
        // authenticate the data
        for (auto it = pending_hmac.begin();
                  it != pending_hmac.end(); ) {
            if (getHmacAddr(*it) == pkt->getAddr()) {
                it = pending_hmac.erase(it);
                // using simple memory, so we can assume hmac
                // will always be verified first and not worry
                // about the case where cipher happens before verification
            } else {
                ++it;
            }
        }

        delete pkt;
        return true;
    }

    // we are no longer in memory
    pending_tree_authentication.erase(pkt->getAddr());
    if (pkt->getAddr() == integrity_levels[root_level]) {
        // value is trusted, authenticate children
        verifyChildren(pkt);
    } else {
        // move from pending address to pending metadata stored
        // in on-chip buffer for authentication
        pending_untrusted_packets.insert(pkt);
    }

    return true;
}


bool
SecureMemory::handleRequest(PacketPtr pkt)
{
    std::vector<uint64_t> metadata_addrs;
    uint64_t child_addr = pkt->getAddr();

    uint64_t hmac_addr = getHmacAddr(child_addr);
    metadata_addrs.push_back(hmac_addr);
    do {
        metadata_addrs.push_back(getParentAddr(child_addr));
        child_addr = metadata_addrs.back();
    } while (child_addr != integrity_levels[root_level]);

    pending_tree_authentication.insert(pkt->getAddr());
    pending_hmac.insert(pkt->getAddr());

    if (pkt->isWrite() && pkt->hasData()) {
        pending_untrusted_packets.insert(pkt);
    } else if (pkt->isRead()) {
        buffer.push(pkt, curTick());
    }

    for (uint64_t addr: metadata_addrs) {
        RequestPtr req = std::make_shared<Request>(addr, BLOCK_SIZE, 0, 0);
        PacketPtr metadata_pkt = Packet::createRead(req);
        metadata_pkt->allocate();

        if (addr != hmac_addr) {
            // note: we can't save the packet itself because it may be deleted
            // by the memory device :-)
            pending_tree_authentication.insert(addr);
        }

        buffer.push(metadata_pkt, curTick());
    }

    return true;
}

} //End namespace gem5
