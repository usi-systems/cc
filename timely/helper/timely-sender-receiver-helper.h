#ifndef TIMELY_SENDER_RECIVER_HELPER_H
#define TIMELY_SENDER_RECIVER_HELPER_H

#include <stdint.h>
#include "ns3/application-container.h"
#include "ns3/node-container.h"
#include "ns3/object-factory.h"
#include "ns3/ipv4-address.h"
#include "ns3/timely-receiver.h"
#include "ns3/timely-sender.h"
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

} // namespace ns3

#endif 
