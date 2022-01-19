#include "timely-sender-receiver-helper.h"
#include "ns3/timely-sender.h"
#include "ns3/timely-receiver.h"
#include "ns3/uinteger.h"
#include "ns3/string.h"

namespace ns3 {

	TimelyReceiverHelper::	TimelyReceiverHelper() { }
	TimelySenderHelper::	TimelySenderHelper() { }

/*RCVER*/
	TimelyReceiverHelper::TimelyReceiverHelper(uint16_t port) {
		m_factory.SetTypeId(TimelyReceiver::GetTypeId());
		SetAttribute("Port", UintegerValue(port));
	}

	void TimelyReceiverHelper::SetAttribute(std::string name, const AttributeValue &value) {
		m_factory.Set(name, value);
	}

	ApplicationContainer TimelyReceiverHelper::Install(NodeContainer c) {
		ApplicationContainer apps;
		for (NodeContainer::Iterator i = c.Begin(); i != c.End(); ++i) {
			Ptr<Node> node = *i;

			m_receiver = m_factory.Create<TimelyReceiver>();
			node->AddApplication(m_receiver);
			apps.Add(m_receiver);

		}
		return apps;
	}

	Ptr<TimelyReceiver> TimelyReceiverHelper::GetServer(void) {
		return m_receiver;
	}

/*SENDER*/
	TimelySenderHelper::TimelySenderHelper(Address address, uint16_t remotePort) {
		m_factory.SetTypeId(TimelySender::GetTypeId());
		SetAttribute("RemoteAddress", AddressValue(address));
		SetAttribute("RemotePort", UintegerValue(remotePort));
	}

	void TimelySenderHelper::SetAttribute(std::string name, const AttributeValue &value) {
		m_factory.Set(name, value);
	}

	Ptr<Application> TimelySenderHelper::Install(Ptr<Node> c) {
		Ptr<TimelySender> client = m_factory.Create<TimelySender>();
		c->AddApplication(client);
		return client;
	}
} // namespace ns3
