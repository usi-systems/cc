#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/log.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-address.h"
#include "ns3/address-utils.h"
#include "ns3/nstime.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/socket.h"
#include "ns3/udp-socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"

#include "timely-receiver.h"

#define DEBUG_CONDITION false //GetIpv4Address() == "810.2.3.1" //&& Simulator::Now().GetMilliSeconds() > 1017

namespace ns3{

NS_LOG_COMPONENT_DEFINE ("TimelyReceiverApplication");
NS_OBJECT_ENSURE_REGISTERED (TimelyReceiver);

TypeId TimelyReceiver::GetTypeId (void) {
	static TypeId tid = TypeId ("ns3::TimelyReceiver")
	.SetParent<Application> ()
	.AddConstructor<TimelyReceiver> ()
	.AddAttribute ("Port", "Port on which we listen for incoming packets.",
							UintegerValue (9),
							MakeUintegerAccessor (&TimelyReceiver::m_port),
							MakeUintegerChecker<uint16_t> ())
	.AddAttribute("PacketSize",
							"Size of ACK/NAcK packets generated. The minimum packet size is 14 bytes which is the size of the header carrying the sequence number and the time stamp.",
							UintegerValue(14),
							MakeUintegerAccessor(&TimelyReceiver::m_pktSize),
							MakeUintegerChecker<uint32_t>(14, 1400))
	;
	return tid;
}

TimelyReceiver::TimelyReceiver () {
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
	m_received.clear();
	m_sent.clear();
	m_connectionStartTime.clear();
}

TimelyReceiver::~TimelyReceiver() {
	NS_LOG_FUNCTION_NOARGS ();
	m_socket = 0;
	m_received.clear();
	m_sent.clear();
	m_connectionStartTime.clear();
}

void TimelyReceiver::DoDispose (void) {
	NS_LOG_FUNCTION_NOARGS ();
	Application::DoDispose ();
}

void TimelyReceiver::StartApplication (void) {
	NS_LOG_FUNCTION_NOARGS ();

	if (m_socket == 0) {
		TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
		m_socket = Socket::CreateSocket (GetNode (), tid);
		InetSocketAddress local = InetSocketAddress (Ipv4Address::GetAny (), m_port);
		int bindHandle = m_socket->Bind (local);
		if(bindHandle) {std::cout << "Unable to bind on socket"; exit(434);}

		// Ptr<UdpSocket> udpSocket = DynamicCast<UdpSocket> (m_socket);
		// udpSocket->MulticastJoinGroup (0, m_local);
	}
	
	m_socket->SetRecvCallback (MakeCallback (&TimelyReceiver::HandleRead, this));
}

std::string TimelyReceiver::GetIpv4Address() const {
	if(m_socket == NULL)
		return "NULL";
	Ptr<Ipv4> ipv4 = m_socket->GetNode()->GetObject<Ipv4> ();
	Ipv4InterfaceAddress iaddr = ipv4->GetAddress (1,0); 
	Ipv4Address ipAddr = iaddr.GetLocal ();

	std::stringstream ss; 
	ss << ipAddr; 
	std::string myIp = ss.str();

	return myIp;
}

void TimelyReceiver::StopApplication () {
	NS_LOG_FUNCTION_NOARGS ();

	if (m_socket != 0)  {
		m_socket->Close ();
		m_socket->SetRecvCallback (MakeNullCallback<void, Ptr<Socket> > ());
	}
}

void TimelyReceiver::CloseConnection(Address from) {
	m_received[from] = 0;
	m_sent[from] = 0;
}

void TimelyReceiver::SendAck(Ptr<Socket> socket, Address remoteAddress, TimelyHeader remoteSeqTs){
	NS_LOG_LOGIC("Echoing packet");
	// std::cout << "ack" << std::endl;

	TimelyHeader seqTs;
	seqTs.SetSeq(m_received[remoteAddress]);
	seqTs.SetPG(remoteSeqTs.GetPG());
	seqTs.SetTs(remoteSeqTs.GetTs());
	seqTs.SetAck();
	Ptr<Packet> ackPkt = Create<Packet>(m_pktSize);
	ackPkt->AddHeader(seqTs);
	socket->SendTo(ackPkt, 0, remoteAddress);
}

void TimelyReceiver::SendNack(Ptr<Socket> socket, Address remoteAddress, TimelyHeader remoteHeader){
	NS_LOG_LOGIC("Sending NACK");
	// std::cout << Simulator::Now() << "  sending nack " << nack_cnt++ << std::endl;

	TimelyHeader seqTs;
	seqTs.SetSeq(m_received[remoteAddress]);
	seqTs.SetPG(remoteHeader.GetPG());
	seqTs.SetTs(0);
	seqTs.SetNack();

	Ptr<Packet> nackPkt = Create<Packet>(m_pktSize);
	nackPkt->AddHeader(seqTs);
	socket->SendTo(nackPkt, 0, remoteAddress);
}

void TimelyReceiver::HandleRead(Ptr<Socket> socket){
	Ptr<Packet> packet;
	Address from;
	while ((packet = socket->RecvFrom(from))) 
	{
		TimelyHeader tHeader;
		packet->RemoveHeader(tHeader);
		uint64_t seqNum = tHeader.GetSeq();

		if(seqNum == 0 && m_received[from] == 0) { // New Connection! We have not seen packets from this connection so far. 
			m_connectionStartTime[from] = Simulator::Now();
			m_received[from] = 0;
		}

		if(m_received[from] != seqNum) { // the SEQ is not what we expect! Send NACK
			if(DEBUG_CONDITION){
				std::cout << "\tsendNack:\t" << Simulator::Now().GetMicroSeconds() - 1000000 << "us     rcvd:" << m_received[from] <<  " !=  seqNum:" << seqNum << std::endl;
			}

			SendNack(socket, from, tHeader);		
		} else { // Seq IS OK!!
			m_received[from] = m_received[from] + packet->GetSize();
			
			if(DEBUG_CONDITION){
				std::cout << "\tRCVD:\t" << Simulator::Now().GetMicroSeconds() - 1000000 << "us     seq:" << seqNum <<	std::endl;
			}
		}

		if (tHeader.IsAckNeeded()){
			if(DEBUG_CONDITION){
				uint64_t timeNow = Simulator::Now ().GetTimeStep ();
				uint64_t rcvdTs = tHeader.GetTs();
				uint64_t oneWayDelay = timeNow - rcvdTs;
				std::cout << "\tONE Way Delay: " << oneWayDelay/1000 << "us, node:" << GetNode()->GetId() << std::endl;
				// std::cout << "\tneeded\t" << Simulator::Now().GetMicroSeconds() - 1000000 << "us \tseq:" <<  seqNum << " ack:" << m_received[from] <<	std::endl;
			}

			// if(GetIpv4Address() == "10.2.2.1") {
			// 	std::cout << "ACK] seq:" << tHeader.GetSeq() << ", from: " << from << ", ip: " << GetIpv4Address() << std::endl;
			// }

			SendAck(socket, from, tHeader);
			m_sent[from] = m_sent[from] + 1;
		}
	}
}

} // Namespace ns3
