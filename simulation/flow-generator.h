#ifndef _FLOW_GENERATOR_
#define _FLOW_GENERATOR_

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/traced-callback.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/node.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/double.h"
#include "ns3/pointer.h"
#include "ns3/queue.h"
#include "ns3/timely-sender.h"
#include "ns3/timely-receiver.h"
#include "ns3/timely-sender-receiver-helper.h"

#define DEBUG_CONDITION_ false // m_appId == 20 //&& Simulator::Now().GetMilliSeconds() > 1017


using namespace ns3;

enum CC_MODE {
	_TIMELY, _TCPBASED /* == NORMAL TCP! */
};

static void _log_tx(Ptr<const Packet> pkt, const TcpHeader &tHeader, Ptr<const TcpSocketBase> tcpSoc) {
	std::cout << "*********" << Simulator::Now().GetMilliSeconds() << "ms   ";
	std::cout << "tx:" << pkt->GetSize() << ", flags:" << (int)tHeader.GetFlags()	<< std::endl;
}

static void _log_rx(Ptr<const Packet> pkt, const TcpHeader &tHeader, Ptr<const TcpSocketBase> tcpSoc) {
	std::cout << "*********" << Simulator::Now().GetMilliSeconds() << "ms   ";
	std::cout << "rx:" << pkt->GetSize() << ", flags:" << (int)tHeader.GetFlags()	<< std::endl;
}

static void _log_rto(Time oldVal, Time newVal) {
	std::cout << "*********" << Simulator::Now().GetMilliSeconds() << "ms      rto:" << newVal.GetMicroSeconds() << "us";
}

class FlowGenerator : public Application {
	CC_MODE					m_mode;

	Ptr<TimelySender> 		m_timelyApp;					// Associated low priority Timely App
	Ptr<Socket>	 			m_tcpSocket;					// Associated socket


	uint32_t				m_initialAckNumber;				// first ACK number rcvd in low priority traffic

	Address		 			m_peer;							// Peer address
	DataRate 				m_cbrRate;						// connection rate

	bool					m_connected;					// True if connected
	bool					m_offPeriod;					// true: no active burst!

	EventId		 			m_startStopEvent;				// Event id for next start or stop event
	EventId		 			m_sendEvent;					// Event id of pending "send packet" event
	EventId					m_poissonArrival;				// Event id for next burst arrival
	EventId					m_generateEvent;				// Event id for the length of a pending burst arrival

	
	double													m_meanBurstArrivals;
	Ptr<ExponentialRandomVariable> 							m_expRandomVar;
	Ptr<UniformRandomVariable> 								m_uniformRandomVar;
	std::vector<std::pair<double, uint32_t>> 				m_burstSizeDistro;			// distribution of the burst size

	uint32_t 							m_appId; 						// a simple ID for this application
	uint32_t 							m_burstId; 						// the id for the current active burst
	uint32_t 							m_lastBurstSize; 				// latest burst size in terms of packet
	Time								m_lastStartTime;				// Time last burst started
	Time								m_lastAckRcvd;					// Time last time I've seen an ACK!
	uint32_t							m_bytesToBeSent;				// number of bytes to be sent for the current active burst!

	uint32_t							m_totalBytes;					// Total high priority packets to be sent so far! some are already sent, some are going to..
	uint32_t							m_retransmitCnt;

	void (*m_logger)(std::string, int, ...);
	void (*m_errLogger)(std::string);

public:
	static uint8_t 	*RANDOM_BUFFER;
	static uint32_t _GVAR_ORACLE_BUFFER_OCCUPANCY;
	static uint32_t _GVAR_ONGOING_BURSTS;
	static uint32_t _GVAR_COMPLETED_BURSTS_LP;
	static double _GVAR_LOG_START;
	static double _GVAR_LOG_END;

	void SetLogger (void (*logger) (std::string, int, ...), void (*errLogger) (std::string)){
		m_logger = logger;
		m_errLogger = errLogger;
	}

	// pFabric / tcp / dctcp
	FlowGenerator	(Ptr<Socket> soc, uint32_t appId, Address remote, std::string bDistro, std::string connectionrate) : FlowGenerator (appId, bDistro, connectionrate) {
		m_mode = _TCPBASED; 
		m_tcpSocket = soc;
		m_peer = remote;
	}

	// Timely
	FlowGenerator	(Ptr<TimelySender> app, uint32_t appId, std::string bDistro, std::string connectionrate) : FlowGenerator (appId, bDistro, connectionrate) {
		m_mode = _TIMELY;
		m_timelyApp = app;
	}
	
	~FlowGenerator() {
		m_connected = false;
		m_lastStartTime = Seconds (0);
		m_lastAckRcvd = Seconds (0);

		m_totalBytes = 0;
		m_timelyApp = NULL;

		m_tcpSocket = NULL;
	}

	bool JobIsDone() {
		bool cond = (m_lastStartTime.GetSeconds() > _GVAR_LOG_END || m_offPeriod) && Simulator::Now().GetSeconds() > _GVAR_LOG_END;
		bool con2 =  m_lastStartTime.GetSeconds() < _GVAR_LOG_END && !m_offPeriod && Simulator::Now().GetSeconds() > _GVAR_LOG_END;
		int since_last_ack = (Simulator::Now() - m_lastAckRcvd).GetMilliSeconds();

		if(con2) {
			std::cout << "len: " << m_lastBurstSize << ", start: " << m_lastStartTime.GetSeconds() << ", last_ack:" << since_last_ack << "ms, id: " << m_appId << std::endl;
		}

		return cond;
	}
	
	/**
	 * \brief Return total bytes sent/isgoingtobeSent by this application.
	 */
	uint32_t GetTotalBytes() const {
		return m_totalBytes;
	}

	void SetMeanBurstIntervalTime(double m) {
		/* if(m_high) m = m/10.0;  we enforce 10% in the StartBurst function. */
		m_meanBurstArrivals = m;
		m_expRandomVar->SetAttribute ("Mean", DoubleValue (1.0/m_meanBurstArrivals));
	}

	static void Retransmit (Ptr<FlowGenerator> app, uint64_t oldVal, uint64_t newVal){
		app->m_retransmitCnt = newVal;
	}

	// is called upon receving an ACK in LOW priority Timely
	static void TimelyAckRcvd (Ptr<FlowGenerator> app, uint32_t oldVal, uint32_t newVal){
		uint32_t ackedData = newVal - oldVal;
		app->TimelyCheckFinishedBurst(newVal, ackedData);
	}

	// Is called for number of bytes in the air for TCP connection
	static void TcpAckRcvd (Ptr<FlowGenerator> app, SequenceNumber32 oldVal, SequenceNumber32 newVal){
		app->TcpCheckFinishedBurst(oldVal.GetValue(), newVal.GetValue());
	}

	static void TcpAnnouncementArrived (Ptr<FlowGenerator> app, uint32_t oldVal, uint32_t newVal){
		if(newVal > 0) app->TcpAnnouncement(newVal);
	}

	uint32_t GetMaxMsgSize() {
		if(m_burstSizeDistro.size() == 0) {
			std::cout << "invalid state" << std::endl;
			exit(4223);
		}
		uint32_t index = m_burstSizeDistro.size() - 1;
		return m_burstSizeDistro[index].second;
	}

protected:
	void DoDispose (void) {
		m_tcpSocket = 0;
		m_connected = false;
		Application::DoDispose ();
	}

private:
	FlowGenerator (uint32_t appId, std::string bDistro, std::string cbrRate) {
		m_appId = appId;
		m_connected = false;
		m_lastStartTime = Seconds (0);
		m_offPeriod = true;
		m_burstId = 0;
		m_bytesToBeSent = 0;
		m_totalBytes = 0;
		m_lastBurstSize = 0;
		m_initialAckNumber = 0;
		m_expRandomVar = CreateObject<ExponentialRandomVariable> (); // inter burst time gap.
		m_uniformRandomVar = CreateObject<UniformRandomVariable> ();
		m_uniformRandomVar->SetAttribute ("Min", DoubleValue (0.0));
		m_uniformRandomVar->SetAttribute ("Max", DoubleValue (100.0));
		m_meanBurstArrivals = 2000;// Mean rate of burst arrivals -> 0.5ms
		m_expRandomVar->SetAttribute ("Mean", DoubleValue (1.0/m_meanBurstArrivals));
		FillInDistro(bDistro);
		setupRandomBuffer();
		m_retransmitCnt = 0;
		m_cbrRate = DataRate(cbrRate);
	}

	void FillInDistro(std::string fileName){
		std::string line;
		std::ifstream myfile (fileName);
		
		if (myfile.is_open()) {
			std::getline (myfile,line); // What is the first line?
			while (std::getline (myfile,line)){
				size_t sIndex = line.find_first_of(' ');
				int len = std::stoi(line.substr(0,sIndex)); 
				double prob = std::stod(line.substr(sIndex + 1, line.length()));
				m_burstSizeDistro.push_back(std::make_pair(prob * 100, len));
			}
			myfile.close();
		} else {
			std::cout << "Unable to open file: " << fileName << std::endl; 
			exit(732);
		} 
	}

	void setupRandomBuffer(void){
		if(RANDOM_BUFFER != NULL)
			return;
		RANDOM_BUFFER = new uint8_t [GetMaxMsgSize() + 1];

		uint i = 0;
		RANDOM_BUFFER[i++] = 'A';
		RANDOM_BUFFER[i++] = 'L';
		RANDOM_BUFFER[i++] = 'I';
		RANDOM_BUFFER[i++] = '#';

		while(true) {
			std::string byteIndex = std::to_string(GetMaxMsgSize() - i);
			uint j = 0;
			while(j < byteIndex.length()) {
				if(i >= GetMaxMsgSize()) return;
				RANDOM_BUFFER[i++] = byteIndex[j++];
			}
			RANDOM_BUFFER[i++] = '#';
		}
	}

	void TimelyCheckFinishedBurst (uint32_t newVal, uint32_t ackedData){
		m_lastAckRcvd = Simulator::Now();
		if(DEBUG_CONDITION_) {
			std::cout << "Check finish newAck: " << newVal << ", total:" << m_totalBytes << std::endl;
		}

		if(ackedData <= m_bytesToBeSent)
			m_bytesToBeSent -= ackedData;
		else{
			std::cout << "ACKED MORE THAN TO BE SENT acked:" << ackedData << ", bytesToBeSent:" << m_bytesToBeSent << std::endl; exit(223332);
		}

		if(newVal >= m_totalBytes) {
			finishBurst();
		}
	}

	void TcpAnnouncement(uint32_t nPackets){
		if(m_offPeriod) return;
		m_tcpSocket -> SetAttribute("AnnouncementArrival", UintegerValue(nPackets));
	}

	void TcpCheckFinishedBurst (uint32_t oldAck, uint32_t newAck){
		m_lastAckRcvd = Simulator::Now();
		if(m_mode != _TCPBASED) throw -1;
		if(m_initialAckNumber == 0) { // first ACK of the connection
			m_initialAckNumber = newAck;
			return;
		}
		uint32_t ackBytes = 0;
		ackBytes = (newAck - m_initialAckNumber);

		if(DEBUG_CONDITION_){
			std::cout << Simulator::Now ().GetMilliSeconds() << "ms) checkFinishBurst p:" << ", bytesToSent:" << m_bytesToBeSent << ", ackN: " << ackBytes << ", total:" << m_totalBytes << std::endl;
		}

		if(ackBytes >= (m_totalBytes)){
			if(m_offPeriod) return; // not to call finish burst twice when we already called it due to delayed ack.
			finishBurst();
		}
	}

	void StartApplication() {
		if(m_mode == _TCPBASED){
			static uint local_port = 30000;
			Address localpoint (InetSocketAddress (local_port++));

			int handle = m_tcpSocket->Bind (localpoint);
			if(handle == 0 && m_tcpSocket->Connect (m_peer) == 0){
				ConnectionSucceeded();
			} else {
				std::cout << "_TCP connect was not successfull! " << m_appId << std::endl; exit(110);
			}
		} else { // Timely connection has been already started!
			ConnectionSucceeded();
		}
	}

	void StopApplication() {
		CancelEvents ();
		if(m_mode == _TCPBASED) {
			if(m_tcpSocket) m_tcpSocket->Close ();
		}
		m_connected = false;
	}
	
	// Helpers
	void CancelEvents () {
		Simulator::Cancel(m_sendEvent);
		Simulator::Cancel(m_startStopEvent);
		
		Simulator::Cancel(m_generateEvent);
		Simulator::Cancel(m_poissonArrival);
	}

	uint32_t GetMaxFlowSize() {
		if(m_burstSizeDistro.size() == 0) {
			std::cout << "invalid state" << std::endl;
			exit(4223);
		}
		uint32_t index = m_burstSizeDistro.size() - 1;
		return m_burstSizeDistro[index].second;
	}
	
	void TcpSendPacket() {
		uint32_t sendSize = m_bytesToBeSent;
		if(sendSize > 10000)
			sendSize = 10000;

		int handle = m_tcpSocket->Send (RANDOM_BUFFER + GetMaxFlowSize() - m_bytesToBeSent, sendSize, 0);
		if(DEBUG_CONDITION_) 
		 	std::cout << Simulator::Now ().GetMilliSeconds() << "ms) Sent pkt" << ", pktToBeSent: " << m_bytesToBeSent  << ", handle: " << handle << std::endl;

		if(handle > 0){
			uint32_t uHandle = handle;
			if(uHandle > m_bytesToBeSent) {
				std::cout << "Invalid handle" << std::endl;
				exit(234232);
			}
			m_bytesToBeSent -= uHandle;
		} else {
			if(DEBUG_CONDITION_){
				std::cout << "Handle: is -1 " << ", bytes to send:" << m_bytesToBeSent << std::endl;
			}
		}

		if(m_bytesToBeSent != 0)
			ScheduleNextTx();
	}

	void ConnectionSucceeded() {
		CancelEvents (); // Insure no pending event
		m_connected = true;
		m_generateEvent = Simulator::Schedule(Seconds(0.0), &FlowGenerator::GenerateBurst, this);
	}

	// uint32_t GetNewBurstLength(){
	// 	return 10000000;
	// }

	uint32_t GetNewBurstLength(){
		double randomVar = m_uniformRandomVar->GetValue ();
		for(uint32_t i = 0; i < m_burstSizeDistro.size(); i++)
			if(m_burstSizeDistro[i].first >= randomVar)
				return m_burstSizeDistro[i].second;

		std::cout << "Invaild State: " << randomVar << ", " << m_burstSizeDistro.size() << std::endl;
		m_errLogger("invalid state");
		exit(4423);
		return 5;
	}

	/**
	 * \ Functions that allows to keep track of the current number of active bursts at time t, nt,
	 * taking into account that their arrival process follows a Poisson process and that their
	 * length is determined by a Pareto distribution.
	 */
	void GenerateBurst() {
		if(m_bytesToBeSent != 0) {m_errLogger("invalid state"); exit(121);}
		if(!m_connected) {m_errLogger("invalid state"); exit(122);}

		Time delayToNextBurst = Seconds (m_expRandomVar->GetValue());
		m_poissonArrival = Simulator::Schedule(delayToNextBurst, &FlowGenerator::StartBurst, this);

		m_bytesToBeSent = GetNewBurstLength();
		m_lastBurstSize = m_bytesToBeSent;

		if(DEBUG_CONDITION_){ // DEBUG
			std::cout << "DELAY To next burst: " << delayToNextBurst.GetMicroSeconds() << "us" << std::endl;
		}
	}

	void LogTcpSocket(){
		m_tcpSocket->TraceConnectWithoutContext ("Tx", 		MakeCallback (&_log_tx));
		m_tcpSocket->TraceConnectWithoutContext ("Rx", 		MakeCallback (&_log_rx));
		m_tcpSocket->TraceConnectWithoutContext ("RTO", 	MakeCallback (&_log_rto));
	}
	
	void StartBurst() {
		if(!m_offPeriod) {std::cout << "Invalid m_offpriod";m_errLogger("invalid state"); exit(1);}
		if(!m_connected) {std::cout << "Invalid connected"; m_errLogger("invalid state"); exit(1);}
		
		m_lastStartTime = Simulator::Now();
		m_offPeriod = false;
		m_burstId++;
		_GVAR_ONGOING_BURSTS++;
		m_totalBytes += m_bytesToBeSent;

		if(m_mode == _TCPBASED) {
			ScheduleNextTx();
		} else if(m_mode == _TIMELY) {
			m_timelyApp->SetAttribute("SendData", UintegerValue(m_bytesToBeSent));
		}

		if(DEBUG_CONDITION_){ //DEBUG
			std::cout << "================================================" << std::endl;
			std::cout << "START BURST: " << m_bytesToBeSent << " ================" << std::endl;
			std::cout << "================================================" << std::endl;

			static bool enabled_log = false;
			if(Simulator::Now().GetMilliSeconds() > 1050) {
				if(!enabled_log)
					LogTcpSocket();
				enabled_log = true;
			}
		}
	}

	void finishBurst() {
		if(!m_connected) return;
		if(m_offPeriod) {std::cout << "Invalid m_offpriod"; m_errLogger("invalid state"); exit(142);}
		if(m_bytesToBeSent != 0) { std::cout << "invalid m_bytesToBeSent"; m_errLogger("invalid state"); exit(144); }

		if(DEBUG_CONDITION_){
			std::cout << "*********************************************" << std::endl;
			std::cout << "FINISHED";
			std::cout << "("<<m_burstId<<") at:" << Simulator::Now().GetMilliSeconds() << "ms    " << m_lastBurstSize << "" << std::endl;
			std::cout << "================================================" << std::endl;
		}

		_GVAR_ONGOING_BURSTS--;
		_GVAR_COMPLETED_BURSTS_LP++;

		std::string bType = "burst";

		if(m_lastStartTime.GetSeconds() >= _GVAR_LOG_START && m_lastStartTime.GetSeconds() < _GVAR_LOG_END)
			m_logger(bType, 2, "int64 time:", (Simulator::Now() - m_lastStartTime).GetMicroSeconds(), "uint size:", m_lastBurstSize);

		m_lastStartTime = Simulator::Now();
		
		m_offPeriod = true;
		m_generateEvent = Simulator::Schedule(Seconds(0.0), &FlowGenerator::GenerateBurst, this);
	}
	
	/**
	 * Function thet generates the packets departure at a constant bit-rate nt x r.
	 * ONLY TCP
	 */
	void ScheduleNextTx() {
		if(!m_connected) throw -1;
		if(m_offPeriod) {std::cout << "scheduleNext Invaild, appId:" << m_appId << std::endl; exit(-1);};
		if(m_mode != _TCPBASED) throw -1;

		Time nextTime (Seconds (1078 * 8 / static_cast<double>(m_cbrRate.GetBitRate())));
		m_sendEvent = Simulator::Schedule(nextTime, &FlowGenerator::TcpSendPacket, this);
	}

	std::string HorL(bool isHigh) {
		if(isHigh)
			return ", H";
		return ", L";
	}

public: 
	Time GetLastAckedTime() {
		if ((Simulator::Now() - m_lastAckRcvd).GetMilliSeconds() > 20) {
			std::cout << m_timelyApp->Print() << std::endl;
		}
		return m_lastAckRcvd;
	}

	uint32_t GetAppId() {
		return m_appId;
	}
};

#endif
