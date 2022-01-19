# Congestion-control in NS3 v30.1
This is an NS-3 simulator for data center congestion control tests. It includes the implementation of Timely, plus all of its the variation for announcements handling. 

It is based on [NS-3 version 3.31](https://www.nsnam.org/releases/ns-allinone-3.31.tar.bz2)

# Installation

* Install [gnuplot](http://gnuplot.sourceforge.net) (to create simulation results). On OSX, you can install with homebrew:
```
$ brew install gnuplot
```

Follow the installation steps below to install in debug mode:
```
$ cd /YOURCHOICE_1/
$ wget https://www.nsnam.org/releases/ns-allinone-3.31.tar.bz2
$ tar -xjf https://www.nsnam.org/releases/ns-allinone-3.31.tar.bz2
$ cd ns-allinone-3.31/
$ cd ns-3-allinone/ns-3.31/
$ ./waf configure  # -d release, optimized
$ ./waf build
```

Clone this repo and install it on top of the NS3 source files:
```
$ cd /YOURCHOICE_2/
$ git clone git@github.com:usi-systems/buffer-sim.git
$ git fetch
$ git checkout rts
$ cd /YOURCHOICE_1/ns-3-allinone-3.31/ns-3.31/
$ ln -s /YOURCHOICE_2/buffer-sim/dctcp ./src/
$ ln -s /YOURCHOICE_2/buffer-sim/timely ./src/
$ ./waf build
``` 

To run the simulation
```
$ cd /YOURCHOICE_1/ns-allinone-3.31/ns-3.31/
$ ln -s /YOURCHOICE_1/buffer-sim/simulation/fat-tree.cc ./scratch/
$ ln -s /YOURCHOICE_1/buffer-sim/simulation/myapp.h ./scratch/
$ ln -s /YOURCHOICE_1/buffer-sim/simulation/flowlet.h ./scratch/
$ ./waf --run "scratch/fat-tree <Other Options>
```
Here are the possible options:
| option        | Description           | Possible Values | 
| ------------- |:-------------| ---------|
| bufferSize | Main buffer size in packet |  1, 2, ...
| priorityPercentage | Percentage of priorty bursts  | 0,1,...,100 
| maxSimulationTime | The end of the simulation time in seconds | 
| packetSize | Packet size in Bytes | 57, 58, ...
| longTcpCnt | Number of concurrent TIMELY connections |
| seed | The random seed for the starting time of short TCP connections |
| TimelyDelta | Timely's delta for increasing the rate  | e.g. 5Gbps
| TimelyInitial | Timely's initial sending rate  | e.g. 5Gpbs
| TimelyThLow | Timely's low RTT threshold | e.g. 100us
| TimelyThHigh | Timely's hight RTT threshold | e.g. 200us
| TimelyHighpMode | How to handle high priority bursts | `normal`: Vanilla Timely, `announc_0`: Start sending immediately after sending announcements, `announc_22`: Wait for 22us after sending the announcements, `skip_annc`: Don't send announcements
| TimelyLowpMode | How to react to announcements | `wait_100p`: calculate the waiting time, wait 100%, `wait_90p`: calculate the waiting time, wait 90%, ..., `skipif_200`: ignore an announcement if there exists only 200 packets left to send, `skip_100`: ignore an announcement if there exists only 100 packets left to send, `ignore_annc`: ignore all announcements
