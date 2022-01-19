#include "ns3/log.h"
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
#include "timely-control.h"
#include "timely-header.h"
#include "timely-packet-filter.h"
#include <stdlib.h>
#include <stdio.h>


// #include "ns3/network-module.h"
// #include "ns3/ipv4.h"

#define MY_IP "10.0.5.1"
#define MY_DEBUG 0

enum HIGHP_MODE_TYPE {
	NORMAL = 100,			// No Announcement!
	ANNOUNCEMENT = 300,		// Send Announcement. Increase sending rate! 
	SKIP_ANNC = 400,		// No Announcement! Increase sending rate!
	ADDAPTIVE = 500			// Addaptive Announcement. Increase sending rate!
};

namespace ns3 
{
	NS_LOG_COMPONENT_DEFINE("TimelyControl");
	NS_OBJECT_ENSURE_REGISTERED(TimelyControl);
	uint32_t _GVAR_NODE_IN_RACK_CNT;

	TypeId TimelyControl::GetTypeId(void) {
		static TypeId tid = TypeId("ns3::TimelyControl")
			.SetParent<Application>()

			.AddConstructor<TimelyControl>()
													
			.AddAttribute("ContrlPort", 
										"The src port for the control traffic",
										UintegerValue(100),
										MakeUintegerAccessor(&TimelyControl::m_ctrlPort),
										MakeUintegerChecker<uint16_t>())
			
			.AddAttribute("Highp_Mode",
										"How to handle high priority traffic!",
										StringValue("normal"),
										MakeStringAccessor(&TimelyControl::SetHighpMode),
										MakeStringChecker ())
			.AddAttribute("NodeId",
										"Unique ID for this host/cntroler",
										UintegerValue(0),
										MakeUintegerAccessor(&TimelyControl::m_nodeId),
										MakeUintegerChecker<uint32_t>())
			;
			
		return tid;
	}

	void TimelyControl::SetHighpMode(std::string mode){
		if(mode == "normal"){
			m_highpMode = HIGHP_MODE_TYPE::NORMAL;
			return;
		}
		if(mode == "announc_0"){
			m_highpMode = HIGHP_MODE_TYPE::ANNOUNCEMENT;
			return;
		}
		if(mode == "skip_annc"){
			m_highpMode = HIGHP_MODE_TYPE::SKIP_ANNC;
			return;
		}
		if(mode == "addaptive"){
			m_highpMode = HIGHP_MODE_TYPE::ADDAPTIVE;
			return;
		}
		std::cout << "invlid highpmode: TimelySender" << std::endl;
		throw -1;
	}

	std::string TimelyControl::GetIpv4Address() const {
		if(m_ctrlSocket == NULL)
			return "NULL";
		Ptr<Ipv4> ipv4 = m_ctrlSocket->GetNode()->GetObject<Ipv4> ();
		Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0); 
		Ipv4Address ipAddr = iaddr.GetLocal ();

		std::stringstream ss; 
		ss << ipAddr; 
		std::string myIp = ss.str();

		return myIp;
	}

	TimelyControl::TimelyControl():
		m_ctrlSocket (0)
	{
		NS_LOG_FUNCTION_NOARGS();	
	}

	void TimelyControl::Init() {
		m_ctrlSocket = 0;
	}

	TimelyControl::~TimelyControl() {
		NS_LOG_FUNCTION_NOARGS();
		m_ctrlSocket = 0;
	}

	void TimelyControl::SetCtrlPort(uint16_t port) {
		m_ctrlPort = port;
	}

	void TimelyControl::DoDispose(void) {
		NS_LOG_FUNCTION_NOARGS();
		Application::DoDispose();
	}

	void TimelyControl::StartApplication(void) {
		NS_LOG_FUNCTION_NOARGS();
		Init();

		if (m_ctrlSocket == 0){
			TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
			m_ctrlSocket = Socket::CreateSocket (GetNode (), tid);
			InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_ctrlPort);
			int bindHandle = m_ctrlSocket->Bind (local);
			if(bindHandle < 0) throw -1;
			m_ctrlSocket->SetAllowBroadcast(true);
		}
		m_ctrlSocket->SetRecvCallback(MakeCallback(&TimelyControl::ReceiveCtrl, this));
	}

	void TimelyControl::StopApplication() {
		NS_LOG_FUNCTION_NOARGS();
		m_ctrlSocket = 0;
	}

	void TimelyControl::AddAnnouncementListener(Ptr<TimelySender> app){
		m_announcementListeners.push_back(app);
	}

	void TimelyControl::SendAnnouncement(Ptr<TimelySender> hpApp, uint32_t nBytes) {
		if (!m_ctrlSocket) throw -1;
		if(m_highpMode == NORMAL) return;
		if (m_highpMode != ANNOUNCEMENT && m_highpMode != SKIP_ANNC &&  m_highpMode != ADDAPTIVE) throw -1;

		TimelyHeader tHeader;
		tHeader.SetSeq(nBytes);
		tHeader.SetPG(TimelyPacketFilter::HIGH_PRIORITY);
		tHeader.SetData();
		tHeader.SetTs (Simulator::Now ().GetTimeStep ());

		Ptr<Packet> pkt = Create<Packet> (8); // small packet!
		pkt->AddHeader(tHeader);

		bool sendAnnouncementFlag = true;
		if(m_highpMode == ADDAPTIVE)
			sendAnnouncementFlag = DecideToSendAnnouncement(nBytes);

		std::string dstIp = "225.1.2.4";
		if(m_highpMode != SKIP_ANNC && sendAnnouncementFlag) {
			Address bcastAddress = Address(InetSocketAddress (Ipv4Address (dstIp.c_str()), m_ctrlPort));
			int handle = m_ctrlSocket->SendTo(pkt, 0, bcastAddress);
			if(handle < 0) {
				std::cout << "faild to send annoucnements";
				exit(51);
			};
		}

		if(m_highpMode == ANNOUNCEMENT || m_highpMode == SKIP_ANNC || m_highpMode == ADDAPTIVE)
			hpApp-> MaximumSendingRate();
		else
		{
			std::cout << "Invalid State <-> timely-control.cc" << std::endl;
			exit(0);
		}
	}

	bool TimelyControl::DecideToSendAnnouncement(uint32_t nBytes){
		int sum = 0;
		for(auto const& app: m_announcementListeners) {
			sum += app->UnderLowTh();
		}
		return sum < 0; // network is congested.
	}

	void TimelyControl::ReceiveCtrl(Ptr<Socket> socket){
		Ptr<Packet> packet;
		Address from;
		while ((packet = socket->RecvFrom(from))) {
			TimelyHeader tHeader;
			Ipv4Header ipHeader;
			packet->PeekHeader(ipHeader);
			packet->RemoveHeader(tHeader);
			uint32_t nBytes = tHeader.GetSeq();

			for(auto const& app: m_announcementListeners) {
				app->ReceiveCtrl(nBytes);
			}
		}
	}
} // Namespace ns3