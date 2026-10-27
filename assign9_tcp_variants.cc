// Assignment 9 : TCP Reno vs Cubic vs BBR in a VANET-style topology (5 vehicles -> RSU -> Internet -> Server)
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/traffic-control-module.h"
#include <iostream>
#include <iomanip>
#include <fstream>
using namespace ns3;

static std::ofstream g_cwnd;
static void CwndTrace(uint32_t oldV, uint32_t newV)
{
    g_cwnd << Simulator::Now().GetSeconds() << " " << newV << "\n";
}
// Vehicle 0 is NodeList/0; its TCP socket exists once BulkSend starts (t=1.0)
static void ConnectCwnd()
{
    Config::ConnectWithoutContext("/NodeList/0/$ns3::TcpL4Protocol/SocketList/0/CongestionWindow",
                                  MakeCallback(&CwndTrace));
}

static void RunVariant(std::string name, std::string sockType)
{
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue(sockType));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(1024));
    Config::SetDefault("ns3::TcpSocket::InitialCwnd", UintegerValue(10));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 20));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 20));
    Config::SetDefault("ns3::TcpSocketState::EnablePacing", BooleanValue(name == "BBR")); // BBR needs pacing

    const uint32_t NV = 5;
    NodeContainer vehicles; vehicles.Create(NV);   // NodeList 0..4
    NodeContainer rsu;      rsu.Create(1);         // Roadside router
    NodeContainer inet;     inet.Create(1);        // "Internet" router
    NodeContainer server;   server.Create(1);      // Traffic control server

    PointToPointHelper wireless; // vehicle -> RSU (abstracted as p2p)
    wireless.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    wireless.SetChannelAttribute("Delay", StringValue("2ms"));
    PointToPointHelper bottleneck; // RSU -> Internet (congested uplink)
    bottleneck.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));
    bottleneck.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("100p"));
    PointToPointHelper core;
    core.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
    core.SetChannelAttribute("Delay", StringValue("20ms"));

    InternetStackHelper stack;
    stack.Install(vehicles); stack.Install(rsu); stack.Install(inet); stack.Install(server);

    TrafficControlHelper tch;
    Ipv4AddressHelper ip;
    for (uint32_t i = 0; i < NV; i++)
    {
        NetDeviceContainer d = wireless.Install(vehicles.Get(i), rsu.Get(0));
        tch.Uninstall(d);
        ip.SetBase(("10.1." + std::to_string(i + 1) + ".0").c_str(), "255.255.255.0");
        ip.Assign(d);
    }
    NetDeviceContainer dB = bottleneck.Install(rsu.Get(0), inet.Get(0));
    tch.Uninstall(dB);
    ip.SetBase("10.1.10.0", "255.255.255.0"); ip.Assign(dB);
    NetDeviceContainer dC = core.Install(inet.Get(0), server.Get(0));
    ip.SetBase("10.1.11.0", "255.255.255.0");
    Ipv4InterfaceContainer ifC = ip.Assign(dC);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // one greedy TCP flow per vehicle, each to its own port on the server
    for (uint32_t i = 0; i < NV; i++)
    {
        uint16_t port = 8080 + i;
        PacketSinkHelper sink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), port));
        ApplicationContainer s = sink.Install(server.Get(0));
        s.Start(Seconds(0.0)); s.Stop(Seconds(30.0));
        BulkSendHelper src("ns3::TcpSocketFactory", InetSocketAddress(ifC.GetAddress(1), port));
        src.SetAttribute("SendSize", UintegerValue(1024));
        src.SetAttribute("MaxBytes", UintegerValue(0));
        ApplicationContainer a = src.Install(vehicles.Get(i));
        a.Start(Seconds(1.0)); a.Stop(Seconds(30.0));
    }

    g_cwnd.open("cwnd-" + name + ".dat");
    Simulator::Schedule(Seconds(1.0001), &ConnectCwnd);

    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> mon = fmh.InstallAll();
    Simulator::Stop(Seconds(30.0));
    Simulator::Run();
    mon->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> cl = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());

    double sumThr = 0, sumSq = 0, sumDelay = 0, tx = 0, rx = 0; int n = 0;
    for (const auto &kv : mon->GetFlowStats())
    {
        Ipv4FlowClassifier::FiveTuple t = cl->FindFlow(kv.first);
        const auto &s = kv.second;
        if (t.destinationPort < 8080 || t.destinationPort >= 8080 + NV) continue; // data flows only
        double dur = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        double mbps = dur > 0 ? s.rxBytes * 8.0 / dur / 1e6 : 0;
        sumThr += mbps; sumSq += mbps * mbps; n++;
        sumDelay += s.rxPackets > 0 ? s.delaySum.GetSeconds() / s.rxPackets * 1000.0 : 0;
        tx += s.txPackets; rx += s.rxPackets;
    }
    double jain = (n > 0 && sumSq > 0) ? (sumThr * sumThr) / (n * sumSq) : 0;
    std::cout << std::fixed << std::setprecision(3)
              << std::setw(8) << std::left << name << std::right
              << std::setw(14) << sumThr << std::setw(14) << (n ? sumDelay / n : 0)
              << std::setw(12) << (tx > 0 ? 100.0 * (tx - rx) / tx : 0)
              << std::setw(12) << jain << "   (cwnd-" << name << ".dat)\n";
    g_cwnd.close();
    Simulator::Destroy();
}

int main(int argc, char *argv[])
{
    std::cout << std::setw(8) << std::left << "Variant" << std::right
              << std::setw(14) << "Thr(Mbps)" << std::setw(14) << "AvgDelay(ms)"
              << std::setw(12) << "Loss(%)" << std::setw(12) << "Jain\n";
    std::cout << std::string(60, '-') << "\n";
    RunVariant("Reno",  "ns3::TcpNewReno");
    RunVariant("Cubic", "ns3::TcpCubic");
    RunVariant("BBR",   "ns3::TcpBbr");

    std::cout << "\nPlot congestion windows:  gnuplot -e \"plot 'cwnd-Reno.dat' w l, 'cwnd-Cubic.dat' w l, 'cwnd-BBR.dat' w l; pause -1\"\n"
              << "\nWhy low delay & reliability matter in ITS:\n"
              << " - Safety messages (collision warning, platooning, emergency braking) are useful only within ~10-100 ms;\n"
              << "   a late packet describes a road state that no longer exists.\n"
              << " - Lost camera/road-condition updates cause stale maps and wrong decisions at the traffic server.\n"
              << " - At 100 km/h a vehicle moves ~28 m/s, so 100 ms extra delay = ~2.8 m of position error.\n"
              << " - Loss-based Reno/Cubic fill the queue (high delay); BBR keeps the queue short (lower delay) but can be less fair to others.\n";
    return 0;
}
