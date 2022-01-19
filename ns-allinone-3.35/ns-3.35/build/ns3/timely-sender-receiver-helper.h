#ifndef TIMELY_SENDER_RECIVER_HELPER_H
#define TIMELY_SENDER_RECIVER_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/timely-receiver.h"
#include "ns3/timely-sender.h"
#include "ns3/timely-control.h"
#include <map>

namespace ns3 {

class TimelyReceiverHelper
{
public:
	TimelyReceiverHelper ();
	TimelyReceiverHelper(uint16_t port);
	void SetAttribute (std::string name, const AttributeValue &value);
	ApplicationContainer Install (NodeContainer c);
	Ptr<TimelyReceiver> GetServer (void);

private:
	ObjectFactory m_factory;
	Ptr<TimelyReceiver> m_receiver;
};

class TimelySenderHelper {

public:
	TimelySenderHelper ();
	TimelySenderHelper (Address ip, uint16_t remotePort);
	void SetAttribute (std::string name, const AttributeValue &value);
	Ptr<Application> Install (Ptr<Node> c);

private:
	ObjectFactory m_factory;
};

class TimelyCntrlHelper {
public:
	TimelyCntrlHelper ();
	TimelyCntrlHelper (uint16_t ctrlPort, uint16_t leaf_cnt, uint16_t node_in_rack_cnt);
	void SetAttribute (std::string name, const AttributeValue &value);
	Ptr<Application> Install (Ptr<Node> c,	uint16_t leafId, uint16_t nodeId);
	void Install (Ptr<TimelySender> lp, Ptr<TimelySender> hp, uint16_t leafId, uint16_t nodeId);

private:
	std::map<uint32_t, Ptr<TimelyControl>> m_allTimelyControlApps;
	ObjectFactory m_factory;
	uint32_t GetIndex (uint16_t leafId, uint16_t nodeId);

	uint16_t LEAF_CNT;
	uint16_t NODE_CNT;
};

} // namespace ns3

#endif 
