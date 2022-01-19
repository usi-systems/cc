/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 Universita' degli Studi di Napoli Federico II
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Authors:  Stefano Avallone <stavallo@unina.it>
 */

#ifndef PFABRIC_QUEUE_DISC_H
#define PFABRIC_QUEUE_DISC_H

#include "ns3/queue-disc.h"
#include "ns3/tcp-header.h"

namespace ns3 {

class pFabricQueueItem {
private: 
	uint32_t GetFlowId(TcpHeader &tcpHeader){
		uint16_t port1 = tcpHeader.GetSourcePort() == 8801 ? tcpHeader.GetDestinationPort() : tcpHeader.GetSourcePort();
		uint16_t port2 = tcpHeader.GetSourcePort() != 8801 ? tcpHeader.GetDestinationPort() : tcpHeader.GetSourcePort();
		return (static_cast<uint32_t>(port2) << 16) | ((static_cast<uint32_t>(port1)) & 0xFFFF);
	}
	
public:


	pFabricQueueItem (Ptr<QueueDiscItem> item, TcpHeader &tcpH, int prio) {
		m_item = item;
		m_flowId = GetFlowId(tcpH);
		m_priority = prio;
	}

	pFabricQueueItem (Ptr<QueueDiscItem> item) {
		m_item = item;
		m_flowId = -1;
		m_priority = -1;

	}

	~pFabricQueueItem() {
		m_item = NULL;
		m_flowId = -1;
		m_priority = -1;
	}

	Ptr<QueueDiscItem> m_item;
	int m_priority;
	public:
	uint32_t m_flowId;
};








class pFabricInternalQueue : public Object {

public: 
	const static uint32_t LOW_PRIORITY_VALUE = std::numeric_limits<uint16_t>::max() - 1;
	const static uint32_t HIGH_PRIORITY_VALUE = 1;

	static TypeId GetTypeId (void);

	pFabricInternalQueue() { m_maxSize = QueueSize (QueueSizeUnit::BYTES, 2000);}
	~pFabricInternalQueue() { }

	bool SetMaxSize(QueueSize size) {
		if(size.GetUnit() == QueueSizeUnit::PACKETS) {
			std::cout << "only bytes sizes are accepted!" << std::endl;
			exit(223);
			return false;
		}
		m_maxSize = size;
		return true;
	}

	// returns NULL of successfull. Returns the dropped item otherwise.
	Ptr<QueueDiscItem> DoEnqueue(Ptr<QueueDiscItem> item);
	Ptr<QueueDiscItem> DoDequeue (void);
	QueueSize GetMaxSize (void) const;
	QueueSize GetCurrentSize (void) const;
	void Print(void);
	void Print2(void);

private:
	uint ReadPriority(Ptr<Packet> pkt);
	std::vector<pFabricQueueItem> m_queue;
	QueueSize m_maxSize;
public:
	TracedValue<uint32_t> m_nPackets;
};








/**
 * \ingroup traffic-control
 *
 * Simple queue disc implementing the FIFO (First-In First-Out) policy.
 *
 */
class pFabricQueueDisc : public QueueDisc {
public:
	/**
	 * \brief Get the type ID.
	 * \return the object TypeId
	 */
	static TypeId GetTypeId (void);
	/**
	 * \brief pFabricQueueDisc constructor
	 *
	 * Creates a queue with a depth of 1000 packets by default
	 */
	pFabricQueueDisc ();

	virtual ~pFabricQueueDisc();

	// Reasons for dropping packets
	static constexpr const char* LIMIT_EXCEEDED_DROP = "Queue disc limit exceeded";  //!< Packet dropped due to queue disc limit exceeded

private:
	void BufferSizeTraceCall(uint32_t oldValue, uint32_t newValue);
	bool SetMaxSize (QueueSize size);
	QueueSize GetMaxSize (void) const;
	virtual bool DoEnqueue (Ptr<QueueDiscItem> item);
	virtual Ptr<QueueDiscItem> DoDequeue (void);
	virtual Ptr<const QueueDiscItem> DoPeek (void);
	virtual bool CheckConfig (void);
	virtual void InitializeParams (void);

	Ptr<pFabricInternalQueue> m_internalQueue;

	TracedCallback<Ptr<const QueueDiscItem> > m_traceEnqueue;
	TracedCallback<Ptr<const QueueDiscItem> > m_traceDrop;
	TracedCallback<uint32_t> m_nPacketsInBuffer;
};

} // namespace ns3

#endif /* FIFO_QUEUE_DISC_H */
