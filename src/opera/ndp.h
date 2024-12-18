// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-        


#ifndef NDP_H
#define NDP_H

/*
 * An NDP source and sink
 */

// modification: the NDP source and sink no longer assign the route
// that is now handled by the sending priority queue on packet egress

#include <list>
#include <map>
#include "config.h"
#include "network.h"
#include "ndppacket.h"
#include "fairpullqueue.h"
#include "eventlist.h"


#define timeInf 0
#define NDP_PACKET_SCATTER

//#define LOAD_BALANCED_SCATTER

//min RTO bound in us
// *** don't change this default - override it by calling NdpSrc::setMinRTO()
#define DEFAULT_RTO_MIN 5000


class NdpSink;

class Queue;

/*
class ReceiptEvent {
  public:
    ReceiptEvent()
	: _path_id(-1), _is_header(false) {};
    ReceiptEvent(uint32_t path_id, bool is_header)
	: _path_id(path_id), _is_header(is_header) {}
    inline int32_t path_id() const {return _path_id;}
    inline bool is_header() const {return _is_header;}
    int32_t _path_id;
    bool _is_header;
};
*/

class NdpSrc : public PacketSink, public EventSource {
    friend class NdpSink;
 public:
    NdpSrc(DynExpTopology* top, NdpLogger* logger, TrafficLogger* pktlogger, EventList &eventlist, int flow_src, int flow_dst);
    uint32_t get_id(){ return id;}
    virtual void connect(NdpSink& sink, simtime_picosec startTime);
    void set_traffic_logger(TrafficLogger* pktlogger);
    void startflow();
    void setCwnd(uint32_t cwnd) {_cwnd = cwnd;}
    static void setMinRTO(uint32_t min_rto_in_us) {_min_rto = timeFromUs((uint32_t)min_rto_in_us);}
    static void setRouteStrategy(RouteStrategy strat) {_route_strategy = strat;}
    void set_flowsize(uint64_t flow_size_in_bytes);
    inline uint64_t get_flowsize() {return _flow_size;} // bytes
    inline void set_start_time(simtime_picosec startTime) {_start_time = startTime;}
    inline simtime_picosec get_start_time() {return _start_time;}

    inline int get_flow_src() {return _flow_src;}
    inline int get_flow_dst() {return _flow_dst;}

    virtual void doNextEvent();
    virtual void receivePacket(Packet& pkt);

    virtual void processRTS(NdpPacket& pkt); // RTS = Return To Sender
    virtual void processAck(const NdpAck& ack);
    virtual void processNack(const NdpNack& nack);

    virtual void rtx_timer_hook(simtime_picosec now, simtime_picosec period);

    Queue* sendToNIC(Packet* pkt);

    // should really be private, but loggers want to see:
    uint64_t _highest_sent;  //seqno is in bytes
    uint64_t _packets_sent;
    uint64_t _new_packets_sent;
    uint64_t _rtx_packets_sent;
    uint64_t _acks_received;
    uint64_t _nacks_received;
    uint64_t _pulls_received;
    uint64_t _implicit_pulls;
    uint64_t _bounces_received;
    uint32_t _cwnd;
    uint64_t _last_acked;
    uint32_t _flight_size;
    uint32_t _acked_packets;
    uint32_t _found_reorder = 0;
    uint64_t _max_hops_per_trip = 0, _last_hop_per_trip = 0, _total_hops = 0;

    // the following are used with SCATTER_PERMUTE, SCATTER_RANDOM and PULL_BASED route strategies

    map<NdpPacket::seq_t, simtime_picosec> _sent_times;
    map<NdpPacket::seq_t, simtime_picosec> _first_sent_times;

    //void print_stats(); // removed

    int _pull_window; // Used to keep track of expected pulls so we
                      // can handle return-to-sender cleanly.
                      // Increase by one for each Ack/Nack received.
                      // Decrease by one for each Pull received.
                      // Indicates how many pulls we expect to
                      // receive, if all currently sent but not yet
                      // acked/nacked packets are lost
                      // or are returned to sender.
    int _first_window_count;

    //round trip time estimate, needed for coupled congestion control
    simtime_picosec _rtt, _rto, _mdev, _base_rtt;

    uint16_t _mss; // maximum segment size
    uint16_t _pkt_size; // packet size. Equal to _flow_size when _flow_size < _mss. Else equal to _mss
 
    uint32_t _drops;

    NdpSink* _sink;
 
    simtime_picosec _rtx_timeout;
    bool _rtx_timeout_pending;

    // const Route* _route; // modification: we don't need this anymore...

    // const Route *choose_route(); // choose a route from available _paths

    void pull_packets(NdpPull::seq_t pull_no, NdpPull::seq_t pacer_no);
    void send_packet(NdpPull::seq_t pacer_no);

    virtual const string& nodename() { return _nodename; }
    inline uint32_t flow_id() const { return _flow.flow_id();}
 
    //debugging hack
    void log_me();
    bool _log_me;

    static uint32_t _global_rto_count;  // keep track of the total number of timeouts across all srcs
    static simtime_picosec _min_rto;
    static RouteStrategy _route_strategy;
    static int _global_node_count;
    static int _rtt_hist[10000000];
    int _node_num;

    int _flow_src; // the sender (source) for this flow
    int _flow_dst; // the receiver (sink) for this flow

    DynExpTopology* _top;

 private:
    // Housekeeping
    NdpLogger* _logger;
    TrafficLogger* _pktlogger;
    // Connectivity
    PacketFlow _flow;
    string _nodename;

    enum  FeedbackType {ACK, NACK, BOUNCE, UNKNOWN};
    static const int HIST_LEN=12;
    FeedbackType _feedback_history[HIST_LEN];
    int _feedback_count;

    // Mechanism
    void clear_timer(uint64_t start,uint64_t end);
    void retransmit_packet();
    void update_rtx_time();
    void process_cumulative_ack(NdpPacket::seq_t cum_ackno);
    void log_rtt(simtime_picosec sent_time);
    NdpPull::seq_t _last_pull;
    simtime_picosec _start_time;
    uint64_t _flow_size;  //The flow size in bytes.  Stop sending after this amount.
    list <NdpPacket*> _rtx_queue; //Packets queued for (hopefuly) imminent retransmission
};

class NdpPullPacer;

class NdpSink : public PacketSink, public DataReceiver, public Logged {
    friend class NdpSrc;
 public:
    //NdpSink(EventList& ev, double pull_rate_modifier); // not used...
    NdpSink(DynExpTopology* top, NdpPullPacer* pacer, int flow_src, int flow_dst);
 

    uint32_t get_id(){ return id;}
    void receivePacket(Packet& pkt);
    NdpAck::seq_t _cumulative_ack; // the packet we have cumulatively acked
    uint32_t _drops;
    uint64_t cumulative_ack() { return _cumulative_ack + _received.size()*9000;}
    uint64_t total_received() const { return _total_received;}
    uint32_t drops(){ return _src->_drops;}
    virtual const string& nodename() { return _nodename; }
    void increase_window() {_pull_no++;} 
    static void setRouteStrategy(RouteStrategy strat) {_route_strategy = strat;}

    list<NdpAck::seq_t> _received; // list of packets above a hole, that we've received
 
    NdpSrc* _src;

    //debugging hack
    void log_me();
    bool _log_me;

    static RouteStrategy _route_strategy;

    int _flow_src; // the sender (source) for this flow
    int _flow_dst; // the receiver (sink) for this flow

    DynExpTopology* _top;

 private:
 
    // Connectivity
    //void connect(NdpSrc& src, Route& route);
    void connect(NdpSrc& src);

    inline uint32_t flow_id() const {
        return _src->flow_id();
    };

    // the following are used with SCATTER_PERMUTE, SCATTER_RANDOM,
    // and PULL_BASED route strategies

    string _nodename;
 
    NdpPullPacer* _pacer;
    NdpPull::seq_t _pull_no; // pull sequence number (local to connection)
    NdpPacket::seq_t _last_packet_seqno; //sequence number of the last
                                         //packet in the connection (or 0 if not known)
    uint64_t _total_received;

    simtime_picosec last_ts = 0;
    unsigned last_hops = 0;
    unsigned last_queueing = 0;
 
    // Mechanism
    void send_ack(simtime_picosec ts, NdpPacket::seq_t ackno, NdpPacket::seq_t pacer_no);
    void send_nack(simtime_picosec ts, NdpPacket::seq_t ackno, NdpPacket::seq_t pacer_no);
};

class NdpPullPacer : public EventSource {
 public:
    NdpPullPacer(EventList& ev, double pull_rate_modifier);  
    NdpPullPacer(EventList& ev, char* fn);  
    // pull_rate_modifier is the multiplier of link speed used when
    // determining pull rate.  Generally 1 for FatTree, probable 2 for BCube
    // as there are two distinct paths between each node pair.

    void sendPacket(Packet* p, NdpPacket::seq_t pacerno, NdpSink *receiver);
    virtual void doNextEvent();
    uint32_t get_id(){ return id;}
    void release_pulls(uint32_t flow_id);

    //debugging hack
    void log_me();
    bool _log_me;

    void set_preferred_flow(int id) { _preferred_flow = id; cout << "Preferring flow "<< id << endl;};

    void sendToNIC(Packet* pkt);

 private:
    void set_pacerno(Packet *pkt, NdpPull::seq_t pacer_no);
//#define FIFO_PULL_QUEUE
#ifdef FIFO_PULL_QUEUE
    FifoPullQueue<NdpPull> _pull_queue;
#else
    FairPullQueue<NdpPull> _pull_queue;
#endif
    simtime_picosec _last_pull;
    simtime_picosec _packet_drain_time;
    NdpPull::seq_t _pacer_no; // pull sequence number, shared by all connections on this pacer

    //pull distribution from real life
    static int _pull_spacing_cdf_count;
    static double* _pull_spacing_cdf;

    //debugging
    double _total_excess;
    int _excess_count;
    int _preferred_flow;
};


class NdpRtxTimerScanner : public EventSource {
 public:
    NdpRtxTimerScanner(simtime_picosec scanPeriod, EventList& eventlist);
    void doNextEvent();
    void registerNdp(NdpSrc &tcpsrc);
 private:
    simtime_picosec _scanPeriod;
    typedef list<NdpSrc*> tcps_t;
    tcps_t _tcps;
};

#endif

