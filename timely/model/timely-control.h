#ifndef TIMELY_CNTRL_H
#define TIMELY_CNTRL_H

#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/ipv4-address.h"
#include "ns3/data-rate.h"
#include "ns3/traced-value.h"
#include "ns3/ipv4-address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/ipv4-address.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/double.h"
#include "ns3/ipv4-end-point.h"
#include "ns3/log.h"
#include "ns3/timely-sender.h"

#include <vector>

namespace ns3 {

	class Socket;
	class Packet;

	extern uint32_t _GVAR_NODE_IN_RACK_CNT;
	
	class TimelyControl : public Application
	{
	public:
		static TypeId GetTypeId(void);

		TimelyControl();

		virtual ~TimelyControl();

		static void NewAnnouncmentReq (Ptr<TimelyControl> app, Ptr<TimelySender> hpApp, uint64_t oldVal, uint64_t newVal){
			app->SendAnnouncement(hpApp, newVal - oldVal);
		}

		void AddAnnouncementListener(Ptr<TimelySender> app);

		/**
		* \brief set the remote address and port
		* \param ip remote IP address
		* \param port remote port
		*/
		void SetCtrlPort(uint16_t port);
		void SetHighpMode(std::string mode);
		std::string GetIpv4Address() const;
		std::string Print() const {
			return  
				std::to_string(Simulator::Now().GetMicroSeconds()) +
				", ip: " 		+ GetIpv4Address() +
				", m_sent: "  	+ std::to_string(m_sent);
		}

	protected:
		virtual void DoDispose(void);

	private:

		virtual void StartApplication(void);
		virtual void StopApplication(void);

		void Init();
		void SendAnnouncement(Ptr<TimelySender>, uint32_t);
		bool DecideToSendAnnouncement(uint32_t);
		void ReceiveCtrl(Ptr<Socket> socket);

		// Basic networking parameters. 
		Ptr<Socket> m_ctrlSocket;
		uint16_t m_ctrlPort;
		int m_highpMode;	// high priory sending announcement mode
		uint64_t m_sent;
		uint16_t 	m_nodeId;
		std::vector<Ptr<TimelySender>> m_announcementListeners;
	};

} // namespace ns3

#endif /* TIMELy_CONTRL_H */
