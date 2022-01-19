#include <fstream>
#include <unistd.h>
#include <string>
#include "ns3/queue.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/monzatcp-socket-factory.h"
#include "ns3/drop-tail-queue.h"
#include "ns3/traffic-control-module.h"
#include "ns3/gnuplot.h"
#include "ns3/timely-control.h"
#include "ns3/monzatcp-packet-filter.h"
#include "ns3/sequence-number.h"
#include "ns3/monzatcp-cntrl-helper.h"
#include "ns3/monzatcp-socket-base.h"
#include "flow-generator.h"
#include <iostream>
#include <iomanip>
#include <map>
#include <stdarg.h>
#include "common-leafspine.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("MONZA_TCP");

struct MyPlot;
class MonzatcpPacketFilter;

void SetupToplogyAndRun();
void printCustomPlots();
void setBufferSizeListner(NetDeviceContainer&, int, int, int);
void setBufferSizeListner(NetDeviceContainer&, int, int);
void SetupFlows(NodeContainer &nodes, MonzatcpCntrlHelper &cntrlHelper, const uint8_t srcRackId, uint conn);
void ExtractFlowStats();
void SetDefaultConfigs();
MyPlot* getMarkPlotter(std::string title);
void my_error(std::string msg);
void PrintInfo(void);

uint32_t FlowGenerator::_GVAR_ORACLE_BUFFER_OCCUPANCY;
uint32_t FlowGenerator::_GVAR_ONGOING_BURSTS;
uint32_t FlowGenerator::_GVAR_COMPLETED_BURSTS_HP;
uint32_t FlowGenerator::_GVAR_COMPLETED_BURSTS_LP;
double FlowGenerator::_GVAR_LOG_START;
double FlowGenerator::_GVAR_LOG_END;
uint8_t *FlowGenerator::RANDOM_BUFFER;


// ********************************************* //
std::string		L4_FACTORY_ID				= "ns3::TcpSocketFactory";
std::string		MIN_RTO						= "87us"; // pfabirc paper (they suggested 3*RTT as best value)
// ==============================================//


static bool _monzadctcp(){
	return (MONZA_DCTCP == SIMULATION_TYPE);
}

static bool _monzatcp(){
	return (MONZA_TCP == SIMULATION_TYPE);
}

static bool _monza(){
	return _monzatcp() || _monzadctcp();
}

static bool _dctcp(){
	return (DCTCP == SIMULATION_TYPE);
}

static bool _tcp(){
	return (TCP == SIMULATION_TYPE);
}

static bool _pfabric(){
	return (PFABRIC == SIMULATION_TYPE);
}

static void RcvdAck (uint32_t flowId, bool isHigh, SequenceNumber32 oldAck, SequenceNumber32 newAck)
{
	if(isHigh){
		if(_GVAR_FIRST_ACK_HP.find(flowId) == _GVAR_FIRST_ACK_HP.end()){
			_GVAR_FIRST_ACK_HP[flowId] = std::make_pair(Simulator::Now(), oldAck.GetValue());
		}
		_GVAR_LAST_ACK_HP[flowId] = std::make_pair(Simulator::Now(), newAck.GetValue());
	}else{
		if(_GVAR_FIRST_ACK.find(flowId) == _GVAR_FIRST_ACK.end()){
			_GVAR_FIRST_ACK[flowId] = std::make_pair(Simulator::Now(), oldAck.GetValue());
		}
		_GVAR_LAST_ACK[flowId] = std::make_pair(Simulator::Now(), newAck.GetValue());
	}

	int64_t timeInMili = Simulator::Now ().GetMilliSeconds();
	if(isHigh){
		if(_GVAR_HP_ACK_TIME.find(timeInMili) == _GVAR_HP_ACK_TIME.end()) {
			_GVAR_HP_ACK_TIME[timeInMili] = (newAck.GetValue() - oldAck.GetValue());
		} else {
			_GVAR_HP_ACK_TIME[timeInMili] = _GVAR_HP_ACK_TIME[timeInMili] + (newAck.GetValue() - oldAck.GetValue());
		}
	}else{
		if(_GVAR_ACK_TIME.find(timeInMili) == _GVAR_ACK_TIME.end()) {
			_GVAR_ACK_TIME[timeInMili] = (newAck.GetValue() - oldAck.GetValue());
			PrintInfo();
		} else {
			_GVAR_ACK_TIME[timeInMili] = _GVAR_ACK_TIME[timeInMili] + (newAck.GetValue() - oldAck.GetValue());
		}
	}
}

static uint64_t _GVAR_MARKED_CNT = 0;
static std::map<int64_t, uint32_t> _GVAR_MARK_TIME; // records the drops in each miliseconds
void QueueMarkTrace (std::string switchId, Ptr<const QueueDiscItem> item, const char* str){
	_GVAR_MARKED_CNT++;

	int64_t timeInMili = Simulator::Now ().GetMilliSeconds() / 10;
	if(_GVAR_MARK_TIME.find(timeInMili) == _GVAR_MARK_TIME.end()) {
		_GVAR_MARK_TIME[timeInMili]= 1;
	} else {
		_GVAR_MARK_TIME[timeInMili] = _GVAR_MARK_TIME[timeInMili] + 1;
	}

	// if(_GVAR_MARKED_CNT % 1000 == 0)
	// std::cout << "Mark " << Simulator::Now() << " -> " << _GVAR_MARKED_CNT << " " << _GVAR_MARKED_CNT * 1.0 /_GVAR_ENQ_CNT << std::endl;
}

void SetDefaultConfigs(){
	ns3::PacketMetadata::Enable (); // avoid error in the packet filter.

	_GVAR_DROP_CNT = 0;
	FlowGenerator::_GVAR_ONGOING_BURSTS = 0;
	FlowGenerator::_GVAR_COMPLETED_BURSTS_HP = 0;
	FlowGenerator::_GVAR_COMPLETED_BURSTS_LP = 0;
	FlowGenerator::_GVAR_LOG_START = LOG_START_TIME;
	FlowGenerator::_GVAR_LOG_END = LOG_END_TIME;

	GlobalValue::Bind ("ChecksumEnabled", 								BooleanValue (false));
	Config::SetDefault ("ns3::Ipv4GlobalRouting::RandomEcmpRouting", 	BooleanValue (true));
	
	Config::SetDefault ("ns3::TcpSocket::DelAckTimeout",		 		StringValue ("2us"));
	Config::SetDefault ("ns3::TcpSocket::ConnTimeout", 					StringValue ("44us")); // pfabirc paper 3*RTT
	Config::SetDefault ("ns3::TcpSocket::InitialCwnd",					UintegerValue (17)); 	// pfabric paper 10Gbps * 14.6us BDP
	Config::SetDefault ("ns3::TcpSocket::SegmentSize", 					UintegerValue (SEGMENT_SIZE));


	Config::SetDefault ("ns3::MonzatcpSocketBase::MinRto", 				StringValue(MIN_RTO));
	Config::SetDefault ("ns3::MonzatcpSocketBase::ClockGranularity", 	StringValue("1us"));
	Config::SetDefault ("ns3::TcpSocketBase::MinRto", 					StringValue(MIN_RTO));
	Config::SetDefault ("ns3::TcpSocketBase::ClockGranularity", 		StringValue("1us"));
	Config::SetDefault ("ns3::RttEstimator::InitialEstimation", 		StringValue("14.6us")); //pfabirc paper
	Config::SetDefault ("ns3::TcpSocketState::MaxPacingRate", 			StringValue(NODE_2_SW_BW));

	Config::SetDefault ("ns3::MonzatcpSocketBase::Timestamp",			StringValue("false"));

	if(_dctcp() || _monzadctcp()){
		Config::SetDefault ("ns3::TcpL4Protocol::SocketType", 			StringValue ("ns3::TcpDctcp"));
		Config::SetDefault ("ns3::MonzatcpL4Protocol::SocketType", 		StringValue ("ns3::TcpDctcp"));
		Config::SetDefault ("ns3::TcpSocket::DelAckCount", 				UintegerValue (2));
		
		Config::SetDefault ("ns3::RedQueueDisc::UseEcn", 				BooleanValue ("true"));
		Config::SetDefault ("ns3::RedQueueDisc::UseHardDrop", 			BooleanValue (false));
		Config::SetDefault ("ns3::RedQueueDisc::MeanPktSize", 			UintegerValue (PACKET_SIZE));
		Config::SetDefault ("ns3::RedQueueDisc::QW", 					DoubleValue (1));
		Config::SetDefault ("ns3::RedQueueDisc::MinTh", 				DoubleValue (22.5)); //pfabric paper
   		Config::SetDefault ("ns3::RedQueueDisc::MaxTh", 				DoubleValue (22.5)); //pfabric paper
		Config::SetDefault ("ns3::RedQueueDisc::LInterm", 				DoubleValue (0.0)); // no drop!
	}

	if(_pfabric()) {
		Config::SetDefault ("ns3::TcpSocketBase::Sack",					StringValue("true"));
	}

	if(_monza()) {
		L4_FACTORY_ID = "ns3::MonzatcpSocketFactory";
	}
}

int main (int argc, char *argv[]){
	SIMULATION_TYPE  = TCP;
	Time::SetResolution (Time::NS);

	// Initialization Complete
	CommandLine cmd;
	cmd.AddValue ("sType", "1: Tcp, 2: DCTCP", SIMULATION_TYPE);
	cmd.AddValue ("maxSimulationTime", "The end of the simulation", MAX_SIMULATION_TIME);
	cmd.AddValue ("connectionCnt", "Number of concurrent connections", CON_CON);
	cmd.AddValue ("seed", "The random seed for the starting time of short TCP connections", RANDOM_SEED); // concurrent connections
	cmd.AddValue ("HMode", "How to handle high priority bursts", HIGHP_MODE);
	cmd.AddValue ("LMode", "How to react to announcements", REACT_MODE);
	cmd.AddValue ("minRto", "Minimum RTO in the TCP/DCTCP", MIN_RTO);
	cmd.AddValue ("LpDistro", "Burst Distribution Files, low priority", BURST_DISTRO_LP);
	cmd.AddValue ("HpDistro", "Burst Distribution Files, high priority", BURST_DISTRO_HP);
	cmd.AddValue ("interval", "1/X seconds is the mean interval time", BURST_MEAN_ARRIVAL);
	cmd.AddValue ("load", "Load in the network, e.g. 60 --> 60% link utilization. -1 for unbound load", NETWORK_LOAD);
	cmd.Parse (argc, argv);
	
	if(RANDOM_SEED > 0)
		RngSeedManager::SetSeed (RANDOM_SEED);
	else
		RngSeedManager::SetSeed (time(0));

	if(NETWORK_LOAD < 0) 
		LOAD_IS_FIXED = false;
	
	_GVAR_NODE_IN_RACK_CNT = NODE_IN_RACK_CNT;

	if(SIMULATION_TYPE != MONZA_TCP && SIMULATION_TYPE != MONZA_DCTCP && SIMULATION_TYPE != TCP && SIMULATION_TYPE != DCTCP && SIMULATION_TYPE != PFABRIC) {
		std::cout << "Invalid Simulation TYPE: " << SIMULATION_TYPE << std::endl;
		return EXIT_FAILURE;
	}
	
	CUSTOM_DATA_FSTREAM.open(CUSTOM_DATA_FILE, std::ios_base::app);
	CUSTOM_EROR_FSTREAM.open(CUSTOM_EROR_FILE, std::ios_base::app);

	SetupToplogyAndRun();
	printCustomPlots();

	CUSTOM_DATA_FSTREAM.close();
	CUSTOM_EROR_FSTREAM.close();

	std::cout << "End of Simulation!" << std::endl;
}

void SetupToplogyAndRun(){
	_printLog( "Building Topo..." );
	
	SetDefaultConfigs();

	PointToPointHelper NodeToSW;
	PointToPointHelper SWToSW;

	NodeToSW.SetDeviceAttribute ("DataRate", 	StringValue (NODE_2_SW_BW));
	NodeToSW.SetChannelAttribute("Delay", 		StringValue (NODE_2_SW_DELAY));
	SWToSW.SetDeviceAttribute 	("DataRate", 	StringValue (SW_2_SW_BW));
	SWToSW.SetChannelAttribute 	("Delay", 		StringValue (SW_2_SW_DELAY));
	
	TrafficControlHelper tchNodes;		// node to swtiche!

	uint16_t hNodes = tchNodes.SetRootQueueDisc ("ns3::PrioQueueDisc", "Priomap", StringValue ("0 1 1 1 1 1 1 1 1 1 1 1 1 1 1 1"));
	TrafficControlHelper::ClassIdList   cid = tchNodes.AddQueueDiscClasses (hNodes, 2, "ns3::QueueDiscClass");
	tchNodes.AddChildQueueDisc (hNodes, cid[0], "ns3::FifoQueueDisc", "MaxSize", StringValue(P_BUFFER_SIZE)); // high priority

	if(_dctcp() || _monzadctcp()) {	
		tchNodes.AddChildQueueDisc (hNodes, cid[1], "ns3::RedQueueDisc", "MaxSize", StringValue(BUFFER_SIZE));
	} else if(_tcp() || _monzatcp()) {
		tchNodes.AddChildQueueDisc (hNodes, cid[1], "ns3::FifoQueueDisc", "MaxSize", StringValue(BUFFER_SIZE));
	} else if(_pfabric()) {
		tchNodes.AddChildQueueDisc (hNodes, cid[1], "ns3::pFabricQueueDisc", "MaxSize", StringValue(BUFFER_SIZE));
	}

	NodeContainer spins, leafs, nodes[LEAF_CNT];
	{ // filling the variables!
		spins.Create (SPIN_CNT);
		leafs.Create(LEAF_CNT);
		for(uint8_t i = 0; i < LEAF_CNT; i++)
			nodes[i].Create(NODE_IN_RACK_CNT);
	}
	
	InternetStackHelper stack;
	{
		if(_monza())
			stack.SetTcp("ns3::MonzatcpL4Protocol");
		else
			stack.SetTcp("ns3::TcpL4Protocol");

		stack.Install(spins);
		stack.Install(leafs);
		for(uint8_t i = 0; i < LEAF_CNT; i++)
			stack.Install(nodes[i]);
	}
	
	//=========================== Creating Topology ==============================//
	NetDeviceContainer node2sw [LEAF_CNT][NODE_IN_RACK_CNT]; 		// [i][j]: rack[i], node[j]
	NetDeviceContainer sw2sw [SPIN_CNT][LEAF_CNT];					// [i][j]: spin[i], rack[j]

	{ // Establish connections
		for(uint8_t i = 0; i < LEAF_CNT; i++)
			for(uint8_t j = 0; j < NODE_IN_RACK_CNT; j++){
				node2sw[i][j] = NodeToSW.Install(nodes[i].Get(j), leafs.Get(i));

				PointerValue ptr;
				node2sw[i][j].Get(1)->GetAttribute("TxQueue", ptr);
				Ptr<Queue<Packet> > txQueue = ptr.Get<Queue<Packet> > ();

				std::string traceId = "TOR(" + std::to_string(i) + "-" + std::to_string(j) + ")";
				txQueue ->TraceConnectWithoutContext ("PacketsInQueue", MakeBoundCallback (&QueueLenTrace, traceId));
				// tracing the drops is not useful. I tried, but it doesn't happen in this p2p devices. 
			}
		
		for(uint8_t i = 0; i < SPIN_CNT; i++)
			for(uint8_t j = 0; j < LEAF_CNT; j++) {
				sw2sw[i][j] = SWToSW.Install(spins.Get(i), leafs.Get(j));

				PointerValue ptr0;
				PointerValue ptr1;
				sw2sw[i][j].Get(0)->GetAttribute("TxQueue", ptr0);
				sw2sw[i][j].Get(1)->GetAttribute("TxQueue", ptr1);
				Ptr<Queue<Packet>> txQueue0 = ptr0.Get<Queue<Packet> > ();
				Ptr<Queue<Packet>> txQueue1 = ptr1.Get<Queue<Packet> > ();

				std::string traceId = "AGG(" + std::to_string(i) + "-" + std::to_string(j) + ")";
				txQueue0 ->TraceConnectWithoutContext ("PacketsInQueue", 	MakeBoundCallback (&QueueLenTrace, traceId));
				txQueue1 ->TraceConnectWithoutContext ("PacketsInQueue", 	MakeBoundCallback (&QueueLenTrace, traceId));
				// tracing the drops is not useful. I tried, but it doesn't happen in this p2p devices. 
			}
	}

	_printLog( "Set buffers ...." );
	{
		for(uint8_t i = 0; i < LEAF_CNT; i++)
			for(uint8_t j = 0; j < NODE_IN_RACK_CNT; j++){
				std::string traceId = "TOR(" + std::to_string(i) + "-" + std::to_string(j) + ")";

				QueueDiscContainer qDisk2 = tchNodes.Install (node2sw[i][j].Get(1));
				
				// packet filter is compatible with pfabric. All are low priority
				Ptr<MonzatcpPacketFilter> pf2 = CreateObject<MonzatcpPacketFilter> ();
				qDisk2.Get(0) ->AddPacketFilter(pf2);
				
				qDisk2.Get(0) ->TraceConnectWithoutContext ("Drop", 				MakeBoundCallback (&QueueDropTrace, H2TOR));
				qDisk2.Get(0) ->TraceConnectWithoutContext ("PacketsInQueue", 		MakeBoundCallback (&QueueLenTrace, traceId));
			}
		
		for(uint8_t i = 0; i < SPIN_CNT; i++)
			for(uint8_t j = 0; j < LEAF_CNT; j++){
				std::string traceId = "AGG(" + std::to_string(i) + "-" + std::to_string(j) + ")";

				QueueDiscContainer qDisk1 = tchNodes.Install (sw2sw[i][j].Get(0));
				QueueDiscContainer qDisk2 = tchNodes.Install (sw2sw[i][j].Get(1));
				
				// packet filter is compatible with pfabric. All are low priority
				Ptr<MonzatcpPacketFilter> pf1 = CreateObject<MonzatcpPacketFilter> ();
				Ptr<MonzatcpPacketFilter> pf2 = CreateObject<MonzatcpPacketFilter> ();
				qDisk1.Get(0) ->AddPacketFilter(pf1);
				qDisk2.Get(0) ->AddPacketFilter(pf2);
				
				qDisk1.Get(0) ->TraceConnectWithoutContext ("Enqueue", MakeBoundCallback (&QueueEnqTrace, traceId));
				qDisk2.Get(0) ->TraceConnectWithoutContext ("Enqueue", MakeBoundCallback (&QueueEnqTrace, traceId));
				
				qDisk1.Get(0) ->TraceConnectWithoutContext ("Drop", 	MakeBoundCallback (&QueueDropTrace, AGG));
				qDisk2.Get(0) ->TraceConnectWithoutContext ("Drop", 	MakeBoundCallback (&QueueDropTrace, TOR));	

				qDisk1.Get(0) ->TraceConnectWithoutContext ("PacketsInQueue", MakeBoundCallback (&QueueLenTrace, traceId));
				qDisk2.Get(0) ->TraceConnectWithoutContext ("PacketsInQueue", MakeBoundCallback (&QueueLenTrace, traceId));
				if(_dctcp() || _monzadctcp()) {
					qDisk1.Get(0) ->TraceConnectWithoutContext ("Mark", MakeBoundCallback (&QueueMarkTrace, traceId));
					qDisk2.Get(0) ->TraceConnectWithoutContext ("Mark", MakeBoundCallback (&QueueMarkTrace, traceId));
				}
			}
	}
	
	//=========================== Assigning the IP address ==============================//
	Ipv4AddressHelper address;
	{
		for(uint8_t i = 0; i < LEAF_CNT; i++)
			for(uint8_t j = 0; j < NODE_IN_RACK_CNT; j++)
			{
				std::string ipBaseStr = "10." + std::to_string(i) + "." + std::to_string(j) + ".0";
				Ipv4Address ipBase = Ipv4Address(ipBaseStr.c_str());
				address.SetBase(ipBase ,"255.255.255.0");
				address.Assign (node2sw[i][j]);
			}
		for(uint8_t i = 0; i < SPIN_CNT; i++)
			for(uint8_t j = 0; j < LEAF_CNT; j++)
			{
				std::string ipBaseStr = "11." + std::to_string(i) + "." + std::to_string(j) + ".0";
				Ipv4Address ipBase = Ipv4Address(ipBaseStr.c_str());
				address.SetBase(ipBase ,"255.255.255.0");
				address.Assign (sw2sw[i][j]);
			}
	}

	if(_monza()){ _printLog( "Set multicast routes ...." );
		for(uint8_t i = 0; i < LEAF_CNT; i++){
			for(uint8_t j = 0; j < NODE_IN_RACK_CNT; j++){
				Ipv4StaticRoutingHelper multicast;
				Ptr<Node> sender = nodes[i].Get (j);
				Ptr<NetDevice> senderIf = node2sw[i][j].Get(0);
				multicast.SetDefaultMulticastRoute (sender, senderIf);
			}
		}

		Ipv4Address multicastSource ("0.0.0.0");
		Ipv4Address multicastGroup ("225.1.2.4"); // Same in all racks

		for(uint32_t i = 0; i < LEAF_CNT; i++){ // ToR
			Ipv4StaticRoutingHelper mHelper;
			Ptr<Node> multicastRouter = leafs.Get(i);
			
			for(uint8_t j = 0; j < NODE_IN_RACK_CNT; j++){
				NetDeviceContainer outputDevices;
				Ptr<NetDevice> inputIf = node2sw[i][j].Get (1);

				for(uint32_t k = 0; k < NODE_IN_RACK_CNT; k++){
					if(k == j) continue;
					outputDevices.Add (node2sw[i][k].Get(1));
				}
				mHelper.AddMulticastRoute (multicastRouter, multicastSource, multicastGroup, inputIf, outputDevices);
			}
		}
	}

	{ // installing Receivers and Cntrlers
		ApplicationContainer sinkApps;
		ApplicationContainer cntrlApps;

		Address hpTcpSinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), CC_HP_PORT));
		Address lpTcpSinkLocalAddress (InetSocketAddress (Ipv4Address::GetAny (), CC_LP_PORT));

		PacketSinkHelper 		hpSinkHelper (L4_FACTORY_ID	, 	hpTcpSinkLocalAddress);
		PacketSinkHelper 		lpSinkHelper (L4_FACTORY_ID	, 	lpTcpSinkLocalAddress);
		MonzatcpCntrlHelper 	cntrlHelper	 (MONZA_CTRL_PORT, 	LEAF_CNT, NODE_IN_RACK_CNT);

		if(_monza())
			cntrlHelper.SetAttribute("Highp_Mode",			StringValue(HIGHP_MODE));
		
		for(uint8_t i = 0; i < LEAF_CNT; i++)
			for(uint8_t j = 0; j < NODE_IN_RACK_CNT; j++){
				sinkApps.Add (lpSinkHelper.Install (nodes[i].Get (j)));

				if(_monza()) {
					sinkApps.Add 	(hpSinkHelper.Install 	(nodes[i].Get (j)));
					cntrlApps.Add 	(cntrlHelper.Install 	(nodes[i].Get (j), i, j));
				}
			}
		sinkApps.Start 	(Seconds (0.0));
		sinkApps.Stop 	(Seconds (MAX_SIMULATION_TIME));

		if(_monza()) {
			cntrlApps.Start (Seconds (0.0));
			cntrlApps.Stop (Seconds (MAX_SIMULATION_TIME));
		}

		for(uint8_t i = 0; i < LEAF_CNT; i++)
			SetupFlows(nodes[i], cntrlHelper, i, CON_CON);
	}

	Ipv4GlobalRoutingHelper::PopulateRoutingTables ();
	ExtractFlowStats();
}

void SetupFlows(NodeContainer &nodes, MonzatcpCntrlHelper& cntrlHelper,  const uint8_t srcRackId, uint nConnections){
	if(nConnections == 0 && srcRackId > 0) return; // debug purposes! only one connection in the whole network.
	else if(nConnections == 0) nConnections++;
	_printLog("Setup Unicast Connections ["+std::to_string(srcRackId)+"]....");

	static uint8_t dstRackId = 0;
	static uint8_t nodeId = 0;
	static uint8_t dstNodeId = 0;

	for (uint32_t j = 0; j < nConnections	; ++j)
	{
		dstRackId = (dstRackId + 1) % LEAF_CNT;
		nodeId = (nodeId + 1) % NODE_IN_RACK_CNT;
		dstNodeId = rand() % NODE_IN_RACK_CNT;
		uint32_t appId = j + srcRackId * 10 * nConnections;

		if(srcRackId == dstRackId) {dstRackId++; dstRackId = dstRackId % LEAF_CNT;}
		if(srcRackId == dstRackId) throw -1;
		// std::cout << "src " << (int)srcRackId << ":" << (int)nodeId << ", dst " << (int)dstRackId << ":" << (int)nodeId << std::endl;

		std::string titlePostfix = 	std::to_string(srcRackId) + ":" + std::to_string(nodeId) + " -> " + 
									std::to_string(dstRackId) + ":" + std::to_string(dstNodeId) + " interval: " + std::to_string(BURST_MEAN_ARRIVAL);

		std::string remoteAddressStr = "10." + std::to_string(dstRackId) + "." + std::to_string(dstNodeId) + ".1";
		Ipv4Address rAddress (remoteAddressStr.c_str());

		Address hpRAddress (InetSocketAddress (rAddress, CC_HP_PORT));
		Address lpRAddress (InetSocketAddress (rAddress, CC_LP_PORT));

		Ptr<Socket> hpSocket = nullptr;
		Ptr<Socket> lpSocket = nullptr;
		Ptr<MonzatcpControl> monzaCntrl = nullptr;
		Ptr<FlowGenerator> fgLp = nullptr;
		Ptr<FlowGenerator> fgHp = nullptr;

		if(_monza()) {
			hpSocket = Socket::CreateSocket (nodes.Get(nodeId), MonzatcpSocketFactory::GetTypeId ());
			lpSocket = Socket::CreateSocket (nodes.Get(nodeId), MonzatcpSocketFactory::GetTypeId ());
			monzaCntrl = cntrlHelper.Get (srcRackId, nodeId);

			fgLp = CreateObject<FlowGenerator> (lpSocket, appId, monzaCntrl, lpRAddress, _BURST_FOLDER + BURST_DISTRO_LP, false);
			fgHp = CreateObject<FlowGenerator> (hpSocket, appId, monzaCntrl, hpRAddress, _BURST_FOLDER + BURST_DISTRO_HP, true);
		} else {
			hpSocket = Socket::CreateSocket (nodes.Get(nodeId), TcpSocketFactory::GetTypeId ());
			lpSocket = Socket::CreateSocket (nodes.Get(nodeId), TcpSocketFactory::GetTypeId ());
			fgLp = CreateObject<FlowGenerator> (lpSocket, appId, lpRAddress, _BURST_FOLDER + BURST_DISTRO_LP);
		}
		ALL_FLOW_GENERATORS.push_back(fgLp);

		if(_monza())
			nodes.Get(nodeId)->AddApplication(fgHp);
		nodes.Get(nodeId)->AddApplication(fgLp);

		if(_monza()) {
			fgHp ->SetMeanBurstIntervalTime(BURST_MEAN_ARRIVAL);
			fgHp ->SetLogger(my_log, my_error);
			ALL_FLOW_GENERATORS.push_back(fgHp);
		}
		fgLp ->SetMeanBurstIntervalTime(BURST_MEAN_ARRIVAL);
		fgLp ->SetLogger(my_log, my_error);

		lpSocket->TraceConnectWithoutContext ("HighestRxAck", 	MakeBoundCallback (&RcvdAck, appId, false));
		lpSocket->TraceConnectWithoutContext ("HighestRxAck", 	MakeBoundCallback (&fgLp->TcpAckRcvd, fgLp));

		if(_monza()) {
			hpSocket->TraceConnectWithoutContext 	("HighestRxAck", 		MakeBoundCallback (&RcvdAck, appId, true));
			hpSocket->TraceConnectWithoutContext 	("HighestRxAck", 		MakeBoundCallback (&fgHp->TcpAckRcvd, fgHp));
			monzaCntrl->TraceConnectWithoutContext 	("AnnouncementRcvd", 	MakeBoundCallback (&fgLp->TcpAnnouncementArrived, fgLp));
		}

		fgLp			->SetStartTime (Seconds (1.0));
		fgLp			->SetStopTime (Seconds (MAX_APPLICATION_TIME));
		if(_monza()) {
			fgHp		->SetStartTime (Seconds (1.0));
			monzaCntrl	->SetStartTime (Seconds (1.0));
			fgHp		->SetStopTime (Seconds (MAX_APPLICATION_TIME));
			monzaCntrl	->SetStopTime (Seconds (MAX_APPLICATION_TIME));
		}
	}
}
