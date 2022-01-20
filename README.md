# Data Center Congestion Control Simulation in NS3

This repo includes the ns3 simulation environment for testing various
congestion control in a typical 2-tier
[leaf-spine](https://www.arubanetworks.com/faq/what-is-spine-leaf-architecture/)
topologies. Currently, it contains the configuration for
[TIMELY](https://dl.acm.org/doi/abs/10.1145/2829988.2787510),
[pFabric](https://dl.acm.org/doi/abs/10.1145/2534169.2486031),
[DCTCP](https://dl.acm.org/doi/abs/10.1145/1851182.1851192) and TCP Reno.
The code is intentionally developed in modules so that it can be extended with
further congestion control schemes in the future. It has been tested with
NS-3.33 and NS-3.35, but it should work with other versions as well. 

# Directories
- **ns-allinone-3.35** It contains the latest tested version of ns3
 simulator. The simulation code is developed in modules, therefore it
 should also work with other versions as well. I put this directory here
 for the sake of simplicity in installation. 
- **pfabric** It contains the implementation for a pFabric switching device
- **timely** It contains the implementation of Timely congestion control
 over UDP. For simplicity, it has two sender and receiver components. This
 directory also contains the Timely headers. 
- **simulation** This is the main directory to start the simulations. The
 contains the code necessary to build the data center topology, together
 with the
 [flow-generator](https://github.com/usi-systems/cc/blob/master/simulation/flow-generator.h)
 and the distribution for [RPC's
 lengths](https://github.com/usi-systems/cc/tree/master/simulation/bursts). 
- **traces** Finally, this directory keeps track of the simulation results. 

# How to Run
## N3 Installation
Here I show the commands required to install the ns3 within this repo. If
you are willing to use an external code base, the steps should be similar. 
```bash
git clone git@github.com:usi-systems/cc.git
cd cc/ns-allinone-3.35/ns-3.35/
./waf configure #<-d optimized> for faster execuation
./waf build # it takes a while
./waf --run "scratch/tcp-leafspine.cc --sType=X" # X=3:pFabric, 4:TCP, 5:DCTCP 
./waf --run "scratch/timely-leafspine.cc " # Timely
```
## Simulation Configs
You can fully configure your simulation using command line arugments or
some other global variables in the code. Below you may find a few of them.
For more, please check the code. 

| option | Type | Description | Plausible Values | 
| ------------- |:---| ----------| ---------|
| --distro | Command-Line | The distribution file to generate RPCs from | "Facebook_HadoopDist_All.txt"
| --interval | Command-Line | The mean value for the pausing time between two sequential RPCs. The pausing time follows an exponential distribution | "Facebook_HadoopDist_All.txt"
| --TimelyThLow | Command-Line | Timely lower threshold | "70us"
| --TimelyThHigh | Command-Line | Timely higher threshold | "170us"

# Refernces
- **Timely** implementation is based on this [repo](https://github.com/bobzhuyb/ns3-rdma)
- **Burst Distributions** are obtained from the [Homa](https://dl.acm.org/doi/abs/10.1145/3230543.3230564) project 
- **pFabric** the implementation is done based on the paper. 

# Known Issues
- There is no PFC implementation.

# Questions
For technical questions, please create an issue in this repo, so other people can benefit from your questions.

For other questions, please contact Ali (fattaa@usi.ch).