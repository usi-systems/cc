#include "ns3/log.h"
#include "ns3/core-module.h"
#include "pfabric-queue-disc.h"
#include "ns3/object-factory.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/ipv4-header.h"
#include <set>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("pFabricQueueDisc");

NS_OBJECT_ENSURE_REGISTERED (pFabricQueueDisc);

TypeId pFabricQueueDisc::GetTypeId (void) {
	static TypeId tid = TypeId ("ns3::pFabricQueueDisc")
		.SetParent<QueueDisc> ()
		.SetGroupName ("pFabric")
		.AddConstructor<pFabricQueueDisc> ()
		.AddAttribute ("MaxSize",
					"The max queue size",
					QueueSizeValue (QueueSize ("500KB")),
					MakeQueueSizeAccessor (&pFabricQueueDisc::SetMaxSize, &pFabricQueueDisc::GetMaxSize),
					MakeQueueSizeChecker ())
		.AddTraceSource ("PacketsInpFabricQueue",
					"number of packets in buffer",
					MakeTraceSourceAccessor (&pFabricQueueDisc::m_nPacketsInBuffer),
					"ns3::TracedValueCallback::Uint32")
		.AddTraceSource ("EnqueueFabric", "Enqueue a packet in the queue disc",
                    MakeTraceSourceAccessor (&pFabricQueueDisc::m_traceEnqueue),
                    "ns3::QueueDiscItem::TracedCallback")
		.AddTraceSource ("DropFabric", "Drop a packet stored in the queue disc",
                    MakeTraceSourceAccessor (&pFabricQueueDisc::m_traceDrop),
                    "ns3::QueueDiscItem::TracedCallback")
	;
	return tid;
}

bool pFabricQueueDisc::SetMaxSize (QueueSize size) {
	QueueDisc::SetMaxSize(size);
	return m_internalQueue->SetMaxSize(size);
}

QueueSize pFabricQueueDisc::GetMaxSize (void) const {
	return QueueDisc::GetMaxSize();
}

pFabricQueueDisc::pFabricQueueDisc () : QueueDisc (QueueDiscSizePolicy::SINGLE_INTERNAL_QUEUE) {
	NS_LOG_FUNCTION (this);
	m_internalQueue = CreateObject <pFabricInternalQueue> ();
	m_internalQueue->TraceConnectWithoutContext ("PacketsInBuffer", MakeCallback (&pFabricQueueDisc::BufferSizeTraceCall, this));
}

void pFabricQueueDisc::BufferSizeTraceCall(uint32_t oldValue, uint32_t newValue){
	m_nPacketsInBuffer(newValue);
}

pFabricQueueDisc::~pFabricQueueDisc () {
	NS_LOG_FUNCTION (this);
	m_internalQueue = NULL;
}

bool pFabricQueueDisc::DoEnqueue (Ptr<QueueDiscItem> item) {
	NS_LOG_FUNCTION (this << item);

	Ptr<QueueDiscItem> enqItem = m_internalQueue->DoEnqueue(item);
	if (enqItem) // if it's not null, it means same/another item has been dropped instead. 
	{
		NS_LOG_LOGIC ("Queue full -- dropping pkt");
		DropBeforeEnqueue (item, LIMIT_EXCEEDED_DROP); // to avoid assert failures
		m_traceDrop(enqItem);
		return false;
	}
	m_traceEnqueue(item);
	GetInternalQueue (0)->Enqueue (item); GetInternalQueue (0)->Dequeue (); // to avoid assert failure

	return true;
}

Ptr<QueueDiscItem> pFabricQueueDisc::DoDequeue (void) {
	NS_LOG_FUNCTION (this);

	// if(Simulator::Now().GetMilliSeconds() >= 1010 && m_internalQueue->m_nPackets.Get() > 10 && m_internalQueue->m_nPackets.Get() < 100) {
	// 	std::cout << "Before: " << std::endl;
	// 	m_internalQueue->Print2();
	// }

	Ptr<QueueDiscItem> item = m_internalQueue->DoDequeue();

	// if(Simulator::Now().GetMilliSeconds() >= 1010 && m_internalQueue->m_nPackets.Get() > 10 && m_internalQueue->m_nPackets.Get() < 100) {
	// 	std::cout << "After: " << std::endl;
	// 	m_internalQueue->Print2();
	// }

	if (!item)
	{
		NS_LOG_LOGIC ("Queue empty");
		return 0;
	}

	return item;
}

Ptr<const QueueDiscItem> pFabricQueueDisc::DoPeek (void) {
	NS_LOG_FUNCTION (this);

	Ptr<const QueueDiscItem> item = GetInternalQueue (0)->Peek ();

	if (!item)
		{
			NS_LOG_LOGIC ("Queue empty");
			return 0;
		}

	return item;
}

bool pFabricQueueDisc::CheckConfig (void) {
	NS_LOG_FUNCTION (this);
	if (GetNQueueDiscClasses () > 0)
		{
			NS_LOG_ERROR ("pFabricQueueDisc cannot have classes");
			return false;
		}

	if (GetNPacketFilters () > 0)
		{
			NS_LOG_ERROR ("pFabricQueueDisc needs no packet filter");
			return false;
		}

	if (GetNInternalQueues () == 0)
		{
			// add a DropTail queue
			AddInternalQueue (CreateObjectWithAttributes<DropTailQueue<QueueDiscItem> >
													("MaxSize", QueueSizeValue (GetMaxSize ())));
		}

	if (GetNInternalQueues () != 1)
		{
			NS_LOG_ERROR ("pFabricQueueDisc needs 1 internal queue");
			return false;
		}

	return true;
}

void pFabricQueueDisc::InitializeParams (void) {
	NS_LOG_FUNCTION (this);
}

/************************************/
/***** pFabric Internal Queue ******/
/************************************/

TypeId pFabricInternalQueue::GetTypeId (void) {
	static TypeId tid = TypeId ("ns3::pFabricInternalQueue")
		.SetParent<QueueDisc> ()
		.SetGroupName ("pFabric")
		.AddConstructor<pFabricInternalQueue> ()
		.AddTraceSource ("PacketsInBuffer",
					"number of packets in buffer",
					MakeTraceSourceAccessor (&pFabricInternalQueue::m_nPackets),
					"ns3::TracedValueCallback::Uint32")
	;
	return tid;
}

uint pFabricInternalQueue::ReadPriority(Ptr<Packet> pkt) {
	if(pkt->GetSize() <= 36) { std::cout << "cannot read priority"; exit(168);}
	uint8_t buf[pkt->GetSize() + 1]; pkt->CopyData(buf, pkt->GetSize());

	uint index = 0;
	uint remainingByte = 0;
	for(uint i = 20; i < pkt->GetSize(); i++){
		if(index && buf[i] == '#'){
			index = 0;
			break;
		}
		if(index) remainingByte = remainingByte * 10 + buf[i] - '0';
		if(buf[i] == '#') index = i;
	}

	if(index == 0) {
		return remainingByte;
	} else 
		return 1;
}

Ptr<QueueDiscItem> pFabricInternalQueue::DoEnqueue(Ptr<QueueDiscItem> item){
	NS_LOG_FUNCTION (this << item);

	Ptr<Packet> packet = item->GetPacket();
	TcpHeader tcpHeader;
	packet->PeekHeader(tcpHeader);
	int itemPriority = LOW_PRIORITY_VALUE;

	uint16_t lpPort = 8801;
	if(tcpHeader.GetDestinationPort() == lpPort || tcpHeader.GetSourcePort() == lpPort)
	{
		if(tcpHeader.GetFlags() & TcpHeader::SYN || packet->GetSize() <= 36){ // only a SYN or empty ACK packets. I don't know what 36 stands for. 
			itemPriority = HIGH_PRIORITY_VALUE;
			goto out_of_priority;
		}
		itemPriority = ReadPriority(packet);
	} else {
		std::cout << "Unknown Pkt: " << packet << std::endl;
		exit(44242);
	}

out_of_priority:
	pFabricQueueItem pItem = pFabricQueueItem(item, tcpHeader, itemPriority);

	Ptr<QueueDiscItem> toBeDroppedItem = nullptr;
	bool dropFlag = false;

	if (GetCurrentSize () + item > GetMaxSize ()) {
		// std::cout << "FULLLLLLLL: " << GetMaxSize() << "  " << GetCurrentSize() << std::endl;

		dropFlag = true;
		NS_LOG_LOGIC ("Queue full -- dropping pkt");
		if(itemPriority == LOW_PRIORITY_VALUE) // incoming item has very low priority.
			return item;
		else { // incoming item has a potentially better priority. Let's drop some other Item. 
			int i = m_queue.size() - 1;
			for(; i >= 0; i--){
				if(m_queue[i].m_priority > itemPriority)
					break;
			}
			if(i < 0) // well, all other entities in the queue are of more priority.
				return item;
			toBeDroppedItem = m_queue[i].m_item;
			m_queue.erase(m_queue.begin() + i);
		}
	}
	m_queue.push_back(pItem);
	if(dropFlag){
		return toBeDroppedItem;
	}
	m_nPackets++;
	return NULL; // successfully enqueuing the item. No packet has been dropped. 
}

void pFabricInternalQueue::Print (void) {
	std::cout << "-------" << std::endl;
	std::cout << "m_nPackets: " << m_nPackets << std::endl;
	std::cout << "[";
	std::map<uint32_t, uint32_t> subQueues; 

	for(uint32_t i = 0; i < m_nPackets; i++) {
		uint32_t pr = m_queue[i].m_priority;
		if(subQueues.find(pr) == subQueues.end())
			subQueues[pr] = 0;
		subQueues[pr]++;
	}

	for (auto const& it : subQueues) {
		std::cout << it.first << ' ' << it.second << ", ";
	}
	std::cout << std::endl;
}

void pFabricInternalQueue::Print2 (void) {
	std::cout << "-------" << std::endl;
	std::cout << "m_nPackets: " << m_nPackets << std::endl;
	std::cout << "[";

	for(uint32_t i = 0; i < m_nPackets; i++) {
		std::cout << ", pr: " << m_queue[i].m_priority
					<< ", flow: " << m_queue[i].m_flowId;
	}

	std::cout << std::endl;
}

Ptr<QueueDiscItem> pFabricInternalQueue::DoDequeue (void) {
	if(m_nPackets.Get() == 0)
		return NULL;

	int minPriority = m_queue[0].m_priority;
	uint32_t minFlowId = m_queue[0].m_flowId;
	uint32_t minItemIndex = 0;
	
	for(uint32_t i = 0; i < m_nPackets; i++){
		if(m_queue[i].m_priority < minPriority){
			minPriority = m_queue[i].m_priority;
			minFlowId = m_queue[i].m_flowId;
			minItemIndex = i;
		}	
	}

	for(uint32_t i = 0; i < m_nPackets; i++){
		if(m_queue[i].m_flowId == minFlowId && m_queue[i].m_priority > m_queue[minItemIndex].m_priority) // previous packets of the same flow
			minItemIndex = i;
	}

	Ptr<QueueDiscItem> item = m_queue[minItemIndex].m_item;
	m_queue.erase(m_queue.begin() + minItemIndex);
	m_nPackets--;
	return item;
}

QueueSize pFabricInternalQueue::GetCurrentSize (void) const{
	NS_LOG_FUNCTION (this);
	uint32_t sum = 0;
	for(uint32_t i = 0; i < m_nPackets; i++){
		sum += m_queue[i].m_item->GetSize();
	}
    return QueueSize (QueueSizeUnit::BYTES, sum);
}

QueueSize pFabricInternalQueue::GetMaxSize (void) const {
	NS_LOG_FUNCTION (this);
	return m_maxSize;
}

} // namespace ns3
