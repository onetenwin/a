// NS3_Exp1 : Simple network, 4 nodes, 2 UDP flows, latency WITHOUT FlowMonitor
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include <map>
#include <iostream>
using namespace ns3;
NS_LOG_COMPONENT_DEFINE("SimpleNetwork");

// per-flow bookkeeping (index 0 = flow1, 1 = flow2)
static std::map<uint64_t, Time> g_txTime[2];
static double   g_delaySum[2] = {0, 0};
static uint32_t g_delayCnt[2] = {0, 0};
static uint32_t g_txPkts[2]   = {0, 0};

static void OnTx(uint32_t f, Ptr<const Packet> p)
{
    g_txTime[f][p->GetUid()] = Simulator::Now();
    g_txPkts[f]++;
}
static void OnRx(uint32_t f, Ptr<const Packet> p, const Address &)
{
    auto it = g_txTime[f].find(p->GetUid());
    if (it != g_txTime[f].end())
    {
        g_delaySum[f] += (Simulator::Now() - it->second).GetSeconds();
        g_delayCnt[f]++;
    }
}

int main(int argc, char *argv[])
{
    NodeContainer nodes;
    nodes.Create(4);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer d01 = p2p.Install(nodes.Get(0), nodes.Get(1));
    NetDeviceContainer d12 = p2p.Install(nodes.Get(1), nodes.Get(2));
    NetDeviceContainer d32 = p2p.Install(nodes.Get(3), nodes.Get(2));

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i01 = address.Assign(d01);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i12 = address.Assign(d12);
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i32 = address.Assign(d32);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // ---- Flow 1: n0 -> n2 (1 Mbps) ----
    uint16_t port1 = 5000;
    PacketSinkHelper sink1("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port1));
    ApplicationContainer sinkApp1 = sink1.Install(nodes.Get(2));
    sinkApp1.Start(Seconds(1.0));
    sinkApp1.Stop(Seconds(10.0));

    OnOffHelper source1("ns3::UdpSocketFactory", InetSocketAddress(i12.GetAddress(1), port1));
    source1.SetAttribute("DataRate", StringValue("1Mbps"));
    source1.SetAttribute("PacketSize", UintegerValue(512));
    // OnTime=1, OffTime=0  -> true CBR (default OnOff is 50% duty cycle!)
    source1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    source1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer srcApp1 = source1.Install(nodes.Get(0));
    srcApp1.Start(Seconds(2.0));
    srcApp1.Stop(Seconds(10.0));

    // ---- Flow 2: n3 -> n0 (500 Kbps) ----
    uint16_t port2 = 6000;
    PacketSinkHelper sink2("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port2));
    ApplicationContainer sinkApp2 = sink2.Install(nodes.Get(0));
    sinkApp2.Start(Seconds(1.0));
    sinkApp2.Stop(Seconds(10.0));

    OnOffHelper source2("ns3::UdpSocketFactory", InetSocketAddress(i01.GetAddress(0), port2));
    source2.SetAttribute("DataRate", StringValue("500Kbps"));
    source2.SetAttribute("PacketSize", UintegerValue(512));
    source2.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    source2.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    ApplicationContainer srcApp2 = source2.Install(nodes.Get(3));
    srcApp2.Start(Seconds(3.0));
    srcApp2.Stop(Seconds(10.0));

    // ---- latency without FlowMonitor: timestamp at Tx, match packet UID at Rx ----
    srcApp1.Get(0)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&OnTx, 0));
    sinkApp1.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&OnRx, 0));
    srcApp2.Get(0)->TraceConnectWithoutContext("Tx", MakeBoundCallback(&OnTx, 1));
    sinkApp2.Get(0)->TraceConnectWithoutContext("Rx", MakeBoundCallback(&OnRx, 1));

    Simulator::Stop(Seconds(10.0));
    Simulator::Run();

    uint64_t rx1 = DynamicCast<PacketSink>(sinkApp1.Get(0))->GetTotalRx();
    uint64_t rx2 = DynamicCast<PacketSink>(sinkApp2.Get(0))->GetTotalRx();
    double dur1 = 8.0; // 2s -> 10s
    double dur2 = 7.0; // 3s -> 10s
    double thr1 = rx1 * 8.0 / dur1 / 1e6;
    double thr2 = rx2 * 8.0 / dur2 / 1e6;
    double lat1 = g_delayCnt[0] ? g_delaySum[0] / g_delayCnt[0] * 1000.0 : 0;
    double lat2 = g_delayCnt[1] ? g_delaySum[1] / g_delayCnt[1] * 1000.0 : 0;

    std::cout << "\n=========== RESULTS ===========\n";
    std::cout << "Flow 1 (n0 -> n2): " << rx1 << " bytes received\n";
    std::cout << "Flow 2 (n3 -> n0): " << rx2 << " bytes received\n";
    std::cout << "Total bytes received : " << (rx1 + rx2) << "\n\n";
    std::cout << "Flow 1 throughput    : " << thr1 << " Mbps\n";
    std::cout << "Flow 2 throughput    : " << thr2 << " Mbps\n";
    std::cout << "Flow 1 avg latency   : " << lat1 << " ms (" << g_delayCnt[0] << "/" << g_txPkts[0] << " pkts)\n";
    std::cout << "Flow 2 avg latency   : " << lat2 << " ms (" << g_delayCnt[1] << "/" << g_txPkts[1] << " pkts)\n";
    std::cout << "\nComparison: Flow 1 crosses 2 links (n0-n1-n2) so its latency is ~2 x (2ms + tx time);\n"
              << "Flow 2 crosses 3 links (n3-n2-n1-n0) so latency is higher. Flow 1 carries 2x the data\n"
              << "rate of Flow 2, so its throughput is ~2x. Neither exceeds the 5 Mbps link -> no loss.\n";

    Simulator::Destroy();
    return 0;
}
