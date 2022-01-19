#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/log.h"
#include "ns3/ipv4-header.h"
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
#include "timely-sender.h"
#include "timely-header.h"
#include <stdlib.h>
#include <stdio.h>


// #include "ns3/network-module.h"
// #include "ns3/ipv4.h"

#define DEBUG_CONDITION false // m_appId == 186 && Simulator::Now().GetMilliSeconds() > 1260

enum LOWP_MODE_TYPE {
	WAIT_100 = 	100100,
	IGNORE_ANNC = 600,
};

namespace ns3 
{
	NS_LOG_COMPONENT_DEFINE("TimelySender");
	NS_OBJECT_ENSURE_REGISTERED(TimelySender);

	TypeId TimelySender::GetTypeId(void) {
		static TypeId tid = TypeId("ns3::TimelySender")
			.SetParent<Application>()

			.AddConstructor<TimelySender>()

			.AddAttribute("SendData",
										"To add bytes in the buffer to be sent",
										UintegerValue(0),
										MakeUintegerAccessor(&TimelySender::BufferAddBytes),
										MakeUintegerChecker<uint32_t>())

			.AddAttribute("RemoteAddress",
										"The destination Address of the outbound packets",
										AddressValue(),
										MakeAddressAccessor(&TimelySender::m_peerAddress),
										MakeAddressChecker())

			.AddAttribute("RemotePort", 
										"The destination port of the outbound packets",
										UintegerValue(100),
										MakeUintegerAccessor(&TimelySender::m_peerPort),
										MakeUintegerChecker<uint16_t>())

			.AddAttribute("PacketSize",
										"Size of packets generated. The minimum packet size is 14 bytes which is the size of the header carrying the sequence number and the time stamp.",
										UintegerValue(1000),
										MakeUintegerAccessor(&TimelySender::m_pktSize),
										MakeUintegerChecker<uint32_t>(14, 1500))
			
			.AddAttribute("AppId",
										"Unique ID for this sender",
										UintegerValue(0),
										MakeUintegerAccessor(&TimelySender::m_appId),
										MakeUintegerChecker<uint32_t>())

			.AddAttribute("InitialRate",
										"Initial send rate in bits per second",
										DataRateValue (DataRate ("0.1Gbps")),
										MakeDataRateAccessor (&TimelySender::m_initRate),
										MakeDataRateChecker ())

			.AddAttribute("LinkSpeed",
										"Bottleneck link speed in bits per second",
										DataRateValue (DataRate ("1Gbps")),
										MakeDataRateAccessor (&TimelySender::m_C),
										MakeDataRateChecker ())

			.AddAttribute("Delta",
										"Additive increase in sending rate",
										DataRateValue (DataRate ("50Mbps")),
										MakeDataRateAccessor (&TimelySender::m_delta),
										MakeDataRateChecker ())

			.AddAttribute("T_high",
										"T_high threshold in microseconds",
										TimeValue (MicroSeconds (500.0)),
										MakeTimeAccessor(&TimelySender::m_t_high),
										MakeTimeChecker (Time("15us"), Time("600us")))

			.AddAttribute("T_low",
										"T_low threshold in microseconds",
										TimeValue (MicroSeconds (50.0)),
										MakeTimeAccessor(&TimelySender::m_t_low),
										MakeTimeChecker (Time("15us"), Time("400us")))


			.AddAttribute("Min_RTT",
										"MIN RTT in microseconds",
										TimeValue (MicroSeconds (20.0)),
										MakeTimeAccessor(&TimelySender::m_min_rtt),
										MakeTimeChecker (Time("13us"), Time("400us")))

			.AddAttribute("Beta",
										"Beta",
										DoubleValue(0.8),
										MakeDoubleAccessor(&TimelySender::m_beta),
										MakeDoubleChecker<double>())

			.AddAttribute("Alpha",
										"Alpha",
										DoubleValue(0.875),
										MakeDoubleAccessor(&TimelySender::m_alpha),
										MakeDoubleChecker<double>())

			.AddAttribute("MaxBurstSize",
										"Maximum BurstSize",
										UintegerValue(16000),
										MakeUintegerAccessor(&TimelySender::m_maxBurstSize),
										MakeUintegerChecker<uint32_t>())
			.AddAttribute("MaxMessageSize",
										"Maximum burst/msg size produced by the FlowGenerator",
										UintegerValue(10000),
										MakeUintegerAccessor(&TimelySender::m_maxMsgSize),
										MakeUintegerChecker<uint32_t>())
			.AddAttribute("MinRateMultiple",
										"MinRateMultiple",
										DoubleValue(0.01),
										MakeDoubleAccessor(&TimelySender::m_minRateMultiple),
										MakeDoubleChecker<double>())

			.AddAttribute("MaxRateMultiple",
										"MaxRateMultiple",
										DoubleValue(0.88),
										MakeDoubleAccessor(&TimelySender::m_maxRateMultiple),
										MakeDoubleChecker<double>())

			.AddTraceSource ("SendRate",
										"Sending Rate",
										MakeTraceSourceAccessor (&TimelySender::m_rate),
										"ns3::TracedValueCallback::DataRate")

			.AddTraceSource ("AckRecvd",
										"Upon receving an Ack number",
										MakeTraceSourceAccessor (&TimelySender::m_received),
										"ns3::TracedValueCallback::Uint32")

			.AddTraceSource ("TotalToSend",
										"Upon having more bytes to sent!",
										MakeTraceSourceAccessor (&TimelySender::m_totalToBeSent),
										"ns3::TracedValueCallback::Uint64")

			.AddAttribute("AckRTO",
										"considering NACK upon a timeout in receiving ACK!",
										TimeValue (MicroSeconds (2000.0)),
										MakeTimeAccessor(&TimelySender::m_ackRto),
										MakeTimeChecker ())
			.AddTraceSource ("AdjRTT",
                						"Adjusted RTT",
                     					MakeTraceSourceAccessor (&TimelySender::m_new_rtt),
                     					"ns3::TracedValueCallback::Time")
			.AddAttribute("ReactMode",
										"How to react to announcements",
										StringValue("wait_100p"),
										MakeStringAccessor(&TimelySender::SetReactMode),
										MakeStringChecker ())
			.AddAttribute("HighPriorityLink", 
										"This is a high priority connection. It announces every burst!",
										BooleanValue (false),
										MakeBooleanAccessor (&TimelySender::m_highPriority),
										MakeBooleanChecker ())

			;
			
		return tid;
	}

	Time TimelySender::CalculateWaitingTime(uint32_t nBytes){
		if(m_reactMode == IGNORE_ANNC) {
			std::cout << "Invalid State!"; exit(55);
		}

		if(nBytes == 0) nBytes++;
		Time delay = GetBurstDuration(nBytes, m_rate);
		delay = std::max(delay, m_sleep);

		return delay;
	}

	void TimelySender::SetReactMode(std::string mode){
		if(mode == "wait_100p"){
			m_reactMode = WAIT_100;
			return;
		}
		if(mode == "ignore_annc"){
			m_reactMode = IGNORE_ANNC;
			return;
		}
		std::cout << "invlid react mode: TimelySender" << std::endl;
		exit(10);
	}

	void TimelySender::ReceiveCtrl(uint32_t nBytes){
		if(DEBUG_CONDITION){
			std::cout << "\tannc rcvd:     " << nBytes << " " << Print() << std::endl;
		}

		if(m_highPriority) throw -1; // it should be normal traffic
		if(BytesInBuffer() == 0) return;
		if(m_reactMode == IGNORE_ANNC) return;

		Time delay = CalculateWaitingTime(nBytes);
		// if(delay < m_min_rtt / 2) return;
		// delay -= m_min_rtt / 2;
		Time previousWaitingTime = Time(m_sendEvent.GetTs() - Simulator::Now ().GetTimeStep());

		if(delay < previousWaitingTime) return;
		Time maxDelay = Time("200us");
		if(delay > maxDelay) delay = maxDelay;

		if(m_sendEvent.IsRunning()){
			Simulator::Cancel(m_sendEvent);
			m_sendEvent = Simulator::Schedule(delay, &TimelySender::Send, this);
		}
	}

	void TimelySender::MaximumSendingRate () {
		if(!m_highPriority) throw -1;
		
		if(m_rate.Get().GetBitRate() < m_maxRate.GetBitRate() * 0.5) {
			m_rate 	= DataRate(m_maxRate.GetBitRate() * 0.5);
			// std::cout << "Increase the rate!" << std::endl;
		} else
			// std::cout << "High Priority Already is High Enough!" << std::endl;

		m_sleep = GetBurstDuration(m_rate);

		if(m_sendEvent.IsRunning()){
			Simulator::Cancel(m_sendEvent);
			m_sendEvent = Simulator::Schedule(m_sleep, &TimelySender::Send, this);
		}
	}

	void TimelySender::SetRandomBuffer(uint8_t *buff) {
		m_randomBuffer = buff;
	}

	std::string TimelySender::GetIpv4Address() const {
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

	void TimelySender::BufferAddBytes(uint32_t nBytes){
		if(DEBUG_CONDITION){
			std::cout << "add      nBytes:" << nBytes << " " << Print() << std::endl;
		}

		if(nBytes > 0){
			if(m_received != m_sent) throw 1249;
  			m_totalToBeSent += nBytes;
		}

		if(DEBUG_CONDITION){
			if(m_highPriority && nBytes > 0) {
				std::cout << "send annc:     " << nBytes << " " << Print() << std::endl;
			}
		}
	}

	TimelySender::TimelySender():
		m_socket (0),
		m_burstCnt (0),
		m_randomBuffer (NULL)
	{
		NS_LOG_FUNCTION_NOARGS();
	}

	void TimelySender::Init() {
		m_sendEvent = EventId();
		m_ackRtoEvent = EventId();

		m_sent = 0;
		m_totalToBeSent = 0;
		m_socket = 0;
		m_rate = m_initRate;
		m_burstSize = m_maxBurstSize;
		m_burstCnt = 0;
		m_sleep = GetBurstDuration(m_rate);
		m_rtt_diff = MicroSeconds(0);
		m_received = 0;
		m_sdel = GetBurstDuration(2, m_C); // since we serialize packet twice (ping and pong), we consider two packet for this purpose. 
		m_prev_rtt = m_sdel;
		m_maxRate = DataRate(m_maxRateMultiple * m_C.GetBitRate());
		m_minRate = DataRate(m_minRateMultiple * m_C.GetBitRate());
		m_HAI = 1;
		m_pg = 0;
	}

	uint32_t TimelySender::BytesInBuffer() const{
		if (m_sent > m_totalToBeSent) { 
			std::cout << Print() << std::endl;
			std::cout << "m_sent > totalToBeSent " << m_sent << " " << m_totalToBeSent << std::endl;
			exit(5);
		}

		return m_totalToBeSent - m_sent;
	}

	uint32_t TimelySender::BytesInFly() const {
		if (m_sent > m_totalToBeSent || m_received > m_sent) { 
			std::cout << Print() << std::endl;
			std::cout << "m_sent -  totalToBeSent - m_received: " << m_sent << " " << m_totalToBeSent << " " << m_received << std::endl;
			exit(5);
		}

		return m_sent - m_received;
	}

	Time TimelySender::GetBurstDuration(DataRate rate) {
		return GetBurstDuration(m_burstSize, rate);
	}

	Time TimelySender::GetBurstDuration(uint nBytes, DataRate rate) {
		return Time(Seconds((double) nBytes * 8.0 / rate.GetBitRate()));
	}

	TimelySender::~TimelySender() {
		NS_LOG_FUNCTION_NOARGS();
		m_socket = 0; // set NULL
	}

	void TimelySender::SetRemote(Ipv4Address ip, uint16_t port) {
		m_peerAddress = Address(ip);
		m_peerPort = port;
	}

	void TimelySender::DoDispose(void) {
		NS_LOG_FUNCTION_NOARGS();
		Application::DoDispose();
	}

	DataRate TimelySender::GetSendingRate(void) {
		return m_rate;
	}

	void TimelySender::StartApplication(void) {
		NS_LOG_FUNCTION_NOARGS();
		Init();
		if (m_socket == 0){
			TypeId tid = TypeId::LookupByName("ns3::UdpSocketFactory");
			m_socket = Socket::CreateSocket(GetNode(), tid);
			if (Ipv4Address::IsMatchingType(m_peerAddress)) {
				int bindHandle = m_socket->Bind();
				int connectHandle = m_socket->Connect(InetSocketAddress(Ipv4Address::ConvertFrom(m_peerAddress), m_peerPort));
				if(bindHandle || connectHandle) {
					std::cout << "Problem in Binding or Connecting"; exit(88);
				}
			} else {
				throw -1; // invalid config!
			}
		}

		m_socket->SetRecvCallback(MakeCallback(&TimelySender::Receive, this));
		m_sendEvent = Simulator::Schedule(Seconds(0.0), &TimelySender::Send, this);
	}

	void TimelySender::StopApplication() {
		NS_LOG_FUNCTION_NOARGS();
		if(m_sendEvent.IsRunning()){
			Simulator::Cancel(m_sendEvent);
		}

		if(m_ackRtoEvent.IsRunning()){
			Simulator::Cancel(m_ackRtoEvent);
		}
		m_socket = 0;
	}

	void TimelySender::Send() {
		NS_ASSERT(m_sendEvent.IsExpired());
		if(m_socket == NULL) return;

		if (BytesInBuffer() > 0){
			SendBurst();
		}
		m_sendEvent = Simulator::Schedule(m_sleep, &TimelySender::Send, this);
	}

	void TimelySender::AckTimeout(uint32_t seqNum) {
		if(DEBUG_CONDITION){
			std::cout << "\tAckTimeout     t-seq:" << seqNum << ", sent:" << m_sent << ", rcvd:" << m_received << " ";
		}
		if(m_sent < seqNum) { // current_seq has reduced due to a nack!
			if(DEBUG_CONDITION){
				std::cout << " Ignored - 1!" << std::endl;
			}
			return;
		}
		
		if(m_received > seqNum) {
			if(DEBUG_CONDITION){
				std::cout << " Ignored - 2!" << std::endl;
			}
			return; 
		}

		if(m_received == seqNum && seqNum == m_sent) {
			if(DEBUG_CONDITION){
				std::cout << " Ignored - 3!" << std::endl;
			}
			return; 
		}

		if(DEBUG_CONDITION){
			std::cout << " RETRANSMIT! old_sent: " << m_received << ", new_sent:" << m_received << std::endl;
		}

		if(m_sendEvent.IsRunning())
			Simulator::Cancel(m_sendEvent);
		m_sent = m_received;
		m_sendEvent = Simulator::Schedule(m_sleep, &TimelySender::Send, this);
	}

	void TimelySender::SendBurst(void) {
		NS_LOG_FUNCTION_NOARGS();
		uint32_t totalSent = 0;
		for (unsigned i = 0; totalSent <= m_burstSize && BytesInBuffer() > 0; i++){
			totalSent += SendPacket();
		}
	}

	uint32_t TimelySender::SendPacket() {
		if(m_socket == NULL) throw -1;

		if(DEBUG_CONDITION){
			std::cout << "\tSEND PKT:     seq:" << m_sent << " " << Print() << "  -> pktInBuffer:" << BytesInBuffer() << std::endl;
		}

		TimelyHeader tHeader;
		tHeader.SetSeq(m_sent);
		tHeader.SetPG(0);
		tHeader.SetData();
		tHeader.SetTs (Simulator::Now ().GetTimeStep ());

		// Set ACK needed flag for each K packets OR when it's the last packet to be sent.
		if (BytesInBuffer() <= m_pktSize || (m_sent + m_pktSize) / m_burstSize > m_burstCnt){
			if(DEBUG_CONDITION){
				std::cout << "\tNEEDED     bCnt:" << m_burstCnt << "  -> pktInBuffer:" << BytesInBuffer() << std::endl;
			}
			tHeader.SetAckNeeded();
			// No need to cancel the previous timeout event. 
			// Indeed we don't need to keep it as a instance variable.
			// if(m_ackRtoEvent.IsRunning()) Simulator::Cancel(m_ackRtoEvent);
			m_ackRtoEvent = Simulator::Schedule(m_ackRto, &TimelySender::AckTimeout, this, m_sent);
		}

		if(DEBUG_CONDITION) {
			tHeader.SetDebug();
		}


		uint32_t sendSize = BytesInBuffer();
		if(sendSize > m_pktSize)
			sendSize = m_pktSize;
		Ptr<Packet> pkt = Create<Packet> (m_randomBuffer + m_maxMsgSize - BytesInBuffer(), sendSize);
		pkt->AddHeader(tHeader);

		int handle = -1;
		if ((handle = m_socket->Send(pkt)) > 0) {
			PacketSentSuccessfully(sendSize);
		} else {
			std::cout << "Error while sending 3:" << handle; exit(29);
		}
		return sendSize; // in case sending is not successfull --> we don't reach here. 
	}

	void TimelySender::PacketSentSuccessfully(uint32_t sentSize){
		if(DEBUG_CONDITION){
			std::cout << "\tPACKET Sent Successfully: pktSize:" << sentSize << ", m_sent: " << m_sent << std::endl;
		}

		m_sent += sentSize;
		m_burstCnt = m_sent / m_burstSize;
	}

	void TimelySender::SetPG(uint16_t newPg) {
		m_pg = newPg;
	}

	void TimelySender::Receive(Ptr<Socket> socket){
		Ptr<Packet> packet;
		Address from;
		while ((packet = socket->RecvFrom(from))) {
			TimelyHeader tHeader;
			packet->RemoveHeader(tHeader);
			uint64_t seqNum = tHeader.GetSeq();

			if(tHeader.IsNack())
			{
				if(seqNum >= m_sent) {
					// duplicated NACK! Ignore it
					return;
				}

				if(DEBUG_CONDITION){
					std::cout << "\tNACK     seqNum:" << seqNum << ", stack:" << m_sent << " " << Print() << std::endl;
				}

				if(seqNum < m_received) {
					std::cout << "Invalid State!! seq:" << seqNum << ", rcvd: " << m_received << ", " << Print() << std::endl;
					exit(888); 
				}
				
				if(m_sendEvent.IsRunning())
					Simulator::Cancel(m_sendEvent);
				if(m_ackRtoEvent.IsRunning())
					Simulator::Cancel(m_ackRtoEvent);
					
				m_sent = seqNum;
				m_sendEvent = Simulator::Schedule(m_sleep, &TimelySender::Send, this);
			} else if(tHeader.IsAck()) {
				if(DEBUG_CONDITION){
					std::cout << "\tACK     seqNum:" << seqNum << ", rcvd:" << m_received << std::endl;
				}

				if(seqNum < m_received) {
					std::cout << seqNum << "   ---     " << m_received << std::endl;
					std::cout << "Invalid State 112: " <<  Print() << std::endl; exit(1112); 
				}
				
				m_received = seqNum;
				m_new_rtt = GenerateRTTSample(TimeStep (tHeader.GetTs()));
				UpdateSendRate();

				if(m_sent < m_received){
					if(DEBUG_CONDITION){
						std::cout << "\tUpdate m_sent: " << m_sent << ", m_rcvd:" << m_received << std::endl;
					}
					m_sent = m_received; // in case a timeout reduced the m_sent and now we got the ack!
				}

				if(m_sendEvent.IsExpired() && m_sent < m_totalToBeSent) {
					m_sendEvent = Simulator::Schedule(m_sleep, &TimelySender::Send, this);
				}
			}
		}
	}

	Time TimelySender::GenerateRTTSample(Time ts) {
		Time rtt_sample = Simulator::Now() - ts;
		Time adjustedSample = rtt_sample - m_sdel;
		if(DEBUG_CONDITION){
			static int64_t max_rtt = 0;
			static int64_t min_rtt = 1000;
			if(max_rtt < adjustedSample.GetMicroSeconds())
				max_rtt = adjustedSample.GetMicroSeconds();
			if(min_rtt > adjustedSample.GetMicroSeconds())
				min_rtt = adjustedSample.GetMicroSeconds();

			std::cout << "\tRTT HIGH: \t" << adjustedSample.GetMicroSeconds() << "us  " << "\tmin:" << min_rtt << "\tmax:" << max_rtt << std::endl;
		}
		return adjustedSample;
	}

	int TimelySender::UnderLowTh(void) const {
		if(BytesInBuffer() == 0) return 0;
		if(m_new_rtt < m_prev_rtt) return 1;
		return -1;
	}

	void TimelySender::UpdateSendRate() {
		Time new_rtt_diff = m_new_rtt - m_prev_rtt;
		DataRate m_newRate = 0;
		m_rtt_diff = MicroSeconds((1 - m_alpha) * m_rtt_diff.GetMicroSeconds() + m_alpha * new_rtt_diff.GetMicroSeconds());
		double normalized_gradiant =  m_rtt_diff.GetMicroSeconds() * 1.0 / m_min_rtt.GetMicroSeconds();
		if(normalized_gradiant > 1) normalized_gradiant = 1.0;

		std::string updaterule;
		if (m_new_rtt < m_t_low)
		{
			updaterule = "t_low";
			m_newRate = DataRate(m_rate.Get().GetBitRate() + m_delta.GetBitRate());
			m_HAI = 1;
		}
		else if (m_new_rtt > m_t_high)
		{
			updaterule = "t_high";
			m_newRate = DataRate(m_rate.Get().GetBitRate() * (1 - m_beta * (1 - m_t_high.GetMicroSeconds() * 1.0 / m_new_rtt.Get().GetMicroSeconds())));
			m_HAI = 1;
		}
		else if (normalized_gradiant < 0)
		{
			m_HAI++;
			updaterule = "negative_grad";
			if(m_HAI < 5)
				m_newRate = DataRate(m_rate.Get().GetBitRate() + m_delta.GetBitRate());
			else
				m_newRate = DataRate(m_rate.Get().GetBitRate() + 5 * m_delta.GetBitRate());
		}
		else
		{
			updaterule = "pos_grad";
			m_newRate = DataRate(m_rate.Get().GetBitRate() * (1 - m_beta * normalized_gradiant));
			if(m_newRate.GetBitRate() > m_maxRate.GetBitRate()) {
				std::cout << "norm: " << normalized_gradiant << std::endl;
				exit(55);
			}
			m_HAI = 1;
		}

		m_newRate = std::min(m_newRate, m_maxRate);
		m_newRate = std::max(m_newRate, m_minRate);

		m_rate = m_newRate;
		m_prev_rtt = m_new_rtt;
		m_sleep = GetBurstDuration(m_rate);

		// std::cout << "Sleep: " << m_sleep.GetMicroSeconds() << "us" << std::endl;

		if(false){
			std::cout 
				<< "\ttime=" << Simulator::Now().GetSeconds() << ", "
				<< "id=" << m_appId << ", "
				<< "sent=" << m_sent << ", "
				<< "recvd=" << m_received << ", "
				<< "rtt_sample=" << m_new_rtt.Get().GetMicroSeconds() << "us, "
				<< "updaterule=" << updaterule.c_str() << ", "
				<< "newrate=" << m_rate.Get().GetBitRate()/1000/1000 << "Mbps" << std::endl;
		}
	}

} // Namespace ns3