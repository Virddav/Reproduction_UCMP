// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        
#include "config.h"
#include <sstream>
#include <strstream>
#include <fstream> // needed to read flows
#include <iostream>
#include <string.h>
#include <math.h>
#include "network.h"
#include "randomqueue.h"
#include "pipe.h"
#include "eventlist.h"
#include "logfile.h"
#include "loggers.h"
#include "clock.h"
#include "tcp.h"
#include "dctcp.h"
#include "compositequeue.h"
#include "topology.h"
#include <list>
#include "main.h"

// Choose the topology here:
#include "dynexp_topology.h"
#include "routing_util.h"
#include "routing.h"
#include "rlb.h"
#include "rlbmodule.h"

// Simulation params

#define PRINT_PATHS 0

uint32_t delay_host2ToR = 0; // host-to-tor link delay in nanoseconds
uint32_t delay_ToR2ToR = 500; // tor-to-tor link delay in nanoseconds

#define DEFAULT_PACKET_SIZE 1500 // MAXIMUM FULL packet size (includes header + payload), Bytes
#define DEFAULT_HEADER_SIZE 64 // header size, Bytes
    // note: there is another parameter defined in `ndppacket.h`: "ACKSIZE". This should be set to the same size.
// set the NDP queue size in units of packets (of length DEFAULT_PACKET_SIZE Bytes)
#define DEFAULT_QUEUE_SIZE 300

string ntoa(double n); // convert a double to a string
string itoa(uint64_t n); // convert an int to a string

EventList eventlist;

Logfile* lg;

void exit_error(char* progr){
    cout << "Usage " << progr << " [UNCOUPLED(DEFAULT)|COUPLED_INC|FULLY_COUPLED|COUPLED_EPSILON] [epsilon][COUPLED_SCALABLE_TCP" << endl;
    exit(1);
}

int main(int argc, char **argv) {

	// set maximum packet size in bytes:
    Packet::set_packet_size(DEFAULT_PACKET_SIZE - DEFAULT_HEADER_SIZE);
    mem_b queuesize = DEFAULT_QUEUE_SIZE * DEFAULT_PACKET_SIZE;

    // defined in flags:
    int cwnd;
    int marking_threshold = 0;
    simtime_picosec slice_duration = 0;
    stringstream filename(ios_base::out);
    string flowfile; // read the flows from a specified file
    string topfile; // read the topology from a specified file
    double pull_rate; // set the pull rate from the command line
    double simtime; // seconds
    double utiltime = .01; // seconds
    int64_t rlbflow = 0; // flow size of "flagged" RLB flows
    int64_t cutoff = 0; // cutoff between NDP and RLB flow sizes. flows < cutoff == NDP.
    bool rlb_enabled = true;
    RoutingAlgorithm routing_alg = SINGLESHORTEST;
    uint64_t routing_opt = ROUTING_NO_OPT;
    int64_t slice_time = 0;
    bool earlyfb = false;

    int i = 1;
    filename << "logout.dat";
    RouteStrategy route_strategy = NOT_SET;

    // parse the command line flags:
    while (i<argc) {
        if (!strcmp(argv[i],"-o")){
	       filename.str(std::string());
	       filename << argv[i+1];
	       i++;
        } else if (!strcmp(argv[i],"-cwnd")){
	       cwnd = atoi(argv[i+1]);
	       i++;
        } else if (!strcmp(argv[i],"-earlyfb")){
	       earlyfb = true;
        } else if (!strcmp(argv[i],"-aging")){
	       routing_opt = routing_opt | ROUTING_OPT_AGING;
        } else if (!strcmp(argv[i],"-alphazero")){
	       routing_opt = routing_opt | ROUTING_OPT_ALPHA0;
        } else if (!strcmp(argv[i],"-norlb")){
	       rlb_enabled = false;
        } else if (!strcmp(argv[i],"-dctcpmarking")){
	       marking_threshold = atoi(argv[i+1]);
	       i++;
        } else if (!strcmp(argv[i],"-slicedur")){
	       slice_duration = strtoull(argv[i+1], NULL, 10);
	       i++;
        } else if (!strcmp(argv[i],"-q")){
	       queuesize = atoi(argv[i+1]) * DEFAULT_PACKET_SIZE;
	       i++;
        /*
        } else if (!strcmp(argv[i],"-strat")){
            if (!strcmp(argv[i+1], "perm")) {
                route_strategy = SCATTER_PERMUTE;
            } else if (!strcmp(argv[i+1], "rand")) {
                route_strategy = SCATTER_RANDOM;
            } else if (!strcmp(argv[i+1], "pull")) {
                route_strategy = PULL_BASED;
            } else if (!strcmp(argv[i+1], "single")) {
                route_strategy = SINGLE_PATH;
            }
            i++;
        */
        } else if (!strcmp(argv[i],"-cutoff")) {
            cutoff = atof(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-rlbflow")) {
            rlbflow = atof(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-routing")) {
            if(!strcmp(argv[i + 1], "SingleShortest")) {
                routing_alg = SINGLESHORTEST;
            } else if (!strcmp(argv[i + 1], "KShortest")) {
                routing_alg = KSHORTEST;
            } else if (!strcmp(argv[i + 1], "VLB")) {
                routing_alg = VLB;
            } else if (!strcmp(argv[i + 1], "ECMP")) {
                routing_alg = ECMP;
            } else if (!strcmp(argv[i + 1], "LongShort")) {
                routing_alg = LONGSHORT;
            } else if (!strcmp(argv[i + 1], "OptiRoute")) {
                routing_alg = OPTIROUTE;
            } else {
                exit_error(argv[0]);
            }
            i++;
        } else if (!strcmp(argv[i],"-flowfile")) {
			flowfile = argv[i+1];
			i++;
        } else if (!strcmp(argv[i],"-topfile")) {
            topfile = argv[i+1];
            i++;
        /*
        } else if (!strcmp(argv[i],"-pullrate")) {
            pull_rate = atof(argv[i+1]);
            i++;
        */
        } else if (!strcmp(argv[i],"-simtime")) {
            simtime = atof(argv[i+1]);
            i++;
        } else if (!strcmp(argv[i],"-utiltime")) {
            utiltime = atof(argv[i+1]);
            i++;
        } else {
            exit_error(argv[0]);
        }
        
        i++;
    }
    srand(13); // random seed

    Routing* routing = new Routing(routing_alg, cutoff);
    routing->set_options(routing_opt);

    eventlist.setEndtime(timeFromSec(simtime)); // in seconds
    Clock c(timeFromSec(5 / 100.), eventlist);

    /*
    route_strategy = SCATTER_RANDOM; // only one routing strategy for now...
      
    if (route_strategy == NOT_SET) {
	   fprintf(stderr, "Route Strategy not set.  Use the -strat param.  \nValid values are perm, rand, pull, rg and single\n");
	   exit(1);
    }
    */

    //cout << "cwnd " << cwnd << endl;
    //cout << "Logging to " << filename.str() << endl;
    Logfile logfile(filename.str(), eventlist);

    lg = &logfile;

    // !!!!!!!!!!! make sure to set StartTime to the correct value !!!!!!!!!!!
    // *set this to be longer than the sim time if you don't want to record anything to the logger
    logfile.setStartTime(timeFromSec(10)); // record events starting at this simulator time

    // NdpSinkLoggerSampling object will iterate through all NdpSinks and log their rate every
    // X microseconds. This is used to get throughput measurements after the experiment ends.
    TcpSinkLoggerSampling sinkLogger = TcpSinkLoggerSampling(timeFromUs(50.), eventlist);

    logfile.addLogger(sinkLogger);
    TcpTrafficLogger traffic_logger = TcpTrafficLogger();
    logfile.addLogger(traffic_logger);
    TcpRtxTimerScanner tcpRtxScanner(timeFromMs(1), eventlist);


// this creates the Expander topology
#ifdef DYNEXP
    DynExpTopology* top = new DynExpTopology(queuesize, &logfile, &eventlist, earlyfb ? ECN_EFB : ECN, topfile, 
                                                routing, marking_threshold, slice_duration);
#endif

	// initialize all sources/sinks
    //NdpSrc::setMinRTO(1000); // microseconds
    //NdpSrc::setRouteStrategy(route_strategy);
    //NdpSink::setRouteStrategy(route_strategy);

    // debug:
    cout << "Loading traffic..." << endl;

    //ifstream input("flows.txt");
    ifstream input(flowfile);
    if (input.is_open()){
        string line;
        int64_t temp;
        int64_t maxflow = 0;
        // get flows. Format: (src) (dst) (bytes) (starttime nanoseconds)
        while(!input.eof()){
            vector<int64_t> vtemp;
            getline(input, line);
            stringstream stream(line);
            if(line.length() <= 0) continue;
            while (stream >> temp)
                vtemp.push_back(temp);
            //cout << "src = " << vtemp[0] << ", dest = " << vtemp[1] << ", bytes =  " << vtemp[2] << ", start_time[us] " << vtemp[3] << endl;

            // source and destination hosts for this flow
            int flow_src = vtemp[0];
            int flow_dst = vtemp[1];

            if (vtemp[2] < cutoff && vtemp[2] != rlbflow) { // priority flow, sent it over NDP

            // generate an DCTCP source/sink:
            TcpSrc* flowSrc = new DCTCPSrc(NULL, NULL, eventlist, top, flow_src, flow_dst, routing);
            //flowSrc->setCwnd(cwnd*Packet::data_packet_size()); // congestion window
            flowSrc->set_flowsize(vtemp[2]); // bytes
            maxflow = std::max(vtemp[2], maxflow);

            // Set the pull rate to something reasonable.
            // we can be more aggressive if the load is lower
            //NdpPullPacer* flowpacer = new NdpPullPacer(eventlist, pull_rate); // 1 = pull at line rate
            //NdpPullPacer* flowpacer = new NdpPullPacer(eventlist, .17);

            TcpSink* flowSnk = new TcpSink();
            tcpRtxScanner.registerTcp(*flowSrc);

            // set up the connection event
            flowSrc->connect(*flowSnk, timeFromNs(vtemp[3]/1.));

            sinkLogger.monitorSink(flowSnk);

            }  else { // background flow, send it over RLB

            // generate an RLB source/sink:

            RlbSrc* flowSrc = new RlbSrc(top, NULL, NULL, eventlist, flow_src, flow_dst);
            // debug:
            //cout << "setting flow size to " << vtemp[2] << " bytes..." << endl;
            flowSrc->set_flowsize(vtemp[2]); // bytes
            RlbSink* flowSnk = new RlbSink(top, eventlist, flow_src, flow_dst);

            // set up the connection event
            flowSrc->connect(*flowSnk, timeFromNs(vtemp[3]/1.));

            }
        }
    }


    cout << "Traffic loaded." << endl;

    if(rlb_enabled){
      RlbMaster* master = new RlbMaster(top, eventlist); // synchronizes the RLBmodules
      master->start();
    }

    // NOTE: UtilMonitor defined in "pipe"
    UtilMonitor* UM = new UtilMonitor(top, eventlist);
    UM->start(timeFromSec(utiltime)); // print utilization every X milliseconds.

    cout << "Starting... " << endl;


    // Record the setup
    //int pktsize = Packet::data_packet_size();
    //logfile.write("# pktsize=" + ntoa(pktsize) + " bytes");
    //logfile.write("# hostnicrate = " + ntoa(HOST_NIC) + " pkt/sec");
    //logfile.write("# corelinkrate = " + ntoa(HOST_NIC*CORE_TO_HOST) + " pkt/sec");
    //double rtt_rack = timeAsUs(timeFromNs(RTT_rack));
    //logfile.write("# rtt_rack = " + ntoa(rtt_rack) + " microseconds");
    //double rtt_net = timeAsUs(timeFromNs(RTT_net));
    //logfile.write("# rtt_net = " + ntoa(rtt_net) + " microseconds");

    // GO!
    while (eventlist.doNextEvent()) {
    }
    //cout << "Done" << endl;

    top->report_packet_reroute();

}

string ntoa(double n) {
    stringstream s;
    s << n;
    return s.str();
}

string itoa(uint64_t n) {
    stringstream s;
    s << n;
    return s.str();
}
