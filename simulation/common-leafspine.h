#ifndef COMMON_LEAFSPINE_H
#define COMMON_LEAFSPINE_H

enum CongestionType{
	PFABRIC = 3,
	TCP = 4,
	DCTCP = 5,
	TIMELY = 6,
};

enum SWITCH_TYPE {
	TOR,
	AGG,
	H2TOR
};

const std :: string _TRACE_FOLDER		=				"../../traces/";
const std :: string _BURST_FOLDER		=				"../../simulation/bursts/";
const std :: string _MGRUP_FOLDER		=				"../../simulation/mgroups/";

static std::map<uint32_t, std::pair<Time, uint32_t>> _GVAR_FIRST_ACK; // records the first ACK recived by each long flow!
static std::map<uint32_t, std::pair<Time, uint32_t>> _GVAR_LAST_ACK; // records the latest ACK recived by each long flow!
static std::map<int64_t, uint32_t> _GVAR_ACK_TIME; // records the ACK in each miliseconds low priority
static std::map<int64_t, uint32_t> _GVAR_HP_ACK_TIME; // records the ACK in each milliseconds; high priority
static double _GVAR_LAST_LOAD = 0;	// records the load in each milliseconds


// ********************************************* //
uint32_t 		SIMULATION_TYPE				= 5; 		// 3: pfabric, 4: tcp, 5:dctcp
bool			PCAP_LOG					= false;
std::string 	BUFFER_SIZE		 			= "325KB";  // based on pfabric paper
std::string 	P_BUFFER_SIZE		 		= "20KB";	// priority buffer for other use cases!
double 			MAX_SIMULATION_TIME	 		= 1.85; 	// Should be greater than MAX_APPLICATION_TIME
double 			MAX_APPLICATION_TIME	 	= 1.80;
double 			LOG_START_TIME	 			= 1.1;		// start logging the RPC right after this time!
double 			LOG_END_TIME	 			= 1.4;		// end logging the RPC after this time
uint32_t 		PACKET_SIZE	 				= 1078; 	// In Flow-Generator we use this DataRate as fixed. Take care of it in case of modification
uint32_t 		SEGMENT_SIZE	 			= 1024;
int				RANDOM_SEED					= 1;		
std::string 	BURST_DISTRO_LP				= "Facebook_HadoopDist_All.txt"; // check the bursts folder
// ********************************************* //
uint32_t 		CON_CON			 			= 32; // concurrent connections in each rack
double			BURST_MEAN_ARRIVAL			= 100.0;
int32_t			NETWORK_LOAD				= 20;
bool			LOAD_IS_FIXED 				= true; // fix the load or fix the waiting time between two bursts?
//===================== MONZA ========================//
uint32_t 		CC_LP_PORT 					= 8801; // dctcp-red-queue accordingly -> DctcpRedQueueDisc::DoEnqueue || are not valid anymore.
//===================== FAT-TREE =====================//
uint8_t			SPIN_CNT					= 2;
uint8_t			LEAF_CNT					= 3;
uint8_t			NODE_IN_RACK_CNT			= 16;
std::string 	SW_2_SW_BW					= "40Gbps"; 
std::string 	SW_2_SW_DELAY				= "0.7us";
std::string 	NODE_2_SW_BW				= "10Gbps"; // In Flow-Generator we use this DataRate as fixed. Take care of it in case of modification
std::string 	NODE_2_SW_DELAY				= "6.6us";


std::vector<struct MyPlot*> ALL_DATASETS;
std::vector<Ptr<FlowGenerator>> ALL_FLOW_GENERATORS;

const std::string CUSTOM_DATA_FILE 			= _TRACE_FOLDER + "DATA.log";
const std::string CUSTOM_EROR_FILE 			= _TRACE_FOLDER + "ERROR.log";

std::fstream 	CUSTOM_DATA_FSTREAM;
std::fstream 	CUSTOM_EROR_FSTREAM;

std::string 	CUSTOM_DATA_KEY 			= "";


struct MyPlot {
	Gnuplot2dDataset dataset;
	std::string fileName;
	std::string title;
	std::string xLegend;
	std::string yLegend;
	std::string xTics;
	std::string xRange;
};

static void _printLog(std::string str) {
	std::cout << str << std::endl;
}

static void my_error(std::string msg){
	CUSTOM_EROR_FSTREAM << msg << " " << CUSTOM_DATA_KEY << std::endl;
}

static std::string _simulation_type(){
	if( TCP == SIMULATION_TYPE) return "tcp";
	if( DCTCP == SIMULATION_TYPE) return "dctcp";
	if( PFABRIC == SIMULATION_TYPE) return "pfabric";
	if( TIMELY == SIMULATION_TYPE) return "timely";
	my_error("invalid simulatin type!");
	throw -1;
}

static void my_log(std::string logType, int n_args, ...){
	if(CUSTOM_DATA_KEY.length() == 0) {
		CUSTOM_DATA_KEY = 
						_simulation_type() + " "
						+ "seed: " + std::to_string(RANDOM_SEED) + " " 
						+ "concon: " + std::to_string(CON_CON) + " "  // concurrent connections
						+ "distroLp: " + BURST_DISTRO_LP + " " 
						+ "interval: " + std::to_string((int) BURST_MEAN_ARRIVAL) + " " 
						+ "load: " 	   + std::to_string((int) NETWORK_LOAD) + " " // target load
		;
	}
	
	std::string value = "";
	va_list ap;
    va_start(ap, n_args);
    for(int i = 0; i < n_args; i++) {
        std::string 	_key(va_arg(ap, const char *));

		if (_key.rfind("int ", 0) == 0) {
			int _value 	= va_arg(ap, int);
			value += _key.erase(0,4) + " " + std::to_string(_value);
		}

		if (_key.rfind("double ", 0) == 0) {
			double _value 	= va_arg(ap, double);
			value += _key.erase(0,7) + " " + std::to_string(_value);
		}

		if (_key.rfind("int64 ", 0) == 0) {
			int64_t _value 	= va_arg(ap, int64_t);
			value += _key.erase(0,5) + " " + std::to_string(_value);
		}

		if (_key.rfind("uint ", 0) == 0) {
			uint32_t _value 	= va_arg(ap, uint32_t);
			value += _key.erase(0,5) + " " + std::to_string(_value);
		}

		if (_key.rfind("uint64 ", 0) == 0) {
			uint64_t _value 	= va_arg(ap, uint64_t);
			value += _key.erase(0,7) + " " + std::to_string(_value);
		}
		value += " ";
    }
	va_end(ap);
	
	CUSTOM_DATA_FSTREAM << logType << " " << CUSTOM_DATA_KEY << value << std::endl;
}

static uint64_t _GVAR_DROP_CNT = 0;
static uint64_t _GVAR_LP_TOR_DROP_CNT = 0;
static uint64_t _GVAR_LP_AGG_DROP_CNT = 0;
static std::map<int64_t, uint32_t> _GVAR_DROP_TIME; // records the drops in each miliseconds
void QueueDropTracePkt (int id, Ptr<const Packet> item){
	_GVAR_DROP_CNT++;

	int64_t timeInMili = Simulator::Now ().GetMilliSeconds() / 10;

	if(_GVAR_DROP_TIME.find(timeInMili) == _GVAR_DROP_TIME.end()) {
		_GVAR_DROP_TIME[timeInMili]= 1;
	} else {
		_GVAR_DROP_TIME[timeInMili] = _GVAR_DROP_TIME[timeInMili] + 1;
	}

	uint port1 = 0;
	uint port2 = 0;
	if(item -> GetSize() >= 4) {
		uint8_t buff[4];
		item->CopyData(buff, 4);

		port1 = buff[0] * 256 + buff[1];
		port2 = buff[2] * 256 + buff[3];
	}
	if(port1 == CC_LP_PORT || port2 == CC_LP_PORT) {
		if(id == AGG) _GVAR_LP_AGG_DROP_CNT++; else _GVAR_LP_TOR_DROP_CNT++;
	} else {
		std::cout << "unknown packet drop!" << std::endl;
	}

	if(port1 * port2 == 0) {
		my_error("Unknown Packet Dropped: " + std::to_string(port1) + ", " + std::to_string(port2));
		return;
	}
}

void QueueDropTrace (int id, Ptr<const QueueDiscItem> item){
	QueueDropTracePkt(id, item->GetPacket());
}


static uint32_t _GVAR_QUEUE_LEN_MAX = 0;
static std::map<uint32_t, uint64_t> _GVAR_QUEUE_LEN; 
void QueueLenTrace (std::string traceId, uint32_t oldValue, uint32_t newValue){
	if(newValue < oldValue) return;
	if(_GVAR_QUEUE_LEN.find(newValue) == _GVAR_QUEUE_LEN.end())
		_GVAR_QUEUE_LEN[newValue] = 0;

	if(Simulator::Now().GetSeconds() >= LOG_START_TIME && Simulator::Now().GetSeconds() < LOG_END_TIME)
		_GVAR_QUEUE_LEN[newValue]++;
	if(newValue > _GVAR_QUEUE_LEN_MAX){
		_GVAR_QUEUE_LEN_MAX = newValue;
	}
}

static uint64_t _GVAR_ENQ_CNT 	= 0;
static std::map<std::string, uint64_t> _GVAR_ENQ_LOG;
void QueueEnqTrace (std::string traceId, Ptr<const QueueDiscItem> item){
	_GVAR_ENQ_CNT++;
	if(traceId[0] == 'T')
		return;
	
	if(_GVAR_ENQ_LOG.find(traceId) == _GVAR_ENQ_LOG.end()) {
		_GVAR_ENQ_LOG[traceId] = 0;
	}
	_GVAR_ENQ_LOG[traceId] = _GVAR_ENQ_LOG[traceId] + item->GetSize();
}

static uint32_t _under_load_cnt = 0;
static uint32_t _above_load_cnt = 0;
void update_burst_generator_rate(double load) {
	if(load < NETWORK_LOAD) {
		_under_load_cnt ++;
		_above_load_cnt = 0;
	}else if(load > NETWORK_LOAD + 5) {
		_above_load_cnt ++;
		_under_load_cnt = 0;
	} else {
		_above_load_cnt = 0;
		_under_load_cnt = 0;
	}

	double new_arrival = BURST_MEAN_ARRIVAL;
	if(_under_load_cnt >= 5) {
		new_arrival = BURST_MEAN_ARRIVAL * 1.09;
		if(_under_load_cnt > 7)
		_under_load_cnt = 1;
	}
	if(_above_load_cnt >= 5) {
		new_arrival = BURST_MEAN_ARRIVAL * 0.91;
		if(_above_load_cnt > 7)
			_above_load_cnt = 1;
	}

	if(new_arrival == BURST_MEAN_ARRIVAL)
		return;
	
	if(new_arrival == 0) new_arrival++;
	for(auto const& fg: ALL_FLOW_GENERATORS) {
		fg->SetMeanBurstIntervalTime(new_arrival);
	}
	BURST_MEAN_ARRIVAL = new_arrival;
}

static void check_if_all_burst_finished() {
	for(auto const& fg: ALL_FLOW_GENERATORS){
		if(!fg->JobIsDone())
			return;
	}
	// all jobs are finished!
	Simulator::Stop ();
}

static uint32_t _GVAR_RECENTLY_COMPLETED_BURSTS_LP = 0;
void PrintInfo(void){
	int64_t timeInMili = Simulator::Now ().GetMilliSeconds();

	std::cout << timeInMili << "ms] Bursts: " << (FlowGenerator::_GVAR_COMPLETED_BURSTS_LP - _GVAR_RECENTLY_COMPLETED_BURSTS_LP);
	_GVAR_RECENTLY_COMPLETED_BURSTS_LP = FlowGenerator::_GVAR_COMPLETED_BURSTS_LP;

	std::cout << "\tDROP:"<< _GVAR_DROP_CNT << " ToR:" << _GVAR_LP_TOR_DROP_CNT << ", Agg:" << _GVAR_LP_AGG_DROP_CNT;
	std::cout << "\tMaxQ:" << _GVAR_QUEUE_LEN_MAX << ", Intv:" << (uint32_t)BURST_MEAN_ARRIVAL;

	std::cout << "\tEnq ";
	double sum = 0;
	uint32_t printCnt = 0;
	for (auto const& x : _GVAR_ENQ_LOG) {
		double enqRate = x.second * 8.0 * 1000 / DataRate("1Gbps").GetBitRate();
		sum += enqRate;
		if(enqRate < 1) continue;  // don't print not loaded links!
		if(printCnt++ < 3)
			std::cout << (int)enqRate << "." << (int)(enqRate * 10) % 10 << "Gbps" << ", ";
		else std::cout << "." ;
	}
	std::cout << "[Total: " << (int)sum << "." << (int)(sum * 10) % 10 << "Gbps, ";
	double load = DataRate(std::to_string(sum) + "Gbps").GetBitRate() * 100.0 / (NODE_IN_RACK_CNT * LEAF_CNT * DataRate(NODE_2_SW_BW).GetBitRate());
	std::cout << (int)load << "." << int(load * 10) % 10 << "%" << "]";
	_GVAR_ENQ_LOG.clear();
	_GVAR_QUEUE_LEN_MAX = 0;
	std::cout << std::endl;

	if(LOAD_IS_FIXED)
		update_burst_generator_rate(load);
	check_if_all_burst_finished();
	_GVAR_LAST_LOAD = load;
}

std::string getFileId(int id){
	std::string tmp("aa");
	tmp[0] = (char)((id/26)%26) + 'a';
	tmp[1] = (char)(id%26) + 'a';
	return tmp;
}

void addFigureDimentions(MyPlot* appPlot, double x1, double x2){
	double tic = (x2-x1)/10;
	appPlot->xRange = "set xrange [" + std::to_string(x1) + ":" + std::to_string(x2) + "]";
	appPlot->xTics = "set xtics " + std::to_string(x1) + "," + std::to_string(tic) + "," + std::to_string(x2) + "";
}

MyPlot* getMarkPlotter(std::string title){
	MyPlot* m = new MyPlot();
	m->title = title;
	m->fileName = "Mark_";
	m->xLegend = "Time";
	m->yLegend = "Mark";
	m->dataset.SetStyle (Gnuplot2dDataset::IMPULSES);
	addFigureDimentions(m, 1 * 100, MAX_APPLICATION_TIME * 100);
	return m;
}

MyPlot* getDropPlotter(std::string title){
	MyPlot* m = new MyPlot();
	m->title = title;
	m->fileName = "drop_" + std::to_string(CON_CON) + "_";
	m->xLegend = "Time";
	m->yLegend = "Drop";
	m->dataset.SetStyle (Gnuplot2dDataset::IMPULSES);
	m->xRange = "set term png size 1024,768";
	return m;
}

uint32_t getRandomId(const uint32_t flow_id, bool isSrc){
	// int allFlows [6][2] = {{0,1},{1,2},{2,3},{0,2},{1,3},{0,3}};
	int allFlows [4][2] = {{1,2},{0,2},{1,3},{0,3}};

	if((flow_id/4)%2 == 1)
		isSrc = !isSrc;

	if(isSrc)
		return allFlows[flow_id%4][0];
	return allFlows[flow_id%4][1];
}

void printCustomPlots(){
#if 0
	std :: cout << std::endl << "----------------" << std::endl;
	std :: cout << "START LOGGING ..." << std::endl;
	int id = 0;
	const std :: string tracesMainDirectory = tracesFolder;// + std::to_string(CC			) + "/"; // concurrent connections
	for (std::vector<MyPlot*>::iterator it 	= ALL_DATASETS.begin() ; it != ALL_DATASETS.end(); ++it){
		std :: string plotFileName			= tracesMainDirectory + (*it)->fileName + getFileId(id) + std::to_string(id) + ".plt";
		std :: string graphicsFileName		= tracesMainDirectory + (*it)->fileName + getFileId(id) + std::to_string(id) + ".png";
		

		std :: cout << "," << id++;
		Gnuplot plot (graphicsFileName);
		plot.SetTitle (_simulation_type() + " || " + (*it)->title);
		plot.SetTerminal ("png");
		plot.SetLegend ((*it)->xLegend, (*it)->yLegend);
		plot.AppendExtra ((*it)->xTics);
		plot.AppendExtra ((*it)->xRange);
		plot.AddDataset ((*it)->dataset);
		std :: ofstream plotFile (plotFileName.c_str());
		plot.GenerateOutput (plotFile);
		plotFile.close ();
	}
	std :: cout << std:: endl << "FINISH LOGGING" << std::endl;
#endif
}

void ExtractFlowStats() {
	std::cout << std::endl << std::endl 
			<< "=========================== SPINE-LEAF " <<
			"(concurrent: " << CON_CON << ", " << _simulation_type() <<
			", load: " << NETWORK_LOAD <<
			")============================" << std::endl;

	Simulator::Stop (Seconds(MAX_SIMULATION_TIME + 0.01));
	Simulator::Run ();
	
	double rxGputSum = 0;
	for(auto it = _GVAR_FIRST_ACK.begin(); it != _GVAR_FIRST_ACK.end(); ++it){
		uint32_t flowId = it->first;
		auto firstTime = it->second.first;
		auto lastTime = _GVAR_LAST_ACK[flowId].first;
		uint32_t firstAck = it->second.second;
		uint32_t lastAck = _GVAR_LAST_ACK[flowId].second;
		double rxTPut = (lastAck - firstAck) * 8.0 / (lastTime - firstTime).GetSeconds();
		if(lastAck <= firstAck){ std::cout << "Somehting is wrong!" << std::endl; exit(100); }
		if(lastTime == firstTime) // Since we don't generate ACK for the ZERO, in case of only one burst, we don't have the start time!
			continue;
		rxGputSum += rxTPut;
		my_log("SingleGput", 1, "double goodput_bps:", rxTPut);
	}

	double 		avg_queue_len = 0.0;
	uint32_t 	sum_queue_len = 0;
	for(auto it = _GVAR_QUEUE_LEN.begin(); it != _GVAR_QUEUE_LEN.end(); ++it){
		avg_queue_len += it->first * it->second;
		sum_queue_len += it->second;
	}
	my_log("AvgQLen", 1, "double avgQ:", avg_queue_len/sum_queue_len);

	my_log("TotalGput", 2, "double bps:", rxGputSum, "double Gbps:", rxGputSum/DataRate("1Gbps").GetBitRate());
	my_log("EnqVolume", 1, "uint64 volume:", _GVAR_ENQ_CNT);
	my_log("DropValue", 	1, "uint64 drop:", _GVAR_DROP_CNT);
	my_log("DropRate", 	1, "double drop:", _GVAR_DROP_CNT * 1.0 / _GVAR_ENQ_CNT);
}

#endif
