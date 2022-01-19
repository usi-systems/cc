#ifndef TIMELY_PACKET_FILTER_H
#define TIMELY_PACKET_FILTER_H

#include "ns3/queue.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/traffic-control-module.h"
#include "timely-header.h"


using namespace ns3;

class TimelyPacketFilter : public PacketFilter
{
public:
	static const uint16_t HIGH_PRIORITY 	= 0; // anything other than 1 is low priority
	static const uint16_t LOW_PRIORITY 		= 1;

	TimelyPacketFilter 	() { }
	~TimelyPacketFilter () { }

private:

	bool CheckProtocol (Ptr<QueueDiscItem> item) const {
		return true;
	}

	int32_t DoClassify (Ptr<QueueDiscItem> item) const {
		uint port1 			= 0;
		uint port2 			= 0;
		uint CC_LP_PORT 	= 8801;
		uint CC_HP_PORT 	= 8802;

		if(item -> GetSize() >= 4) {
			uint8_t buff[4];
			item->GetPacket()->CopyData(buff, 4);

			port1 = buff[0] * 256 + buff[1];
			port2 = buff[2] * 256 + buff[3];
		}
		if((port1 == CC_LP_PORT && port2 > 8900) || (port2 == CC_LP_PORT && port1 > 8900)) {
			return LOW_PRIORITY;
		} else if((port1 == CC_HP_PORT && port2 > 8900) || (port2 == CC_HP_PORT && port1 > 8900)) {
			return LOW_PRIORITY;
		} 
		else {
			return HIGH_PRIORITY;
		}
		std::cout << "invalid packet: " << port1 << ", " << port2;
		exit(5);
		return 0;
	}
};

#endif